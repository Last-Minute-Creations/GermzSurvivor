/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/bob.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/system.h>
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/rand.h>
#include <ace/managers/sprite.h>
#include <ace/managers/ptplayer.h>
#include <ace/utils/palette.h>
#include <ace/utils/chunky.h>
#include "survivor.h"
#include "game_math.h"

#define HUD_SIZE_Y 16
#define MAP_TILES_X 32
#define MAP_TILES_Y 32
#define MAP_TILE_SHIFT 4
#define MAP_TILE_SIZE  (1 << MAP_TILE_SHIFT)
#define GAME_BPP 5
#define SPRITE_CHANNEL_CURSOR 4

#define COLLISION_SIZE_X 8
#define COLLISION_SIZE_Y 8

#define PLAYER_BOB_SIZE_X 32
#define PLAYER_BOB_SIZE_Y 32
// From the top-left of the collision rectangle
#define PLAYER_BOB_OFFSET_X 12
#define PLAYER_BOB_OFFSET_Y 19

#define ENEMY_BOB_SIZE_X 16
#define ENEMY_BOB_SIZE_Y 24
// From the top-left of the collision rectangle
#define ENEMY_BOB_OFFSET_X 4
#define ENEMY_BOB_OFFSET_Y 15

#define ENEMY_COUNT 30
#define PROJECTILE_COUNT 20
#define PROJECTILE_LIFETIME 50

#define BG_BYTES_PER_BITPLANE_ROW (MAP_TILES_X * MAP_TILE_SIZE / 8)
#define BG_BYTES_PER_PIXEL_ROW (BG_BYTES_PER_BITPLANE_ROW * GAME_BPP)

typedef UWORD tFix10p6;
static inline tFix10p6 fix10p6Add(tFix10p6 a, tFix10p6 b) {return a + b; }
static inline tFix10p6 fix10p6FromUword(UWORD x) {return x << 6; }
static inline tFix10p6 fix10p6ToUword(UWORD x) {return x >> 6; }
static inline tFix10p6 fix10p6Sin(UBYTE ubAngle) {return csin(ubAngle) >> 10;}
static inline tFix10p6 fix10p6Cos(UBYTE ubAngle) {return ccos(ubAngle) >> 10;}

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

typedef struct tCharacter {
	tUwCoordYX sPos; ///< Top-left coordinate of collision box.
	tBob sBob;
	tPlayerFrame eFrame;
	UBYTE ubFrameCooldown;
} tCharacter;

typedef struct tProjectile {
	tFix10p6 fX;
	tFix10p6 fY;
	tFix10p6 fDx;
	tFix10p6 fDy;
	ULONG pPrevOffsets[2];
	UBYTE ubLife;
} tProjectile;

static tView *s_pView;
static tVPort *s_pVpMain;
static tVPort *s_pVpHud;
static tSimpleBufferManager *s_pBufferMain;
static tBitMap *s_pPristineBuffer;
static UBYTE *s_pPristinePlanes;
static UBYTE *s_pBackPlanes;
static tSimpleBufferManager *s_pBufferHud;
static tBitMap *s_pTileset;
static tBitMap *s_pBmCrosshair;
static tRandManager g_sRand;
static tSprite *s_pSpriteCursor;

static tBitMap *s_pPlayerFrames[DIRECTION_COUNT];
static tBitMap *s_pPlayerMasks[DIRECTION_COUNT];
static tFrameOffset s_pPlayerFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tCharacter s_sPlayer;

static tBitMap *s_pEnemyFrames[DIRECTION_COUNT];
static tBitMap *s_pEnemyMasks[DIRECTION_COUNT];
static tFrameOffset s_pEnemyFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tCharacter s_pEnemies[ENEMY_COUNT];
static tProjectile s_pProjectiles[PROJECTILE_COUNT];

static tPtplayerMod *s_pMod;
static UBYTE s_ubBufferCurr;
static tProjectile *s_pCurrentProjectile;

static const UBYTE s_pBulletMaskFromX[] = {
	BV(7), BV(6), BV(5), BV(4), BV(3), BV(2), BV(1), BV(0)
};

static ULONG s_pRowOffsetFromY[MAP_TILES_Y * MAP_TILE_SIZE];

static tCharacter *s_pCollisionTiles[MAP_TILES_X * MAP_TILE_SIZE / COLLISION_SIZE_X][MAP_TILES_Y * MAP_TILE_SIZE / COLLISION_SIZE_Y];

