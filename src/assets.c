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

	g_pSfxRifle[0] = ptplayerSfxCreateFromPath("data/sfx/rifle_shot_1.sfx", 1);
	g_pSfxRifle[1] = ptplayerSfxCreateFromPath("data/sfx/rifle_shot_2.sfx", 1);
	g_pSfxAssault[0] = ptplayerSfxCreateFromPath("data/sfx/assault_shot_1.sfx", 1);
	g_pSfxAssault[1] = ptplayerSfxCreateFromPath("data/sfx/assault_shot_2.sfx", 1);
	g_pSfxSmg[0] = ptplayerSfxCreateFromPath("data/sfx/smg_shot_1.sfx", 1);
	g_pSfxSmg[1] = ptplayerSfxCreateFromPath("data/sfx/smg_shot_2.sfx", 1);
	g_pSfxShotgun[0] = ptplayerSfxCreateFromPath("data/sfx/shotgun_shot_1.sfx", 1);
	g_pSfxShotgun[1] = ptplayerSfxCreateFromPath("data/sfx/shotgun_shot_2.sfx", 1);
	g_pSfxImpact[0] = ptplayerSfxCreateFromPath("data/sfx/impact_1.sfx", 1);
	g_pSfxImpact[1] = ptplayerSfxCreateFromPath("data/sfx/impact_2.sfx", 1);
	g_pSfxBite[0] = ptplayerSfxCreateFromPath("data/sfx/bite_1.sfx", 1);
	g_pSfxBite[1] = ptplayerSfxCreateFromPath("data/sfx/bite_2.sfx", 1);
	g_pSfxReload = ptplayerSfxCreateFromPath("data/sfx/reload_1.sfx", 1);
}

void assetsGameDestroy(void) {
	fontDestroy(g_pFontSmall);
	bitmapDestroy(g_pLogo);

	ptplayerModDestroy(g_pModGame);
	ptplayerModDestroy(g_pModMenu);

	ptplayerSfxDestroy(g_pSfxRifle[0]);
	ptplayerSfxDestroy(g_pSfxRifle[1]);
	ptplayerSfxDestroy(g_pSfxAssault[0]);
	ptplayerSfxDestroy(g_pSfxAssault[1]);
	ptplayerSfxDestroy(g_pSfxSmg[0]);
	ptplayerSfxDestroy(g_pSfxSmg[1]);
	ptplayerSfxDestroy(g_pSfxShotgun[0]);
	ptplayerSfxDestroy(g_pSfxShotgun[1]);
	ptplayerSfxDestroy(g_pSfxImpact[0]);
	ptplayerSfxDestroy(g_pSfxImpact[1]);
	ptplayerSfxDestroy(g_pSfxBite[0]);
	ptplayerSfxDestroy(g_pSfxBite[1]);
	ptplayerSfxDestroy(g_pSfxReload);
}

tFont *g_pFont;
tFont *g_pFontSmall;

tBitMap *g_pLogo;
tTextBitMap *g_pLineBuffer;

tPtplayerMod *g_pModMenu;
tPtplayerMod *g_pModGame;
// tPtplayerSamplePack *g_pModSamples;

tPtplayerSfx *g_pSfxRifle[2];
tPtplayerSfx *g_pSfxAssault[2];
tPtplayerSfx *g_pSfxSmg[2];
tPtplayerSfx *g_pSfxShotgun[2];
tPtplayerSfx *g_pSfxImpact[2];
tPtplayerSfx *g_pSfxBite[2];
tPtplayerSfx *g_pSfxReload;
