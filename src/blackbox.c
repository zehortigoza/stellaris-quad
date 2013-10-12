#include "main.h"
#include "radio.h"

static char msg[MAX_STRING];

static unsigned int count = 0;

static unsigned short throttle = 0, m_fl = 0, m_fr = 0, m_bl = 0, m_br = 0;
static short command_pitch = 0, command_roll = 0;

static float roll = 0, pitch = 0;
static float pitch_integrated_error = 0, roll_integrated_error = 0;
static float pitch_i_output = 0, roll_i_output = 0;
static float pitch_p_output = 0, roll_p_output = 0;

void blackbox_init(void)
{

}

void blackbox_gains_set(float p, float i)
{
    snprintf(msg, sizeof(msg), "^;z;1;\"gains\":{\"p\": %0.3f,\"i\": %0.3f};$\n", p, i);
    radio_send(msg);
}

void blackbox_commands_set(unsigned short t, short cmd_pitch, short cmd_roll)
{
    throttle = t;
    command_pitch = cmd_pitch;
    command_roll = cmd_roll;
}

void blackbox_velocity_set(unsigned short fl, unsigned short fr, unsigned short bl, unsigned short br)
{
    m_fl = fl;
    m_fr = fr;
    m_bl = bl;
    m_br = br;
}

void blackbox_pid_values(char pid, float integrated_error, float i_output, float p_output)
{
    if (pid == 'r')
    {
        roll_integrated_error = integrated_error;
        roll_i_output = i_output;
        roll_p_output = p_output;
    }
    else
    {
        pitch_integrated_error = integrated_error;
        pitch_i_output = i_output;
        pitch_p_output = p_output;
    }
}

void blackbox_orientation_set(float r, float p)
{
    roll = r;
    pitch = p;
}

void blackbox_send(void)
{
    sprintf(msg, "^;z;1;");
    snprintf(msg, sizeof(msg), "%s\"roll\":{\"a\": %0.3f,\"ie\": %0.3f,\"io\": %0.3f,\"po\": %0.3f,\"o\": %d},", msg, roll, roll_integrated_error, roll_i_output, roll_p_output, command_roll);
    snprintf(msg, sizeof(msg), "%s\"pitch\":{\"a\": %0.3f,\"ie\": %0.3f,\"io\": %0.3f,\"po\": %0.3f,\"o\": %d},", msg, pitch, pitch_integrated_error, pitch_i_output, pitch_p_output, command_pitch);
    snprintf(msg, sizeof(msg), "%s\"motors\":{\"fl\": %d,\"fr\": %d,\"bl\": %d,\"br\": %d},", msg, m_fl, m_fr, m_bl, m_br);
    count++;
    snprintf(msg, sizeof(msg), "%s\"misc\":{\"t\": %d,\"c\": %d};$\n", msg, throttle, count);
    radio_send(msg);
}