__attribute__((always_inline))
static inline void undrawNextProjectile(void) {
	if(s_pCurrentProjectile == &s_pProjectiles[PROJECTILE_COUNT]) {
		return;
	}

	if(s_pCurrentProjectile->ubLife) {
		ULONG ulOffset = s_pCurrentProjectile->pPrevOffsets[s_ubBufferCurr];
		UBYTE *pTargetPlanes = &s_pBackPlanes[ulOffset];
		UBYTE *pBgPlanes = &s_pPristinePlanes[ulOffset];

		for(UBYTE ubPlane = GAME_BPP; ubPlane--;) {
			*pTargetPlanes = *pBgPlanes;
			pTargetPlanes += BG_BYTES_PER_BITPLANE_ROW;
			pBgPlanes += BG_BYTES_PER_BITPLANE_ROW;
		}

		--s_pCurrentProjectile->ubLife;
		if(s_pCurrentProjectile->ubLife) {
			s_pCurrentProjectile->fX = fix10p6Add(s_pCurrentProjectile->fX, s_pCurrentProjectile->fDx);
			s_pCurrentProjectile->fY = fix10p6Add(s_pCurrentProjectile->fY, s_pCurrentProjectile->fDy);
		}
	}

	++s_pCurrentProjectile;
}

__attribute__((always_inline))
static inline void drawNextProjectile(void) {
	if(s_pCurrentProjectile == &s_pProjectiles[PROJECTILE_COUNT]) {
		return;
	}

	UBYTE *pTargetPlanes = s_pBackPlanes;
	if(s_pCurrentProjectile->ubLife) {
		UWORD uwProjectileX = fix10p6ToUword(s_pCurrentProjectile->fX);
		UWORD uwProjectileY = fix10p6ToUword(s_pCurrentProjectile->fY);
		// TODO: collision with enemies
		if(uwProjectileX >= MAP_TILES_X * MAP_TILE_SIZE || uwProjectileY >= MAP_TILES_Y * MAP_TILE_SIZE) {
			s_pCurrentProjectile->ubLife = 0;
		}
		else {
			UBYTE ubMask = s_pBulletMaskFromX[uwProjectileX & 0x7];
			ULONG ulOffset = s_pRowOffsetFromY[uwProjectileY] + (uwProjectileX / 8);
			s_pCurrentProjectile->pPrevOffsets[s_ubBufferCurr] = ulOffset;
			for(UBYTE ubPlane = GAME_BPP; ubPlane--;) {
				pTargetPlanes[ulOffset] |= ubMask;
				ulOffset += BG_BYTES_PER_BITPLANE_ROW;
			}
		}

	}
	++s_pCurrentProjectile;
}

void bobOnBegin(void) {
	undrawNextProjectile();
}

void bobOnEnd(void) {
	drawNextProjectile();
}

static void setTile(UBYTE ubTileIndex, UWORD uwTileX, UWORD uwTileY) {
	blitCopyAligned(
		s_pTileset, 0, ubTileIndex * MAP_TILE_SIZE,
		s_pBufferMain->pBack, uwTileX * MAP_TILE_SIZE, uwTileY * MAP_TILE_SIZE,
		MAP_TILE_SIZE, MAP_TILE_SIZE
	);
	blitCopyAligned(
		s_pTileset, 0, ubTileIndex * MAP_TILE_SIZE,
		s_pBufferMain->pFront, uwTileX * MAP_TILE_SIZE, uwTileY * MAP_TILE_SIZE,
		MAP_TILE_SIZE, MAP_TILE_SIZE
	);
	blitCopyAligned(
		s_pTileset, 0, ubTileIndex * MAP_TILE_SIZE,
		s_pPristineBuffer, uwTileX * MAP_TILE_SIZE, uwTileY * MAP_TILE_SIZE,
		MAP_TILE_SIZE, MAP_TILE_SIZE
	);
}

static inline UBYTE isPositionCollidingWithCharacter(
	tUwCoordYX sPos, const tCharacter *pCharacter
) {
	WORD Dx = sPos.uwX - pCharacter->sPos.uwX;
	WORD Dy = sPos.uwY - pCharacter->sPos.uwY;
	return (
		-COLLISION_SIZE_X < Dx && Dx < COLLISION_SIZE_X &&
		-COLLISION_SIZE_Y < Dy && Dy < COLLISION_SIZE_Y
	);
}

static inline tCharacter *characterGetNearPos(
	UWORD uwPosX, BYTE bLookupAddX, UWORD uwPosY, BYTE bLookupAddY
) {
	UWORD uwLookupX = uwPosX / COLLISION_SIZE_X + bLookupAddX;
	UWORD uwLookupY = uwPosY / COLLISION_SIZE_Y + bLookupAddY;
	return s_pCollisionTiles[uwLookupX][uwLookupY];
}

