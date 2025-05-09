/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "survivor.h"
#include <ace/generic/main.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/ptplayer.h>
#include <ace/contrib/managers/audio_mixer.h>
#include "game.h"
#include "cutscene.h"
#include "assets.h"

tStateManager *g_pGameStateManager;

void genericCreate(void) {
	keyCreate();
	mouseCreate(MOUSE_PORT_1);
	g_pGameStateManager = stateManagerCreate();

	// Bare minimum
	ptplayerCreate(1);
	ptplayerSetChannelsForPlayer(0b0111);
	ptplayerSetMasterVolume(20);
	audioMixerCreate();

	assetsGlobalCreate();
	// statePush(g_pGameStateManager, &g_sStateCutscene);
	statePush(g_pGameStateManager, &g_sStateGame);
}

void genericProcess(void) {
	keyProcess();
	mouseProcess();
	stateProcess(g_pGameStateManager);
}

void genericDestroy(void) {
	audioMixerDestroy();
	ptplayerDestroy();

	assetsGlobalDestroy();
	stateManagerDestroy(g_pGameStateManager);
	keyDestroy();
	mouseDestroy();
}
