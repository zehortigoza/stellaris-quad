#include "main.h"
#include "pid.h"

void pid_init(pid_data *pid, float p_gaing, float i_gain, float d_gain, float integraded_error_limit, float time_period, char id)
{
    pid->p_gain = p_gaing;
    pid->i_gain = i_gain;
    pid->d_gain = d_gain;
    pid->integraded_error_limit = integraded_error_limit;
    pid->time_period = time_period;
    pid->last_error = pid->integrated_error = 0;
    pid->id = id;
}

float pid_update(pid_data *pid, int target, int current)
{
    double pid_error = target - current;
    double d_err, output;
    double p_output, i_output, d_output;

    //integral term
    pid->integrated_error += (pid_error * pid->time_period);
    i_output = pid->i_gain * pid->integrated_error;
    if (i_output < (-pid->integraded_error_limit))
        i_output = -pid->integraded_error_limit;
    else if (i_output > pid->integraded_error_limit)
        i_output = pid->integraded_error_limit;

    //derivative term
    d_err = (pid_error - pid->last_error) / pid->time_period;
    d_output = pid->d_gain * d_err;
    pid->last_error = pid_error;

    //proportional term
    p_output = pid->p_gain * pid_error;

    output = p_output + i_output + d_output;

#if BLACKBOX_ENABLED
    blackbox_pid_values(pid->id, target, pid_error, pid->integrated_error, p_output, i_output, d_output);
#endif

    return output;
}