__attribute__((always_inline))
static inline UBYTE characterTryMoveBy(tCharacter *pCharacter, LONG lDeltaX, LONG lDeltaY) {
	tUwCoordYX sGoodPos = pCharacter->sPos;
	UBYTE isMoved = 0;

	if (lDeltaX) {
		tUwCoordYX sNewPos = sGoodPos;
		sNewPos.uwX += lDeltaX;
		UWORD uwTestX = sNewPos.uwX;
		if(lDeltaX < 0) {
			uwTestX -= PLAYER_BOB_OFFSET_X;
		}
		UBYTE isColliding = (uwTestX >= MAP_TILES_X * MAP_TILE_SIZE - (PLAYER_BOB_SIZE_X - PLAYER_BOB_OFFSET_X));

		// collision with upper corner
		if(!isColliding) {
			tCharacter *pUp = characterGetNearPos(sGoodPos.uwX, SGN(lDeltaX), sGoodPos.uwY, 0);
			if(pUp && pUp != pCharacter) {
				isColliding = isPositionCollidingWithCharacter(sNewPos, pUp);
			}
		}

		// collision with lower corner
		if (!isColliding && (sGoodPos.uwY & (COLLISION_SIZE_Y - 1))) {
			tCharacter *pDown = characterGetNearPos(sGoodPos.uwX, SGN(lDeltaX), sGoodPos.uwY, +1);
			if(pDown && pDown != pCharacter) {
				isColliding = isPositionCollidingWithCharacter(sNewPos, pDown);
			}
		}

		if(!isColliding) {
			isMoved = 1;
			sGoodPos = sNewPos;
		}
	}

	if (lDeltaY) {
		tUwCoordYX sNewPos = sGoodPos;
		sNewPos.uwY += lDeltaY;

		UWORD uwTestY = sNewPos.uwY;
		if(lDeltaY < 0) {
			uwTestY -= PLAYER_BOB_OFFSET_Y;
		}
		UBYTE isColliding = (uwTestY >= MAP_TILES_Y * MAP_TILE_SIZE - (PLAYER_BOB_SIZE_Y - PLAYER_BOB_OFFSET_Y));

		// collision with left corner
		if(!isColliding) {
			tCharacter *pLeft = characterGetNearPos(sGoodPos.uwX, 0, sGoodPos.uwY, SGN(lDeltaY));
			if(pLeft && pLeft != pCharacter) {
				isColliding = isPositionCollidingWithCharacter(sNewPos, pLeft);
			}
		}

		// collision with right corner
		if (!isColliding && (sGoodPos.uwX & (COLLISION_SIZE_X - 1))) {
			tCharacter *pRight = characterGetNearPos(sGoodPos.uwX, +1, sGoodPos.uwY, SGN(lDeltaY));
			if(pRight && pRight != pCharacter) {
				isColliding = isPositionCollidingWithCharacter(sNewPos, pRight);
			}
		}

		if(!isColliding) {
			isMoved = 1;
			sGoodPos = sNewPos;
		}
	}

	if(isMoved) {
		pCharacter->sPos = sGoodPos;
		ULONG ubOldLookupX = pCharacter->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubOldLookupY = pCharacter->sPos.uwY / COLLISION_SIZE_Y;
		// Update lookup
		if(
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY] &&
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY] != pCharacter
		) {
			logWrite(
				"ERR: Erasing other character %p\n",
				s_pCollisionTiles[ubOldLookupX][ubOldLookupY]
			);
		}
		s_pCollisionTiles[ubOldLookupX][ubOldLookupY] = 0;

		ULONG ubNewLookupX = pCharacter->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubNewLookupY = pCharacter->sPos.uwY / COLLISION_SIZE_Y;
		if(s_pCollisionTiles[ubNewLookupX][ubNewLookupY]) {
			logWrite(
				"ERR: Overwriting other character %p in lookup with %p\n",
				s_pCollisionTiles[ubNewLookupX][ubNewLookupY], pCharacter
			);
		}
		s_pCollisionTiles[ubNewLookupX][ubNewLookupY] = pCharacter;
	}

	return isMoved;
}

static void cameraCenterAtOptimized(
	tCameraManager *pManager, ULONG ulCenterX, ULONG ulCenterY
) {
	LONG lTop = ulCenterX - 320 / 2;
	LONG lLeft = ulCenterY - (256 - HUD_SIZE_Y) / 2;
	pManager->uPos.uwX = CLAMP(lTop, 0, pManager->uMaxPos.uwX);
	pManager->uPos.uwY = CLAMP(lLeft, 0, pManager->uMaxPos.uwY);
}

