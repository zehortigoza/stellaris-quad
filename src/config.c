#include "main.h"
#include "config.h"

//as memory is filled with "1"s, to valid that a version is on flash
//only choose version that is not full of zeros
#define VERSION 10
#define DEFAULT_P_GAIN 5
#define DEFAULT_I_GAIN 0.005

#define FLASH_OFFSET (0x0003FFFC-sizeof(config))

//only set this to 1 when develop was finish
//write flash every time stellaris is programed
//will decrease the flash life.
#define ALLOW_WRITE_IN_FLASH 1

void config_write(config *conf)
{
#if ALLOW_WRITE_IN_FLASH
    FlashErase(FLASH_OFFSET);
    FlashProgram((unsigned long *)conf, (unsigned long)FLASH_OFFSET, sizeof(config));
#endif
}

void config_write_initial(void)
{
    config conf;
    conf.version = VERSION;
    conf.p_gaing = DEFAULT_P_GAIN;
    conf.i_gaing = DEFAULT_I_GAIN;
    config_write(&conf);
}

void config_read(config *conf)
{
    config *internal = (config *)FLASH_OFFSET;
    memcpy(conf, internal, sizeof(config));
    if (conf->version != VERSION)
    {
#if ALLOW_WRITE_IN_FLASH
        config_write_initial();
        memcpy(conf, internal, sizeof(config));
#else
        conf->version = VERSION;
        conf->p_gaing = DEFAULT_P_GAIN;
        conf->i_gaing = DEFAULT_I_GAIN;
#endif
    }
}

void config_init(void)
{
    FlashUsecSet(SysCtlClockGet() / 1000000);
    FlashProtectSet((unsigned long)FLASH_OFFSET, FlashReadWrite);
}
