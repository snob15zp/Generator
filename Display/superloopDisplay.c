/*!
\file
\brief High Level Display control

\todo SLD_PowerState
*/	


/*!
\brief info on display for debug ACC 

*/
//  #define def_debug_AccDispay

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "GlobalKey.h"
#include "superloopDisplay.h"
#include "mainFSM.h"
#include "board_PowerModes.h"
#include "BoardSetup.h"
#include "SuperLoop_Comm2.h"
#include "superloop.h"
#include "SuperLoop_Player.h"
#include "romfs_files.h"
#include "SL_CommModbus.h"
#include "version.h"




//extern uint16_t SLPl_ui16_NumOffiles;

 #define D_statusstring_Length 50
char statusstring[D_statusstring_Length]="initial state";
bool bstatusstring;
void SetStatusString(char* s)
{
	bstatusstring=true;
	strncpy(statusstring,s,D_statusstring_Length-1);
};


//-------------------------for main-----------------------------------------------

typedef enum  
{SLD_FSM_InitialWait  		//work
,SLD_FSM_Off  						//work
,SLD_FSM_OnTransition 		//work
,SLD_FSM_On 							//work
,SLD_FSM_PopulateList	    //work
,SLD_FSM_NotFlash					//work
,SLD_FSM_OffTransition 		//work
,SLD_FSM_DontMindSleep		//e_PS_DontMindSleep
,SLD_FSM_SleepTransition 	//work
,SLD_FSM_Sleep           	//  ready for sleep
,SLD_FSM_WakeTransition  	//work
,SLD_FSM_NumOfEl	
} e_SLD_FSM;

/**
\brief Map e_SLD_FSM onto e_PowerState

e_PS_Work,e_PS_DontMindSleep,e_PS_ReadySleep
*/
const e_PowerState SLD_Encoder[SLD_FSM_NumOfEl]=
{e_PS_Work						//SLD_FSM_InitialWait 0
,e_PS_Work						//SLD_FSM_Off         	1
,e_PS_Work						//SLD_FSM_OnTransition	2
,e_PS_Work						//SLD_FSM_On 						3
,e_PS_Work						//SLD_FSM_PopulateList	4
,e_PS_Work            //SLD_FSM_NotFlash			5
,e_PS_Work						//SLD_FSM_OffTransition 6
,e_PS_DontMindSleep		//SLD_FSM_DontMindSleep	7
,e_PS_DontMindSleep		//SLD_FSM_SleepTransition	8
,e_PS_ReadySleep			//SLD_FSM_Sleep						9
,e_PS_Work						//SLD_FSM_WakeTransition	10
};

static e_SLD_FSM state_inner;

//---------------------------------for power sleep---------------------------------------------
//static e_PowerState SLD_PowerState; 
static bool SLD_GoToSleep;

__inline e_PowerState SLD_GetPowerState(void)
{
	 return SLD_Encoder[state_inner];
};

__inline e_PowerState SLD_SetSleepState(bool state)
{
	SLD_GoToSleep=state;
	return SLD_Encoder[state_inner];
};


//------------------------ for Display update ----------------------------------
static systemticks_t LastUpdateTime;
#define DisplayUpdatePeriod 1000


//---------------------- Control grafical objects------------------------------
int SLDw(void);
void displayACC(void);
int SLDwACC(void);
void DisplayBatteryStatus(void);
void DisplaySelectedFile(uint32_t n);
//------------------------FSM control--------------------------------------------
int SLD_DisplInit(void);
int SLD_DisplReInit(void);
int SLD_DisplDeInit(void);
e_FunctionReturnState fileListRead(void);
void fileListInitStart(void);

static bool bListUpdate;
static uint8_t FSM_fileListUpdate_state;
//static s32_t File_List;
static uint8_t filename[D_FileNameLengthD+1];
static uint8_t filename1[D_FileNameLengthD+1];
static uint8_t fileCount;
static uint32_t offset;
static bool bListUpdate1;

//-------------------------------------- call back-------------------------

/**
* Calls when freq.pls writing is done
*/
void on_playlist_write_done()
{
	playFileSector=0;
	bListUpdate=true;
}





//---------------------------------------super loop------------------------
extern  GHandle	ghList1;   
#define SLD_SleepDelay 1000

