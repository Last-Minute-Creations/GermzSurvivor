// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SURVIVOR_GAME_H
#define SURVIVOR_GAME_H

#include <ace/managers/state.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/utils/font.h>

// #define GAME_COLLISION_DEBUG
#define GAME_BPP 5
#define GAME_HUD_VPORT_SIZE_Y 16
#define GAME_MAIN_VPORT_SIZE_X 320
#define GAME_MAIN_VPORT_SIZE_Y (256 - GAME_HUD_VPORT_SIZE_Y)

#define GAME_CURSOR_OFFSET_X 4
#define GAME_CURSOR_OFFSET_Y 4

extern tState g_sStateGame;
extern tSimpleBufferManager *g_pGameBufferMain;
extern tTextBitMap *g_pGameLineBuffer;
extern tBitMap *g_pGamePristineBuffer;

void gameStart(void);

void gameProcessCursor(UWORD uwMouseX, UWORD uwMouseY);

#endif // SURVIVOR_GAME_H
