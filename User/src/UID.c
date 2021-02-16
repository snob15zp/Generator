#include "stm32g0xx.h"
#include "UID.h"
#include <stdio.h>

#define unique_ID_base_addr 0x1FFF7590

uint16_t *idBase0 = (uint16_t*)(unique_ID_base_addr);
uint16_t *idBase1 = (uint16_t*)(unique_ID_base_addr + 0x02);
uint32_t *idBase2 = (uint32_t*)(unique_ID_base_addr + 0x04);
uint32_t *idBase3 = (uint32_t*)(unique_ID_base_addr + 0x08);

char buffer[96] = {0,};

void getUID(void)
{
	sprintf(buffer,"UID %x-%x-%x-%x\n",*idBase0,*idBase1,*idBase2,*idBase3);
}
