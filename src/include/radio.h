#ifndef RADIO_H
#define RADIO_H

#define MAX_STRING 300

typedef void (*radio_data_callback)(const char *text);

void radio_init(radio_data_callback func);
unsigned char radio_send(const char *txt);

void uart1_radio_interruption(void);

#endif
