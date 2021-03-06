#ifndef BLACKBOX_H
#define BLACKBOX_H

void blackbox_init(void);
void blackbox_gains_set(float p, float i);
void blackbox_commands_set(unsigned short t, short cmd_pitch, short cmd_roll);
void blackbox_velocity_set(unsigned short fl, unsigned short fr, unsigned short bl, unsigned short br);
void blackbox_pid_values(char pid, int target, float error, float integrated_error, float p_output, float i_output, float d_output);
void blackbox_orientation_set(float r, float p);
void blackbox_send(void);

#endif