static void gameGsCreate(void) {
	logBlockBegin("gameGsCreate()");
	s_pTileset = bitmapCreateFromPath("data/tiles.bm", 0);

	s_pView = viewCreate(0, TAG_END);

	s_pVpHud = vPortCreate(0,
		TAG_VPORT_BPP, GAME_BPP,
		TAG_VPORT_HEIGHT, HUD_SIZE_Y,
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

	s_pBufferMain = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, MAP_TILES_X * MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, MAP_TILES_Y * MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
	TAG_END);

	blitRect(s_pBufferHud->pBack, 0, 0, 320, HUD_SIZE_Y, 15);

	s_pPristineBuffer = bitmapCreate(
		bitmapGetByteWidth(s_pBufferMain->pBack) * 8,
		s_pBufferMain->pBack->Rows, GAME_BPP, BMF_INTERLEAVED
	);
	s_pPristinePlanes = s_pPristineBuffer->Planes[0];

	paletteLoadFromPath("data/game.plt", s_pVpHud->pPalette, 1 << GAME_BPP);

	s_pMod = ptplayerModCreateFromPath("data/germz1.mod");

	randInit(&g_sRand, 2184, 1911);

	setTile(0, 0, 0);
	setTile(1, MAP_TILES_X - 1, 0);
	setTile(2, 0, MAP_TILES_Y - 1);
	setTile(3, MAP_TILES_X - 1, MAP_TILES_Y - 1);

	for(UBYTE ubX = 1; ubX < MAP_TILES_X - 1; ++ubX) {
		setTile(randUwMinMax(&g_sRand, 4, 6), ubX, 0);
		setTile(randUwMinMax(&g_sRand, 13, 15), ubX, MAP_TILES_Y - 1);
	}

	for(UBYTE ubY = 1; ubY < MAP_TILES_Y - 1; ++ubY) {
		setTile(randUwMinMax(&g_sRand, 7, 9), 0, ubY);
		setTile(randUwMinMax(&g_sRand, 10, 12), MAP_TILES_X - 1, ubY);
	}

	for(UBYTE ubX = 1; ubX < MAP_TILES_X - 1; ++ubX) {
		for(UBYTE ubY = 1; ubY < MAP_TILES_Y - 1; ++ubY) {
			setTile(randUwMinMax(&g_sRand, 16, 24), ubX, ubY);
		}
	}

	for(UWORD uwY = 0; uwY < MAP_TILES_Y * MAP_TILE_SIZE; ++uwY) {
		s_pRowOffsetFromY[uwY] = uwY * BG_BYTES_PER_PIXEL_ROW;
	}

	s_ubBufferCurr = 0;

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
		s_pBufferMain->pFront, s_pBufferMain->pBack,
		s_pPristineBuffer, MAP_TILES_Y * MAP_TILE_SIZE
	);

	s_sPlayer.sPos.uwX = 180;
	s_sPlayer.sPos.uwY = 180;
	s_sPlayer.eFrame = 0;
	s_sPlayer.ubFrameCooldown = 0;
	s_pCollisionTiles[s_sPlayer.sPos.uwX / COLLISION_SIZE_X][s_sPlayer.sPos.uwY / COLLISION_SIZE_Y] = &s_sPlayer;
	bobInit(&s_sPlayer.sBob, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, 1, 0, 0, 0, 0);

	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		s_pEnemies[i].sPos.uwX = 32 + (i % 8) * 32;
		s_pEnemies[i].sPos.uwY = 32 + (i / 8) * 32;
		s_pEnemies[i].eFrame = 0;
		s_pEnemies[i].ubFrameCooldown = 0;
		s_pCollisionTiles[s_pEnemies[i].sPos.uwX / COLLISION_SIZE_X][s_pEnemies[i].sPos.uwY / COLLISION_SIZE_Y] = &s_pEnemies[i];
		bobInit(&s_pEnemies[i].sBob, ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_Y, 1, 0, 0, 0, 0);
	}

	for(UBYTE i = 0; i < PROJECTILE_COUNT; ++i) {
		s_pProjectiles[i].fX = fix10p6FromUword(32 + (i % 8) * 4);
		s_pProjectiles[i].fY = fix10p6FromUword(180 + (i / 8) * 4);

		s_pProjectiles[i].fDx = 0;
		s_pProjectiles[i].fDy = 0;

		s_pProjectiles[i].ubLife = PROJECTILE_LIFETIME;
		s_pProjectiles[i].pPrevOffsets[0] = 0;
		s_pProjectiles[i].pPrevOffsets[1] = 0;
	}

	bobReallocateBuffers();
	gameMathInit();

	s_pBmCrosshair = bitmapCreateFromPath("data/cursor_crosshair.bm", 0);
	spriteManagerCreate(s_pView, 0, 0);
	s_pSpriteCursor = spriteAdd(SPRITE_CHANNEL_CURSOR, s_pBmCrosshair);
	systemSetDmaBit(DMAB_SPRITE, 1);
	mouseSetBounds(MOUSE_PORT_1, 0, HUD_SIZE_Y, 320, 256);

	systemUnuse();
	viewLoad(s_pView);
	viewProcessManagers(s_pView);
	viewProcessManagers(s_pView);
	logBlockEnd("gameGsCreate()");
	ptplayerLoadMod(s_pMod, 0, 0);
	ptplayerEnableMusic(1);
}

