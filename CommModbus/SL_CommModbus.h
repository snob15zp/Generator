#ifndef SL_CommModbus_h
#define SL_CommModbus_h

#include <stdbool.h>
#include "BoardSetup.h"

#define USBcommPause 40000 //max
#define USBcommPauseErase 5000;

typedef void (*flag_cb_t)(); /**< Flags callback typedef */
typedef struct
{
    flag_cb_t tx_done;  /**< Calls when flags register has set tx_done bit, signalize that transmission is ended */
    flag_cb_t play;     /**< Play callback, should have same function as 'Start' button */
    flag_cb_t stop;     /**< Stop callback, should have same function as 'Stop' button */
    flag_cb_t prev;     /**< Previous file callback, should have same function as 'Prev' button */
    flag_cb_t next;     /**< Next file callback, should have same function as 'Next' Button */
} mb_flags_cb_t;

/**
* Set modbus flags callbacks
* @see flag_cb_t
* @see mb_flags_cb_t
* @param cb Callbacks list
*/
void set_mb_flags_cb(mb_flags_cb_t* cbs);

extern int  SL_CommModbus(void);
extern int SL_CommModbusInit(void);

extern systemticks_t MODBUScommLastTime;
//extern bool USBcommLastTimeSet;

#endif 
