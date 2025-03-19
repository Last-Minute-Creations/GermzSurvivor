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
#include <ace/managers/sprite.h>
#include <ace/managers/ptplayer.h>
#include <ace/utils/palette.h>
#include "survivor.h"
#include "game_math.h"

#define HUD_HEIGHT 16
#define MAP_SIZE_X 40
#define MAP_SIZE_Y 40
#define MAP_TILE_SHIFT 4
#define MAP_TILE_SIZE  (1 << MAP_TILE_SHIFT)
#define GAME_BPP 5
#define SPRITE_CHANNEL_CURSOR 4

#define PLAYER_BOB_SIZE_X 32
#define PLAYER_BOB_SIZE_Y 32
#define PLAYER_BOB_OFFSET_X 16
#define PLAYER_BOB_OFFSET_Y 25

#define ENEMY_BOB_SIZE_X 16
#define ENEMY_BOB_SIZE_Y 24
#define ENEMY_BOB_OFFSET_X 8
#define ENEMY_BOB_OFFSET_Y 25

#define ENEMY_COUNT 30

typedef enum tDirection {
	DIRECTION_SE,
	DIRECTION_S,
	DIRECTION_SW,
	DIRECTION_NW,
	DIRECTION_N,
	DIRECTION_NE,
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
	tPlayerFrame eFrame;
	UBYTE ubFrameCooldown;
} tPlayer;

static tView *s_pView;
static tVPort *s_pVpMain;
static tVPort *s_pVpHud;
static tTileBufferManager *s_pBufferMain;
static tBitMap *s_pPristineBuffer;
static tSimpleBufferManager *s_pBufferHud;
static tBitMap *s_pTileset;
static tBitMap *s_pBmCrosshair;
static tRandManager g_sRand;
static tSprite *s_pSpriteCursor;

static tBitMap *s_pPlayerFrames[DIRECTION_COUNT];
static tBitMap *s_pPlayerMasks[DIRECTION_COUNT];
static tFrameOffset s_pPlayerFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tPlayer s_sPlayer;

static tBitMap *s_pEnemyFrames[DIRECTION_COUNT];
static tBitMap *s_pEnemyMasks[DIRECTION_COUNT];
static tFrameOffset s_pEnemyFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tPlayer s_pEnemies[ENEMY_COUNT];

static tPtplayerMod *s_pMod;