int SLD(void)
{
	e_FunctionReturnState rstatel;
	systemticks_t SLD_LastButtonPress;
	switch (state_inner)
	{
		case SLD_FSM_InitialWait: // initial on
			if (bVSYS) 
         {state_inner=SLD_FSM_OnTransition;
				 };
			break;
		case SLD_FSM_Off:	// off
			if (button_sign&&bVSYS)
			{
				state_inner=SLD_FSM_OnTransition;
				button_sign=0;
			}
			else
			{ 
				SLD_LastButtonPress=BS_LastButtonPress;
				if (((SystemTicks-SLD_LastButtonPress)>SLD_SleepDelay)) 
					 state_inner=SLD_FSM_DontMindSleep;
			};
			break;
		case SLD_FSM_OnTransition: //on transition
				PM_OnOffPWR(PM_Display,true );
				SLD_DisplReInit();
		    bListUpdate=true;
		    gfxSleepMilliseconds(10); 
		    DisplaySelectedFile(playFileSector);
		    state_inner=SLD_FSM_On;
      break;
		case SLD_FSM_On: // on
			DisplayBatteryStatus();
		  if (!SLC_SPIFFS_State())
			{ //state_inner=SLD_FSM_NotFlash;
			}
			else
			{
//				if (SLC_SPIFFS_State())
				{
					bListUpdate1=bListUpdate1||bListUpdate;
					bListUpdate=0;
					if (bListUpdate1)
					{	fileListInitStart();
						state_inner=SLD_FSM_PopulateList;
					}
					else
          {
#ifdef def_debug_AccDispay
	    	SLDwACC();
#else
		    SLDw();
#endif		
					};
				}	
			
		  };
			if ( (!bVSYS) || button_sign)
				{
					button_sign=0;
					state_inner=SLD_FSM_OffTransition;
				};
				break;
		case SLD_FSM_PopulateList:	
				if (SLC_SPIFFS_State()) 
				{
//      	do
					{
						rstatel=fileListRead();
						if (e_FRS_Done==rstatel)
						{	gwinListAddItem(ghList1, (char*)filename, gTrue);	
						  gfxSleepMilliseconds(10);
						};
						if (e_FRS_DoneError==rstatel)
						{	state_inner=SLD_FSM_On;	
						  DisplaySelectedFile(playFileSector);
							bListUpdate1=false;
						}	
					}
				}
				else 
				{	
					state_inner=SLD_FSM_On;	
				};	
//				while (e_FRS_DoneError!=rstatel);
      break;	
		case SLD_FSM_NotFlash:	
			  DisplayBatteryStatus();
			  if (SLC_SPIFFS_State())
					state_inner=SLD_FSM_On;
		  break;
		case SLD_FSM_OffTransition: 
      	SLD_DisplDeInit();               //off transition
        PM_OnOffPWR(PM_Display,false );				
				state_inner=SLD_FSM_Off;
		  break;	
		case SLD_FSM_DontMindSleep:
			  SLD_LastButtonPress=BS_LastButtonPress;
		    if (SLD_GoToSleep) 
						state_inner=SLD_FSM_SleepTransition;
				if (((SystemTicks-SLD_LastButtonPress)<SLD_SleepDelay)) 
						state_inner=SLD_FSM_Off;  //has more priority
			break;
		case SLD_FSM_SleepTransition:// sleep transition
		  //reset interrupt pending
		  PM_ClearPendingButton;
		  state_inner=SLD_FSM_Sleep; 
		  //break;
		case SLD_FSM_Sleep:
			//SLD_PowerState= e_PS_ReadySleep;
//			SLD_PWR_State=	false;		
        SLD_LastButtonPress=BS_LastButtonPress;
				if ((!SLD_GoToSleep) || ((SystemTicks-SLD_LastButtonPress)<SLD_SleepDelay)) 
				{state_inner=SLD_FSM_WakeTransition;
				};
		    
			break;
		case SLD_FSM_WakeTransition: //wake transition
		  state_inner=SLD_FSM_Off;
		break;
    default: state_inner=SLD_FSM_InitialWait;		
	};
	return 0;
}

