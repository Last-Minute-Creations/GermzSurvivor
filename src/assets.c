// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "assets.h"
#include "game.h"

void assetsGlobalCreate(void) {
	g_pFont = fontCreateFromPath("data/uni54.fnt");
	g_pLineBuffer = fontCreateTextBitMap(GAME_MAIN_VPORT_SIZE_X, g_pFont->uwHeight);
	// g_pModSamples = ptplayerSampleDataCreateFromPath("data/music.samplepack");
}

void assetsGlobalDestroy(void) {
	fontDestroy(g_pFont);
	fontDestroyTextBitMap(g_pLineBuffer);

	// ptplayerSamplePackDestroy(g_pModSamples);
}

void assetsGameCreate(void) {
	g_pFontSmall = fontCreateFromPath("data/uni54_small.fnt");
	g_pLogo = bitmapCreateFromPath("data/logo.bm", 0);

	g_pModGame = ptplayerModCreateFromPath("data/game.mod");
	g_pModMenu = ptplayerModCreateFromPath("data/menu.mod");
}

void assetsGameDestroy(void) {
	fontDestroy(g_pFontSmall);
	bitmapDestroy(g_pLogo);

	ptplayerModDestroy(g_pModGame);
	ptplayerModDestroy(g_pModMenu);
}

tFont *g_pFont;
tFont *g_pFontSmall;

tBitMap *g_pLogo;
tTextBitMap *g_pLineBuffer;

tPtplayerMod *g_pModMenu;
tPtplayerMod *g_pModGame;
// tPtplayerSamplePack *g_pModSamples;
