/*!
\file
\brief High Level Display control

\todo SLD_PowerState
*/	


/*!
\brief info on display for debug ACC 

*/
//#define def_debug_AccDispay

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "superloopDisplay.h"
#include "mainFSM.h"
#include "board_PowerModes.h"

//-------------------------for main-----------------------------------------------
uint16_t playFileSector;


uint8_t fileSect=0;


typedef enum  
{SLD_FSM_InitialWait  		//work
,SLD_FSM_Off  						//work
,SLD_FSM_OnTransition 		//work
,SLD_FSM_On 							//work
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
{e_PS_Work						//SLD_FSM_InitialWait
,e_PS_Work						//SLD_FSM_Off
,e_PS_Work						//SLD_FSM_OnTransition
,e_PS_Work						//SLD_FSM_On
,e_PS_Work						//SLD_FSM_OffTransition 
,e_PS_DontMindSleep		//SLD_FSM_DontMindSleep	
,e_PS_DontMindSleep		//SLD_FSM_SleepTransition
,e_PS_ReadySleep			//SLD_FSM_Sleep
,e_PS_Work						//SLD_FSM_WakeTransition
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
//------------------------FSM control--------------------------------------------
int SLD_DisplInit(void);
int SLD_DisplReInit(void);
int SLD_DisplDeInit(void);

#define SLD_SleepDelay 1000

int SLD(void)
{
	systemticks_t SLD_LastButtonPress;
	switch (state_inner)
	{
		case SLD_FSM_InitialWait: // initial on
			if (bVSYS) {state_inner=SLD_FSM_OnTransition;};
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
				SLD_DisplInit();
//		    gwinRedrawDisplay(NULL,true);
		    state_inner=SLD_FSM_On;
//   break;
		case SLD_FSM_On: // on
#ifdef def_debug_AccDispay
	    	SLDwACC();
#else
		    SLDw();
#endif		
  		if ((!bVSYS)|button_sign)
			{
				button_sign=0;
				state_inner=SLD_FSM_OffTransition;
			};
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

int SLD_init(void)
{
	return 0;
};


////------------------------Display control--------------------------------------------
GListener	gl;
GHandle	ghLabel1, ghLabel2, ghLabel3, ghLabel4, ghLabel5, ghLabel6, ghLabel7;
GHandle ghLabel8, ghLabel9, ghLabel10, ghLabel11, ghLabel12;
GHandle	ghList1;

static	GEvent* pe;
//static const gOrientation	orients[] = { gOrientation0, gOrientation90, gOrientation180, gOrientation270 };
//static	unsigned which;

static void createDebugLabels(void);

//--------------------------------for uGFX--------------------------------------
extern char	heap[GFX_OS_HEAP_SIZE];
extern gThread	hThread;

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
	GFXPreinit();
	SLD_DisplInit();

	return 0;
}


volatile t_fpgaFlags fpgaFlags;

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
volatile int playFileInList;
uint8_t fileName[50];
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

static void createLists(void) {
	GWidgetInit	wi;

	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;

	// Create the label for the first list
	wi.g.width = 150; wi.g.height = 20; wi.g.x = 10, wi.g.y = 0;
	wi.text = "Files:";
	ghLabel1 = gwinLabelCreate(0, &wi);

	// The first list widget
	wi.g.width = 220;
	wi.g.height = 140;
	wi.g.y = 20;
	wi.g.x = 10;
	wi.text = "Name of list 1";
	ghList1 = gwinListCreate(0, &wi, gFalse);
//	gwinListSetScroll(ghList1, scrollSmooth);
}
//static GListener gl;
static GHandle  ghButton1, ghButton2;

static void createButtons(void) {
	GWidgetInit	wi;

	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;

	// Apply the first button parameters
	wi.g.width = 100;
	wi.g.height = 30;
	wi.g.y = 280;
	wi.g.x = 10;
	wi.text = "Start";
	ghButton1 = gwinButtonCreate(0, &wi);
	
	// Apply the second button parameters
	wi.g.width = 100;
	wi.g.height = 30;
	wi.g.y = 280;
	wi.g.x = 130;
	wi.text = "Stop";
	ghButton2 = gwinButtonCreate(0, &wi);
}

static void createLabels(void) {
	GWidgetInit	wi;
	
	// Apply some default values for GWIN
	gwinWidgetClearInit(&wi);
	wi.g.show = gTrue;
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 170;
//	wi.text = "Self test: OK";
	wi.text = "Init OK";
	ghLabel3 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel3,100,"Self test:");
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 170;
	wi.text = "Self test:";
	ghLabel8 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 190;
	wi.text = "Stop";
	ghLabel4 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel4,100,"Status:");
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 190;
	wi.text = "Status:";
	ghLabel9 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 210;
	wi.text = "Not selected";
	ghLabel5 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel5,100,"Program:");

	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 210;
	wi.text = "Program:";
	ghLabel10 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 230;
	wi.text = "00:00:00";
	ghLabel6 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel6,100,"Program timer:");

	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 230;
	wi.text = "Program timer:";
	ghLabel11 = gwinLabelCreate(0, &wi);
	
	wi.g.width = 110; wi.g.height = 20; wi.g.x = 120, wi.g.y = 250;
	wi.text = "00:00:00";
	ghLabel7 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel7,100,"Total timer:");

	wi.g.width = 110; wi.g.height = 20; wi.g.x = 10, wi.g.y = 250;
	wi.text = "Total timer:";
	ghLabel12 = gwinLabelCreate(0, &wi);
}



