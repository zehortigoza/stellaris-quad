#ifndef _PID_H
#define _PID_H

typedef struct _pid_data {
    float p_gain, i_gain, d_gain;
    float last_error;
    float integrated_error;
    float integraded_error_limit;
    float time_period;
    char id;
} pid_data;

void pid_init(pid_data *pid, float p_gaing, float i_gain, float d_gain, float integraded_error_limit, float time_period, char id);

float pid_update(pid_data *pid, int target, int current);

#endif
