/*---------------------------------------------------------------------------------

	Copyright (C) 2017
		Dave Murphy (WinterMute)

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any
	damages arising from the use of this software.

	Permission is granted to anyone to use this software for any
	purpose, including commercial applications, and to alter it and
	redistribute it freely, subject to the following restrictions:

	1.	The origin of this software must not be misrepresented; you
		must not claim that you wrote the original software. If you use
		this software in a product, an acknowledgment in the product
		documentation would be appreciated but is not required.
	2.	Altered source versions must be plainly marked as such, and
		must not be misrepresented as being the original software.
	3.	This notice may not be removed or altered from any source
		distribution.


---------------------------------------------------------------------------------*/
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/bios.h>

#define BASE_DELAY (100)

void twlEnableSlot1(void) {
	int oldIME = enterCriticalSection();

	while((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_REQUEST_OFF) swiDelay(1 * BASE_DELAY);

	if((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_OFF) {

		REG_SCFG_MC = (REG_SCFG_MC & ~SCFG_MC_PWR_MASK) | SCFG_MC_PWR_RESET;
		swiDelay(10 * BASE_DELAY);
		REG_SCFG_MC = (REG_SCFG_MC & ~SCFG_MC_PWR_MASK) | SCFG_MC_PWR_ON;
		swiDelay(10 * BASE_DELAY);
	}
	leaveCriticalSection(oldIME);
}

void twlDisableSlot1(void) {
	int oldIME = enterCriticalSection();

	while((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_REQUEST_OFF) swiDelay(1 * BASE_DELAY);

	if((REG_SCFG_MC & SCFG_MC_PWR_MASK) == SCFG_MC_PWR_ON) {

		REG_SCFG_MC = (REG_SCFG_MC & ~SCFG_MC_PWR_MASK) | SCFG_MC_PWR_REQUEST_OFF;
		while((REG_SCFG_MC & SCFG_MC_PWR_MASK) != SCFG_MC_PWR_OFF) swiDelay(1 * BASE_DELAY);
	}

	leaveCriticalSection(oldIME);
}
