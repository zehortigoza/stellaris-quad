#include "main.h"
#include "agent.h"

int main(void)
{
    /*
     * Enable lazy stacking for interrupt handlers.  This allows floating-point
     * instructions to be used within interrupt handlers, but at the expense of
     * extra stack usage.
     */
    FPUEnable();
    FPULazyStackingEnable();

    //Precision Internal Oscillator 16mhz
    SysCtlClockSet(SYSCTL_SYSDIV_1|SYSCTL_USE_OSC|SYSCTL_OSC_INT);

    //Enable processor interrupts.
    IntMasterEnable();

    agent_init();

    //Put processor to sleep
    SysCtlSleep();
}
