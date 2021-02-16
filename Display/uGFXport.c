#include "uGFXport.h"



/*************************************************************************************************************************
*
*                                   For uGFX Delays    
*	
**************************************************************************************************************************/

systemticks_t gfxSystemTicks(void)
{
	return SystemTicks;
}

systemticks_t gfxMillisecondsToTicks(delaytime_t ms)
{
	return ms;
}




