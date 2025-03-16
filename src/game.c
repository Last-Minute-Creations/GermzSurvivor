/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/key.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/rand.h>
#include <ace/utils/palette.h>
#include "survivor.h"

#define MAP_SIZE_X 40
#define MAP_SIZE_Y 40
#define MAP_TILE_SHIFT 4
#define GAME_BPP 5

static tView *s_pView;
static tVPort *s_pVpMain;
static tVPort *s_pVpHud;
static tTileBufferManager *s_pBufferMain;
static tSimpleBufferManager *s_pBufferHud;
static tBitMap *s_pTileset;
static tRandManager g_sRand;

static void gameGsCreate(void) {
	logBlockBegin("gameGsCreate()");
	s_pTileset = bitmapCreateFromPath("data/tiles.bm", 0);

	s_pView = viewCreate(0, TAG_END);

	s_pVpHud = vPortCreate(0,
		TAG_VPORT_BPP, GAME_BPP,
		TAG_VPORT_HEIGHT, 16,
		TAG_VPORT_VIEW, s_pView,
	TAG_END);

	s_pBufferHud = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
		TAG_SIMPLEBUFFER_VPORT, s_pVpHud,
	TAG_END);

	s_pVpMain = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, GAME_BPP,
	TAG_END);

	s_pBufferMain = tileBufferCreate(0,
		TAG_TILEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_TILEBUFFER_BOUND_TILE_X, MAP_SIZE_X,
		TAG_TILEBUFFER_BOUND_TILE_Y, MAP_SIZE_Y,
		TAG_TILEBUFFER_IS_DBLBUF, 1,
		TAG_TILEBUFFER_MAX_TILESET_SIZE, 25,
		TAG_TILEBUFFER_TILE_SHIFT, MAP_TILE_SHIFT,
		TAG_TILEBUFFER_VPORT, s_pVpMain,
		TAG_TILEBUFFER_REDRAW_QUEUE_LENGTH, 100,
		TAG_TILEBUFFER_TILESET, s_pTileset,
	TAG_END);

	paletteLoadFromPath("data/game.plt", s_pVpHud->pPalette, 1 << GAME_BPP);

	randInit(&g_sRand, 2184, 1911);

	s_pBufferMain->pTileData[0][0] = 0;
	s_pBufferMain->pTileData[MAP_SIZE_X - 1][0] = 1;
	s_pBufferMain->pTileData[0][MAP_SIZE_Y - 1] = 2;
	s_pBufferMain->pTileData[MAP_SIZE_X - 1][MAP_SIZE_Y - 1] = 3;

	for(UBYTE ubX = 1; ubX < MAP_SIZE_X - 1; ++ubX) {
		s_pBufferMain->pTileData[ubX][0] = randUwMinMax(&g_sRand, 4, 6);
		s_pBufferMain->pTileData[ubX][MAP_SIZE_Y - 1] = randUwMinMax(&g_sRand, 13, 15);
	}

	for(UBYTE ubY = 1; ubY < MAP_SIZE_Y - 1; ++ubY) {
		s_pBufferMain->pTileData[0][ubY] = randUwMinMax(&g_sRand, 7, 9);
		s_pBufferMain->pTileData[MAP_SIZE_X - 1][ubY] = randUwMinMax(&g_sRand, 10, 12);

	}

	for(UBYTE ubX = 1; ubX < MAP_SIZE_X - 1; ++ubX) {
		for(UBYTE ubY = 1; ubY < MAP_SIZE_Y - 1; ++ubY) {
			s_pBufferMain->pTileData[ubX][ubY] = randUwMinMax(&g_sRand, 16, 24);
		}
	}

	systemUnuse();
	tileBufferRedrawAll(s_pBufferMain);
	viewLoad(s_pView);
	logBlockEnd("gameGsCreate()");
}

static void gameGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		statePop(g_pGameStateManager);
		return;
	}

	viewProcessManagers(s_pView);
	copProcessBlocks();
	vPortWaitUntilEnd(s_pVpMain);
}

static void gameGsDestroy(void) {
	viewLoad(0);
	systemUse();

	viewDestroy(s_pView);
	bitmapDestroy(s_pTileset);
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy,
};
