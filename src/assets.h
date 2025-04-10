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

extern tPtplayerMod *g_pModMenu;
extern tPtplayerMod *g_pModGame;
// extern tPtplayerSamplePack *g_pModSamples;

#endif // SURVIVOR_ASSETS_H

