#include "main.h"
#include "pid.h"

void pid_init(pid_data *pid, float p_gaing, float i_gain, float d_gain, float integraded_error_limit, float time_period)
{
    pid->p_gain = p_gaing;
    pid->i_gain = i_gain;
    pid->d_gain = d_gain;
    pid->integraded_error_limit = integraded_error_limit;
    pid->time_period = time_period;
    pid->last_error = pid->integrated_error = 0;
}

float pid_update(pid_data *pid, float target, float current)
{
    float error = target - current;
    float d_err;
    float output;
    float i_output;
#if BLACKBOX_ENABLED
    static unsigned char is_roll_pid = 1;
#endif

    //integral term
    pid->integrated_error += (error * pid->time_period);

    i_output = pid->i_gain * pid->integrated_error;

    if (i_output < (-pid->integraded_error_limit))
        i_output = -pid->integraded_error_limit;
    else if (i_output > pid->integraded_error_limit)
        i_output = pid->integraded_error_limit;

    //derivative term
    d_err = (error - pid->last_error) / pid->time_period;

    output = (pid->p_gain * error) +
             (pid->d_gain * d_err) +
             i_output;

    pid->last_error = error;

#if BLACKBOX_ENABLED
    blackbox_pid_values(is_roll_pid ? 'r' : 'p', pid->integrated_error, i_output, pid->p_gain * error);
    is_roll_pid = !is_roll_pid;
#endif

    return output;
}
