#include "main.h"
#include "mpu6050.h"
#include "i2c.h"
//#include "MadgwickAHRS.h"
#include "MahonyAHRS.h"
#include <math.h>

#define MPU6050_ADDR 0x0068//ADO = low

#define SAMPLES_TO_CALIBRATE 500*3//3s
static short _calibrate_euler = 0;
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
}

static void _accel_config_cb(void)
{
    //enable data ready interruption
    i2c_reg_uchar_write(REG_INT_ENABLE, GPIO_PIN_0, _int_enable_cb);
}

static void _gyro_config_cb(void)
{
    //acell Full Scale Range = +-4g
    i2c_reg_uchar_write(REG_ACCEL_CONFIG, GPIO_PIN_3, _accel_config_cb);
}

static void _sampler_divider_cb(void)
{
    //gyro Full Scale Range = +-500 º/s
    i2c_reg_uchar_write(REG_GYRO_CONFIG, GPIO_PIN_3, _gyro_config_cb);
}

static void _config_cb(void)
{
    //divider = 1+1 = 2; 1k/2=500hz
    i2c_reg_uchar_write(REG_SMPRT_DIV, 1, _sampler_divider_cb);
}

static void _pw_mgmt_cb(void)
{
    //set Gyroscope Output Rate = 1k and config the low pass filter.
    i2c_reg_uchar_write(REG_CONFIG, GPIO_PIN_0, _config_cb);
}

static void _who_am_i_cb(unsigned char *data)
{
    if (data[0] != MPU6050_ADDR)
        return;
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

    _calibrate_euler = SAMPLES_TO_CALIBRATE;
    _calibrate_gyro = SAMPLES_TO_CALIBRATE;

    //second highest priority
    IntPrioritySet(INT_GPIOA, 1);

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

#define SCALE(val, val2) (((float)val) / val2)

static void _scale_calc(mpu6050_raw *raw, float *gx, float *gy, float *gz, float *ax, float *ay, float *az)
{
    //dont need scale accel, fusion algorithm work with raw values
    *ax = raw->value.x_accel;
    *ay = raw->value.y_accel;
    *az = raw->value.z_accel;

    *gx = SCALE(raw->value.x_gyro, GYRO_SCALE);
    *gy = SCALE(raw->value.y_gyro, GYRO_SCALE);
    *gz = SCALE(raw->value.z_gyro, GYRO_SCALE);
}

static float _deg2rad(float deg)
{
    //0.0174532925 = M_PI / 180.0
    return deg * 0.0174532925;
}

static float _rad2deg(float rad)
{
    return rad * 57.2957795;
}

static void _euler_angles_calc(float *roll, float *pitch, float *yaw, float q0, float q1, float q2, float q3)
{
    //FIXME: inverting roll and pitch
    //check if this is right before start flight
    *pitch = atan2(2 * (q0 * q1 + q2 * q3), 1 - 2 * (q1 * q1 + q2 * q2));
    *roll = asin(2 * (q0 * q2 - q1 * q3));
    *yaw = atan2(2 * (q0 * q3 + q1 * q2), 1 - 2 * (q2 * q2 + q3 * q3));

    *roll = _rad2deg(*roll);
    *pitch = _rad2deg(*pitch) * -1;
    *yaw = _rad2deg(*yaw);
}

static unsigned char _remove_euler_offsets(float *roll, float *pitch, float *yaw)
{
    static float offset_pitch, offset_roll, offset_yaw;

    if (_calibrate_euler)
    {
        if (_calibrate_euler == SAMPLES_TO_CALIBRATE)
        {
            offset_pitch = *pitch;
            offset_roll = *roll;
            offset_yaw = *yaw;
        }
        else
        {
            offset_pitch = (offset_pitch + *pitch) / 2;
            offset_roll = (offset_roll + *roll) / 2;
            offset_yaw = (offset_yaw + *yaw) / 2;
        }
        _calibrate_euler--;
        return 1;
    }
    else
    {
        *roll -= offset_roll;
        *pitch -= offset_pitch;
        *yaw -= offset_yaw;
        return 0;
    }
}

static unsigned char _remove_gyro_offsets(mpu6050_raw *raw)
{
    static int16_t x_offset, y_offset, z_offset;

    if (_calibrate_gyro)
    {
        if (_calibrate_gyro == SAMPLES_TO_CALIBRATE)
        {
            x_offset = raw->value.x_gyro;
            y_offset = raw->value.y_gyro;
            z_offset = raw->value.z_gyro;
        }
        else
        {
            x_offset = (x_offset + raw->value.x_gyro) / 2;
            y_offset = (y_offset + raw->value.y_gyro) / 2;
            z_offset = (z_offset + raw->value.z_gyro) / 2;
        }
        _calibrate_gyro--;
        return 1;
    }
    else
    {
        raw->value.x_gyro -= x_offset;
        raw->value.y_gyro -= y_offset;
        raw->value.z_gyro -= z_offset;
        return 0;
    }
}

static void _raw_cb(unsigned char *data)
{
    mpu6050_raw raw;
    float gx, gy, gz, ax, ay, az;
    float roll = 0, pitch = 0, yaw = 0;

    memcpy(&raw, data, sizeof(raw));
    _raw_swap(&raw);

    //while calibrating, do not update fusion algorithm
    if (_remove_gyro_offsets(&raw))
        return;

    _scale_calc(&raw, &gx, &gy, &gz, &ax, &ay, &az);

    gx = _deg2rad(gx);
    gy = _deg2rad(gy);
    gz = _deg2rad(gz);

    //MadgwickAHRSupdateIMU(gx, gy, gz, ax, ay, az, &roll, &pitch, &yaw, _euler_angles_calc);
    MahonyAHRSupdateIMU(gx, gy, gz, ax, ay, az, &roll, &pitch, &yaw, _euler_angles_calc);

    //while calibrating, do not send values to agent module
    if (_remove_euler_offsets(&roll, &pitch, &yaw))
        return;

    sensor_cb(roll, pitch, yaw);
}

static void _status_cb(unsigned char *data)
{
    if (data[0] & GPIO_PIN_0)
    {
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

    i2c_reg_read(REG_INT_STATUS, 1, _status_cb);
}
