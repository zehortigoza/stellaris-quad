#include "main.h"
#include "agent.h"
#include "protocol.h"
#include "motors.h"
#include "mpu6050.h"

static unsigned short throttle = 0;

/*
 * bit0 = requesting orientation
 */
static unsigned char flags = 0;
#define REQUESTING_ORIENTATION GPIO_PIN_0
static unsigned char orientation_delay = 0;

static void _timer1_reset(void);

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
                    motors_velocity_set(throttle, throttle, throttle, throttle);
                    break;
                }
                case AXIS_X:
                {
                    //num [-3-3]
                    if (!num)
                        motors_velocity_set(throttle, throttle, throttle, throttle);
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
                        motors_velocity_set(throttle, throttle, throttle, throttle);
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
                        motors_velocity_set(throttle, throttle, throttle, throttle);
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

//called each 5ms
static void
_sensor_cb(float roll, float pitch, float yaw)
{
    if (flags & REQUESTING_ORIENTATION)
    {
        if (orientation_delay == 50)
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

    _timer1_config();
}

void timer1_500ms_interruption(void)
{
    //clear interruption, must be the first thing.
    //more info read documentarion
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

    //remove movements
    motors_velocity_set(throttle, throttle, throttle, throttle);

    //value do count down
    TimerLoadSet(TIMER1_BASE, TIMER_A, SysCtlClockGet() / 2);
}
