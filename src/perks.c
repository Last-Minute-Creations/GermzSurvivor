// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "perks.h"
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/rand.h>
#include "comm/comm.h"
#include "comm/button.h"
#include "menu.h"
#include "assets.h"
#include "game.h"
#include "survivor.h"


#define PERK_CHOICE_COUNT 4
#define PERK_ICON_SIZE 32
#define PERK_ICON_MARGIN_X ((COMM_DISPLAY_WIDTH - PERK_CHOICE_COUNT * PERK_ICON_SIZE) / (PERK_CHOICE_COUNT + 1))
#define PERK_ICON_OFFSET_X PERK_ICON_MARGIN_X
#define PERK_ICON_OFFSET_TOP 20
#define PERK_ICON_OFFSET_BOTTOM 10

typedef struct tPerkText {
	const char *szTitle;
	const char *szDescription;
} tPerkText;

typedef enum tPerksButton {
	PERKS_BUTTON_ACCEPT,
	PERKS_BUTTON_CANCEL,
} tPerksButton;

static tPerk s_pAvailablePerks[PERK_COUNT];
static UBYTE s_ubUnlockedPerkCount;
static UBYTE s_isGoBackToGame;
static tPerk s_pPerkChoice[4];
static UBYTE s_ubChoiceCount;
static UBYTE s_ubSelectedPerkIndex;
static UBYTE s_isPendingChoice;

static const tPerkText s_pPerkTexts[PERK_COUNT] = {
	[PERK_GRIM_DEAL] = {
		.szTitle = "GRIM DEAL",
		.szDescription = (
			"You are going to die anyway. Why not take the most out of it?\n"
			"Increase your EXP by 20% and die instantly."
		),
	},
	[PERK_FATAL_LOTTERY] = {
		.szTitle = "FATAL LOTTERY",
		.szDescription = (
			"Flip a coin.\n"
			"If heads, you get 20 000 EXP.\n"
			"If tails, you will die instantly."
		),
	},
	[PERK_INSTANT_WINNER] = {
		.szTitle = "INSTANT WINNER",
		.szDescription = (
			"2000 EXP, no strings attached.\n"
			"Take it or leave it."
		),
	},
	[PERK_BANDAGE] = {
		.szTitle = "BANDAGE",
		.szDescription = "Heals up to 10% of your health.",
	},
	[PERK_THICK_SKINNED] = {
		.szTitle = "THICK SKINNED",
		.szDescription = "Trade 25% of your health to receive 20% less damage.",
	},
	[PERK_DEATH_CLOCK] = {
		.szTitle = "DEATH CLOCK",
		.szDescription = (
			"You are immortal for the rest of your life.\n"
			"Which will end exactly in 25 seconds.\n"
			"Tick tock."
		),
	},
	[PERK_RETALIATION] = {
		.szTitle = "RETALIATION",
		.szDescription = "When zombie bites you, punch it back in the face. Hard.",
	},
	[PERK_MY_FAVOURITE_WEAPON] = {
		.szTitle = "MY FAVOURITE WEAPON",
		.szDescription = (
			"You love your weapon so much, you want to keep it till death do you part.\n"
			"No more random weapon pickups, increases base magazine capacity by 2."
		),
	},
	[PERK_AMMO_MANIAC] = {
		.szTitle = "AMMO MANIAC",
		.szDescription = "Squeeze more bullets in the magazine, increasing its capacity by 20%.",
	},
	[PERK_ANXIOUS_LOADER] = {
		.szTitle = "ANXIOUS LOADER",
		.szDescription = (
			"All those hordes gives you anxiety, especially when you run out of ammo.\n"
			"Clicking while reloading shortens the reload time."
		),
	},
};

static void perksUnlockPending(void) {
	// TODO: check last level of unlocks
	UBYTE ubLevel = gameGetLevel();
	if(ubLevel >= 5) {
		perksUnlock(PERK_FATAL_LOTTERY);
	}
}

static void perksPrepareChoice(void) {
	s_ubChoiceCount = 0;
	for(UBYTE i = 0; i < PERK_CHOICE_COUNT; ++i) {
		s_pPerkChoice[i] = PERK_COUNT;
	}
	while(s_ubChoiceCount < PERK_CHOICE_COUNT && s_ubChoiceCount < s_ubUnlockedPerkCount) {
		tPerk ePickedPerk = s_pAvailablePerks[randUwMax(&g_sRand, PERK_COUNT - 1)];
		if(ePickedPerk == PERK_COUNT) {
			continue;
		}
		UBYTE isDuplicate = 0;
		for(UBYTE i = 0; i < s_ubChoiceCount; ++i) {
			if(ePickedPerk == s_pPerkChoice[i]) {
				isDuplicate = 1;
				break;
			}
		}
		if(!isDuplicate) {
			s_pPerkChoice[s_ubChoiceCount++] = ePickedPerk;
		}
	}
}

static tUwCoordYX perksGetIconPosition(UBYTE ubIndex) {
	tUwCoordYX sOrigin = commGetOriginDisplay();
	tUwCoordYX sPos = {
		.uwX = sOrigin.uwX + PERK_ICON_OFFSET_X + ubIndex * (PERK_ICON_MARGIN_X + PERK_ICON_SIZE),
		.uwY = sOrigin.uwY + PERK_ICON_OFFSET_TOP
	};
	return sPos;
}

