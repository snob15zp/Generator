#ifndef GlobalKey_h
#define GlobalKey_h

#include <stdbool.h>
#include "BoardSetup.h"

#define PowerUSE
#define LCDUSE
#define ACCUSE
#define COMM2  //SPIFFS + MODBUS
#define PLAYER

#if defined COMM1 || defined COMM2
#define COMMS
#endif

#ifdef COMM2
#define MODBUS
#endif


//#define Flashw25qxx
#define Flashat25sf321
#define SPIFFS

//#define D_TestOuputToUart1

#define D_FileNameLength 32
#define D_FileNameLengthInFL 32
#define D_FN_Max_Length 28
extern const char FN_Read_Template[];

void Error(char* s);

typedef enum  
{
 PS_Int_No
,PS_Int_USB_No
,PS_Int_USB  		//work
,PS_Int_BLE_No 	
,PS_Int_BLE  						//work
,PS_Int_NumOfEl	
} e_PS_Int;

//extern e_PS_Int PS_Int;
extern bool byte_TX_DLE;



#define DLE 27 //esc
#define DTD '$'
#define DRD '%'
#define SMODBUSBegin 0
#define SMODBUSEnd 1

#endif