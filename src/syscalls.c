#include <sys/types.h>          //Needed for caddr_t

#include "inc/hw_memmap.h"      //Needed for GPIO Pins/UART base
#include "inc/hw_types.h"       //Needed for SysTick Types
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

char *heap_end = 0;

caddr_t _sbrk(unsigned int incr);
int _close(int file);
int _fstat(int file);
int _isatty(int file);
int _lseek(int file, int ptr, int dir);
int _open(const char *name, int flags, int mode);
int _read(int file, char *ptr, int len);
int _write(int file, char *ptr, unsigned int len);

caddr_t _sbrk(unsigned int incr)
{
    while(1);
    return NULL;
}

int _close(int file)
{
    return -1;
}

int _fstat(int file)
{
    return -1;
}

int _isatty(int file)
{
    return -1;
}

int _lseek(int file, int ptr, int dir)
{
    return -1;
}

int _open(const char *name, int flags, int mode)
{
    return -1;
}

int _read(int file, char *ptr, int len)
{
    int i;
    for( i = 0; i < len; i++ ){
        ptr[i] = (char)MAP_UARTCharGet(UART0_BASE);
    }
    return len;
}

int _write(int file, char *ptr, unsigned int len)
{
    unsigned int i;
    for(i = 0; i < len; i++){
        MAP_UARTCharPut(UART0_BASE, ptr[i]);
    }
    return i;
}
