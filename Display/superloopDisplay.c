/*!
\file
\brief High Level Display control


*/	


/*!
\brief info on display for debug ACC 

*/
#define def_debug_AccDispay

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "superloopDisplay.h"
#include "mainFSM.h"
#include "board_PowerModes.h"

extern char	heap[GFX_OS_HEAP_SIZE];
extern gThread	hThread;
void GFXPreinit (void)
{ 
	uint32_t i;

	for (i=0;i<GFX_OS_HEAP_SIZE;i++)
	{heap[i]=0;};
	hThread=0;
};

//------------------------- for power--------------------------------------
bool SuperLoop_Disp_SleepIn(void)
{
	return true;
};
bool SuperLoop_Disp_SleepOut(void)
{
	return true;
};

//------------------------for FSM-------------------------------------------
static e_FSMState_SuperLoopDisplay SLD_FSM_State;

//------------------------ for update ------------------------------------------
static systemticks_t LastUpdateTime;
#define DisplayUpdatePeriod 1000

//--------------------------------for uGFX--------------------------------------
GListener	gl;
GHandle		ghLabel1, ghLabel2, ghLabel3, ghLabel4, ghLabel5, ghLabel6, ghLabel7, ghLabel8;
GHandle		ghList1;
static	GEvent* pe;
//static const gOrientation	orients[] = { gOrientation0, gOrientation90, gOrientation180, gOrientation270 };
static	unsigned which;

static void createDebugLabels(void);
//---------------------- Control grafical objects------------------------------
int SLDw(void);
void displayACC(void);
int SLDwACC(void);
//------------------------FSM control--------------------------------------------
int SLD_DisplInit(void);
int SLD_DisplReInit(void);
int SLD_DisplDeInit(void);


__inline e_FSMState_SuperLoopDisplay SLD_FSMState(void)
{
	return SLD_FSM_State;
};
static uint8_t state_inner;
//-------------------------for main-----------------------------------------------
int SLD(void)
{
	switch (state_inner)
	{
		case 0: 
			SLD_FSM_State=	e_FSMS_SLD_Off;
			if (bVSYS)
			{
				while (e_FRS_Done!=MainTransition_P_Displ(e_FSMS_SLD_On,e_FSMS_SLD_Off));
				while (e_FRS_Done!=MainTransition_P_Displ(e_FSMS_SLD_On,e_FSMS_SLD_On));
				delayms(100);
				SLD_DisplInit();

				gwinRedrawDisplay(NULL,true);
				
				SLD_FSM_State=e_FSMS_SLD_On;
		    state_inner=3;
			}	
			break;
		case 1:	
			SLD_FSM_State=	e_FSMS_SLD_Off;
			if (button_sign&&bVSYS)
			{
				state_inner=2;
				button_sign=0;
			};
			break;
		case 2:
				while (e_FRS_Done!=MainTransition_P_Displ(e_FSMS_SLD_On,e_FSMS_SLD_Off));
				while (e_FRS_Done!=MainTransition_P_Displ(e_FSMS_SLD_On,e_FSMS_SLD_On));
				SLD_DisplInit();
		    gwinRedrawDisplay(NULL,true);
				SLD_FSM_State=e_FSMS_SLD_On;
		    state_inner=3;

		case 3: 
#ifdef def_debug_AccDispay
	    	SLDwACC();
#else
		    SLDw();
#endif		
  		if ((!bVSYS)|button_sign)
			{
				SLD_DisplDeInit();
				
				while (e_FRS_Done!=MainTransition_P_Displ(e_FSMS_SLD_Off,e_FSMS_SLD_On));
				while (e_FRS_Done!=MainTransition_P_Displ(e_FSMS_SLD_Off,e_FSMS_SLD_Off));
				
				button_sign=0;
				SLD_FSM_State=e_FSMS_SLD_Off;
				state_inner=1;
			};
			break;
    default: SLD_FSM_State=	e_FSMS_SLD_Off;		
	};
	return 0;
}

int SLD_init(void)
{
	return 0;
};

////------------------------FSM control--------------------------------------------

//GListener	gl;
//GHandle		ghLabel1, ghLabel2, ghLabel3, ghLabel4, ghLabel5, ghLabel6, ghLabel7;
//GHandle		ghList1 /*, ghList2*/;
//static	GEvent* pe;
//static	unsigned which;
//static GHandle  ghButton1, ghButton2;

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
/*
volatile struct fpgaFlags {
	uint16_t playStart						:1;
	uint16_t playBegin						:1;
	uint16_t fpgaConfig						:1;
	uint16_t playStop							:1;
	uint16_t fpgaConfigComplete		:1;
	uint16_t fileListUpdate				:1;
	uint16_t labelsUpdate					:1;
	uint16_t clockStart						:1;
	uint16_t nextFreq							:1;
	uint16_t endOfFile						:1;
} ;
*/
uint8_t spiDispCapture;	//0 - free, 1 - busy
uint8_t totalTimeArr[]={'0','0',':','0','0',':','0','0',0};
uint8_t fileTimeArr[]={'0','0',':','0','0',':','0','0',0};
uint8_t totalSec=0;
uint8_t totalMin=0;
uint8_t totalHour=0;
uint8_t fileSec=0;
uint8_t fileMin=0;
uint8_t fileHour=0;
volatile uint32_t playClk;
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
		case e_FSM_Charge:		strcpy(str,"mainFMSstate: Charge"); 			break;
		case 	e_FSM_Rest:    	strcpy(str,"mainFMSstate: Rest"); 				break;
		case e_FSM_ChargeOff:	strcpy(str,"mainFMSstate: ChargeOff"); 		break;
		case e_FSM_RestOff:  	strcpy(str,"mainFMSstate: RestOff"); 		break;
		case e_FSM_Init: 			strcpy(str,"mainFMSstate: Init");					break;
		default: 							strcpy(str,"mainFMSstate: Out of Range");
	}	
	gwinSetText(ghLabel7, str, TRUE);
	
	
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
	
	wi.g.width = 220; wi.g.height = 20; wi.g.x = 10, wi.g.y = 170;