mb_flags_cb_t mb_cbs = 
{
    .tx_done = on_tx_done_cb,
    .play = play_cb,
    .stop = stop_cb,
    .prev = prev_cb,
    .next = next_cb,
};

int SLD_init(void)
{
	spiffs_on_write_playlist_done(on_playlist_write_done);
  set_mb_flags_cb(&mb_cbs);
	return 0;
;
};





//--------------------------------for uGFX INIT/DEINIT--------------------------------
extern char	heap[GFX_OS_HEAP_SIZE];
extern gThread	hThread;

static e_SLAcc_BatStatus batStateOld;
static uint16_t Old_mFSM_BQ28z610_RSOC;
static e_SLPl_FSM PlStateOld;
static t_DisplayFlags ButtonFlags;
//static uint32_t FileListCurrentPage;
//uint8_t fileSect=0;

void GFXPreinit (void)
{ 
	uint32_t i;

	for (i=0;i<GFX_OS_HEAP_SIZE;i++)
	{heap[i]=0;};
	hThread=0;
};


int SLD_DisplDeInit(void)
{
	
	gfxDeinit();
	
	return 0;
};	

int SLD_DisplReInit(void)
{
	batStateOld=0;
	PlStateOld=0;
	Old_mFSM_BQ28z610_RSOC=0;
	
	GFXPreinit();
	SLD_DisplInit();

	return 0;
}


////------------------------Display control objects--------------------------------------------
const bool SLPl_ButtonColour[SLPl_FSM_NumOfElements]=
{false      						//SLPl_FSM_InitialWait
,false						//SLPl_FSM_off
,true									//SLPl_FSM_OnTransition
,true									//SLPl_FSM_On
,false									//SLPl_FSM_OffTransition
};

const GWidgetStyle GreenStyle = {
	HTML2COLOR(0xFFFFFF),			// window background
	HTML2COLOR(0x2A8FCD),			// focused
 
	// enabled color set
	{
		HTML2COLOR(0x000000),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0x33ff33),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	},
 
	// disabled color set
	{
		HTML2COLOR(0xC0C0C0),		// text
		HTML2COLOR(0x808080),		// edge
		HTML2COLOR(0xE0E0E0),		// fill
		HTML2COLOR(0xC0E0C0)		// progress - active area
	},
 
	// pressed color set
	{
		HTML2COLOR(0x404040),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0x808080),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	}
};

const GWidgetStyle RedStyle = {
	HTML2COLOR(0xFFFFFF),			// window background
	HTML2COLOR(0x2A8FCD),			// focused
 
	// enabled color set
	{
		HTML2COLOR(0x000000),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0xff3333),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	},
 
	// disabled color set
	{
		HTML2COLOR(0xC0C0C0),		// text
		HTML2COLOR(0x808080),		// edge
		HTML2COLOR(0xE0E0E0),		// fill
		HTML2COLOR(0xC0E0C0)		// progress - active area
	},
 
	// pressed color set
	{
		HTML2COLOR(0x404040),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0x808080),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	}
};
const GWidgetStyle GreenTextStyle = {
	HTML2COLOR(0xFFFFFF),			// window background
	HTML2COLOR(0x2A8FCD),			// focused
 
	// enabled color set
	{
		HTML2COLOR(0x33ff33),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0xE0E0E0),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	},

	// disabled color set
	{
		HTML2COLOR(0xC0C0C0),		// text
		HTML2COLOR(0x808080),		// edge
		HTML2COLOR(0xE0E0E0),		// fill
		HTML2COLOR(0xC0E0C0)		// progress - active area
	},

	// pressed color set
	{
		HTML2COLOR(0x404040),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0x808080),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	}
};

const GWidgetStyle RedTextStyle = {
	HTML2COLOR(0xFFFFFF),			// window background
	HTML2COLOR(0x2A8FCD),			// focused
 
	// enabled color set
	{
		HTML2COLOR(0xff3333),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0xE0E0E0),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	},

	// disabled color set
	{
		HTML2COLOR(0xC0C0C0),		// text
		HTML2COLOR(0x808080),		// edge
		HTML2COLOR(0xE0E0E0),		// fill
		HTML2COLOR(0xC0E0C0)		// progress - active area
	},

	// pressed color set
	{
		HTML2COLOR(0x404040),		// text
		HTML2COLOR(0x404040),		// edge
		HTML2COLOR(0x808080),		// fill
		HTML2COLOR(0x00E000)		// progress - active area
	}
};




