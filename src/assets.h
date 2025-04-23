// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SURVIVOR_ASSETS_H
#define SURVIVOR_ASSETS_H

#include <ace/managers/ptplayer.h>
#include <ace/utils/font.h>

void assetsGlobalCreate(void);

void assetsGlobalDestroy(void);

void assetsGameCreate(void);

void assetsGameDestroy(void);

extern tFont *g_pFont;
extern tFont *g_pFontSmall;

extern tBitMap *g_pLogo;
extern tTextBitMap *g_pLineBuffer;

extern tBitMap *g_pPerkIcons;

extern tPtplayerMod *g_pModMenu;
extern tPtplayerMod *g_pModGame;
// extern tPtplayerSamplePack *g_pModSamples;

extern tPtplayerSfx *g_pSfxRifle;
extern tPtplayerSfx *g_pSfxAssault;
extern tPtplayerSfx *g_pSfxSmg;
extern tPtplayerSfx *g_pSfxShotgun;
extern tPtplayerSfx *g_pSfxImpact;
extern tPtplayerSfx *g_pSfxBite;
extern tPtplayerSfx *g_pSfxReload;

#endif // SURVIVOR_ASSETS_H