static void perksDrawPerk(UBYTE ubIndex) {
	tUwCoordYX sPerkIconPos = perksGetIconPosition(ubIndex);
	blitRect(
		commGetDisplayBuffer(), sPerkIconPos.uwX, sPerkIconPos.uwY,
		PERK_ICON_SIZE, PERK_ICON_SIZE, (s_ubSelectedPerkIndex == ubIndex) ? COMM_DISPLAY_COLOR_TEXT_HOVER : COMM_DISPLAY_COLOR_TEXT_DARK
	);
}

static void perksDrawActiveDescription(void) {
	UWORD uwOffsY = PERK_ICON_OFFSET_TOP + PERK_ICON_OFFSET_BOTTOM + PERK_ICON_SIZE;
	commErase(0, uwOffsY, COMM_DISPLAY_WIDTH, COMM_DISPLAY_HEIGHT - uwOffsY - buttonGetHeight());
	if(s_ubSelectedPerkIndex >= s_ubChoiceCount) {
		return;
	}

	const tPerkText *pPerkText = &s_pPerkTexts[s_pPerkChoice[s_ubSelectedPerkIndex]];
	commDrawTitle(0, uwOffsY, pPerkText->szTitle);
	uwOffsY += commGetLineHeight();
	commDrawMultilineText(pPerkText->szDescription, 0, uwOffsY);
}

//------------------------------------------------------------------- PUBLIC FNS

void perksReset(void) {
	s_isPendingChoice = 0;
	s_ubUnlockedPerkCount = 0;
	for(tPerk ePerk = 0; ePerk < PERK_COUNT; ++ePerk) {
		s_pAvailablePerks[ePerk] = PERK_COUNT;
	}
}

void perksUnlock(tPerk ePerk) {
	s_pAvailablePerks[ePerk] = ePerk;
	++s_ubUnlockedPerkCount;
}

void perksLock(tPerk ePerk) {
	s_pAvailablePerks[ePerk] = PERK_COUNT;
	--s_ubUnlockedPerkCount;
}

//-------------------------------------------------------------------- GAMESTATE

static void perksGsCreate(void) {
	logBlockBegin("menuGsCreate()");
	commShow();
	commDrawText(COMM_DISPLAY_WIDTH / 2, 0, "PICK A PERK", FONT_HCENTER | FONT_COOKIE, COMM_DISPLAY_COLOR_TEXT);
	if(!s_isPendingChoice) {
		perksUnlockPending();
		perksPrepareChoice();
		s_isPendingChoice = 1;
	}
	s_ubSelectedPerkIndex = 0;

	for(UBYTE i = 0; i < s_ubChoiceCount; ++i) {
		perksDrawPerk(i);
	}
	perksDrawActiveDescription();

	buttonReset();
	UWORD uwY = COMM_DISPLAY_HEIGHT - buttonGetHeight();
	buttonAdd("Accept", COMM_DISPLAY_WIDTH / 3, uwY);
	buttonAdd("Cancel", 2 * COMM_DISPLAY_WIDTH / 3, uwY);
	buttonDrawAll(commGetDisplayBuffer());

	viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
	copProcessBlocks();
	logBlockEnd("menuGsCreate()");
}

static void perksGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		s_isGoBackToGame = 1;
		statePop(g_pGameStateManager);
		return;
	}

	if(mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
		UWORD uwCursorAbsX = mouseGetX(MOUSE_PORT_1) + g_pGameBufferMain->pCamera->uPos.uwX;
		UWORD uwCursorAbsY = mouseGetY(MOUSE_PORT_1) + g_pGameBufferMain->pCamera->uPos.uwY - GAME_HUD_VPORT_SIZE_Y;
		UBYTE ubPrevSelectedPerk = s_ubSelectedPerkIndex;
		for(UBYTE i = 0; i < PERK_CHOICE_COUNT; ++i) {
			tUwCoordYX sIconPos = perksGetIconPosition(i);
			if(
				s_pPerkChoice[i] != PERK_COUNT &&
				sIconPos.uwX <= uwCursorAbsX && uwCursorAbsX <= sIconPos.uwX + PERK_ICON_SIZE &&
				sIconPos.uwY <= uwCursorAbsY && uwCursorAbsY <= sIconPos.uwY + PERK_ICON_SIZE
			) {
				s_ubSelectedPerkIndex = i;
				break;
			}
		}
		if(s_ubSelectedPerkIndex != ubPrevSelectedPerk) {
			perksDrawPerk(ubPrevSelectedPerk);
			perksDrawPerk(s_ubSelectedPerkIndex);
			perksDrawActiveDescription();
		}

		UBYTE ubSelectedButton = buttonGetSelected();
		switch(ubSelectedButton) {
			case PERKS_BUTTON_ACCEPT:
				gameApplyPerk(s_pPerkChoice[s_ubSelectedPerkIndex]);
				s_isPendingChoice = 0;
				s_isGoBackToGame = 1;
				statePop(g_pGameStateManager);
				break;
			case PERKS_BUTTON_CANCEL:
				s_isGoBackToGame = 1;
				statePop(g_pGameStateManager);
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

static void perksGsDestroy(void) {
	if(s_isGoBackToGame) {
		// Sync with game's double buffering
		viewProcessManagers(g_pGameBufferMain->sCommon.pVPort->pView);
		copProcessBlocks();
		commHide();
		gameEnableFrameCounter();
	}
	else {
		gameDiscardUndraw();
	}
}

tState g_sStatePerks = {
	.cbCreate = perksGsCreate, .cbLoop = perksGsLoop, .cbDestroy = perksGsDestroy
};
