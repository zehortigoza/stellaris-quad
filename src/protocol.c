#include "main.h"
#include "protocol.h"
#include "radio.h"
#include "mpu6050.h"

static protocol_msg_callback protocol_msg_func = NULL;

//^;<type>;<1=request | 0=reply>;param1;...;paramn;$
//cel -> quad
static void _radio_cb(const char *received)
{
    Protocol_Msg_Type type;
    char request;
    char text[MAX_STRING];
    char *pch;

    sprintf(text, "%s", received);
    pch = strtok(text, ";");

    if (!pch || pch[0] != '^')
        return;

    //type
    pch = strtok(NULL, ";");
    if (!pch || pch[0] == '$')
        return;
    type = pch[0];

    //request/reply
    pch = strtok(NULL, ";");
    if (!pch || pch[0] == '$')
        return;
    request = pch[0];

    switch (type)
    {
        case MOVE:
        {
            unsigned int num;//only request
            Protocol_Axis axis;

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            axis = pch[0];

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            num = atoi(pch);

            protocol_msg_func(type, request, axis, num);
            break;
        }
        case PING:
        {
            int num;
            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            num = atoi(pch);
            protocol_msg_func(type, request, num);
            break;
        }
        case BATTERY:
        case RADIO_LEVEL:
        case CALIBRATE:
        case GYRO:
        case ACCELEROMETER:
        case CONFIGS:
        case ARM:
        {
            protocol_msg_func(type, request);
            break;
        }
        case ORIENTATION:
        case ESC_CONFIG:
        {
            unsigned char enable;

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            enable = atoi(pch);
            protocol_msg_func(type, request, enable);
            break;
        }
        case CONFIGS_WRITE:
        {
            float p, i;

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            p = atof(pch);

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            i = atof(pch);

            protocol_msg_func(type, request, p, i);
            break;
        }
        case ESC_CONFIG_DATA:
        {
            unsigned int fl, fr, bl, br;

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            fl = atoi(pch);

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            fr = atoi(pch);

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            bl = atoi(pch);

            pch = strtok(NULL, ";");
            if (!pch || pch[0] == '$')
                return;
            br = atoi(pch);

            protocol_msg_func(type, request, fl, fr, bl, br);
            break;
        }
        default:
        {
            break;
        }
    }
}

void procotol_init(protocol_msg_callback cb)
{
    radio_init(_radio_cb);
    protocol_msg_func = cb;
}

unsigned char protocol_msg_send(Protocol_Msg_Type type, char request, ...)
{
    char tx_buffer[MAX_STRING];
    va_list ap;

    va_start(ap, request);
    switch (type)
    {
        case PING:
        {
            //quad request to cel
            if (request)
                sprintf(tx_buffer, "^;%c;1;2;$\n", type);
            else
            {
                int num = va_arg(ap, int);
                sprintf(tx_buffer, "^;%c;0;%d;$\n", type, num);
            }
            break;
        }
        case BATTERY:
        case RADIO_LEVEL:
        {
            int num = va_arg(ap, int);//only reply
            sprintf(tx_buffer, "^;%c;0;%d;$\n", type, num);
            break;
        }
        case GYRO:
        case ACCELEROMETER:
        {
            double x, y, z;//only reply
            x = va_arg(ap, double);
            y = va_arg(ap, double);
            z = va_arg(ap, double);
            sprintf(tx_buffer, "^;%c;0;%.3f;%.3f;%.3f;$\n", type, x, y, z);
            break;
        }
        case CALIBRATE:
        case MOVE:
        case CONFIGS_WRITE:
        case ESC_CONFIG:
        case ESC_CONFIG_DATA:
        case ARM:
        {
            //only reply
            sprintf(tx_buffer, "^;%c;0;$\n", type);
            break;
        }
        case DEBUG:
        {
            const char *txt = va_arg(ap, char*);
            sprintf(tx_buffer, "^;d;1;%s;$\n", txt);
            break;
        }
        case ORIENTATION:
        {
            float roll, pitch, yaw;
            roll = va_arg(ap, double);
            pitch = va_arg(ap, double);
            yaw = va_arg(ap, double);
            sprintf(tx_buffer, "^;%c;0;%.3f;%.3f;%.3f;$\n", type, roll, pitch, yaw);
            break;
        }
        case CONFIGS:
        {
            float p, i;
            p = va_arg(ap, double);
            i = va_arg(ap, double);
            sprintf(tx_buffer, "^;%c;0;%.3f;%.3f;$\n", type, p, i);
            break;
        }
        default:
        {
            va_end(ap);
            return 0;
        }
    }

    va_end(ap);
    return radio_send(tx_buffer);
}
