// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "pause.h"
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include "comm/comm.h"
#include "comm/button.h"
#include "menu.h"
#include "assets.h"
#include "game.h"
#include "survivor.h"

typedef enum tPauseButton {
	PAUSE_BUTTON_RESUME,
	PAUSE_BUTTON_EXIT,
} tPauseButton;

static UBYTE s_isGoBackToGame;

static void pauseGsCreate(void) {
	logBlockBegin("menuGsCreate()");
	commShow();
	tUwCoordYX sOrigin = commGetOriginDisplay();
	blitCopy(
		g_pLogo, 0, 0, commGetDisplayBuffer(),
		sOrigin.uwX + (COMM_DISPLAY_WIDTH - LOGO_SIZE_X) / 2, sOrigin.uwY,
		LOGO_SIZE_X, LOGO_SIZE_Y, MINTERM_COOKIE
	);

	buttonReset();
	UWORD uwY = LOGO_SIZE_Y + 16;
	buttonAdd("Resume", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("Exit to main menu", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonDrawAll(commGetDisplayBuffer());

	viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
	copProcessBlocks();
	ptplayerEnableMusic(0);
	logBlockEnd("menuGsCreate()");
}

static void pauseGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		s_isGoBackToGame = 1;
		statePop(g_pGameStateManager);
		return;
	}

	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		UBYTE ubSelectedButton = buttonGetSelected();
		switch(ubSelectedButton) {
			case PAUSE_BUTTON_RESUME:
				s_isGoBackToGame = 1;
				statePop(g_pGameStateManager);
				break;
			case PAUSE_BUTTON_EXIT:
				stateChange(g_pGameStateManager, &g_sStateMenu);
				break;
		}
		return;
	}

	UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
	UWORD uwMouseY = mouseGetY(MOUSE_PORT_1) - GAME_HUD_VPORT_SIZE_Y;
	buttonProcess(uwMouseX, uwMouseY, commGetDisplayBuffer());
	gameProcessCursor(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));
	vPortWaitForEnd(g_pGameBufferMain->sCommon.pVPort);
}

static void pauseGsDestroy(void) {
	if(s_isGoBackToGame) {
		// Sync with game's double buffering
		viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
		copProcessBlocks();
		commHide();
		gameEnableFrameCounter();
		ptplayerEnableMusic(1);
	}
	else {
		gameDiscardUndraw();
	}
}

tState g_sStatePause = {
	.cbCreate = pauseGsCreate, .cbLoop = pauseGsLoop, .cbDestroy = pauseGsDestroy
};
