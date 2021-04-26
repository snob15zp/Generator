#include <string.h>
#include "SL_CommModbus.h"
#include "SuperLoop_Comm2.h"

#include "mb.h"
#include "mbport.h"
#include "tim1.h"
#include "version.h"
#include "fs.h"

#define VERSION_MAJOR_REG 0x10
#define VERSION_MINOR_REG 0x11
#define VERSION_BUILD_REG 0x12

#define FLAGS_REG  0x20
#define SERIAL_REG 0x30
#define ERASE_FN_EXT_REG0 0x38
#define ERASE_FN_EXT_REG1 0x39

#define ERASE_ALL_START_COIL 0x80
#define ERASE_FN_EXT_START_COIL 0x81
#define FS_FORMAT_COIL 0x82


#define REG_HOLDING_START   1000
#define REG_HOLDING_NREGS   5

#define EVT_IS_REBOOT 1
#define EVT_IS_FORMAT 2

/**
* Flags reg struct
*/
typedef struct 
{
    uint8_t tx_done :1; /**< Transfer is done */
    uint8_t play_btn :1;    /**< Starts player */
    uint8_t stop_btn :1;    /**< Stops player */
    uint8_t prv_btn: 1; /**< Same as clicking 'Prev' button */
    uint8_t next_btn: 1; /**< Same as clicking 'Next' button */
    uint8_t : 2;        /**< 9 empty bits */
    uint8_t : 7;
    uint8_t reboot :1;  /**< Reboots device */
} flags_reg_t;    


systemticks_t MODBUScommLastTime = -USBcommPause;
static uint8_t erase_fn_ext_reg[4]; /**< Erase filename extension */
static uint8_t evt_flags = 0;
static uint8_t evt_timeout = 0;

static mb_flags_cb_t mb_flags_cb = {};

void set_mb_flags_cb(mb_flags_cb_t* cbs)
{
    if (cbs != NULL)
    {
        if(cbs->tx_done != NULL)
        {
            mb_flags_cb.tx_done = cbs->tx_done;
        }
        
        if(cbs->play != NULL)
        {
            mb_flags_cb.play = cbs->play;
        }
        
        if(cbs->stop != NULL)
        {
            mb_flags_cb.stop = cbs->stop;
        }
        
        if(cbs->prev != NULL)
        {
            mb_flags_cb.prev = cbs->prev;
        }

        if(cbs->next != NULL)
        {
            mb_flags_cb.next = cbs->next;
        }
    }
}
    
    
int SL_CommModbus(void)
{
    eMBPoll();
    task_modbus();		
    
    if (evt_flags & EVT_IS_REBOOT)
    {
        evt_timeout--;
        if (evt_timeout == 0)
        {
            NVIC_SystemReset();
            evt_flags = 0;
            evt_timeout = 0;
        }
        return 0;
    }
    
    if (evt_flags & EVT_IS_FORMAT)
    {
        evt_timeout--;
        if (evt_timeout == 0)
        {
            spiffs_format_flash();
            evt_flags = 0;
            evt_timeout = 0;
        }
    }
    
	return 0;
}

int SL_CommModbusInit(void)
{
    eMBErrorCode eStatus;
	eStatus = eMBInit( MB_RTU, 0x0A, 0, 115200UL, MB_PAR_NONE );
    eStatus = eMBEnable();	
    return 0;
}
 

eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                            USHORT usNCoils, eMBRegisterMode eMode )
{

    
    if (eMode == MB_REG_WRITE)
    {
        MODBUScommLastTime=SystemTicks;
        if (!SLC_FFSEnable())   //get error if FFS is busy
        {   
             return MB_ENORES;
        }

        if (usAddress == (ERASE_ALL_START_COIL+1))
        {
            return spiffs_erase_all();
        }
        
        if (usAddress == (ERASE_FN_EXT_START_COIL+1) &&
            erase_fn_ext_reg[0] != 0)
        {
            int res = spiffs_erase_by_ext((const char*)erase_fn_ext_reg);
            memset(erase_fn_ext_reg,0,sizeof(erase_fn_ext_reg));
            
            return res;
        }
        
        if (usAddress == (FS_FORMAT_COIL+1))
        {
            evt_flags = EVT_IS_FORMAT;
            return MB_ENOERR;
        }
    }        
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
	//MODBUScommLastTime=SystemTicks;
 //   USBcommLastTimeSet=true;
	
    return MB_ENOREG;
}

eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
	
//	 MODBUScommLastTime=SystemTicks;
//   USBcommLastTimeSet=true;
	
    uint8_t reg = pucRegBuffer[0];
    
    if(reg >= VERSION_MAJOR_REG && reg <= VERSION_BUILD_REG)
    {
        for(uint16_t i = 0; i<  usNRegs; i++)
        {
            pucRegBuffer[i << 1] = 0;
            switch(reg + i)
            {
                case VERSION_MAJOR_REG: 
                      pucRegBuffer[(i << 1)+1] = *((uint8_t *)FLASH_VERSION_ADDRESS);
                break;
                
                case VERSION_MINOR_REG:
                      pucRegBuffer[(i << 1)+1] = *((uint8_t *)FLASH_VERSION_ADDRESS + 1);
                break;
                
                case VERSION_BUILD_REG:    
                     pucRegBuffer[(i << 1)+1] = *((uint8_t *)FLASH_VERSION_ADDRESS + 2);
                break;
                
                default:  return MB_ENOREG;
            }
        }
        
      return MB_ENOERR;
    }
    
        
    if(reg >= SERIAL_REG && reg < (SERIAL_REG + 6))
    {     

        memcpy((void*)pucRegBuffer,(const void*)SERIAL, 2+2+4+4);
        return MB_ENOERR;
    }
    
    return MB_ENOREG;
}


eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
                              USHORT usNRegs, eMBRegisterMode eMode )
{
    
    
    if (eMode == MB_REG_WRITE && 
        (usAddress == (ERASE_FN_EXT_REG0+1) || usAddress == (ERASE_FN_EXT_REG1+1)))
    {
        MODBUScommLastTime = SystemTicks; 
        if (!SLC_FFSEnable())  //get error if FFS is busy
        {   
             return MB_ENORES;
        } 
			
        uint8_t offset = (usAddress - (ERASE_FN_EXT_REG0+1))<<1;
        if ((offset + (usNRegs<<1)) > sizeof(erase_fn_ext_reg))
        {
            return MB_EINVAL;
        }
        
        for(uint8_t iter = 0; iter < usNRegs; iter++)
        {
            erase_fn_ext_reg[offset+ (iter<<1) ] = pucRegBuffer[1 + (iter << 1)];
            erase_fn_ext_reg[offset + (iter<<1) + 1 ] = pucRegBuffer[iter << 1];
        }
        
        return MB_ENOERR;
    }
    
    if (eMode == MB_REG_WRITE && usAddress == (FLAGS_REG+1))
    {
        flags_reg_t* flags = (flags_reg_t*)pucRegBuffer;
        evt_flags =  flags->reboot ? EVT_IS_REBOOT : evt_flags;
        
        if (flags->tx_done && mb_flags_cb.tx_done != NULL)
        {
            mb_flags_cb.tx_done();
        }

        if (flags->play_btn && mb_flags_cb.play != NULL)
        {
            mb_flags_cb.play();
        }

        if (flags->stop_btn && mb_flags_cb.stop != NULL)
        {
            mb_flags_cb.stop();
        }

        if (flags->prv_btn && mb_flags_cb.prev != NULL)
        {
            mb_flags_cb.prev();
        }

        if (flags->next_btn && mb_flags_cb.next != NULL)
        {
            mb_flags_cb.next();
        }    
        return MB_ENOERR;
    }
    
    return MB_ENOREG;
}
