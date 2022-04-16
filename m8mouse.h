// SPDX-License-Identifier: MIT
#ifndef M8MOUSE_H
#define M8MOUSE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <hidapi/hidapi.h>
#include <unistd.h>

#include "log.h"


#define LOG_PRINT_MEM_WIDTH 16

#define ENO_SUCCESS 0x0
#define ENO_GENERAL 0x1
#define ENO_DEVICE  0x21
#define ENO_SEND    0x22
#define ENO_RECV    0x23

//vendor and product id
//1bcf:Sunplus Innovation Technology Inc.
//08a0:Gaming mouse [Philips SPK9304]
#define USB_M8_VID 0x1bcf
#define USB_M8_PID 0x08a0


#define M8_PKT_SIZE 8
#define M8_PKT_TRANSFER_COUNT 43
#define M8_DEV_MEM_SIZE 256
#define M8_DEV_MEMBUFF_SIZE 512

#define OUT_RID 0x04
#define OUT_B1C 0x07
#define INN_RID 0x04
#define INN_B7C 0xcc
#define INN_B7E 0xee

//14-15ms is needed
#define DELAY_AFTER_SET_MS 14

#define M8_DPI_ADDR         0x30
#define M8_DPI_MODE_MASK    0x0F

#define M8_DPI_RES_ADDR     0x6
#define M8_DPI_RES_COUNT    0x6

#define M8_LED_ADDR         0x32
#define M8_LED_MODE_MASK    0x0F
#define M8_LED_SPEED_MASK   0xF0


#define M8_MODE_LED_HALF_ADDR   0x2F
#define M8_MODE_LED_HALF_ON     0x47
#define M8_MODE_LED_HALF_OFF    0x7F

typedef enum {
    M8_DEVICE_MODE_DPI,
    M8_DEVICE_MODE_DPI_RES,
    M8_DEVICE_MODE_LED,
    M8_DEVICE_MODE_SPEED
} M8_DEVICE_MODES;

typedef unsigned char packet_buffer[M8_PKT_SIZE];

typedef struct {
    char           *label;
    unsigned char   value;
} mode;


struct devicedata {
    hid_device*     devhandle;
    int             devconfirmed;
    unsigned char   memdata[M8_DEV_MEMBUFF_SIZE];
    int             membuffer;
    int             memindex;
    int             memsize;
    int             active_dpi;
    int             active_led;
    int             active_speed;
    int             dpires_values[M8_DPI_RES_COUNT];
    mode           *modes_dpi;
    mode           *modes_dpires;
    mode           *modes_led;
    mode           *modes_speed;
};



int                 device_init();
void                device_shutdown();
int                 device_query();
int                 device_update();

int                 device_update_state();
int                 device_set_modes(int dpi, int led, int speed);
mode               *device_get_active_mode(M8_DEVICE_MODES which_mode);
mode               *device_get_mode_value(M8_DEVICE_MODES which_mode, int index);
mode               *device_get_all_modes(M8_DEVICE_MODES which_mode);
int                 device_mem_size();
unsigned char      *device_mem_raw();




#endif
