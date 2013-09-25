#ifndef _CONFIG_H
#define _CONFIG_H

//keep multiple of 4
typedef struct _config {
    int version;//4bytes
    float p_gaing;
    float i_gaing;//float == 4bytes
} config;

void config_write(config *conf);
void config_write_initial(void);
void config_read(config *conf);
void config_init(void);

#endif
