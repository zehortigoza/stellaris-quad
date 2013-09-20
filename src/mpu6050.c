#include "main.h"
#include "mpu6050.h"
#include "i2c.h"
#include "dcm_filter.h"

#define M_PI 3.14159265358979323846

#define MPU6050_ADDR 0x0068//ADO = low

#define SAMPLES_TO_CALIBRATE 400//2s
static short _calibrate_gyro = 0;

#define ACCEL_SCALE 8192.0
#define GYRO_SCALE 65.5

//registers
#define REG_WHO_AM_I 0x75
#define REG_PWR_MGMT_1 0x6B
#define REG_CONFIG 0x1A
#define REG_SMPRT_DIV 0x19
#define REG_GYRO_CONFIG 0x1B
#define REG_ACCEL_CONFIG 0x1C
#define REG_INT_ENABLE 0x38
#define REG_INT_STATUS 0x3A
#define REG_ACCEL_XOUT_H 0x3B

typedef union _mpu6050_raw
{
    struct
    {
        unsigned char x_accel_h;
        unsigned char x_accel_l;
        unsigned char y_accel_h;
        unsigned char y_accel_l;
        unsigned char z_accel_h;
        unsigned char z_accel_l;
        unsigned char t_h;
        unsigned char t_l;
        unsigned char x_gyro_h;
        unsigned char x_gyro_l;
        unsigned char y_gyro_h;
        unsigned char y_gyro_l;
        unsigned char z_gyro_h;
        unsigned char z_gyro_l;
  } reg;
  struct
  {
      int16_t x_accel;
      int16_t y_accel;
      int16_t z_accel;
      int16_t temperature;
      int16_t x_gyro;
      int16_t y_gyro;
      int16_t z_gyro;
  } value;
} mpu6050_raw;

static sensor_data_ready_callback sensor_cb;

static void _int_enable_cb(void)
{
    //all registers configured

    //200hz as set in reg REG_SMPRT_DIV
    dcm_init(200);
}

static void _accel_config_cb(void)
{
    i2c_bus_init(MPU6050_ADDR);
    //enable data ready interruption
    i2c_reg_uchar_write(REG_INT_ENABLE, GPIO_PIN_0, _int_enable_cb);
}

static void _gyro_config_cb(void)
{
    i2c_bus_init(MPU6050_ADDR);
    //acell Full Scale Range = +-4g
    i2c_reg_uchar_write(REG_ACCEL_CONFIG, GPIO_PIN_3, _accel_config_cb);
}

static void _sampler_divider_cb(void)
{
    i2c_bus_init(MPU6050_ADDR);
    //gyro Full Scale Range = +-500 º/s
    i2c_reg_uchar_write(REG_GYRO_CONFIG, GPIO_PIN_3, _gyro_config_cb);
}

static void _config_cb(void)
{
    i2c_bus_init(MPU6050_ADDR);
    //divider = 4+1 = 5; 1k/5=200hz
    i2c_reg_uchar_write(REG_SMPRT_DIV, 4, _sampler_divider_cb);
}

static void _pw_mgmt_cb(void)
{
    i2c_bus_init(MPU6050_ADDR);
    //set Gyroscope Output Rate = 1k and config the low pass filter.
    i2c_reg_uchar_write(REG_CONFIG, GPIO_PIN_0, _config_cb);
}

static void _who_am_i_cb(unsigned char *data)
{
    if (data[0] != MPU6050_ADDR)
        return;
    i2c_bus_init(MPU6050_ADDR);
    //get out of sleep and clock from gyro z
    i2c_reg_uchar_write(REG_PWR_MGMT_1, 3, _pw_mgmt_cb);
}

int mpu6050_init(sensor_data_ready_callback func)
{
    sensor_cb = func;

    i2c_bus_init(MPU6050_ADDR);
    i2c_reg_read(REG_WHO_AM_I, 1, _who_am_i_cb);

    //enable GPIOA peripheral
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
    //set PIN5 of GPIOA as input
    GPIOPinTypeGPIOInput(GPIO_PORTA_BASE, GPIO_PIN_5);
    //enable interruption in GPIOA
    IntEnable(INT_GPIOA);
    //enable interruption in PIN 5 of GPIOA
    GPIOPinIntEnable(GPIO_PORTA_BASE, GPIO_PIN_5);
    //trigger interruption only in rising edge
    GPIOIntTypeSet(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_RISING_EDGE);

    _calibrate_gyro = SAMPLES_TO_CALIBRATE;

    return 0;
}

