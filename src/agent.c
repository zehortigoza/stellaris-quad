#include "main.h"
#include "agent.h"
#include "protocol.h"
#include "motors.h"
#include "mpu6050.h"
#include "pid.h"
#include "config.h"

static config configuration;

#define MPU_6050_FREQUENCY 500

//at this rate, pid and motors will be updated
#define COMMAND_FREQUENCY 100
#define COMMAND_COUNTER MPU_6050_FREQUENCY/COMMAND_FREQUENCY

//data received from controller
static unsigned short throttle = 0;
static unsigned short min_throttle;
static short receiver_pitch = 0;
static short receiver_roll = 0;
static short receiver_yaw = 0;
#define ROLL_SCALE 10
#define PITCH_SCALE 10
#define YAW_SCALE 10

static pid_data pid_roll;
static pid_data pid_pitch;

/*
 * bit0 = requesting orientation
 * bit1 = esc config
 * bit2 = arm
 * bit3 = motor on
 */
static unsigned char flags = 0;
#define REQUESTING_ORIENTATION GPIO_PIN_0

#define ESC_CONFIG_FLAG GPIO_PIN_1
#define ARMED_FLAG GPIO_PIN_2

#define MOTOR_ON_FLAG GPIO_PIN_3

static void _timer1_reset(void);

#define YAW_DIRECTION 1//or -1 to other side

//to quad X mode
static void motor_command_apply(short command_roll, short command_pitch, short command_yaw)
{
    unsigned short fl, fr, bl, br;

    if (throttle < min_throttle)
        throttle = min_throttle;

    fl  = throttle - command_pitch + command_roll - (YAW_DIRECTION * command_yaw);
    fr = throttle - command_pitch - command_roll + (YAW_DIRECTION * command_yaw);
    bl = throttle + command_pitch + command_roll + (YAW_DIRECTION * command_yaw);
    br = throttle + command_pitch - command_roll - (YAW_DIRECTION * command_yaw);

    //to avoid turn motors off in air
    if (fl == 0)
        fl = 1;
    if (fr == 1)
        fr = 1;
    if (bl == 0)
        bl = 1;
    if (br == 0)
        br = 1;
    motors_velocity_set(fl, fr, bl, br, 1);

#if BLACKBOX_ENABLED
    blackbox_commands_set(throttle, command_pitch, command_roll);
    blackbox_velocity_set(fl, fr, bl, br);
#endif
}

/*
 FL\---00---/FR
    \      /
     \    /         y
      \  /          ^
       \/           |
       /\           |
      /  \    x<-----
     /    \
    /      \
 BL/        \BR
*/
static void _msg_cb(Protocol_Msg_Type type, char request, ...)
{
    va_list ap;

    va_start(ap, request);
    switch (type)
    {
        case MOVE:
        {
            Protocol_Axis axis;
            unsigned short num;

            if (!(flags & ARMED_FLAG))
                break;

            axis = va_arg(ap, int);
            num = va_arg(ap, unsigned int);

            switch (axis)
            {
                case AXIS_Z://throttle
                {
                    if (throttle == 0 && num > 0)
                        flags |= MOTOR_ON_FLAG;

                    throttle = num;
                    //Only received throttle=0 when pressed Turn off on controller
                    if (throttle == 0)
                    {
                        motors_velocity_set(0, 0, 0, 0, 0);
                        flags &= ~ARMED_FLAG;
                        flags &= ~MOTOR_ON_FLAG;
                    }
                    receiver_pitch = receiver_roll = receiver_yaw = 0;
                    break;
                }
                case AXIS_X://roll
                {
                    receiver_pitch = receiver_yaw = 0;
                    //num [-3,3]
                    if (!num)
                        receiver_roll = 0;
                    else
                        receiver_roll = ROLL_SCALE * num;
                    break;
                }
                case AXIS_Y://pitch
                {
                    //num [-3,3]
                    receiver_roll = receiver_yaw = 0;
                    if (!num)
                        receiver_pitch = 0;
                    else
                        receiver_pitch = PITCH_SCALE * num;
                    break;
                }
                case AXIS_ROTATE://yaw
                {
                    receiver_pitch = receiver_roll = 0;
                    if (!num)
                        receiver_yaw = 0;
                    else
                        receiver_yaw = YAW_SCALE * num;
                    break;
                }
            }

            protocol_msg_send(type, 0);
            _timer1_reset();
            break;
        }
        case PING:
        {
            int num = va_arg(ap, int);
            protocol_msg_send(type, 0, num+1);
            break;
        }
        case BATTERY:
        {
            //TODO setup AD
            break;
        }
        case RADIO_LEVEL:
        {
            //TODO send at comand to xbee to get radio level
            break;
        }
        case ORIENTATION:
        {
            unsigned char enable = va_arg(ap, unsigned int);
            if (!(flags & ARMED_FLAG))
            {
                if (enable)
                    flags |= REQUESTING_ORIENTATION;
                else
                    flags &= ~REQUESTING_ORIENTATION;
            }
            break;
        }
        case CONFIGS:
        {
            protocol_msg_send(type, 0, configuration.p_gaing, configuration.i_gaing);
            break;
        }
        case CONFIGS_WRITE:
        {
            float p, i;

            p = va_arg(ap, double);
            i = va_arg(ap, double);
            configuration.p_gaing = p;
            configuration.i_gaing = i;
            config_write(&configuration);
            protocol_msg_send(type, 0);
            break;
        }
        case ESC_CONFIG:
        {
            unsigned char enable = va_arg(ap, unsigned int);
            if (!(flags & ARMED_FLAG))
            {
                if (enable)
                    flags |= ESC_CONFIG_FLAG;
                else
                {
                    flags &= ~ESC_CONFIG_FLAG;
                    motors_velocity_set(0, 0, 0, 0, 0);
                }
                protocol_msg_send(type, 0);
            }
            break;
        }
        case ESC_CONFIG_DATA:
        {
            unsigned int fl = va_arg(ap, unsigned int);
            unsigned int fr = va_arg(ap, unsigned int);
            unsigned int bl = va_arg(ap, unsigned int);
            unsigned int br = va_arg(ap, unsigned int);

            if (flags & ESC_CONFIG_FLAG)
            {
                motors_velocity_set(fl, fr, bl, br, 0);
                protocol_msg_send(type, 0);
            }
            break;
        }
        case ARM:
        {
            if ((flags & ESC_CONFIG_FLAG) || (flags & REQUESTING_ORIENTATION) || (flags & MOTOR_ON_FLAG))
                return;
            //this will set motors to the lowest velocity(1000)
            motors_velocity_set(1, 1, 1, 1, 0);
            flags |= ARMED_FLAG;
            protocol_msg_send(type, 0);
            break;
        }
        default:
        {
            break;
        }
    }
    va_end(ap);
}