static void onTileDraw(
	UNUSED_ARG UWORD uwTileX, UNUSED_ARG UWORD uwTileY,
	tBitMap *pBitMap, UWORD uwBitMapX, UWORD uwBitMapY
) {
	blitCopyAligned(
		pBitMap, uwBitMapX, uwBitMapY, s_pPristineBuffer, uwBitMapX, uwBitMapY,
		MAP_TILE_SIZE, MAP_TILE_SIZE
	);
}

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
		TAG_TILEBUFFER_CALLBACK_TILE_DRAW, onTileDraw,
	TAG_END);
	s_pPristineBuffer = bitmapCreate(
		bitmapGetByteWidth(s_pBufferMain->pScroll->pBack) * 8,
		s_pBufferMain->pScroll->pBack->Rows, GAME_BPP, BMF_INTERLEAVED
	);

	paletteLoadFromPath("data/game.plt", s_pVpHud->pPalette, 1 << GAME_BPP);

	s_pMod = ptplayerModCreateFromPath("data/germz1.mod");

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

	s_pEnemyFrames[DIRECTION_NE] = bitmapCreateFromPath("data/zombie_ne.bm", 0);
	s_pEnemyFrames[DIRECTION_N] = bitmapCreateFromPath("data/zombie_n.bm", 0);
	s_pEnemyFrames[DIRECTION_NW] = bitmapCreateFromPath("data/zombie_nw.bm", 0);
	s_pEnemyFrames[DIRECTION_SW] = bitmapCreateFromPath("data/zombie_sw.bm", 0);
	s_pEnemyFrames[DIRECTION_S] = bitmapCreateFromPath("data/zombie_s.bm", 0);
	s_pEnemyFrames[DIRECTION_SE] = bitmapCreateFromPath("data/zombie_se.bm", 0);

	s_pEnemyMasks[DIRECTION_NE] = bitmapCreateFromPath("data/zombie_ne_mask.bm", 0);
	s_pEnemyMasks[DIRECTION_N] = bitmapCreateFromPath("data/zombie_n_mask.bm", 0);
	s_pEnemyMasks[DIRECTION_NW] = bitmapCreateFromPath("data/zombie_nw_mask.bm", 0);
	s_pEnemyMasks[DIRECTION_SW] = bitmapCreateFromPath("data/zombie_sw_mask.bm", 0);
	s_pEnemyMasks[DIRECTION_S] = bitmapCreateFromPath("data/zombie_s_mask.bm", 0);
	s_pEnemyMasks[DIRECTION_SE] = bitmapCreateFromPath("data/zombie_se_mask.bm", 0);

	for(tDirection eDir = 0; eDir < DIRECTION_COUNT; ++eDir) {
		for(tPlayerFrame eFrame = 0; eFrame < PLAYER_FRAME_COUNT; ++eFrame) {
			s_pEnemyFrameOffsets[eDir][eFrame].pPixels = bobCalcFrameAddress(s_pEnemyFrames[eDir], eFrame * ENEMY_BOB_SIZE_Y);
			s_pEnemyFrameOffsets[eDir][eFrame].pMask = bobCalcFrameAddress(s_pEnemyMasks[eDir], eFrame * ENEMY_BOB_SIZE_Y);
		}
	}

	bobManagerCreate(
		s_pBufferMain->pScroll->pFront, s_pBufferMain->pScroll->pBack,
		s_pPristineBuffer, s_pBufferMain->pScroll->uwBmAvailHeight
	);

	s_sPlayer.uwX = 100;
	s_sPlayer.uwY = 100;
	s_sPlayer.eFrame = 0;
	s_sPlayer.ubFrameCooldown = 0;
	bobInit(&s_sPlayer.sBob, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, 1, 0, 0, 0, 0);

	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		s_pEnemies[i].uwX = 32 + (i % 8) * 32;
		s_pEnemies[i].uwY = 32 + (i / 8) * 32;
		s_pEnemies[i].eFrame = 0;
		s_pEnemies[i].ubFrameCooldown = 0;
		bobInit(&s_pEnemies[i].sBob, ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_Y, 1, 0, 0, 0, 0);
	}

	bobReallocateBuffers();
	gameMathInit();

	s_pBmCrosshair = bitmapCreateFromPath("data/cursor_crosshair.bm", 0);
	spriteManagerCreate(s_pView, 0, 0);
	s_pSpriteCursor = spriteAdd(SPRITE_CHANNEL_CURSOR, s_pBmCrosshair);
	systemSetDmaBit(DMAB_SPRITE, 1);
	mouseSetBounds(MOUSE_PORT_1, 0, HUD_HEIGHT, 320, 256);

	systemUnuse();
	tileBufferRedrawAll(s_pBufferMain);
	viewLoad(s_pView);
	logBlockEnd("gameGsCreate()");
	ptplayerLoadMod(s_pMod, 0, 0);
	ptplayerEnableMusic(1);
}

