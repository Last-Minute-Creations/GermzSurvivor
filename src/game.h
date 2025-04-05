// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SURVIVOR_GAME_H
#define SURVIVOR_GAME_H

#include <ace/managers/state.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/rand.h>
#include <ace/utils/font.h>
#include "perks.h"

// #define GAME_COLLISION_DEBUG
#define GAME_BPP 5
#define GAME_HUD_VPORT_SIZE_Y 16
#define GAME_MAIN_VPORT_SIZE_X 320
#define GAME_MAIN_VPORT_SIZE_Y (256 - GAME_HUD_VPORT_SIZE_Y)
#define GAME_FPS 25

#define CURSOR_SIZE 9
#define GAME_CURSOR_OFFSET_X (CURSOR_SIZE / 2)
#define GAME_CURSOR_OFFSET_Y (CURSOR_SIZE / 2)

extern tState g_sStateGame;
extern tSimpleBufferManager *g_pGameBufferMain;
extern tBitMap *g_pGamePristineBuffer;
extern tRandManager g_sRand;

void gameStart(void);

ULONG gameGetSurviveTime(void);

ULONG gameGetKills(void);

UBYTE gameGetLevel(void);

ULONG gameGetExp(void);

void gameEnableFrameCounter(void);

void gameDiscardUndraw(void);

void gameProcessCursor(UWORD uwMouseX, UWORD uwMouseY);

void gameApplyPerk(tPerk ePerk);

#endif // SURVIVOR_GAME_H