static void gameGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		statePop(g_pGameStateManager);
		return;
	}

	s_pBackPlanes = s_pBufferMain->pBack->Planes[0];
	s_pCurrentProjectile = &s_pProjectiles[0];
	bobBegin(s_pBufferMain->pBack);
	while(s_pCurrentProjectile != &s_pProjectiles[PROJECTILE_COUNT]) {
		undrawNextProjectile();
	}

	UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
	UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);

	UBYTE ubAngle = getAngleBetweenPoints( // 0 is right, going clockwise
		s_sPlayer.sPos.uwX - s_pBufferMain->pCamera->uPos.uwX,
		s_sPlayer.sPos.uwY - s_pBufferMain->pCamera->uPos.uwY,
		uwMouseX, uwMouseY - HUD_SIZE_Y
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
		characterTryMoveBy(&s_sPlayer, bDeltaX, bDeltaY);
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
	s_sPlayer.sBob.sPos.uwX = s_sPlayer.sPos.uwX - PLAYER_BOB_OFFSET_X;
	s_sPlayer.sBob.sPos.uwY = s_sPlayer.sPos.uwY - PLAYER_BOB_OFFSET_Y;
	bobPush(&s_sPlayer.sBob);

	s_pSpriteCursor->wX = uwMouseX - 4;
	s_pSpriteCursor->wY = uwMouseY - 4;
	spriteProcess(s_pSpriteCursor);
	spriteProcessChannel(SPRITE_CHANNEL_CURSOR);

	cameraCenterAtOptimized(s_pBufferMain->pCamera, s_sPlayer.sPos.uwX, s_sPlayer.sPos.uwY);

	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		tCharacter *pEnemy = &s_pEnemies[i];
		bDeltaX = (randUw(&g_sRand) & 1) ? -1 : 1;
		bDeltaY = (randUw(&g_sRand) & 1) ? -1 : 1;
		characterTryMoveBy(pEnemy, bDeltaX, bDeltaY);
		pEnemy->sBob.sPos.uwX = pEnemy->sPos.uwX - ENEMY_BOB_OFFSET_X;
		pEnemy->sBob.sPos.uwY = pEnemy->sPos.uwY - ENEMY_BOB_OFFSET_Y;
		tFrameOffset *pOffset = &s_pEnemyFrameOffsets[eDir][pEnemy->eFrame];
		bobSetFrame(&pEnemy->sBob, pOffset->pPixels, pOffset->pMask);
		bobPush(&pEnemy->sBob);
	}

	s_pCurrentProjectile = &s_pProjectiles[0];
	while(s_pCurrentProjectile != &s_pProjectiles[PROJECTILE_COUNT]) {
		if(!s_pCurrentProjectile->ubLife) {
			s_pCurrentProjectile->ubLife = PROJECTILE_LIFETIME;
			s_pCurrentProjectile->fX = fix10p6FromUword(30);
			s_pCurrentProjectile->fY = fix10p6FromUword(30);
			UBYTE ubAngle = randUwMinMax(&g_sRand, 0, ANGLE_360 - 1);
			s_pCurrentProjectile->fDx = fix10p6Cos(ubAngle);
			s_pCurrentProjectile->fDy = fix10p6Sin(ubAngle);
		}
		++s_pCurrentProjectile;
	}

	bobPushingDone();
	s_pCurrentProjectile = &s_pProjectiles[0];
	bobEnd();
	while(s_pCurrentProjectile != &s_pProjectiles[PROJECTILE_COUNT]) {
		drawNextProjectile();
	}
	s_ubBufferCurr = !s_ubBufferCurr;

	vPortProcessManagers(s_pVpMain);
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
