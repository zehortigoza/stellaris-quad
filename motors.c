#include "main.h"

static unsigned char motor_index = 0;

#define TRICKS_1MS (SysCtlClockGet() / 1000)

//same as (SysCtlClockGet() / 1000) * 2
#define TRICKS_2MS (SysCtlClockGet() / 500)

//same as (SysCtlClockGet() / 1000) * 4
#define TRICKS_4MS (SysCtlClockGet() / 250)

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
    //more info read documentarion
    TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

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
    SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //config pin to output
    GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_3);
    //set pin to 0
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
    TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);
    //enable interruption in timer
    TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
    //load value to count down
    TimerLoadSet(TIMER0_BASE, TIMER_A, TRICKS_2MS);
    //enable timer count
    TimerEnable(TIMER0_BASE, TIMER_A);

    return 0;
}

static unsigned int _tricks_calc(unsigned char value)
{
    if (!value)
        return 0;
    return ((TRICKS_1MS * value) / 255) + TRICKS_1MS;
}

/*
 * 255 = 2ms up
 * 1 = 1ms up
 * 0 = motor turn off
 */
void motors_velocity_set(unsigned char fr, unsigned char fl, unsigned char br, unsigned char bl)
{
    motors_tricks[0] = _tricks_calc(fr);
    motors_tricks[1] = _tricks_calc(fl);
    motors_tricks[2] = _tricks_calc(br);
    motors_tricks[3] = _tricks_calc(bl);
}

static unsigned char _power_calc(unsigned short count)
{
    if (!count)
        return 0;
    return ((count - TRICKS_1MS) * 255) / TRICKS_1MS;
}

void motors_velocity_get(unsigned char *fr, unsigned char *fl, unsigned char *br, unsigned char *bl)
{
    if (fr)
        *fr = _power_calc(motors_tricks[0]);
    if (fl)
        *fl = _power_calc(motors_tricks[1]);
    if (br)
        *br = _power_calc(motors_tricks[2]);
    if (bl)
        *bl = _power_calc(motors_tricks[3]);
}
