// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "menu.h"
#include <ace/managers/mouse.h>
#include <ace/managers/game.h>
#include <ace/managers/key.h>
#include <ace/contrib/managers/audio_mixer.h>
#include <ace/utils/string.h>
#include "comm/comm.h"
#include "comm/button.h"
#include "survivor.h"
#include "game.h"
#include "hi_score.h"
#include "assets.h"

#define MENU_BUTTON_MARGIN_Y 5
#define LOGO_SIZE_X 112
#define LOGO_SIZE_Y 48

typedef enum tMenuButton {
	MENU_BUTTON_SURVIVE,
	MENU_BUTTON_HOWTO,
	MENU_BUTTON_SCORES,
	MENU_BUTTON_CREDITS,
	MENU_BUTTON_QUIT,
} tMenuButton;

//---------------------------------------------------------------------- CREDITS

static void menuCreditsGsCreate(void);
static void menuCreditsGsLoop(void);
static tState s_sStateMenuCredits = {.cbCreate = menuCreditsGsCreate, .cbLoop = menuCreditsGsLoop};

static const char * const s_pCreditsLines[] = {
	"GermZ Survivor by Last Minute Creations",
	"lastminutecreations.itch.io/germz-survivor",
	"",
	"Created for RAM#3 Gamedev Competition",
	"",
	"Beorning: sounds",
	"Luc3k: music",
	"KaiN: code, game design",
	"Softiron: graphics",
	"",
	"Made with Amiga C Engine",
	"github.com/AmigaPorts/ACE",
	"",
	"Thanks for playing!"
};

static void menuCreditsGsCreate(void) {
	logBlockBegin("menuCreditsGsCreate()");
	commEraseAll();
	UWORD uwOffsY = 0;
	UBYTE ubLineHeight = commGetLineHeight() - 2;
	for(UBYTE i = 0; i < ARRAY_SIZE(s_pCreditsLines); ++i) {
		if(stringIsEmpty(s_pCreditsLines[i])) {
			uwOffsY += ubLineHeight;
		}
		else {
			uwOffsY += commDrawMultilineText(s_pCreditsLines[i], 0, uwOffsY) * ubLineHeight;
		}
	}
	logBlockEnd("menuCreditsGsCreate()");
}

static void menuCreditsGsLoop(void) {
	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		audioMixerPlaySfx(g_pSfxImpact, 0, 0, 0);
		statePop(g_pGameStateManager);
		return;
	}

	gameProcessCursor(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));
	vPortWaitForEnd(g_pGameBufferMain->sCommon.pVPort);
}

//----------------------------------------------------------------------- HOW TO

static void menuHowtoGsCreate(void);
static void menuHowtoGsLoop(void);
static tState s_sStateMenuHowto = {.cbCreate = menuHowtoGsCreate, .cbLoop = menuHowtoGsLoop};

static const char * const s_pHowtoLines[] = {
	"Move with WSAD, aim with mouse, shoot with Left Mouse Button",
	"",
	"Killing enemies gives you exp, filling exp bar gives you levels",
	"",
	"After level up, select perks after pressing Right Mouse Button",
	"",
	"Watch out: as you level up, enemies get stronger and faster"
};

static void menuHowtoGsCreate(void) {
	logBlockBegin("menuHowtoGsCreate()");
	commEraseAll();
	UWORD uwOffsY = 0;
	UBYTE ubLineHeight = commGetLineHeight() - 2;
	for(UBYTE i = 0; i < ARRAY_SIZE(s_pHowtoLines); ++i) {
		if(stringIsEmpty(s_pHowtoLines[i])) {
			uwOffsY += ubLineHeight;
		}
		else {
			uwOffsY += commDrawMultilineText(s_pHowtoLines[i], 0, uwOffsY) * ubLineHeight;
		}
	}
	logBlockEnd("menuHowtoGsBegin()");
}

static void menuHowtoGsLoop(void) {
	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		audioMixerPlaySfx(g_pSfxImpact, 0, 0, 0);
		statePop(g_pGameStateManager);
		return;
	}

	gameProcessCursor(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));
	vPortWaitForEnd(g_pGameBufferMain->sCommon.pVPort);
}

//----------------------------------------------------------------------- SCORES

static void menuScoreGsCreate(void);
static void menuScoreGsLoop(void);
static tState s_sStateMenuScore = {.cbCreate = menuScoreGsCreate, .cbLoop = menuScoreGsLoop};

