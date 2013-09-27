#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef enum
{
  PING = 'p',
  BATTERY = 'b',
  RADIO_LEVEL = 'r',
  MOVE = 'm',
  GYRO = 'g',
  ACCELEROMETER = 'a',
  CALIBRATE = 'c',
  DEBUG = 'd',
  ORIENTATION = 'o',
  CONFIGS = 'f',//controller ask for configs
  CONFIGS_WRITE = 'w',//controller write configs
  ESC_CONFIG = 'e',//controller enable esc config mode
  ESC_CONFIG_DATA = 'h'//controller send esc configs data
} Protocol_Msg_Type;

typedef enum
{
    AXIS_X = 'x',
    AXIS_Y = 'y',
    AXIS_Z = 'z',
    AXIS_ROTATE = 'r'
} Protocol_Axis;

typedef void (*protocol_msg_callback)(Protocol_Msg_Type type, char request, ...);

void procotol_init(protocol_msg_callback cb);
unsigned char protocol_msg_send(Protocol_Msg_Type type, char request, ...);

#endif
