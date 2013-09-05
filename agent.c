#include "main.h"
#include "agent.h"
#include "protocol.h"
#include "motors.h"

static unsigned short z_value = 0;

static void _timer1_reset(void);

/*
 BL\        /FL
    \      /
     \    /         x
      \  /          ^
       \/           |
       /\           |
      /  \    y<-----
     /    \
    /      \
 BR/        \FR
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
                    z_value = num;
                    motors_velocity_set(z_value, z_value, z_value, z_value);
                    break;
                }
                case AXIS_X:
                {
                    //num [-3-3]
                    if (!num)
                        motors_velocity_set(z_value, z_value, z_value, z_value);
                    else if (num < 0)//left
                    {
                        //TODO calc
                        //slow rotations on both left motors
                        //fast rotations on both right motors
                    } else//right
                    {
                        //TODO calc
                        //fast rotations on both left motors
                        //slow rotations on both right motors
                    }
                    //calc all for values basead variables but dont set
                    //only send to motors
                    break;
                }
                case AXIS_Y:
                {
                    //num [-3-3]
                    if (!num)
                        motors_velocity_set(z_value, z_value, z_value, z_value);
                    else if (num > 0)//front
                    {
                        //slow rotations on both front motors
                        //fast rotations on both back motors
                    }
                    else
                    {
                        //fast rotations on both front motors
                        //slow rotations on both back motors
                    }
                    break;
                }
                case AXIS_ROTATE:
                {
                    if (!num)
                        motors_velocity_set(z_value, z_value, z_value, z_value);
                    else if (num > 0)
                    {
                        //fast rotations on fl and br
                        //slow rotations on fr and bl
                    }
                    else
                    {
                        //slow rotations on fl and br
                        //fast rotations on fr and bl
                    }
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

//called each 5ms
static void
_sensor_cb(float roll, float pitch, float yaw)
{
}

void agent_init(void)
{
    z_value = 0;
    motors_init();
    procotol_init(_msg_cb);
    mpu6050_init(_sensor_cb);

    _timer1_config();
}

void timer1_500ms_interruption(void)
{
    //clear interruption, must be the first thing.
    //more info read documentarion
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //remove movements
    motors_velocity_set(z_value, z_value, z_value, z_value);

    //value do count down
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 2);
}