static void menuScoreGsCreate(void) {
	logBlockBegin("menuScoreGsCreate()");
	commEraseAll();
	hiScoreDrawAll();
	logBlockEnd("menuScoreGsCreate()");
}

static void menuScoreGsLoop(void) {
	if(hiScoreIsEnteringNew()) {
		hiScoreEnteringProcess();
	}
	else {
		if(mouseUse(MOUSE_PORT_1, MOUSE_LMB) || keyUse(KEY_RETURN)) {
			audioMixerPlaySfx(g_pSfxImpact, 0, 0, 0);
			statePop(g_pGameStateManager);
			return;
		}
	}

	gameProcessCursor(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));
	vPortWaitForEnd(g_pGameBufferMain->sCommon.pVPort);
}

//---------------------------------------------------------------------- SUMMARY

#define SUMMARY_COLUMN_OFFSET_X 40

static void menuSummaryGsCreate(void);
static void menuSummaryGsLoop(void);
static tState s_sStateMenuSummary = {.cbCreate = menuSummaryGsCreate, .cbLoop = menuSummaryGsLoop};

static void menuSummaryGsCreate(void) {
	logBlockBegin("menuSummaryGsCreate()");
	commEraseAll();
	UWORD uwOffsY = 0;
	UBYTE ubLineHeight = commGetLineHeight() - 2;
	commDrawText(
		COMM_DISPLAY_WIDTH / 2, uwOffsY, "YOU WILL SERVE.",
		FONT_COOKIE | FONT_HCENTER, COMM_DISPLAY_COLOR_TEXT_HOVER
	);
	uwOffsY += 2 * ubLineHeight;

	char szBuffer[20];
	ULONG ulTicks = gameGetSurviveTime();
	ulTicks /= 50;
	UBYTE ubMinutes = ulTicks / 60;
	UBYTE ubSeconds = ulTicks % 60;
	sprintf(szBuffer, "%0hu:%02hu", ubMinutes, ubSeconds);
	commDrawText(SUMMARY_COLUMN_OFFSET_X, uwOffsY, "Time survived:", FONT_COOKIE, COMM_DISPLAY_COLOR_TEXT);
	commDrawText(COMM_DISPLAY_WIDTH - SUMMARY_COLUMN_OFFSET_X, uwOffsY, szBuffer, FONT_COOKIE | FONT_RIGHT, COMM_DISPLAY_COLOR_TEXT);
	uwOffsY += 2 * ubLineHeight;

	stringDecimalFromULong(gameGetKills(), szBuffer);
	commDrawText(SUMMARY_COLUMN_OFFSET_X, uwOffsY, "Kills:", FONT_COOKIE, COMM_DISPLAY_COLOR_TEXT);
	commDrawText(COMM_DISPLAY_WIDTH - SUMMARY_COLUMN_OFFSET_X, uwOffsY, szBuffer, FONT_COOKIE | FONT_RIGHT, COMM_DISPLAY_COLOR_TEXT);
	uwOffsY += 2 * ubLineHeight;

	stringDecimalFromULong(gameGetLevel(), szBuffer);
	commDrawText(SUMMARY_COLUMN_OFFSET_X, uwOffsY, "Level:", FONT_COOKIE, COMM_DISPLAY_COLOR_TEXT);
	commDrawText(COMM_DISPLAY_WIDTH - SUMMARY_COLUMN_OFFSET_X, uwOffsY, szBuffer, FONT_COOKIE | FONT_RIGHT, COMM_DISPLAY_COLOR_TEXT);
	uwOffsY += 2 * ubLineHeight;

	stringDecimalFromULong(gameGetExp(), szBuffer);
	commDrawText(SUMMARY_COLUMN_OFFSET_X, uwOffsY, "EXP gained:", FONT_COOKIE, COMM_DISPLAY_COLOR_TEXT);
	commDrawText(COMM_DISPLAY_WIDTH - SUMMARY_COLUMN_OFFSET_X, uwOffsY, szBuffer, FONT_COOKIE | FONT_RIGHT, COMM_DISPLAY_COLOR_TEXT);
	uwOffsY += 2 * ubLineHeight;

	hiScoreSetup(gameGetExp(), 1);
	if(hiScoreIsEnteringNew()) {
		commDrawText(
			COMM_DISPLAY_WIDTH / 2, uwOffsY, "New record!",
			FONT_COOKIE | FONT_HCENTER, COMM_DISPLAY_COLOR_TEXT
		);
		uwOffsY += ubLineHeight;
	}
	commDrawText(
		COMM_DISPLAY_WIDTH / 2, uwOffsY, "Click to continue",
		FONT_COOKIE | FONT_HCENTER, COMM_DISPLAY_COLOR_TEXT_HOVER
	);
	while(mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
		mouseProcess();
	}
	logBlockEnd("menuSummaryGsCreate()");
}

