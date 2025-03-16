/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "survivor.h"
#define GENERIC_MAIN_LOOP_CONDITION g_pGameStateManager->pCurrent
#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/ptplayer.h>

tStateManager *g_pGameStateManager;

void genericCreate(void) {
	keyCreate();
	mouseCreate(MOUSE_PORT_1);
	g_pGameStateManager = stateManagerCreate();

	// Bare minimum
	ptplayerCreate(1);
	ptplayerSetChannelsForPlayer(0b0111);
	ptplayerSetMasterVolume(8);
	//  g_pFont = fontCreateFromFd(GET_SUBFILE_PREFIX("uni54.fnt"));

	// statePush(g_pGameStateManager, &g_sStateLogo);
}

void genericProcess(void) {
	gameExit();
	keyProcess();
	mouseProcess();
	//  stateProcess(g_pGameStateManager);
}

void genericDestroy(void) {
	//  fontDestroy(g_pFont);
	ptplayerDestroy();

	//  stateManagerDestroy(g_pGameStateManager);
	keyDestroy();
	mouseDestroy();
}
