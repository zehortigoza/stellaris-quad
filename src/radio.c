#include "main.h"
#include "radio.h"

static radio_data_callback radio_data_func = NULL;

static char tx_buffer[MAX_STRING+1];
static int tx_index = 0;

static char rx_buffer[MAX_STRING];
static int rx_index = 0;

void radio_init(radio_data_callback func)
{
    radio_data_func = func;

    memset(&rx_buffer[0], 0, MAX_STRING);
    memset(&tx_buffer[0], 0, MAX_STRING);

    //Enable the peripherals
    //uart0 is connected to virtual pins
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    //Set GPIO A0 and A1 as UART pins.
    GPIOPinConfigure(GPIO_PB0_U1RX);
    GPIOPinConfigure(GPIO_PB1_U1TX);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0);
    GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_1);

    //Configure the UART for 9600, 8-N-1 operation.
    UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600,
                        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    //Enable the UART interrupt.
    UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_TX);
    IntEnable(INT_UART1);

    //level of FIFO, rise these values can lead to a message be hold
    UARTFIFOLevelSet(UART1_BASE, UART_FIFO_TX1_8, UART_FIFO_RX1_8);

    //disable hardware flow control
    UARTFlowControlSet(UART1_BASE, UART_FLOWCONTROL_NONE);

    UARTEnable(UART1_BASE);

    //normal priority
    IntPrioritySet(INT_UART1, 2);

    radio_send("init\n");//open tcp connection, this avoid trash been send
}

static void _tx_buffer_fill(void)
{
    unsigned char not_full = 1;
    while (tx_buffer[tx_index] && not_full)
    {
        not_full = UARTCharPutNonBlocking(UART1_BASE, tx_buffer[tx_index]);
        tx_index++;
    }

    if (!tx_buffer[tx_index])
        tx_index = 0;
}

unsigned char radio_send(char *txt)
{
    if (strlen(txt) > MAX_STRING)
        return 0;
    if (tx_index)
        return 0;

    sprintf(tx_buffer, "%s", txt);
    tx_index = 0;

    _tx_buffer_fill();
    return 1;
}

void
uart1_radio_interruption(void)
{
    unsigned long ulStatus;

    //Get the interrrupt status.
    ulStatus = UARTIntStatus(UART1_BASE, true);

    //clear interruption
    UARTIntClear(UART1_BASE, ulStatus);

    if (ulStatus & UART_INT_TX)
    {
        if (tx_index)
            _tx_buffer_fill();
    }

    if (ulStatus & UART_INT_RX)
    {
        char c;
        while(UARTCharsAvail(UART1_BASE))
        {
            c = (char)UARTCharGetNonBlocking(UART1_BASE);
            if ((c == '\n') || (rx_index == sizeof(rx_buffer)))
            {
                radio_data_func(rx_buffer);
                rx_index = 0;
            }
            else
            {
                rx_buffer[rx_index] = c;
                rx_index++;
            }
        }
    }
}