static void gameGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		statePop(g_pGameStateManager);
		return;
	}

	bobBegin(s_pBufferMain->pScroll->pBack);

	UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
	UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);

	UBYTE ubAngle = getAngleBetweenPoints( // 0 is right, going clockwise
		s_sPlayer.uwX - s_pBufferMain->pCamera->uPos.uwX,
		s_sPlayer.uwY - s_pBufferMain->pCamera->uPos.uwY,
		uwMouseX, uwMouseY - HUD_HEIGHT
	);

	BYTE bDeltaX = 0;
	BYTE bDeltaY = 0;
	if(keyCheck(KEY_W)) {
		bDeltaY = -2;
	}
	else if(keyCheck(KEY_S)) {
		bDeltaY = 2;
	}
	if(keyCheck(KEY_A)) {
		bDeltaX = -2;
	}
	else if(keyCheck(KEY_D)) {
		bDeltaX = 2;
	}
	if(bDeltaX || bDeltaY) {
		s_sPlayer.uwX += bDeltaX;
		s_sPlayer.uwY += bDeltaY;
		if(s_sPlayer.ubFrameCooldown >= 2) {
			s_sPlayer.eFrame = (s_sPlayer.eFrame + 1);
			if(s_sPlayer.eFrame > PLAYER_FRAME_WALK_8) {
				s_sPlayer.eFrame = PLAYER_FRAME_WALK_1;
			}
			s_sPlayer.ubFrameCooldown = 0;
		}
		else {
			++s_sPlayer.ubFrameCooldown;
		}
	}
	else {
		s_sPlayer.eFrame = PLAYER_FRAME_WALK_1;
		s_sPlayer.ubFrameCooldown = 0;
	}

	tDirection eDir;
	if(ubAngle < ANGLE_45) {
		eDir = DIRECTION_SE;
	}
	else if(ubAngle < ANGLE_45 + ANGLE_90) {
		eDir = DIRECTION_S;
	}
	else if(ubAngle < ANGLE_180) {
		eDir = DIRECTION_SW;
	}
	else if(ubAngle < ANGLE_180 + ANGLE_45) {
		eDir = DIRECTION_NW;
	}
	else if(ubAngle < ANGLE_180 + ANGLE_45 + ANGLE_90) {
		eDir = DIRECTION_N;
	}
	else {
		eDir = DIRECTION_NE;
	}

	tFrameOffset *pOffset = &s_pPlayerFrameOffsets[eDir][s_sPlayer.eFrame];
	bobSetFrame(&s_sPlayer.sBob, pOffset->pPixels, pOffset->pMask);
	s_sPlayer.sBob.sPos.uwX = s_sPlayer.uwX - PLAYER_BOB_OFFSET_X;
	s_sPlayer.sBob.sPos.uwY = s_sPlayer.uwY - PLAYER_BOB_OFFSET_Y;
	bobPush(&s_sPlayer.sBob);

	s_pSpriteCursor->wX = uwMouseX - 4;
	s_pSpriteCursor->wY = uwMouseY - 4;
	spriteProcess(s_pSpriteCursor);
	spriteProcessChannel(SPRITE_CHANNEL_CURSOR);

	cameraCenterAt(s_pBufferMain->pCamera, s_sPlayer.uwX, s_sPlayer.uwY);

	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		tPlayer *pEnemy = &s_pEnemies[i];
		pEnemy->sBob.sPos.uwX = pEnemy->uwX - ENEMY_BOB_OFFSET_X;
		pEnemy->sBob.sPos.uwY = pEnemy->uwY - ENEMY_BOB_OFFSET_Y;
		tFrameOffset *pOffset = &s_pEnemyFrameOffsets[eDir][pEnemy->eFrame];
		bobSetFrame(&pEnemy->sBob, pOffset->pPixels, pOffset->pMask);
		bobPush(&pEnemy->sBob);
	}

	bobPushingDone();
	bobEnd();

	viewProcessManagers(s_pView);
	copProcessBlocks();
	systemIdleBegin();
	vPortWaitUntilEnd(s_pVpMain);
	systemIdleEnd();
}

static void gameGsDestroy(void) {
	viewLoad(0);
	ptplayerStop();
	systemUse();

	ptplayerModDestroy(s_pMod);

	spriteManagerDestroy();
	bitmapDestroy(s_pBmCrosshair);

	bobManagerDestroy();
	for(tDirection eDir = 0; eDir < DIRECTION_COUNT; ++eDir) {
		bitmapDestroy(s_pPlayerFrames[eDir]);
		bitmapDestroy(s_pPlayerMasks[eDir]);

		bitmapDestroy(s_pEnemyFrames[eDir]);
		bitmapDestroy(s_pEnemyMasks[eDir]);
	}

	viewDestroy(s_pView);
	bitmapDestroy(s_pTileset);
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy,
};
