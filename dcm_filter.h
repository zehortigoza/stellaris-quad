#ifndef _DCM_FILTER_H
#define _DCM_FILTER_H

void dcm_init(int frequency);

void dcm_update(float gyro_x, float gyro_y, float gyro_z, float accel_x, float accel_y, float accel_z, float *roll, float *pitch, float *yaw);
#endif