static inline void _swap(unsigned char *h, unsigned char *l)
{
    unsigned char temp;

    temp = *h;
    *h = *l;
    *l = temp;
}

static void _raw_swap(mpu6050_raw *raw)
{
    _swap(&raw->reg.t_h, &raw->reg.t_l);
    _swap(&raw->reg.x_accel_h, &raw->reg.x_accel_l);
    _swap(&raw->reg.y_accel_h, &raw->reg.y_accel_l);
    _swap(&raw->reg.z_accel_h, &raw->reg.z_accel_l);
    _swap(&raw->reg.x_gyro_h, &raw->reg.x_gyro_l);
    _swap(&raw->reg.y_gyro_h, &raw->reg.y_gyro_l);
    _swap(&raw->reg.z_gyro_h, &raw->reg.z_gyro_l);
}

static void _offset_remove(mpu6050_raw *raw)
{
    static int16_t gx_offset = 0, gy_offset = 0, gz_offset = 0;

    if (_calibrate_gyro)
    {
        if (!gx_offset && !gy_offset && !gz_offset)
        {
            gx_offset = raw->value.x_gyro;
            gy_offset = raw->value.y_gyro;
            gz_offset = raw->value.z_gyro;
        }
        else
        {
            gx_offset = (raw->value.x_gyro + gx_offset) / 2;
            gy_offset = (raw->value.y_gyro + gy_offset) / 2;
            gz_offset = (raw->value.z_gyro + gz_offset) / 2;
        }
        _calibrate_gyro--;
    }

    raw->value.x_gyro -= gx_offset;
    raw->value.y_gyro -= gy_offset;
    raw->value.z_gyro -= gz_offset;
}

#define SCALE(val, val2) (((float)val) / val2)

static void _scale_calc(mpu6050_raw *raw, float *gx, float *gy, float *gz, float *ax, float *ay, float *az)
{
    *ax = SCALE(raw->value.x_accel, ACCEL_SCALE);
    *ay = SCALE(raw->value.y_accel, ACCEL_SCALE);
    *az = SCALE(raw->value.z_accel, ACCEL_SCALE);

    *gx = SCALE(raw->value.x_gyro, GYRO_SCALE);
    *gy = SCALE(raw->value.y_gyro, GYRO_SCALE);
    *gz = SCALE(raw->value.z_gyro, GYRO_SCALE);
}

static void _raw_cb(unsigned char *data)
{
    mpu6050_raw raw;
    float gx, gy, gz, ax, ay, az;
    float roll, pitch, yaw;

    memcpy(&raw, data, sizeof(raw));
    _raw_swap(&raw);
    _offset_remove(&raw);

    //while calibrating, do not start update dcm
    if (_calibrate_gyro)
        return;
    _scale_calc(&raw, &gx, &gy, &gz, &ax, &ay, &az);

    //degrees/second to rads/second
    gx = gx * (M_PI / 180.0);
    gy = gy * (M_PI / 180.0);
    gz = gz * (M_PI / 180.0);

    dcm_update(gx, gy, gz, ax, ay, az, &roll, &pitch, &yaw);

    sensor_cb(roll, pitch, yaw);
}

static void _status_cb(unsigned char *data)
{
    if ((data[0] & GPIO_PIN_6) || (data[0] & GPIO_PIN_4) || (data[0] & GPIO_PIN_3))
        printf("another status interruption");

    if (data[0] & GPIO_PIN_0)
    {
        i2c_bus_init(MPU6050_ADDR);
        i2c_reg_read(REG_ACCEL_XOUT_H, sizeof(mpu6050_raw), _raw_cb);
    }
}

void gpioa_mpu6050_interruption(void)
{
    if (!GPIOPinIntStatus(GPIO_PORTA_BASE, GPIO_PIN_5))
    {
        return;
    }

    GPIOPinIntClear(GPIO_PORTA_BASE, GPIO_PIN_5);

    i2c_bus_init(MPU6050_ADDR);
    i2c_reg_read(REG_INT_STATUS, 1, _status_cb);
}
