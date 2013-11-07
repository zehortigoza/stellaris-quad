#ifndef MOTORS_H
#define MOTORS_H

int motors_init(void);
void motors_velocity_set(unsigned short fl, unsigned short fr, unsigned short bl, unsigned short br, unsigned char fly_mode);

unsigned short motors_min_velocity_in_flight(void);

void timer0_motors_interruption(void);

#endif
