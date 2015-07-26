#include "main.h"
#include "agent.h"
#include <string.h>
#include <stdio.h>

int main(void)
{
    char msg[512];
    int a;

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

    //debug
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
    GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

#if ENABLE_PRINTF
    //uart virtual com
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                        UART_CONFIG_PAR_NONE));
    UARTEnable(UART0_BASE);
#endif

    //agent_init();

    a = strlen("Test");
    a++;
    snprintf(msg, sizeof(msg), "^;z;1;\"gains\":{\"p\": %d,\"i\": %d};$\n", 0, 0);

    snprintf(msg, sizeof(msg), "^;z;1;\"gains\":{\"p\": %0.3f,\"i\": %0.3f};$\n", 0.0, 0.0);

    while(1)
        SysCtlDelay(100);

    return 0;
}