GListener	gl;
GHandle	ghLabel1, ghLabel2, ghLabel3, ghLabel4, ghLabel5, ghLabel6, ghLabel7;
GHandle ghLabel8, ghLabel9, ghLabel10, ghLabel11, ghLabel12,ghLabel13_RSOC, ghLabel14, ghLabelVersion;
GHandle	ghList1;
GHandle ghImage1;
GHandle ghProgBarWin;
GHandle ghProgBar;

static GHandle  ghButton1, /*ghButton2,*/ ghButton3, ghButton4;

static	GEvent* pe;
//static const gOrientation	orients[] = { gOrientation0, gOrientation90, gOrientation180, gOrientation270 };
//static	unsigned which;

#define D_image_wigx 195
#define D_image_wigy 0
#define D_image_wigwidth 35
#define D_image_wigheight 20

//-------------------BEGIN  OF BEBUG ACC Display-------------

void play_cb()
{
    ButtonFlags.playStart = 1;
}



void stop_cb()
{
    ButtonFlags.playStop = 1;
}

void prev_cb()
{
     ButtonFlags.fileListUp = 1;
}

void next_cb()
{
    ButtonFlags.fileListDown = 1;
}


void displayACC(void)
{ char str[30];	
	if ( (SystemTicks-LastUpdateTime)<DisplayUpdatePeriod ) 
                               {return;};
	LastUpdateTime=SystemTicks;
  //sprintf(str, "%d", mainFMSstate);
	switch (mainFMSstate)
	{
		case e_FSM_Charge:		strcpy(str,"A: Charge"); 			break;
		case 	e_FSM_Rest:    	strcpy(str,"A: Rest"); 				break;
		case e_FSM_ChargeOff:	strcpy(str,"A: ChargeOff"); 		break;
		case e_FSM_RestOff:  	strcpy(str,"A: RestOff"); 		break;
		case e_FSM_Init: 			strcpy(str,"A: Init");					break;
		default: 							strcpy(str,"A: Out of Range");
	}	
	gwinSetText(ghLabel12, str, TRUE);
	
	
	sprintf(str, "V acc: %d", pv_BQ28z610_Voltage);
	gwinSetText(ghLabel3, str, TRUE);

	sprintf(str, "T acc: %d", mFSM_BQ28z610_Temperature);
	gwinSetText(ghLabel4, str, TRUE);

	sprintf(str, "V87: %d", V87);
	gwinSetText(ghLabel5, str, TRUE);
	
	sprintf(str, "I acc: %d", pvIcharge-pvIdescharge);
	gwinSetText(ghLabel6, str, TRUE);
	
	sprintf(str, "RSOC: %d", mFSM_BQ28z610_RSOC);
	gwinSetText(ghLabel8, str, TRUE);
	
}


int SLDwACC(void)
{ 
	//event handling
	pe = geventEventWait(&gl,10 ); //gDelayForever
	displayACC();
	return 0;
}


//-------------------END  OF DEBUG ACC Display-------------



volatile t_fpgaFlags SLD_fpgaFlags;

uint8_t spiDispCapture;	//0 - free, 1 - busy
uint8_t totalTimeArr[]={'0','0',':','0','0',':','0','0',0};
uint8_t fileTimeArr[]={'0','0',':','0','0',':','0','0',0};
extern uint8_t totalSec;
extern uint8_t totalMin;
extern uint8_t totalHour;
extern uint8_t fileSec;
extern uint8_t fileMin;
extern uint8_t fileHour;
//volatile uint32_t playClk;
uint16_t playFileInList;
//uint8_t fileName[50];

//--------------------create uGFX Objects------------------------
static void SetButtonColour(bool first)
{
	static bool last;
	static bool b1;
	b1=SLPl_ButtonColour[Get_SLPl_FSM_State()];
  if (first || (b1!=last) )
	{if(b1)
	    {gwinSetStyle(ghButton1,&RedStyle);gwinSetText(ghButton1,"Stop",gFalse); gwinSetStyle(ghLabel4,&GreenTextStyle);}
			else
	    {gwinSetStyle(ghButton1,&GreenStyle);gwinSetText(ghButton1,"Start",gFalse); gwinSetStyle(ghLabel4,&RedTextStyle);};
	};		
	last=b1;
};


