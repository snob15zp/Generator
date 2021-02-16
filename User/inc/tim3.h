#include "stm32g0xx.h"
#include <stdint.h>

void tim3Init(void);
void delay_ms(uint32_t delayTime);
uint32_t getTick(void);
extern volatile uint32_t playClk;
