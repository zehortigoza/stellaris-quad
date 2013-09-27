#include "main.h"
#include "i2c.h"

#define I2C_BUFFER_SIZE 32

#define BUS_BUSY 0x0001 //BIT0
#define WRITE_MODE 0x0002 //BIT1
#define REGISTER_SENT 0x0004 //BIT2
#define CONNECTION_RESTARDED 0x0008 //BIT3
#define BUS_INITIALIZED 0x0010 //BIT4
#define WAITING_FINISH 0x00000020 //BIT5
static char state = 0;

static unsigned char slave_address;

static i2c_data_read_callback data_read_func;
static i2c_data_write_callback data_write_func;
static unsigned char buffer[I2C_BUFFER_SIZE];
static unsigned char *ptr_data;
static int data_size = 0;

#define I2C_RECEIVE true
#define I2C_TRANSMIT false

void i2c_bus_init(unsigned char address)
{
    if (state & BUS_BUSY)
        return;
    //enable i2c1 and gpio B
    SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

    GPIOPinConfigure(GPIO_PA6_I2C1SCL);
    GPIOPinConfigure(GPIO_PA7_I2C1SDA);

    GPIOPinTypeI2CSCL(GPIO_PORTA_BASE, GPIO_PIN_6);
    GPIOPinTypeI2C(GPIO_PORTA_BASE, GPIO_PIN_7);

    I2CMasterInitExpClk(I2C1_MASTER_BASE, SysCtlClockGet(), false);

    I2CMasterIntEnable(I2C1_MASTER_BASE);
    IntEnable(INT_I2C1);

    //second highest priority
    IntPrioritySet(INT_I2C1, 1);

    slave_address = address;
    state = BUS_INITIALIZED;
}

void i2c_reg_read(unsigned char reg, unsigned int size, i2c_data_read_callback func)
{
    if (!(state & BUS_INITIALIZED))
        return;
    if (state & BUS_BUSY)
        return;

    data_size = size;
    ptr_data = buffer;
    data_read_func = func;
    state |= BUS_BUSY;

    I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, slave_address, I2C_TRANSMIT);
    I2CMasterDataPut(I2C1_MASTER_BASE, reg);
    I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_SINGLE_SEND);
}

void i2c_reg_uchar_write(unsigned char reg, unsigned char value, i2c_data_write_callback func)
{
    if (!(state & BUS_INITIALIZED))
        return;
    if (state & BUS_BUSY)
        return;

    data_size = 1;
    ptr_data = buffer;
    buffer[0] = value;
    data_write_func = func;
    state |= BUS_BUSY + WRITE_MODE;

    I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, slave_address, I2C_TRANSMIT);
    I2CMasterDataPut(I2C1_MASTER_BASE, reg);
    I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_START);
}

void i2c1_mpu6050_interruption(void)
{
    I2CMasterIntClear(I2C1_MASTER_BASE);

    if (state & WRITE_MODE)
    {
        if (state & WAITING_FINISH)
        {
            state = BUS_INITIALIZED;
            data_write_func();
        }
        else if (data_size == 1)
        {
            I2CMasterDataPut(I2C1_MASTER_BASE, *ptr_data);
            I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);
            state |= WAITING_FINISH;
        }
        else
        {
            I2CMasterDataPut(I2C1_MASTER_BASE, *ptr_data);
            I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_SEND_CONT);
            ptr_data++;
            data_size--;
        }
    }
    else
    {
        if (!(state & CONNECTION_RESTARDED))
        {
            I2CMasterSlaveAddrSet(I2C1_MASTER_BASE, slave_address, I2C_RECEIVE);
            I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_START);

            state |= CONNECTION_RESTARDED;
        }
        else if (state & WAITING_FINISH)
        {
            state = BUS_INITIALIZED;
            data_read_func(buffer);
        }
        else
        {
            *ptr_data = I2CMasterDataGet(I2C1_MASTER_BASE);
            data_size--;
            ptr_data++;
            if (!data_size)
            {
                state |= WAITING_FINISH;
                I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
            }
            else
                I2CMasterControl(I2C1_MASTER_BASE, I2C_MASTER_CMD_BURST_RECEIVE_CONT);
        }
    }
}
