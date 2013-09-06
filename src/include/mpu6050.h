#ifndef MPU6050_H
#define MPU6050_H

typedef void (*sensor_data_ready_callback)(float roll, float pitch, float yaw);

int mpu6050_init(sensor_data_ready_callback func);

#endif
