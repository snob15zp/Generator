#ifndef __ugfxport_H
#define __ugfxport_H

#include "BoardSetup.h"
#include "gfx.h"

extern systemticks_t gfxSystemTicks(void);
extern systemticks_t gfxMillisecondsToTicks(delaytime_t ms);

#endif
