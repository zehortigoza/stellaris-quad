#ifndef MOTORS_H
#define MOTORS_H

int motors_init(void);
void motors_velocity_set(unsigned short fl, unsigned short fr, unsigned short bl, unsigned short br, unsigned char fly_mode);

#endif
