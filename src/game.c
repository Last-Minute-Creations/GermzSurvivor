/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include "survivor.h"

static void gameGsCreate(void) {
	systemUnuse();
}

static void gameGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		statePop(g_pGameStateManager);
		return;
	}


}

static void gameGsDestroy(void) {
	systemUse();
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy,
};