//	wi.text = "Self test: OK";
	wi.text = "Init OK";
	ghLabel3 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel3,100,"Self test:");
	
	wi.g.width = 220; wi.g.height = 20; wi.g.x = 10, wi.g.y = 190;
	wi.text = "Stop";
	ghLabel4 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel4,100,"Status:");
	
	wi.g.width = 220; wi.g.height = 20; wi.g.x = 10, wi.g.y = 210;
	wi.text = "Not selected";
	ghLabel5 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel5,100,"Program:");
	
	wi.g.width = 220; wi.g.height = 20; wi.g.x = 10, wi.g.y = 230;
	wi.text = "00:00:00";
	ghLabel6 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel6,100,"Program timer:");
	
	wi.g.width = 220; wi.g.height = 20; wi.g.x = 10, wi.g.y = 250;
	wi.text = "00:00:00";
	ghLabel7 = gwinLabelCreate(0, &wi);
//	gwinLabelSetAttribute(ghLabel7,100,"Total timer:");

#ifdef def_debug_AccDispay
	wi.g.width = 220; wi.g.height = 20; wi.g.x = 120, wi.g.y = 170;
	wi.text = "RSOC";
	ghLabel8 = gwinLabelCreate(0, &wi);	    

#endif

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
				fpgaFlags.labelsUpdate=1;
			}
			if (((GEventGWinButton*)pe)->gwin == ghButton2){
				fpgaFlags.playStop=1;
			}
			break;
		default:
			break;
	}
	
	//information output to the display
	
	
	if(fpgaFlags.labelsUpdate==1){
		fpgaFlags.labelsUpdate=0;
		if(fpgaFlags.fileListUpdate==1){
			gwinListAddItem(ghList1, (char*)fileName, gTrue);
		}
		if(fpgaFlags.fpgaConfigComplete==1){
			gwinSetText(ghLabel3,"Config OK",gTrue);
		}
		else{
			gwinSetText(ghLabel3,"Config failed",gTrue);
		}
		if(fpgaFlags.playBegin==1){
			gwinSetText(ghLabel4,"Start",gTrue);
			gwinSetText(ghLabel5,gwinListGetSelectedText(ghList1),gTrue);
		}
		if(fpgaFlags.playStop==1){
			fpgaFlags.playStop=0;
			gwinSetText(ghLabel3,"Init OK",gTrue);
			gwinSetText(ghLabel4,"Stop",gTrue);
			gwinSetText(ghLabel6,"00:00:00",gTrue);
			gwinSetText(ghLabel7,"00:00:00",gTrue);
			totalSec=0;
			totalMin=0;
			totalHour=0;
			fileSec=0;
			fileMin=0;
			fileHour=0;
		}
	}
	
	if(playClk>=999){
		playClk=0;
		
		//Program timer
		if(fpgaFlags.endOfFile==1){
			fpgaFlags.endOfFile=0;
			fileHour=0;
			fileMin=0;
			fileSec=0;
		}
		else{
			if(fileSec==59){
				fileSec=0;
				if(fileMin==59){
					fileMin=0;
					if(fileHour==99){
						fileHour=0;
					}
					else{
						fileHour++;
					}
				}
				else{
					fileMin++;
				}
			}
			else{
				fileSec++;
			}
			fileTimeArr[0]=fileHour/10;
			fileTimeArr[1]=fileHour%10;
			fileTimeArr[3]=fileMin/10;
			fileTimeArr[4]=fileMin%10;
			fileTimeArr[6]=fileSec/10;
			fileTimeArr[7]=fileSec%10;
			timeToString(fileTimeArr);
			gwinSetText(ghLabel6,fileTimeArr,gTrue);
		}
		
		//Total timer
		if(totalSec==59){
			totalSec=0;
			if(totalMin==59){
				totalMin=0;
				if(totalHour==99){
					totalHour=0;
				}
				else{
					totalHour++;
				}
			}
			else{
				totalMin++;
			}
		}
		else{
			totalSec++;
		}
		totalTimeArr[0]=totalHour/10;
		totalTimeArr[1]=totalHour%10;
		totalTimeArr[3]=totalMin/10;
		totalTimeArr[4]=totalMin%10;
		totalTimeArr[6]=totalSec/10;
		totalTimeArr[7]=totalSec%10;
		timeToString(totalTimeArr);
		gwinSetText(ghLabel7,totalTimeArr,gTrue);
	}

return 0;	
};
