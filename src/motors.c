#include "main.h"

static unsigned char motor_index = 0;

static unsigned int TRICKS_1MICRO;

#define TRICKS_2MS (TRICKS_1MICRO * 2000)

#define TRICKS_4MS (TRICKS_1MICRO * 4000)

//index 0 = front right, 1 = front left, 2 = back right, 3 = back left
static unsigned int motors_tricks[4];

//TODO remove:
//this in only used because I had not setup the other motors pins
static unsigned char temp_motor_state[4];

static unsigned char motor_state_get(unsigned char index)
{
    if (index == 0)
        return GPIOPinRead(GPIO_PORTB_BASE, GPIO_PIN_3);
    else
        return temp_motor_state[index];
}

static void motor_state_set(unsigned char index, unsigned char up)
{
    if (up)
    {
        if (index == 0)
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, GPIO_PIN_3);
        else
            temp_motor_state[index] = 1;
    }
    else
    {
        if (index == 0)
            GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);
        else
            temp_motor_state[index] = 0;
    }
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
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);
    //set pin to 3
    GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_3, 0);

    motors_tricks[0] = 0;
    motors_tricks[1] = 0;
    motors_tricks[2] = 0;
    motors_tricks[3] = 0;

    //TODO remove
    temp_motor_state[1] = 0;
    temp_motor_state[2] = 0;
    temp_motor_state[3] = 0;

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

static unsigned int
_velocity_set(unsigned short value)
{
    if (value > MAX_VALUE)
        value = MAX_VALUE;
    else if (value <  MIN_VALUE && value != 0)
        value = MIN_VALUE;
    return TRICKS_1MICRO * value;
}

/*
 * value in micro seconds
 * min rotation = 1000
 * max rotation = 2000
 * motor turn off = 0
 */
void motors_velocity_set(unsigned short fl, unsigned short fr, unsigned short bl, unsigned short br)
{
    motors_tricks[0] = _velocity_set(fl);
    motors_tricks[1] = _velocity_set(fr);
    motors_tricks[2] = _velocity_set(bl);
    motors_tricks[3] = _velocity_set(br);
}
