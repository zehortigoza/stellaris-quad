#include "main.h"

static unsigned char motor_index = 0;

static unsigned int TRICKS_1MICRO;

#define TRICKS_2MS (TRICKS_1MICRO * 2000)

#define TRICKS_4MS (TRICKS_1MICRO * 4000)

//index 0 = front left, 1 = front right, 2 = back left, 3 = back right
static unsigned int motors_tricks[4];

#define FL GPIO_PIN_4
#define FR GPIO_PIN_2
#define BL GPIO_PIN_6
#define BR GPIO_PIN_7
static unsigned char _motors_pins[] = {FL, FR, BL, BR};

static unsigned char motor_state_get(unsigned char index)
{
    return GPIOPinRead(GPIO_PORTB_BASE, _motors_pins[index]);
}

static void motor_state_set(unsigned char index, unsigned char up)
{
    if (up)
        GPIOPinWrite(GPIO_PORTB_BASE, _motors_pins[index], _motors_pins[index]);
    else
        GPIOPinWrite(GPIO_PORTB_BASE, _motors_pins[index], 0);
}

void timer0_motors_interruption(void)
{
    //clear interruption, must be the first thing.
    //more info read documentation
    TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);

    if (motor_state_get(motor_index))
    {
        motor_state_set(motor_index, 0);
        TimerLoadSet(TIMER0_BASE, TIMER_A, TRICKS_2MS);
    }
    else
    {
        if (motors_tricks[motor_index])
        {
            motor_state_set(motor_index, 1);
            TimerLoadSet(TIMER0_BASE, TIMER_A, motors_tricks[motor_index]);
            return;
        }
        else
            TimerLoadSet(TIMER0_BASE, TIMER_A, TRICKS_4MS);
    }

    motor_index++;
    if (motor_index == 4)
        motor_index = 0;
}

int motors_init(void)
{
    TRICKS_1MICRO = SysCtlClockGet() / 1000000;

    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //config pin to output
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, FL);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, FR);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, BL);
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, BR);
    //set pin to 3
    GPIOPinWrite(GPIO_PORTB_BASE, FL|FR|BL|BR, 0);

    motors_tricks[0] = 0;
    motors_tricks[1] = 0;
    motors_tricks[2] = 0;
    motors_tricks[3] = 0;

    //configure to count down
    TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
    //enable interruption in timer
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    //set counter to timer0
    TimerLoadSet(TIMER0_BASE, TIMER_A, TRICKS_2MS);
    //enable interruption in timer0
    IntEnable(INT_TIMER0A);
    //enable interruption in timer
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    //enable timer count
    TimerEnable(TIMER0_BASE, TIMER_A);

    //highest priority
    IntPrioritySet(INT_TIMER0A, 0);

    return 0;
}

#define MIN_VALUE 1000
#define MAX_VALUE 2000

#define MIN_IN_FLY 1150
#define MAX_IN_FLY 1950

static unsigned int
_velocity_set(unsigned short value, unsigned short min, unsigned short max)
{
    if (value > max)
        value = max;
    else if (value <  min && value != 0)
        value = min;
    return TRICKS_1MICRO * value;
}

/*
 * value in micro seconds
 * min rotation = 1000
 * max rotation = 2000
 * motor turn off = 0
 */
void motors_velocity_set(unsigned short fl, unsigned short fr, unsigned short bl, unsigned short br, unsigned char fly_mode)
{
    unsigned short min = fly_mode ? MIN_IN_FLY : MIN_VALUE;
    unsigned short max = fly_mode ? MAX_IN_FLY : MAX_VALUE;
    motors_tricks[0] = _velocity_set(fl, min, max);
    motors_tricks[1] = _velocity_set(fr, min, max);
    motors_tricks[2] = _velocity_set(bl, min, max);
    motors_tricks[3] = _velocity_set(br, min, max);
}