static void createButtons(void) {
	GWidgetInit	wi;

	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;

	// Apply the START button parameters
	wi.g.width = 100;
	wi.g.height = 30;
	wi.g.y = 280;
	wi.g.x = 70;
	wi.text = "Start";
	ghButton1 = gwinButtonCreate(0, &wi);
	

	
	// Apply the STOP button parameters
//	wi.g.width = 100;
//	wi.g.height = 30;
//	wi.g.y = 280;
//	wi.g.x = 130;
//	wi.text = "Stop";
//	ghButton2 = gwinButtonCreate(0, &wi);
//	
//	gwinSetStyle(ghButton2,&RedStyle);
	
	// Apply the PREW button parameters
	wi.g.width = 35;
	wi.g.height = 60;
	wi.g.y = 20;
	wi.g.x = 195;
	wi.text = "Prev";
	ghButton3 = gwinButtonCreate(0, &wi);
	
	// Apply the NEXT button parameters
	wi.g.width = 35;
	wi.g.height = 60;
	wi.g.y = 90;
	wi.g.x = 195;
	wi.text = "Next";
	ghButton4 = gwinButtonCreate(0, &wi);
}

	
static void createLabels(void) {
	GWidgetInit	wi;
	
	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;
	
	wi.g.width = 115; wi.g.height = 20; wi.g.x = 120, wi.g.y = 155;
//	wi.text = "Self test: OK";
	wi.text = "Sys init,pls wait";
	ghLabel3 = gwinLabelCreate(0,&wi);
//	gwinLabelSetAttribute(ghLabel3,100,"Self test:");
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 155;
	wi.text = "Self test:";
	ghLabel8 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 173;
	wi.text = "Stopped";
	ghLabel4 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel4,100,"Status:");
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 173;
	wi.text = "Status:";
	ghLabel9 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 191;
	wi.text = "Not selected";
	ghLabel5 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel5,100,"Program:");

	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 191;
	wi.text = "Program:";
	ghLabel10 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 209;
	wi.text = "00:00:00";
	ghLabel6 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel6,100,"Program timer:");

	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 209;
	wi.text = "Program timer:";
	ghLabel11 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 227;
	wi.text = "00:00:00";
	ghLabel7 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel7,100,"Total timer:");

	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 227;
	wi.text = "Total timer:";
	ghLabel12 = gwinLabelCreate(0, &wi);

	wi.g.width = D_image_wigx-150; wi.g.height = 20; wi.g.x = 150; wi.g.y = 0;
	wi.text = "  --%";
	ghLabel13_RSOC = gwinLabelCreate(0, &wi);


	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 245;
	wi.text = "Version:";
	ghLabel14 = gwinLabelCreate(0, &wi);

    static char versionString[32] = {}; 
    snprintf(versionString, sizeof(versionString), "%d.%d.%d", 
            *((uint8_t *)FLASH_VERSION_ADDRESS), *((uint8_t *)FLASH_VERSION_ADDRESS + 1), *((uint8_t *)FLASH_VERSION_ADDRESS + 2));
        
    wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 245;
	wi.text = versionString;
    ghLabelVersion = gwinLabelCreate(0, &wi);
}


static void createLists(void) {
	GWidgetInit	wi;

	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;

	// Create the label for the list
	wi.g.width = 150; wi.g.height = 20; wi.g.x = 10, wi.g.y = 0;
	wi.text = "Files:";
	ghLabel1 = gwinLabelCreate(0, &wi);

	// The list widget
	wi.g.width = 180;
	wi.g.height = 135;
	wi.g.y = 20;
	wi.g.x = 10;
	wi.text = "Name of list 1";
	ghList1 = gwinListCreate(0, &wi, gFalse);
//	gwinListSetScroll(ghList1, scrollSmooth);
}



static void createImage_batMedLev(void)
{
	GWidgetInit	wi;

	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;
 
	// create the first image widget
	wi.g.x = D_image_wigx; wi.g.y = D_image_wigy; wi.g.width = D_image_wigwidth; wi.g.height = D_image_wigheight;
	ghImage1 = gwinImageCreate(0, &wi.g);
	gwinImageOpenMemory(ghImage1, batMedLev);
}