static void _timer1_config(void)
{
    static char first_time = 1;

    if (first_time)
    {
        //enable timer1
        SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
        //enable interruption in timer1
        IntEnable(INT_TIMER1A);
        //normal priority
        IntPrioritySet(INT_TIMER1A, 2);

        first_time = 0;
    }
    //configure to count down
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    //value to count down
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 2);
    //enable generate a interruption in timer timeout
    TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
    //enable timer count
    TimerEnable(TIMER1_BASE, TIMER_A);
}

static void _timer1_reset(void)
{
    TimerDisable(TIMER1_BASE, TIMER_A);
    _timer1_config();
}

static void _orientation_send(float roll, float pitch, float yaw)
{
    static unsigned char orientation_delay = 0;

    if (orientation_delay == (MPU_6050_FREQUENCY / 8))//each 0.125 second
    {
        protocol_msg_send(ORIENTATION, 0, roll, pitch, yaw);
        orientation_delay = 0;
    }
    else
        orientation_delay++;
}

static void
_sensor_cb(float roll, float pitch, float yaw)
{
    static unsigned short command_counter = 0;
    short command_roll, command_pitch;
    int int_roll, int_pitch;

    if (!(flags & MOTOR_ON_FLAG))
    {
        if (flags & REQUESTING_ORIENTATION)
            _orientation_send(roll, pitch, yaw);
#if BLACKBOX_ENABLED
        blackbox_orientation_set(roll, pitch);
#endif
        return;
    }

    //only will update pid and motors values at COMMAND_FREQUENCY
    if (command_counter != COMMAND_COUNTER)
    {
        command_counter++;
        return;
    }
    command_counter = 0;

    int_roll = roll;
    int_pitch = pitch;
    command_roll = pid_update(&pid_roll, receiver_roll, int_roll);
    command_pitch = pid_update(&pid_pitch, receiver_pitch, int_pitch);

    motor_command_apply(command_roll, command_pitch, 0);

    _orientation_send(roll, pitch, yaw);
#if BLACKBOX_ENABLED
    blackbox_orientation_set(roll, pitch);
#endif
}

void agent_init(void)
{
    config_init();
    config_read(&configuration);

    throttle = 0;
    motors_init();
    procotol_init(_msg_cb);
    mpu6050_init(_sensor_cb);

    pid_init(&pid_roll, configuration.p_gaing, configuration.i_gaing, 0, 300, (1.0/COMMAND_FREQUENCY), 'r');
    pid_init(&pid_pitch, configuration.p_gaing, configuration.i_gaing, 0, 300, (1.0/COMMAND_FREQUENCY), 'p');

#if BLACKBOX_ENABLED
    blackbox_init();
    blackbox_gains_set(configuration.p_gaing, configuration.i_gaing);
#endif

    _timer1_config();

    min_throttle = motors_min_velocity_in_flight();
}

void timer1_500ms_interruption(void)
{
    //clear interruption, must be the first thing.
    //more info read documentation
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //remove movements
    receiver_pitch = receiver_roll = receiver_yaw = 0;

    //value do count down
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 2);

#if BLACKBOX_ENABLED
    static char send = 0;
    //each second
    if (send == 2)
    {
        blackbox_send();
        send = 0;
    }
    else
        send++;
#endif
}
