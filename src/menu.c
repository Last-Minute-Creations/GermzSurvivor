// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "menu.h"
#include <ace/managers/mouse.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include "comm/comm.h"
#include "comm/button.h"
#include "survivor.h"
#include "game.h"

#define MENU_BUTTON_MARGIN_Y 5

typedef enum tMenuButton {
	MENU_BUTTON_SURVIVE,
	MENU_BUTTON_HOWTO,
	MENU_BUTTON_SCORES,
	MENU_BUTTON_CREDITS,
	MENU_BUTTON_QUIT,
} tMenuButton;

//-------------------------------------------------------------------- GAMESTATE

static void menuGsCreate(void) {
	commShow();

	buttonReset();
	UWORD uwY = 64;
	buttonAdd("Survive", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("How to play", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("High scores", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("Credits", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("Quit", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonDrawAll(commGetDisplayBuffer());

	viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
	copProcessBlocks();
}

static void menuGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		gameExit();
		return;
	}

	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		UBYTE ubSelectedButton = buttonGetSelected();
		switch(ubSelectedButton) {
			case MENU_BUTTON_SURVIVE:
				gameStart();
				statePop(g_pGameStateManager);
				break;
			case MENU_BUTTON_HOWTO:
				break;
			case MENU_BUTTON_SCORES:
				break;
			case MENU_BUTTON_CREDITS:
				break;
			case MENU_BUTTON_QUIT:
				gameExit();
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

static void menuGsDestroy(void) {
	commHide();
	viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
	copProcessBlocks();
}

tState g_sStateMenu = {
	.cbCreate = menuGsCreate, .cbLoop = menuGsLoop, .cbDestroy = menuGsDestroy
};