static void createImage_batCharging(void)
{
	GWidgetInit	wi;

	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;
 
	// create the first image widget
	wi.g.x = D_image_wigx; wi.g.y = D_image_wigy; wi.g.width = D_image_wigwidth; wi.g.height = D_image_wigheight;
	ghImage1 = gwinImageCreate(0, &wi.g);
	gwinImageOpenMemory(ghImage1, batCharging);
}




static void createProgBar(void)
{
	GWindowInit	wi;
	
	gwinClearInit(&wi);
  wi.show = gTrue; wi.x = 20; wi.y = 200; wi.width = 200; wi.height = 40;
  ghProgBarWin = gwinWindowCreate(0, &wi);
	
	gwinSetColor(ghProgBarWin, GFX_BLACK);
  gwinSetBgColor(ghProgBarWin, GFX_GRAY);
	gwinClear(ghProgBarWin);
	gwinDrawBox(ghProgBarWin, 10, 10, 180, 20);
	gwinFillArea(ghProgBarWin, 10, 10, 50, 20);
}

//--------------------END Create uGFX Objects------------------------
#define D_StringInList 7
static uint32_t CurrentPage;

void fileListInitStart(void)
{
	//bfileListInit=true;
	fileCount=0;
	FSM_fileListUpdate_state=0;
};

e_FunctionReturnState fileNameRead(uint16_t filenum)
{	e_FunctionReturnState rstate;
	char byteBuff[D_FileNameLengthD+1];
	int8_t bytesCount;
	uint32_t offset;

	rstate=e_FRS_Processing;

	offset=filenum*D_FileNameLengthD;
	SPIFFS_lseek(&fs, File_List,offset,SPIFFS_SEEK_SET);

	bytesCount=SPIFFS_read(&fs, File_List, &byteBuff, D_FileNameLengthD);
	if (bytesCount<1)
	{	
		return e_FRS_DoneError;
	}
	else					
	{ 
		_sscanf( byteBuff,FN_Read_Template,filename1);                        //change if chainge D_FileNameLength
		if (0<strlen(byteBuff))
		{		
			return e_FRS_Done;
		}
		else
		{
			return e_FRS_DoneError;
		}
	}		
	return rstate;
}


uint16_t a;

e_FunctionReturnState fileListRead(void)
{	e_FunctionReturnState rstate;
	char byteBuff[D_FileNameLengthD+1];
	int8_t bytesCount;
  uint32_t i;	
	uint32_t offset;

	rstate=e_FRS_Processing;

  switch (FSM_fileListUpdate_state)
	{	case 0: 
		
			  gfxSleepMilliseconds(10);
		    gwinListDeleteAll(ghList1);
		    gfxSleepMilliseconds(10);
  			//gwinListAddItem(ghList1, "_1.txt", gTrue);///RDD debug
	     // gfxSleepMilliseconds(1);
//		  File_List=SPIFFS_open(&fs,"freq.pls",SPIFFS_O_RDONLY,0);
		  fileCount=0;
		  offset=D_StringInList*CurrentPage*D_FileNameLengthD;
		  //gwinListDeleteAll(ghList1);
		  SPIFFS_lseek(&fs, File_List,offset,SPIFFS_SEEK_SET);
		  FSM_fileListUpdate_state++;
		  break;
		case 1: 
      while(1){
				if(fileCount<D_StringInList){
					bytesCount=SPIFFS_read(&fs, File_List, &byteBuff, D_FileNameLengthD);
					a=bytesCount;//for debug
					if (bytesCount<1)
					{	FSM_fileListUpdate_state=101;
						break;
					}
					else					
					{ 
	//				    for(i=0;i<bytesCount;i++)
	//					  if (13==byteBuff[i]) break;
	//					  offset+=i+1;
	//					SPIFFS_lseek(&fs, File_List,offset,SPIFFS_SEEK_SET);
						_sscanf( byteBuff,FN_Read_Template,filename);                        //change if chainge D_FileNameLength
						if (0<strlen(byteBuff))
						{		fileCount++;
								FSM_fileListUpdate_state=100;
								//gwinListAddItem(ghList1, (char*)fileName, gTrue);
								break;
						};
					};
				}
				else{
					FSM_fileListUpdate_state=101;
					break;
				}
			}		
      break;			
		case 100: //Done
			//SPIFFS_close(&fs, File_List);
			rstate=e_FRS_Done;
		  FSM_fileListUpdate_state=1;
			break;
		case 101: //DoneError
//			SPIFFS_close(&fs, File_List);
			rstate=e_FRS_DoneError;
		  FSM_fileListUpdate_state=0;
		  offset=0;
			break;
		default:FSM_fileListUpdate_state =0;
	}
	
	return rstate;
}




