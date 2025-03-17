/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/bob.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/tilebuffer.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/rand.h>
#include <ace/utils/palette.h>
#include "survivor.h"
#include "game_math.h"

#define HUD_HEIGHT 16
#define MAP_SIZE_X 40
#define MAP_SIZE_Y 40
#define MAP_TILE_SHIFT 4
#define GAME_BPP 5

#define PLAYER_BOB_SIZE_X 32
#define PLAYER_BOB_SIZE_Y 32
#define PLAYER_BOB_OFFSET_X 16
#define PLAYER_BOB_OFFSET_Y 25

typedef enum tDirection {
	DIRECTION_NE,
	DIRECTION_N,
	DIRECTION_NW,
	DIRECTION_SW,
	DIRECTION_S,
	DIRECTION_SE,
	DIRECTION_COUNT
} tDirection;

typedef enum tPlayerFrame {
	PLAYER_FRAME_WALK_1,
	PLAYER_FRAME_WALK_2,
	PLAYER_FRAME_WALK_3,
	PLAYER_FRAME_WALK_4,
	PLAYER_FRAME_WALK_5,
	PLAYER_FRAME_WALK_6,
	PLAYER_FRAME_WALK_7,
	PLAYER_FRAME_WALK_8,

	PLAYER_FRAME_COUNT
} tPlayerFrame;

typedef struct tFrameOffset {
	UBYTE *pPixels;
	UBYTE *pMask;
} tFrameOffset;

typedef struct tPlayer {
	UWORD uwX;
	UWORD uwY;
	tBob sBob;
} tPlayer;

static tView *s_pView;
static tVPort *s_pVpMain;
static tVPort *s_pVpHud;
static tTileBufferManager *s_pBufferMain;
static tSimpleBufferManager *s_pBufferHud;
static tBitMap *s_pTileset;
static tRandManager g_sRand;
static tBitMap *s_pPlayerFrames[DIRECTION_COUNT];
static tBitMap *s_pPlayerMasks[DIRECTION_COUNT];
static tFrameOffset s_pPlayerFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tPlayer s_sPlayer;

static void gameGsCreate(void) {
	logBlockBegin("gameGsCreate()");
	s_pTileset = bitmapCreateFromPath("data/tiles.bm", 0);

	s_pView = viewCreate(0, TAG_END);

	s_pVpHud = vPortCreate(0,
		TAG_VPORT_BPP, GAME_BPP,
		TAG_VPORT_HEIGHT, HUD_HEIGHT,
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

	// Frames
	s_pPlayerFrames[DIRECTION_NE] = bitmapCreateFromPath("data/player_ne.bm", 0);
	s_pPlayerFrames[DIRECTION_N] = bitmapCreateFromPath("data/player_n.bm", 0);
	s_pPlayerFrames[DIRECTION_NW] = bitmapCreateFromPath("data/player_nw.bm", 0);
	s_pPlayerFrames[DIRECTION_SW] = bitmapCreateFromPath("data/player_sw.bm", 0);
	s_pPlayerFrames[DIRECTION_S] = bitmapCreateFromPath("data/player_s.bm", 0);
	s_pPlayerFrames[DIRECTION_SE] = bitmapCreateFromPath("data/player_se.bm", 0);

	s_pPlayerMasks[DIRECTION_NE] = bitmapCreateFromPath("data/player_ne_mask.bm", 0);
	s_pPlayerMasks[DIRECTION_N] = bitmapCreateFromPath("data/player_n_mask.bm", 0);
	s_pPlayerMasks[DIRECTION_NW] = bitmapCreateFromPath("data/player_nw_mask.bm", 0);
	s_pPlayerMasks[DIRECTION_SW] = bitmapCreateFromPath("data/player_sw_mask.bm", 0);
	s_pPlayerMasks[DIRECTION_S] = bitmapCreateFromPath("data/player_s_mask.bm", 0);
	s_pPlayerMasks[DIRECTION_SE] = bitmapCreateFromPath("data/player_se_mask.bm", 0);

	for(tDirection eDir = 0; eDir < DIRECTION_COUNT; ++eDir) {
		for(tPlayerFrame eFrame = 0; eFrame < PLAYER_FRAME_COUNT; ++eFrame) {
			s_pPlayerFrameOffsets[eDir][eFrame].pPixels = bobCalcFrameAddress(s_pPlayerFrames[eDir], eFrame * PLAYER_BOB_SIZE_Y);
			s_pPlayerFrameOffsets[eDir][eFrame].pMask = bobCalcFrameAddress(s_pPlayerMasks[eDir], eFrame * PLAYER_BOB_SIZE_Y);
		}
	}

	bobManagerCreate(
		s_pBufferMain->pScroll->pFront, s_pBufferMain->pScroll->pBack,
		s_pBufferMain->pScroll->uwBmAvailHeight
	);

	s_sPlayer.uwX = 100;
	s_sPlayer.uwY = 100;
	bobInit(&s_sPlayer.sBob, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, 1, 0, 0, 0, 0);

	bobReallocateBuffers();
	gameMathInit();

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

	bobBegin(s_pBufferMain->pScroll->pBack);

	// UBYTE ubAngle = getAngleBetweenPoints(
	// 	s_sPlayer.uwX, s_sPlayer.uwY + HUD_HEIGHT,
	// 	mouseGetX(MOUSE_PORT_1), mouseGetY(MOUSE_PORT_1)
	// );
	tDirection eDir = DIRECTION_NE; // TODO
	tFrameOffset *pOffset = &s_pPlayerFrameOffsets[eDir][0]; // TODO
	s_sPlayer.sBob.sPos.uwX = s_sPlayer.uwX - PLAYER_BOB_OFFSET_X;
	s_sPlayer.sBob.sPos.uwY = s_sPlayer.uwY - PLAYER_BOB_OFFSET_Y;
	bobSetFrame(&s_sPlayer.sBob, pOffset->pPixels, pOffset->pMask);
	bobPush(&s_sPlayer.sBob);
	// cameraCenterAt(s_pBufferMain->pCamera, s_sPlayer.uwX, s_sPlayer.uwY);

	bobPushingDone();
	bobEnd();

	viewProcessManagers(s_pView);
	copProcessBlocks();
	vPortWaitUntilEnd(s_pVpMain);
}

static void gameGsDestroy(void) {
	viewLoad(0);
	systemUse();

	bobManagerDestroy();
	for(tDirection eDir = 0; eDir < DIRECTION_COUNT; ++eDir) {
		bitmapDestroy(s_pPlayerFrames[eDir]);
		bitmapDestroy(s_pPlayerMasks[eDir]);
	}

	viewDestroy(s_pView);
	bitmapDestroy(s_pTileset);
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy,
};