int SLD_DisplInit(void)
{ 
GFXPreinit();	
gfxInit();	

	
	
	GEvent* pe;


	// Set the widget defaults
	gwinSetDefaultFont(gdispOpenFont("U11"));
	gwinSetDefaultStyle(&WhiteWidgetStyle, gFalse);
	gdispClear(GFX_WHITE);

	// create the widget
	createButtons();
	createLists();
	createLabels();

	// We want to listen for widget events
	geventListenerInit(&gl);
	gwinAttachListener(&gl);
	gdispSetBacklight(50);
	
	fileListInit();
	
return 0;	
};

int SLDw(void)
{ 
	//event handling
	pe = geventEventWait(&gl,10 ); //gDelayForever
	switch(pe->type){
		case GEVENT_GWIN_BUTTON:
			if (((GEventGWinButton*)pe)->gwin == ghButton1){
				playFileInList=gwinListGetSelected(ghList1);
				fpgaFlags.playStart=1;
				fpgaFlags.fpgaConfig=1;
//				fpgaFlags.labelsUpdate=1;
			}
			if (((GEventGWinButton*)pe)->gwin == ghButton2){
				if(curState==3){
					fpgaFlags.playStop=1;
				}
			}
			break;
		default:
			break;
	}
	
	//information output to the display
	if(fpgaFlags.fileListUpdate==1){
//		fpgaFlags.fileListUpdate=0;
		if(fpgaFlags.addListItem==1){
			fpgaFlags.addListItem=0;
			gwinListAddItem(ghList1, (char*)fileName, gTrue);
		}
	}
	
	if(fpgaFlags.addNewListItem==1){
		fpgaFlags.addNewListItem=0;
		gwinListAddItem(ghList1, (char*)fileName, gTrue);
	}
	
	if(fpgaFlags.clearList==1){
		fpgaFlags.clearList=0;
		gwinListDeleteAll(ghList1);
	}
	
	if(fpgaFlags.endOfFile==1){
		fpgaFlags.endOfFile=0;
		gwinListSetSelected(ghList1,playFileSector,TRUE);
		gwinSetText(ghLabel5,gwinListGetSelectedText(ghList1),gFalse);
//			playFileInList=gwinListGetSelected(ghList1);
	}
	
	if(fpgaFlags.playStop==1){
//		fpgaFlags.playStop=0;
		gwinSetText(ghLabel3,"Init OK",gFalse);
		gwinSetText(ghLabel4,"Stop",gFalse);
		gwinSetText(ghLabel5,"Not selected",gFalse);
		gwinSetText(ghLabel6,"00:00:00",gFalse);
		gwinSetText(ghLabel7,"00:00:00",gFalse);
		totalSec=0;
		totalMin=0;
		totalHour=0;
		fileSec=0;
		fileMin=0;
		fileHour=0;
	}
	
	if(fpgaFlags.fpgaConfig==1){
		fpgaFlags.fpgaConfig=0;
		gwinSetText(ghLabel3,"Config. Please wait",gFalse);
	}
	
	if(fpgaFlags.labelsUpdate==1){
		fpgaFlags.labelsUpdate=0;
		if(fpgaFlags.fpgaConfigComplete==1){
			gwinSetText(ghLabel3,"Config OK",gFalse);
		}
		else{
			gwinSetText(ghLabel3,"Config failed",gFalse);
		}
		if(fpgaFlags.playBegin==1){
			gwinSetText(ghLabel4,"Start",gFalse);
			gwinSetText(ghLabel5,gwinListGetSelectedText(ghList1),gFalse);
		}
	}
	
//	if(fpgaFlags.timeUpdate==1){
//		fpgaFlags.timeUpdate=0;
//	
//		gwinSetText(ghLabel6,fileTimeArr,gFalse);
//		
//		
//		gwinSetText(ghLabel7,totalTimeArr,gFalse);
//		
//	}
	
	if(playClk>=999){
		playClk=0;
		
		//Program timer
		if(fileSec==0){
			fileSec=59;
			if(fileMin==0){
				fileMin=59;
				if(fileHour==0){
					fileHour=99;
				}
				else{
					fileHour--;
				}
			}
			else{
				fileMin--;
			}
		}
		else{
			fileSec--;
		}
		fileTimeArr[0]=fileHour/10;
		fileTimeArr[1]=fileHour%10;
		fileTimeArr[3]=fileMin/10;
		fileTimeArr[4]=fileMin%10;
		fileTimeArr[6]=fileSec/10;
		fileTimeArr[7]=fileSec%10;
		timeToString(fileTimeArr);
		gwinSetText(ghLabel6,fileTimeArr,gFalse);
	
		//Total timer
		if(totalSec==0){
			totalSec=59;
			if(totalMin==0){
				totalMin=59;
				if(totalHour==0){
					totalHour=99;
				}
				else{
					totalHour--;
				}
			}
			else{
				totalMin--;
			}
		}
		else{
			totalSec--;
		}
		totalTimeArr[0]=totalHour/10;
		totalTimeArr[1]=totalHour%10;
		totalTimeArr[3]=totalMin/10;
		totalTimeArr[4]=totalMin%10;
		totalTimeArr[6]=totalSec/10;
		totalTimeArr[7]=totalSec%10;
		timeToString(totalTimeArr);
		gwinSetText(ghLabel7,totalTimeArr,gFalse);
	}

return 0;	
};
