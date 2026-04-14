// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "assets.h"
#include "game.h"

void assetsGlobalCreate(void) {
	g_pFont = fontCreateFromPath("data/uni54.fnt");
	g_pLineBuffer = fontCreateTextBitMap(GAME_MAIN_VPORT_SIZE_X, g_pFont->uwHeight);
	g_pModSamples = ptplayerSampleDataCreateFromPath("data/music.samplepack");
}

void assetsGlobalDestroy(void) {
	fontDestroy(g_pFont);
	fontDestroyTextBitMap(g_pLineBuffer);

	ptplayerSamplePackDestroy(g_pModSamples);
}

void assetsGameCreate(void) {
	g_pFontSmall = fontCreateFromPath("data/uni54_small.fnt");
	g_pLogo = bitmapCreateFromPath("data/logo.bm", 0);
	g_pPerkIcons = bitmapCreateFromPath("data/perk_icons.bm", 0);

	g_pModGame = ptplayerModCreateFromPath("data/game.mod");
	g_pModMenu = ptplayerModCreateFromPath("data/menu.mod");
	g_pModGameOver = ptplayerModCreateFromPath("data/dead_hare.mod");

	g_pSfxRifle = ptplayerSfxCreateFromPath("data/sfx/rifle_shot_1.sfx", 1);
	g_pSfxAssault = ptplayerSfxCreateFromPath("data/sfx/assault_shot_1.sfx", 1);
	g_pSfxSmg = ptplayerSfxCreateFromPath("data/sfx/smg_shot_1.sfx", 1);
	g_pSfxShotgun = ptplayerSfxCreateFromPath("data/sfx/shotgun_shot_1.sfx", 1);
	g_pSfxImpact = ptplayerSfxCreateFromPath("data/sfx/impact_1.sfx", 1);
	g_pSfxBite = ptplayerSfxCreateFromPath("data/sfx/bite_1.sfx", 1);
	g_pSfxReloadClicks[0] = ptplayerSfxCreateFromPath("data/sfx/reload_1.sfx", 1);
	g_pSfxReloadClicks[1] = ptplayerSfxCreateFromPath("data/sfx/reload_2.sfx", 1);
	g_pSfxReloadClicks[2] = ptplayerSfxCreateFromPath("data/sfx/reload_3.sfx", 1);
	g_pSfxReloadClicks[3] = ptplayerSfxCreateFromPath("data/sfx/reload_4.sfx", 1);
	g_pSfxReloadFinal = ptplayerSfxCreateFromPath("data/sfx/reload_final.sfx", 1);
}

void assetsGameDestroy(void) {
	fontDestroy(g_pFontSmall);
	bitmapDestroy(g_pLogo);
	bitmapDestroy(g_pPerkIcons);

	ptplayerModDestroy(g_pModGame);
	ptplayerModDestroy(g_pModMenu);
	ptplayerModDestroy(g_pModGameOver);

	ptplayerSfxDestroy(g_pSfxRifle);
	ptplayerSfxDestroy(g_pSfxAssault);
	ptplayerSfxDestroy(g_pSfxSmg);
	ptplayerSfxDestroy(g_pSfxShotgun);
	ptplayerSfxDestroy(g_pSfxImpact);
	ptplayerSfxDestroy(g_pSfxBite);
	ptplayerSfxDestroy(g_pSfxReloadClicks[0]);
	ptplayerSfxDestroy(g_pSfxReloadClicks[1]);
	ptplayerSfxDestroy(g_pSfxReloadClicks[2]);
	ptplayerSfxDestroy(g_pSfxReloadClicks[3]);
	ptplayerSfxDestroy(g_pSfxReloadFinal);
}

tFont *g_pFont;
tFont *g_pFontSmall;

tBitMap *g_pLogo;
tTextBitMap *g_pLineBuffer;

tBitMap *g_pPerkIcons;

tPtplayerMod *g_pModMenu;
tPtplayerMod *g_pModGame;
tPtplayerMod *g_pModGameOver;
tPtplayerSamplePack *g_pModSamples;

tPtplayerSfx *g_pSfxRifle;
tPtplayerSfx *g_pSfxAssault;
tPtplayerSfx *g_pSfxSmg;
tPtplayerSfx *g_pSfxShotgun;
tPtplayerSfx *g_pSfxImpact;
tPtplayerSfx *g_pSfxBite;
tPtplayerSfx *g_pSfxReloadFinal;
tPtplayerSfx *g_pSfxReloadClicks[4];
