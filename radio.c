#include "main.h"
#include "radio.h"

static radio_data_callback radio_data_func = NULL;

static char tx_buffer[MAX_STRING+1];
static char *tx_ptr = NULL;
static char tx_buffer_next[MAX_STRING+1];

static char rx_buffer[MAX_STRING];
static char *rx_ptr = rx_buffer;

void radio_init(radio_data_callback func)
{
    radio_data_func = func;

    memset(rx_buffer, 0, MAX_STRING);
    memset(tx_buffer, 0, MAX_STRING);

    //Enable the peripherals
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    //Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PA0_U0RX);
    GPIOPinConfigure(GPIO_PA1_U0TX);
    GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //Configure the UART for 9600, 8-N-1 operation.
    UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    //Enable the UART interrupt.
    IntEnable(INT_UART0);
    UARTIntEnable(UART0_BASE, UART_INT_RX | UART_INT_TX);

    //Enable UART keep running, when CPU is sleeping
    SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);

    UARTEnable(UART0_BASE);

    radio_send("init\n");//open tcp connection, this avoid trash been send
}

unsigned char radio_send(char *txt)
{
    if (strlen(txt) > MAX_STRING)
        return 0;

    if (tx_ptr)
    {
        if (!tx_buffer_next[0])
        {
            sprintf(tx_buffer_next, "%s", txt);
            return 1;
        }
        return 0;
    }

    sprintf(tx_buffer, "%s", txt);

    UARTCharPutNonBlocking(UART0_BASE, tx_buffer[0]);
    tx_ptr = tx_buffer + 1;

    return 1;
}

void
uart0_interruption(void)
{
    unsigned long ulStatus;

    //Get the interrrupt status.
    ulStatus = UARTIntStatus(UART0_BASE, true);

    //clear interruption
    UARTIntClear(UART0_BASE, ulStatus);

    if (ulStatus & UART_INT_TX)
    {
        if (*tx_ptr)
        {
            UARTCharPutNonBlocking(UART0_BASE, *tx_ptr);
            tx_ptr++;
        }
        else
        {
            //end of transmition
            if (tx_buffer_next[0])
            {
                sprintf(tx_buffer, "%s", tx_buffer_next);
                tx_buffer_next[0] = 0;
                UARTCharPutNonBlocking(UART0_BASE, tx_buffer[0]);
                tx_ptr = tx_buffer + 1;
            }
        }
    }

    if (ulStatus & UART_INT_RX)
    {
        char c;
        while(UARTCharsAvail(UART0_BASE))
        {
            c = (char)UARTCharGetNonBlocking(UART0_BASE);
            if (c == '\n' || (rx_ptr == rx_buffer + sizeof(rx_buffer)))
            {
                radio_data_func(rx_buffer);
                rx_ptr = rx_buffer;
                memset(rx_buffer, 0, MAX_STRING);
            }
            else
            {
                *rx_ptr = c;
                rx_ptr++;
            }
        }
    }
}