int SLD_DisplInit(void)
{ 
	
GFXPreinit();	
gfxInit();	

	// Set the widget defaults
	gwinSetDefaultFont(gdispOpenFont("Roboto_Light12"));
	gwinSetDefaultStyle(&WhiteWidgetStyle, gFalse);
	gdispClear(GFX_WHITE);

	// create the widgets
	createButtons();
	createLists();
	createLabels();
	createImage_batCharging();
//	createProgBar();
	// We want to listen for widget events
	geventListenerInit(&gl);
	gwinAttachListener(&gl);
	gdispSetBacklight(50);
	
	fileListInitStart();
	SetButtonColour(true);
return 0;	
};

void DisplayBatteryStatus(void)
{
	char str[20];
	
	if (batStateOld!=Get_SLAcc_BatteryStatus())
	switch(Get_SLAcc_BatteryStatus())
	{
		case SLAcc_batLowLev://createImage_batLowLev();
				gwinImageOpenMemory(ghImage1, batLowLev);
			break;
		case SLAcc_batMedLev://createImage_batMedLev();
				gwinImageOpenMemory(ghImage1, batMedLev);
			break;
		case SLAcc_batHighLev://createImage_batHighLev(); 
				gwinImageOpenMemory(ghImage1, batHighLev);
			break;
		case SLAcc_batFull://createImage_batFull();        
				gwinImageOpenMemory(ghImage1, batFull);
		  break;
		case SLAcc_batCharging://createImage_batCharging();
				gwinImageOpenMemory(ghImage1, batCharging);
			break;
		default://SLAcc_batEmpty:createImage_batEmpty(); 
		    gwinImageOpenMemory(ghImage1, batEmpty);
	};
	 
	if (Old_mFSM_BQ28z610_RSOC!=mFSM_BQ28z610_RSOC)
	{
		sprintf(str, "%4d%% ", mFSM_BQ28z610_RSOC);
	  gwinSetText(ghLabel13_RSOC, str, TRUE);
	};
	
	
	  batStateOld=Get_SLAcc_BatteryStatus();
	  Old_mFSM_BQ28z610_RSOC=mFSM_BQ28z610_RSOC;
	
};

void GetEvent()
{	
	pe = geventEventWait(&gl,10 ); //gDelayForever
	switch(pe->type)
	{
		case GEVENT_GWIN_BUTTON:
			ButtonFlags.playStart|=(((GEventGWinButton*)pe)->gwin == ghButton1);
		 // ButtonFlags.playStop|=(((GEventGWinButton*)pe)->gwin == ghButton2);
		  ButtonFlags.fileListUp|=(((GEventGWinButton*)pe)->gwin == ghButton3);
		  ButtonFlags.fileListDown|=(((GEventGWinButton*)pe)->gwin == ghButton4);
		break;
		default:;
	}	
			
};

void FileListUpDown()
{
	if (ButtonFlags.fileListDown)	//prev 10 files
			{ 
				ButtonFlags.fileListDown=0;
				playFileSector+=D_StringInList;
				if (playFileSector>=SLPl_ui16_NumOffiles)
					  playFileSector=SLPl_ui16_NumOffiles-1;
				if (0==SLPl_ui16_NumOffiles)
					  playFileSector=0;
				DisplaySelectedFile(playFileSector);
		 };
			
			if (ButtonFlags.fileListUp)	//next 10 files
			{ ButtonFlags.fileListUp=0;
				if (playFileSector>=D_StringInList)
				{playFileSector-=D_StringInList;
  			 DisplaySelectedFile(playFileSector);
				};
			};
}

