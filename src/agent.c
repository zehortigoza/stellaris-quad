#include "main.h"
#include "agent.h"
#include "protocol.h"
#include "motors.h"
#include "mpu6050.h"
#include "pid.h"

#define MPU_6050_FREQUENCY 500

//data received from controller
static unsigned short throttle = 0;
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
 */
static unsigned char flags = 0;
#define REQUESTING_ORIENTATION GPIO_PIN_0
static unsigned char orientation_delay = 0;

static void _timer1_reset(void);

#define YAW_DIRECTION 1//or -1 to other side

//to quad X mode
static void motor_command_apply(short command_roll, short command_pitch, short command_yaw)
{
    unsigned short fl, fr, bl, br;
    fl  = throttle - command_pitch + command_roll - (YAW_DIRECTION * command_yaw);
    fr = throttle - command_pitch - command_roll + (YAW_DIRECTION * command_yaw);
    bl = throttle + command_pitch + command_roll + (YAW_DIRECTION * command_yaw);
    br = throttle + command_pitch - command_roll - (YAW_DIRECTION * command_yaw);
    motors_velocity_set(fl, fr, bl, br);
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

            axis = va_arg(ap, int);
            num = va_arg(ap, unsigned int);

            switch (axis)
            {
                case AXIS_Z:
                {
                    throttle = num;
                    receiver_pitch = receiver_roll = receiver_yaw = 0;
                    motors_velocity_set(throttle, throttle, throttle, throttle);
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
            unsigned char enable = va_arg(ap, int);
            if (enable)
                flags |= REQUESTING_ORIENTATION;
            else
                flags &= ~REQUESTING_ORIENTATION;
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
    //enable timer1
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
    //configure to count down
    TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
    //value to count down
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 2);
    //enable interruption in timer1
    IntEnable(INT_TIMER1A);
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

static void
_sensor_cb(float roll, float pitch, float yaw)
{
    short command_roll, command_pitch;

    command_roll = pid_update(&pid_roll, receiver_roll, roll);
    command_pitch = pid_update(&pid_pitch, receiver_pitch, pitch);

    motor_command_apply(command_roll, command_pitch, 0);

    if (flags & REQUESTING_ORIENTATION)
    {
        if (orientation_delay == (MPU_6050_FREQUENCY/4))//each 0.25 second
        {
            protocol_msg_send(ORIENTATION, 0, roll, pitch, yaw);
            orientation_delay = 0;
        }
        else
            orientation_delay++;
    }
}

void agent_init(void)
{
    throttle = 0;
    motors_init();
    procotol_init(_msg_cb);
    mpu6050_init(_sensor_cb);

    pid_init(&pid_roll, 50, 2, 0, 500, (1.0/MPU_6050_FREQUENCY));
    pid_init(&pid_pitch, 50, 2, 0, 500, (1.0/MPU_6050_FREQUENCY));

    _timer1_config();
}

void timer1_500ms_interruption(void)
{
    //clear interruption, must be the first thing.
    //more info read documentarion
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //remove movements
    receiver_pitch = receiver_roll = receiver_yaw = 0;

    //value do count down
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 2);
}