static void menuSummaryGsLoop(void) {
	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		audioMixerPlaySfx(g_pSfxShotgun, 0, 0, 0);
		stateChange(g_pGameStateManager, &s_sStateMenuScore);
		return;
	}

	gameProcessCursor(mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1));
	vPortWaitForEnd(g_pGameBufferMain->sCommon.pVPort);
}

//------------------------------------------------------------------- PUBLIC FNS

void menuPush(UBYTE isDead) {
	statePush(g_pGameStateManager, &g_sStateMenu);
	if(isDead) {
		statePush(g_pGameStateManager, &s_sStateMenuSummary);
	}
}

//-------------------------------------------------------------------- GAMESTATE

static void menuGsCreate(void) {
	logBlockBegin("menuGsCreate()");
	UBYTE isShown = commIsShown();
	if(!isShown) {
		commShow();
	}
	else {
		commEraseAll();
	}
	tUwCoordYX sOrigin = commGetOriginDisplay();
	blitCopy(
		g_pLogo, 0, 0, commGetDisplayBuffer(),
		sOrigin.uwX + (COMM_DISPLAY_WIDTH - LOGO_SIZE_X) / 2, sOrigin.uwY,
		LOGO_SIZE_X, LOGO_SIZE_Y, MINTERM_COOKIE
	);

	buttonReset();
	UWORD uwY = LOGO_SIZE_Y + 16;
	buttonAdd("Survive", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("How to play", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("High scores", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("Credits", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonAdd("Quit", COMM_DISPLAY_WIDTH / 2, uwY); uwY += buttonGetHeight() + MENU_BUTTON_MARGIN_Y;
	buttonDrawAll(commGetDisplayBuffer());

	commDrawText(
		COMM_DISPLAY_WIDTH / 2, COMM_DISPLAY_HEIGHT, "v" GAME_VERSION,
		FONT_COOKIE | FONT_BOTTOM | FONT_HCENTER, COMM_DISPLAY_COLOR_TEXT_DARK
	);

	if(!isShown) {
		viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
		copProcessBlocks();
	}
	ptplayerLoadMod(g_pModMenu, 0, 0); // g_pModSamples
	ptplayerEnableMusic(1);
	logBlockEnd("menuGsCreate()");
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
				audioMixerPlaySfx(g_pSfxShotgun, 0, 0, 0);
				gameStart();
				statePop(g_pGameStateManager);
				break;
			case MENU_BUTTON_HOWTO:
				audioMixerPlaySfx(g_pSfxShotgun, 0, 0, 0);
				statePush(g_pGameStateManager, &s_sStateMenuHowto);
				break;
			case MENU_BUTTON_SCORES:
				audioMixerPlaySfx(g_pSfxShotgun, 0, 0, 0);
				hiScoreSetup(0, 0);
				statePush(g_pGameStateManager, &s_sStateMenuScore);
				break;
			case MENU_BUTTON_CREDITS:
				audioMixerPlaySfx(g_pSfxShotgun, 0, 0, 0);
				statePush(g_pGameStateManager, &s_sStateMenuCredits);
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

static void menuGsResume(void) {
	commEraseAll();
	tUwCoordYX sOrigin = commGetOriginDisplay();
	blitCopy(
		g_pLogo, 0, 0, commGetDisplayBuffer(),
		sOrigin.uwX + (COMM_DISPLAY_WIDTH - LOGO_SIZE_X) / 2, sOrigin.uwY,
		LOGO_SIZE_X, LOGO_SIZE_Y, MINTERM_COOKIE
	);
	buttonDrawAll(commGetDisplayBuffer());
}

tState g_sStateMenu = {
	.cbCreate = menuGsCreate, .cbLoop = menuGsLoop, .cbDestroy = menuGsDestroy,
	.cbResume = menuGsResume,
};