void DisplayPlayStop()
{
		gwinSetText(ghLabel3,"Init OK",gFalse);
		gwinSetText(ghLabel4,"Stopped",gFalse);
		gwinSetText(ghLabel5,"Not selected",gFalse);
		gwinSetText(ghLabel6,"00:00:00",gFalse);
		gwinSetText(ghLabel7,"00:00:00",gFalse);

};

void on_format_flash()
{					
  SetStatusString("Formatting FS. Please wait");
	//gfxSleepMilliseconds(10);
}


void Start(void)
{
	uint32_t nof,cf;
    if (ButtonFlags.playStart)
    { 
        ButtonFlags.playStart=0;
        nof = gwinListItemCount(ghList1);	
        if (nof > 0)
        {
            SetStatusString("Config. Please wait");
            gfxSleepMilliseconds(10);
            cf = gwinListGetSelected(ghList1)+CurrentPage*D_StringInList;
            SLPl_Start(cf);
        }	
    };	
};

void Stop(void)
{
    if (ButtonFlags.playStart)
    { 
        ButtonFlags.playStart=0;
        //DisplayPlayStop();
        SLPl_Stop();
    };
}


void DisplaySelectedFile(uint32_t n)
{
	uint32_t n1;
	if (CurrentPage!=n/D_StringInList)
	{
		CurrentPage=n/D_StringInList;
	  bListUpdate=true;
	}
	else
	{
		n1=n-CurrentPage*D_StringInList;
    gwinListSetSelected(ghList1,n1,TRUE);
	};
};



void DisplayStatusString(void)
{
  if (bstatusstring)
	{	bstatusstring=false;
		gwinSetText(ghLabel3,statusstring,gFalse);
	};
};

void DisplayTimers(void)
{
		fileTimeArr[0]=fileHour/10;
		fileTimeArr[1]=fileHour%10;
		fileTimeArr[3]=fileMin/10;
		fileTimeArr[4]=fileMin%10;
		fileTimeArr[6]=fileSec/10;
		fileTimeArr[7]=fileSec%10;
		timeToString(fileTimeArr);
		gwinSetText(ghLabel6,(const char*)fileTimeArr,gFalse);
	
		totalTimeArr[0]=totalHour/10;
		totalTimeArr[1]=totalHour%10;
		totalTimeArr[3]=totalMin/10;
		totalTimeArr[4]=totalMin%10;
		totalTimeArr[6]=totalSec/10;
		totalTimeArr[7]=totalSec%10;
		timeToString(totalTimeArr);
		gwinSetText(ghLabel7,(const char*)totalTimeArr,gFalse);

};

int SLDw(void)
{ uint16_t tt;
	//event handling

	GetEvent();
	
	switch(Get_SLPl_FSM_State())
	{
		case SLPl_FSM_InitialWait:
			  
			break;
		case  SLPl_FSM_off:
			    Start();
			    if (PlStateOld!=Get_SLPl_FSM_State())
			      DisplayPlayStop();
					if (SLC_SPIFFS_State())
						FileListUpDown();
			break;
		case  SLPl_FSM_OnTransition:
			
			break;
		case  SLPl_FSM_On:
		    if (PlStateOld!=Get_SLPl_FSM_State())
			      gwinSetText(ghLabel4,"Running",gFalse);

			  Stop();
	      if ((SLD_fpgaFlags.endOfFile==1)||(PlStateOld!=Get_SLPl_FSM_State()))
        	{
        		DisplaySelectedFile(playFileSector);
						fileNameRead(playFileSector);
						gwinSetText(ghLabel5,(char*)filename1,gFalse);//
        		SLD_fpgaFlags.endOfFile=0;
        	}
			
			break;
		case  SLPl_FSM_OffTransition:
			     
			break;
		default:;
			
	};
	PlStateOld=Get_SLPl_FSM_State();
	SetButtonColour(false);
	
	

	DisplayStatusString();
	 
	if (1==SLD_fpgaFlags.timeUpdate)
	{
		SLD_fpgaFlags.timeUpdate=0;
		DisplayTimers();
	};
	
			ButtonFlags.playStart=0;
		  ButtonFlags.playStop=0;
		  ButtonFlags.fileListUp=0;
		  ButtonFlags.fileListDown=0;

	
return 0;	
};
