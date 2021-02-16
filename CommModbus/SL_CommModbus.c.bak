#include "SL_CommModbus.h"

#include "mb.h"
#include "mbport.h"
#include "tim1.h"
#include "version.h"

#define VERSION_MAJOR_REG 0x10
#define VERSION_MINOR_REG 0x11
#define VERSION_BUILD_REG 0x12

#define SERIAL_REG 0x30
#define ERASE_FILENAME_REG 0x38


#define REG_HOLDING_START   1000
#define REG_HOLDING_NREGS   5

static USHORT   usRegHoldingStart = REG_HOLDING_START;
static USHORT   usRegHoldingBuf[REG_HOLDING_NREGS];

eMBErrorCode eStatus;	
	
	
int SL_CommModbus(void)
{
		eMBPoll();
		task_modbus();		
		usRegHoldingBuf[0] = mbTick_1sec;
	  return 0;
};
int SL_CommModbusInit(void)
{
	eStatus = eMBInit( MB_RTU, 0x0A, 0, 15200UL, MB_PAR_NONE );
  eStatus = eMBEnable();	
  return 0;
};


eMBErrorCode eMBRegCoilsCB( UCHAR * pucRegBuffer, USHORT usAddress,
                            USHORT usNCoils, eMBRegisterMode eMode )
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegDiscreteCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNDiscrete )
{
    return MB_ENOREG;
}

eMBErrorCode eMBRegInputCB( UCHAR * pucRegBuffer, USHORT usAddress, USHORT usNRegs )
{
    uint8_t reg = pucRegBuffer[0];
    
    if(reg >= VERSION_MAJOR_REG && reg <= VERSION_BUILD_REG)
    {
        for(uint16_t i = 0; i<  usNRegs; i++)
        {
            switch(reg + i)
            {
                case VERSION_MAJOR_REG: 
                      pucRegBuffer[i << 1] = VERSION_MAJOR >> 8;
                      pucRegBuffer[(i << 1)+1] = VERSION_MAJOR & 0xff;
                break;
                
                case VERSION_MINOR_REG:
                      pucRegBuffer[i << 1] = VERSION_MINOR >> 8;
                      pucRegBuffer[(i << 1)+1] = VERSION_MINOR & 0xff;
                break;
                
                case VERSION_BUILD_REG:
                     pucRegBuffer[i << 1] = VERSION_BUILD >> 8;
                     pucRegBuffer[(i << 1)+1] = VERSION_BUILD & 0xff;
                break;
                
                default:  return MB_ENOREG;
            }
        }
        
      return MB_ENOERR;
    }
    
        
    if(reg >= SERIAL_REG && reg < (SERIAL_REG + (sizeof(SERIAL)>>1)))
    {     
        memcpy(pucRegBuffer,SERIAL, sizeof(SERIAL));
        return MB_ENOERR;
    }
    
    return MB_ENOREG;
}


eMBErrorCode eMBRegHoldingCB( UCHAR * pucRegBuffer, USHORT usAddress,
                              USHORT usNRegs, eMBRegisterMode eMode )
{
    eMBErrorCode    eStatus = MB_ENOERR;
    int             iRegIndex;

    if( ( usAddress >= REG_HOLDING_START ) &&
        ( usAddress + usNRegs <= REG_HOLDING_START + REG_HOLDING_NREGS ) )
    {
        iRegIndex = ( int )( usAddress - usRegHoldingStart );
        switch ( eMode )
        {
        case MB_REG_READ:
            while( usNRegs > 0 )
            {
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] >> 8 );
                *pucRegBuffer++ = ( UCHAR ) ( usRegHoldingBuf[iRegIndex] & 0xFF );
                iRegIndex++;
                usNRegs--;
            }
            break;
        case MB_REG_WRITE:
            while( usNRegs > 0 )
            {
                usRegHoldingBuf[iRegIndex] = *pucRegBuffer++ << 8;
                usRegHoldingBuf[iRegIndex] |= *pucRegBuffer++;
                iRegIndex++;
                usNRegs--;
            }
            break;
        }
    }
    else
    {
        eStatus = MB_ENOREG;
    }
    return eStatus;
}
