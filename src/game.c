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
#include <ace/contrib/managers/audio_mixer.h>
#include <ace/utils/palette.h>
#include <ace/utils/chunky.h>
#include "survivor.h"
#include "game_math.h"

// #define GAME_COLLISION_DEBUG

#define WEAPON_MAX_BULLETS_IN_MAGAZINE 30
#define STAIN_FRAME_COUNT 3
#define STAIN_FRAME_PRESET_COUNT 16
#define STAINS_MAX 20
#define STAIN_SIZE_X 16
#define STAIN_SIZE_Y 16
#define STAIN_BYTES_PER_BITPLANE_ROW (STAIN_SIZE_X / 8)
#define STAIN_BYTES_PER_PIXEL_ROW (STAIN_BYTES_PER_BITPLANE_ROW * GAME_BPP)

#define WEAPON_DAMAGE_BASE_RIFLE 6
#define WEAPON_DAMAGE_SMG 5
#define WEAPON_DAMAGE_ASSAULT_RIFLE 7
#define WEAPON_DAMAGE_SHOTGUN 7
#define WEAPON_DAMAGE_SAWOFF 9

#define WEAPON_COOLDOWN_BASE_RIFLE 15
#define WEAPON_COOLDOWN_SMG 3
#define WEAPON_COOLDOWN_ASSAULT_RIFLE 4
#define WEAPON_COOLDOWN_SHOTGUN 18
#define WEAPON_COOLDOWN_SAWOFF 18

#define HUD_SIZE_Y 16
#define HUD_WEAPON_SIZE_X 48
#define HUD_WEAPON_SIZE_Y 15
#define HUD_AMMO_FIELD_OFFSET_X (HUD_WEAPON_SIZE_X + 2)
#define HUD_AMMO_FIELD_OFFSET_Y 2
#define HUD_AMMO_BULLET_COUNT_Y 2
#define HUD_AMMO_BULLET_COUNT_X ((WEAPON_MAX_BULLETS_IN_MAGAZINE + HUD_AMMO_BULLET_COUNT_Y - 1) / HUD_AMMO_BULLET_COUNT_Y)
#define HUD_AMMO_BULLET_SIZE_X 2
#define HUD_AMMO_BULLET_SIZE_Y 6
#define HUD_AMMO_FIELD_SIZE_X (HUD_AMMO_BULLET_COUNT_X * (HUD_AMMO_BULLET_SIZE_X + 1) - 1)
#define HUD_AMMO_FIELD_SIZE_Y (HUD_AMMO_BULLET_COUNT_Y * (HUD_AMMO_BULLET_SIZE_Y + 1) - 1)
#define HUD_AMMO_COUNT_FORCE_REDRAW 0xFF

#define BULLET_OFFSET_Y_SHELL 0
#define BULLET_OFFSET_Y_BULLET 6

#define SFX_CHANNEL_SHOOT 0
#define SFX_PRIORITY_SHOOT 0
#define SFX_CHANNEL_RELOAD 0
#define SFX_PRIORITY_RELOAD 1
#define SFX_CHANNEL_BITE 2
#define SFX_PRIORITY_BITE 0
#define SFX_CHANNEL_IMPACT 1
#define SFX_PRIORITY_IMPACT 0

#define MAP_TILES_X 32
#define MAP_TILES_Y 32
#define MAP_MARGIN_TILES 2
#define MAP_TILE_SHIFT 4
#define MAP_TILE_SIZE  (1 << MAP_TILE_SHIFT)
#define GAME_BPP 5
#define SPRITE_CHANNEL_CURSOR 0

#define MAIN_VPORT_SIZE_X 320
#define MAIN_VPORT_SIZE_Y (256 - HUD_SIZE_Y)

#define COLOR_HUD_BG 6
#define COLOR_BAR_BG 10
#define COLOR_RED 16

#define COLLISION_SIZE_X 8
#define COLLISION_SIZE_Y 8
#define COLLISION_LOOKUP_SIZE_X (MAP_TILES_X * MAP_TILE_SIZE / COLLISION_SIZE_X)
#define COLLISION_LOOKUP_SIZE_Y (MAP_TILES_Y * MAP_TILE_SIZE / COLLISION_SIZE_Y)
#define RESPAWN_SLOTS_PER_POSITION 4

#define PLAYER_BOB_SIZE_X 32
#define PLAYER_BOB_SIZE_Y 32
// From the top-left of the collision rectangle
#define PLAYER_BOB_OFFSET_X 12
#define PLAYER_BOB_OFFSET_Y 19
#define PLAYER_ATTACK_COOLDOWN 12
#define PLAYER_HEALTH_MAX 100

#define ENEMY_BOB_SIZE_X 16
#define ENEMY_BOB_SIZE_Y 24
// From the top-left of the collision rectangle
#define ENEMY_BOB_OFFSET_X 4
#define ENEMY_BOB_OFFSET_Y 15
#define ENEMY_ATTACK_COOLDOWN 15
#define ENEMY_HEALTH_MAX 20
#define ENEMY_SPEED 1

#define ENEMY_COUNT 25
#define PROJECTILE_COUNT 20
#define PROJECTILE_LIFETIME 25
#define PROJECTILE_SPEED 5
#define SPREAD_SIDE_COUNT 40

#define SORTED_CHARS_COUNT (ENEMY_COUNT + 1)

#define BG_BYTES_PER_BITPLANE_ROW (MAP_TILES_X * MAP_TILE_SIZE / 8)
#define BG_BYTES_PER_PIXEL_ROW (BG_BYTES_PER_BITPLANE_ROW * GAME_BPP)

typedef UWORD tFix10p6;

typedef enum tDirection {
	DIRECTION_SE,
	DIRECTION_S,
	DIRECTION_SW,
	DIRECTION_NW,
	DIRECTION_N,
	DIRECTION_NE,
	DIRECTION_COUNT
} tDirection;

typedef enum tCharacterFrame {
	PLAYER_FRAME_WALK_1,
	PLAYER_FRAME_WALK_2,
	PLAYER_FRAME_WALK_3,
	PLAYER_FRAME_WALK_4,
	PLAYER_FRAME_WALK_5,
	PLAYER_FRAME_WALK_6,
	PLAYER_FRAME_WALK_7,
	PLAYER_FRAME_WALK_8,

	PLAYER_FRAME_COUNT
} tCharacterFrame;

typedef enum tWeaponKind {
	WEAPON_KIND_BASE_RIFLE,
	WEAPON_KIND_SMG,
	WEAPON_KIND_ASSAULT_RIFLE,
	WEAPON_KIND_SHOTGUN,
	WEAPON_KIND_SAWOFF,
} tWeaponKind;

typedef struct tFrameOffset {
	UBYTE *pPixels;
	UBYTE *pMask;
} tFrameOffset;

typedef struct tCharacter {
	tUwCoordYX sPos; ///< Top-left coordinate of collision box.
	tBob sBob;
	tCharacterFrame eFrame;
	tWeaponKind eWeapon;
	WORD wHealth;
	UBYTE ubFrameCooldown;
	UBYTE ubAttackCooldown;
	UBYTE ubAmmo;
	UBYTE ubReloadCooldown;
} tCharacter;

typedef struct tProjectile {
	tFix10p6 fX;
	tFix10p6 fY;
	tFix10p6 fDx;
	tFix10p6 fDy;
	ULONG pPrevOffsets[2];
	UBYTE ubLife;
	UBYTE ubDamage;
} tProjectile;

typedef enum tHudState {
	HUD_STATE_DRAW_HEALTH,
	HUD_STATE_DRAW_WEAPON,
	HUD_STATE_DRAW_BULLETS,
	HUD_STATE_END,
} tHudState;

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
static tBitMap *s_pHudWeapons;
static tRandManager g_sRand;
static tSprite *s_pSpriteCursor;
static volatile ULONG s_ulFrameCount;
static ULONG s_ulFrameWaitCount;

static tBitMap *s_pPlayerFrames[DIRECTION_COUNT];
static tBitMap *s_pPlayerMasks[DIRECTION_COUNT];
static tFrameOffset s_pPlayerFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tCharacter s_sPlayer;

static tBitMap *s_pEnemyFrames[DIRECTION_COUNT];
static tBitMap *s_pEnemyMasks[DIRECTION_COUNT];
static tFrameOffset s_pEnemyFrameOffsets[DIRECTION_COUNT][PLAYER_FRAME_COUNT];
static tCharacter s_pEnemies[ENEMY_COUNT];

static tCharacter *s_pSortedChars[SORTED_CHARS_COUNT];

static tBitMap *s_pBulletFrames;
static tBitMap *s_pBulletMasks;

static tBitMap *s_pStainFrames;
static tBitMap *s_pStainMasks;
static tFrameOffset s_pStainFrameOffsets[STAIN_FRAME_PRESET_COUNT];
static tFrameOffset *s_pNextStainOffset;

static tPtplayerMod *s_pMod;
static UBYTE s_ubBufferCurr;
static tProjectile *s_pCurrentProjectile;
static tFix10p6 s_pSin10p6[GAME_MATH_ANGLE_COUNT];

static const UBYTE s_pBulletMaskFromX[] = {
	BV(7), BV(6), BV(5), BV(4), BV(3), BV(2), BV(1), BV(0)
};
static const UBYTE s_pWeaponAmmo[] = {
	[WEAPON_KIND_BASE_RIFLE] = 10,
	[WEAPON_KIND_SMG] = 30,
	[WEAPON_KIND_ASSAULT_RIFLE] = 25,
	[WEAPON_KIND_SHOTGUN] = 12,
	[WEAPON_KIND_SAWOFF] = 12,
};
static const UBYTE s_pWeaponReloadCooldowns[] = {
	[WEAPON_KIND_BASE_RIFLE] = 30,
	[WEAPON_KIND_SMG] = 40,
	[WEAPON_KIND_ASSAULT_RIFLE] = 40,
	[WEAPON_KIND_SHOTGUN] = 80,
	[WEAPON_KIND_SAWOFF] = 80,
};

static ULONG s_pRowOffsetFromY[MAP_TILES_Y * MAP_TILE_SIZE];

static tCharacter *s_pCollisionTiles[COLLISION_LOOKUP_SIZE_X][COLLISION_LOOKUP_SIZE_Y];
static tUwCoordYX s_pRespawnSlots[COLLISION_LOOKUP_SIZE_X][COLLISION_LOOKUP_SIZE_Y][RESPAWN_SLOTS_PER_POSITION];

static tProjectile s_pProjectiles[PROJECTILE_COUNT];
static tProjectile *s_pFreeProjectiles[PROJECTILE_COUNT];
static UBYTE s_ubFreeProjectileCount;
static BYTE s_pSpreadSide1[SPREAD_SIDE_COUNT];
static BYTE s_pSpreadSide2[SPREAD_SIDE_COUNT];
static BYTE s_pSpreadSide3[SPREAD_SIDE_COUNT];
static BYTE s_pSpreadSide10[SPREAD_SIDE_COUNT];

static tHudState s_eHudState;
static UWORD s_uwHudHealth;
static UBYTE s_ubHudAmmoCount;
static tUbCoordYX s_pHudBulletOffsets[WEAPON_MAX_BULLETS_IN_MAGAZINE];

static tPtplayerSfx *s_pSfxRifle[2];
static tPtplayerSfx *s_pSfxAssault[2];
static tPtplayerSfx *s_pSfxSmg[2];
static tPtplayerSfx *s_pSfxShotgun[2];
static tPtplayerSfx *s_pSfxImpact[2];
static tPtplayerSfx *s_pSfxBite[2];
static tPtplayerSfx *s_pSfxReload;

static tBob s_pStainBobs[STAINS_MAX];
static tBob *s_pFreeStains[STAINS_MAX];
static tBob *s_pPushStains[STAINS_MAX];
static tBob *s_pWaitStains[STAINS_MAX];
static tBob **s_pNextFreeStain;
static tBob **s_pNextPushStain;
static tBob **s_pNextWaitStain;

//------------------------------------------------------------------ PRIVATE FNS

static inline tFix10p6 fix10p6Add(tFix10p6 a, tFix10p6 b) {return a + b; }
static inline tFix10p6 fix10p6FromUword(UWORD x) {return x << 6; }
static inline tFix10p6 fix10p6ToUword(UWORD x) {return x >> 6; }
#define fix10p6Sin(x) s_pSin10p6[x]
#define fix10p6Cos(x) (((x) < 3 * ANGLE_90) ? fix10p6Sin(ANGLE_90 + (x)) : fix10p6Sin((x) - (3 * ANGLE_90)))

static void gameSetTile(UBYTE ubTileIndex, UWORD uwTileX, UWORD uwTileY) {
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

__attribute__((always_inline))
static inline UBYTE isPositionCollidingWithCharacter(
	tUwCoordYX sPos, const tCharacter *pCharacter
) {
	WORD Dx = sPos.uwX - pCharacter->sPos.uwX;
	WORD Dy = sPos.uwY - pCharacter->sPos.uwY;
	return (
		-COLLISION_SIZE_X <= Dx && Dx <= COLLISION_SIZE_X &&
		-COLLISION_SIZE_Y <= Dy && Dy <= COLLISION_SIZE_Y
	);
}

__attribute__((always_inline))
static inline tCharacter *characterGetNearPos(
	UWORD uwPosX, BYTE bLookupAddX, UWORD uwPosY, BYTE bLookupAddY
) {
	UWORD uwLookupX = uwPosX / COLLISION_SIZE_X + bLookupAddX;
	UWORD uwLookupY = uwPosY / COLLISION_SIZE_Y + bLookupAddY;
	// Watch out for accessing underflowed -1
	if(uwLookupX >= COLLISION_LOOKUP_SIZE_X || uwLookupY >= COLLISION_LOOKUP_SIZE_Y) {
		return 0;
	}
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
		ULONG ubOldLookupX = pCharacter->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubOldLookupY = pCharacter->sPos.uwY / COLLISION_SIZE_Y;
		pCharacter->sPos = sGoodPos;
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

__attribute__((always_inline))
static inline void projectileUndrawNext(void) {
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
		else {
			s_pFreeProjectiles[s_ubFreeProjectileCount++] = s_pCurrentProjectile;
		}
	}

	++s_pCurrentProjectile;
}

__attribute__((always_inline))
static inline void projectileDrawNext(void) {
	if(s_pCurrentProjectile == &s_pProjectiles[PROJECTILE_COUNT]) {
		return;
	}

	UBYTE *pTargetPlanes = s_pBackPlanes;
	if(s_pCurrentProjectile->ubLife > 1) {
		UWORD uwProjectileX = fix10p6ToUword(s_pCurrentProjectile->fX);
		UWORD uwProjectileY = fix10p6ToUword(s_pCurrentProjectile->fY);
		tCharacter *pEnemy;
		if(uwProjectileX >= MAP_TILES_X * MAP_TILE_SIZE || uwProjectileY >= MAP_TILES_Y * MAP_TILE_SIZE) {
			// TODO: Remove in favor of dummy entries in collision tiles at the edges
			s_pCurrentProjectile->ubLife = 1; // so that it will be undrawn on both buffers
		}
		else if(
			((pEnemy = characterGetNearPos(uwProjectileX, 0, uwProjectileY, 0)) && pEnemy != &s_sPlayer) ||
			((pEnemy = characterGetNearPos(uwProjectileX, -1, uwProjectileY, 0)) && pEnemy != &s_sPlayer) ||
			((pEnemy = characterGetNearPos(uwProjectileX, 0, uwProjectileY, -1)) && pEnemy != &s_sPlayer) ||
			((pEnemy = characterGetNearPos(uwProjectileX, -1, uwProjectileY, -1)) && pEnemy != &s_sPlayer)
		) {
			if(s_pNextFreeStain != &s_pFreeStains[0]) {
				tBob *pStain = *(--s_pNextFreeStain);
				pStain->sPos.uwX = uwProjectileX;
				pStain->sPos.uwY = uwProjectileY;
				*(s_pNextPushStain++) = pStain;
			}

			s_pCurrentProjectile->ubLife = 1; // so that it will be undrawn on both buffers
			pEnemy->wHealth -= s_pCurrentProjectile->ubDamage;
			audioMixerPlaySfx(s_pSfxImpact[0], SFX_CHANNEL_IMPACT, SFX_PRIORITY_IMPACT, 0);
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
	projectileUndrawNext();
}

void bobOnEnd(void) {
	projectileDrawNext();
}

static void cameraCenterAtOptimized(
	tCameraManager *pManager, ULONG ulCenterX, ULONG ulCenterY
) {
	LONG lTop = ulCenterX - MAIN_VPORT_SIZE_X / 2;
	LONG lLeft = ulCenterY - (MAIN_VPORT_SIZE_Y - HUD_SIZE_Y) / 2;
	pManager->uPos.uwX = CLAMP(lTop, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_X - MAP_MARGIN_TILES) * MAP_TILE_SIZE - MAIN_VPORT_SIZE_X);
	pManager->uPos.uwY = CLAMP(lLeft, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_Y - MAP_MARGIN_TILES) * MAP_TILE_SIZE - MAIN_VPORT_SIZE_Y);
}

static void hudProcess(void) {
	switch(s_eHudState) {
		case HUD_STATE_DRAW_HEALTH: {
			UWORD uwCurrentHealth = s_sPlayer.wHealth;
			if(s_uwHudHealth != uwCurrentHealth) {
				if(uwCurrentHealth > s_uwHudHealth) {
					blitRect(s_pBufferHud->pBack, 200 + s_uwHudHealth, 10, uwCurrentHealth - s_uwHudHealth, 4, COLOR_RED);
				}
				else {
					blitRect(s_pBufferHud->pBack, 200 + uwCurrentHealth, 10, s_uwHudHealth - uwCurrentHealth, 4, COLOR_BAR_BG);
				}
				s_uwHudHealth = uwCurrentHealth;
			}
			++s_eHudState;
		} break;
		case HUD_STATE_DRAW_WEAPON: {
			blitCopyAligned(
				s_pHudWeapons, 0, s_sPlayer.eWeapon * HUD_WEAPON_SIZE_Y,
				s_pBufferHud->pBack, 0, 0, HUD_WEAPON_SIZE_X, HUD_WEAPON_SIZE_Y
			);
			++s_eHudState;
		} break;
		case HUD_STATE_DRAW_BULLETS: {
			if(s_ubHudAmmoCount != s_sPlayer.ubAmmo) {
				if(s_ubHudAmmoCount == HUD_AMMO_COUNT_FORCE_REDRAW) {
					s_ubHudAmmoCount = 0;
					blitRect(
						s_pBufferHud->pBack, HUD_AMMO_FIELD_OFFSET_X, HUD_AMMO_FIELD_OFFSET_Y,
						HUD_AMMO_FIELD_SIZE_X, HUD_AMMO_FIELD_SIZE_Y, COLOR_HUD_BG
					);
				}
				else if(s_ubHudAmmoCount < s_sPlayer.ubAmmo) {
					UWORD uwSrcY;
					switch(s_sPlayer.eWeapon) {
						case WEAPON_KIND_SAWOFF:
						case WEAPON_KIND_SHOTGUN:
							uwSrcY = BULLET_OFFSET_Y_SHELL;
							break;
						default:
							uwSrcY = BULLET_OFFSET_Y_BULLET;
					}
					blitCopyMask(
						s_pBulletFrames, 0, uwSrcY, s_pBufferHud->pBack,
						s_pHudBulletOffsets[s_ubHudAmmoCount].ubX,
						s_pHudBulletOffsets[s_ubHudAmmoCount].ubY,
						HUD_AMMO_BULLET_SIZE_X, HUD_AMMO_BULLET_SIZE_Y, s_pBulletMasks->Planes[0]
					);
					++s_ubHudAmmoCount;
				}
				else {
					--s_ubHudAmmoCount;
					blitRect(
						s_pBufferHud->pBack,
						s_pHudBulletOffsets[s_ubHudAmmoCount].ubX,
						s_pHudBulletOffsets[s_ubHudAmmoCount].ubY,
						HUD_AMMO_BULLET_SIZE_X, HUD_AMMO_BULLET_SIZE_Y, COLOR_HUD_BG
					);
				}
			}
			++s_eHudState;
		} break;
		case HUD_STATE_END:
			s_eHudState = 0;
			break;
	}
}

static void playerSetWeapon(tWeaponKind eWeaponKind) {
	s_sPlayer.eWeapon = eWeaponKind;
	s_sPlayer.ubAmmo = s_pWeaponAmmo[eWeaponKind];

	s_ubHudAmmoCount = HUD_AMMO_COUNT_FORCE_REDRAW;
	// TOOD: set ammo
}

__attribute__((always_inline))
static inline void playerStartReloadWeapon(void) {
	s_sPlayer.ubReloadCooldown = s_pWeaponReloadCooldowns[s_sPlayer.eWeapon];
	// s_sPlayer.ubReloadCooldown = 1;
	s_sPlayer.ubAmmo = s_pWeaponAmmo[s_sPlayer.eWeapon];
	audioMixerPlaySfx(s_pSfxReload, SFX_CHANNEL_RELOAD, SFX_PRIORITY_RELOAD, 0);
}

__attribute__((always_inline))
static inline void playerShootProjectile(BYTE bAngle, BYTE pSpreadSide[static SPREAD_SIDE_COUNT], UBYTE ubDamage) {
	static UBYTE s_ubSpread = 0;
	static UBYTE s_ubSpreadSide = 0;

	bAngle += pSpreadSide[s_ubSpreadSide];
	if(++s_ubSpreadSide >= SPREAD_SIDE_COUNT) {
		s_ubSpreadSide = 0;
	}

	if(bAngle < 0) {
		bAngle += ANGLE_360;
	}
	if(s_ubFreeProjectileCount) {
		tProjectile *pProjectile = s_pFreeProjectiles[--s_ubFreeProjectileCount];
		pProjectile->ubLife = PROJECTILE_LIFETIME;
		pProjectile->ubDamage = ubDamage;
		pProjectile->fDx = fix10p6Cos(bAngle) * PROJECTILE_SPEED;
		pProjectile->fDy = fix10p6Sin(bAngle) * PROJECTILE_SPEED;
		pProjectile->fX = fix10p6FromUword(s_sPlayer.sPos.uwX);
		pProjectile->fY = fix10p6FromUword(s_sPlayer.sPos.uwY);
		if(s_ubSpread == 0) {
			++s_ubSpread;
		}
		else if(s_ubSpread == 1) {
			pProjectile->fX += pProjectile->fDx;
			pProjectile->fY += pProjectile->fDy;
			++s_ubSpread;
		}
		else { // if(s_ubSpread == 2)
			pProjectile->fX -= pProjectile->fDx;
			pProjectile->fY -= pProjectile->fDy;
			s_ubSpread = 0;
		}
	}
	else {
		logWrite("ERR: No free projectiles");
	}
}

static void playerShootWeapon(UBYTE ubAimAngle) {
	// TODO: precalculate random stuff
	switch(s_sPlayer.eWeapon) {
		case WEAPON_KIND_BASE_RIFLE:
			playerShootProjectile(ubAimAngle, s_pSpreadSide1, WEAPON_DAMAGE_BASE_RIFLE);
			audioMixerPlaySfx(s_pSfxRifle[0], SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			s_sPlayer.ubAttackCooldown = WEAPON_COOLDOWN_BASE_RIFLE;
			break;
			case WEAPON_KIND_SMG:
			playerShootProjectile(ubAimAngle, s_pSpreadSide3, WEAPON_DAMAGE_SMG);
			audioMixerPlaySfx(s_pSfxSmg[0], SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			s_sPlayer.ubAttackCooldown = WEAPON_COOLDOWN_SMG;
			break;
		case WEAPON_KIND_ASSAULT_RIFLE:
			playerShootProjectile(ubAimAngle, s_pSpreadSide2, WEAPON_DAMAGE_ASSAULT_RIFLE);
			audioMixerPlaySfx(s_pSfxAssault[0], SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			s_sPlayer.ubAttackCooldown = WEAPON_COOLDOWN_ASSAULT_RIFLE;
			break;
		case WEAPON_KIND_SHOTGUN:
			for(UBYTE i = 0; i < 10; ++i) {
				playerShootProjectile(ubAimAngle, s_pSpreadSide3, WEAPON_DAMAGE_SHOTGUN);
			}
			audioMixerPlaySfx(s_pSfxShotgun[0], SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			s_sPlayer.ubAttackCooldown = WEAPON_COOLDOWN_SHOTGUN;
			break;
		case WEAPON_KIND_SAWOFF:
			for(UBYTE i = 0; i < 10; ++i) {
				playerShootProjectile(ubAimAngle, s_pSpreadSide10, WEAPON_DAMAGE_SAWOFF);
			}
			audioMixerPlaySfx(s_pSfxShotgun[0], SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			s_sPlayer.ubAttackCooldown = WEAPON_COOLDOWN_SAWOFF;
			break;
	}
	// TODO: remove ammo from magazine
}

static void gameStart(void) {
	gameSetTile(0, MAP_MARGIN_TILES, MAP_MARGIN_TILES);
	gameSetTile(1, MAP_TILES_X - 1 - MAP_MARGIN_TILES, MAP_MARGIN_TILES);
	gameSetTile(2, MAP_MARGIN_TILES, MAP_TILES_Y - 1 - MAP_MARGIN_TILES);
	gameSetTile(3, MAP_TILES_X - 1 - MAP_MARGIN_TILES, MAP_TILES_Y - 1 - MAP_MARGIN_TILES);

	for(UBYTE ubX = MAP_MARGIN_TILES + 1; ubX < MAP_TILES_X - 1 - MAP_MARGIN_TILES; ++ubX) {
		gameSetTile(randUwMinMax(&g_sRand, 4, 6), ubX, MAP_MARGIN_TILES);
		gameSetTile(randUwMinMax(&g_sRand, 13, 15), ubX, MAP_TILES_Y - 1 - MAP_MARGIN_TILES);
	}

	for(UBYTE ubY = MAP_MARGIN_TILES + 1; ubY < MAP_TILES_Y - 1 - MAP_MARGIN_TILES; ++ubY) {
		gameSetTile(randUwMinMax(&g_sRand, 7, 9), MAP_MARGIN_TILES, ubY);
		gameSetTile(randUwMinMax(&g_sRand, 10, 12), MAP_TILES_X - 1 - MAP_MARGIN_TILES, ubY);
	}

	for(UBYTE ubX = MAP_MARGIN_TILES + 1; ubX < MAP_TILES_X - 1 - MAP_MARGIN_TILES; ++ubX) {
		for(UBYTE ubY = MAP_MARGIN_TILES + 1; ubY < MAP_TILES_Y - 1 - MAP_MARGIN_TILES; ++ubY) {
			gameSetTile(randUwMinMax(&g_sRand, 16, 24), ubX, ubY);
		}
	}

	s_uwHudHealth = 0;
	s_ubHudAmmoCount = HUD_AMMO_COUNT_FORCE_REDRAW;
	s_eHudState = 0;

	s_sPlayer.wHealth = PLAYER_HEALTH_MAX;
	s_sPlayer.sPos.uwX = 180;
	s_sPlayer.sPos.uwY = 180;
	s_sPlayer.eFrame = 0;
	s_sPlayer.ubFrameCooldown = 0;
	s_sPlayer.ubAttackCooldown = PLAYER_ATTACK_COOLDOWN;
	playerSetWeapon(WEAPON_KIND_BASE_RIFLE);
	s_pCollisionTiles[s_sPlayer.sPos.uwX / COLLISION_SIZE_X][s_sPlayer.sPos.uwY / COLLISION_SIZE_Y] = &s_sPlayer;

	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		s_pEnemies[i].wHealth = ENEMY_HEALTH_MAX;
		s_pEnemies[i].sPos.uwX = 32 + (i % 8) * 32;
		s_pEnemies[i].sPos.uwY = 32 + (i / 8) * 32;
		s_pEnemies[i].eFrame = 0;
		s_pEnemies[i].ubFrameCooldown = 0;
		s_pEnemies[i].ubAttackCooldown = 0;
		s_pCollisionTiles[s_pEnemies[i].sPos.uwX / COLLISION_SIZE_X][s_pEnemies[i].sPos.uwY / COLLISION_SIZE_Y] = &s_pEnemies[i];
		s_pSortedChars[i] = &s_pEnemies[i];
	}
	s_pSortedChars[ENEMY_COUNT] = &s_sPlayer;

	for(UBYTE i = 0; i < PROJECTILE_COUNT; ++i) {
		s_pProjectiles[i].ubLife = 0;
		s_pFreeProjectiles[i] = &s_pProjectiles[i];
	}
	s_ubFreeProjectileCount = PROJECTILE_COUNT;
	s_ulFrameCount = 0;
	s_ulFrameWaitCount = 1;
}

__attribute__((always_inline))
static inline void playerProcess(void) {
	UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
	UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);

	if(s_sPlayer.wHealth > 0) {
		if(keyUse(KEY_1)) {
			playerSetWeapon(WEAPON_KIND_BASE_RIFLE);
		}
		else if(keyUse(KEY_2)) {
			playerSetWeapon(WEAPON_KIND_SMG);
		}
		else if(keyUse(KEY_3)) {
			playerSetWeapon(WEAPON_KIND_ASSAULT_RIFLE);
		}
		else if(keyUse(KEY_4)) {
			playerSetWeapon(WEAPON_KIND_SHOTGUN);
		}
		else if(keyUse(KEY_5)) {
			playerSetWeapon(WEAPON_KIND_SAWOFF);
		}

		UBYTE ubAimAngle = getAngleBetweenPoints( // 0 is right, going clockwise
			s_sPlayer.sPos.uwX - s_pBufferMain->pCamera->uPos.uwX,
			s_sPlayer.sPos.uwY - s_pBufferMain->pCamera->uPos.uwY,
			uwMouseX, uwMouseY - HUD_SIZE_Y
		);

		BYTE bDeltaX = 0;
		BYTE bDeltaY = 0;
		if(keyCheck(KEY_W)) {
			bDeltaY = -3;
		}
		else if(keyCheck(KEY_S)) {
			bDeltaY = 3;
		}
		if(keyCheck(KEY_A)) {
			bDeltaX = -3;
		}
		else if(keyCheck(KEY_D)) {
			bDeltaX = 3;
		}
		if(bDeltaX || bDeltaY) {
			characterTryMoveBy(&s_sPlayer, bDeltaX, bDeltaY);
			if(s_sPlayer.ubFrameCooldown >= 1) {
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
		if(ubAimAngle < ANGLE_45) {
			eDir = DIRECTION_SE;
		}
		else if(ubAimAngle < ANGLE_45 + ANGLE_90) {
			eDir = DIRECTION_S;
		}
		else if(ubAimAngle < ANGLE_180) {
			eDir = DIRECTION_SW;
		}
		else if(ubAimAngle < ANGLE_180 + ANGLE_45) {
			eDir = DIRECTION_NW;
		}
		else if(ubAimAngle < ANGLE_180 + ANGLE_45 + ANGLE_90) {
			eDir = DIRECTION_N;
		}
		else {
			eDir = DIRECTION_NE;
		}

		tFrameOffset *pOffset = &s_pPlayerFrameOffsets[eDir][s_sPlayer.eFrame];
		bobSetFrame(&s_sPlayer.sBob, pOffset->pPixels, pOffset->pMask);
		s_sPlayer.sBob.sPos.uwX = s_sPlayer.sPos.uwX - PLAYER_BOB_OFFSET_X;
		s_sPlayer.sBob.sPos.uwY = s_sPlayer.sPos.uwY - PLAYER_BOB_OFFSET_Y;

		if(!s_sPlayer.ubAttackCooldown) {
			if(!s_sPlayer.ubReloadCooldown) {
				if(!s_sPlayer.ubAmmo) {
					playerStartReloadWeapon();
				}
				else {
					if(mouseCheck(MOUSE_PORT_1, MOUSE_LMB)) {
						--s_sPlayer.ubAmmo;
						playerShootWeapon(ubAimAngle);
					}
					else if(keyUse(KEY_R) && s_sPlayer.ubAmmo < s_pWeaponAmmo[s_sPlayer.eWeapon]) {
						playerStartReloadWeapon();
					}
				}
			}
			else {
				--s_sPlayer.ubReloadCooldown;
			}
		}
		else {
			--s_sPlayer.ubAttackCooldown;
		}
	}
	else {
		s_sPlayer.wHealth = 0; // Get rid of negative value for HUD etc
	}
	bobPush(&s_sPlayer.sBob);

	s_pSpriteCursor->wX = uwMouseX - 4;
	s_pSpriteCursor->wY = uwMouseY - 4;
	spriteProcess(s_pSpriteCursor);
	spriteProcessChannel(SPRITE_CHANNEL_CURSOR);

	cameraCenterAtOptimized(s_pBufferMain->pCamera, s_sPlayer.sPos.uwX, s_sPlayer.sPos.uwY);
}

__attribute__((always_inline))
static inline void enemyProcess(tCharacter *pEnemy) {
	BYTE bDeltaX, bDeltaY;
	tDirection eDir;
	if(pEnemy->wHealth > 0) {
		WORD wDistanceToPlayerX = s_sPlayer.sPos.uwX - pEnemy->sPos.uwX;
		WORD wDistanceToPlayerY = s_sPlayer.sPos.uwY - pEnemy->sPos.uwY;
		if(wDistanceToPlayerX < 0) {
			bDeltaX = -ENEMY_SPEED;
			if(wDistanceToPlayerY < 0) {
				bDeltaY = -ENEMY_SPEED;
				eDir = DIRECTION_NW;
				wDistanceToPlayerY = -wDistanceToPlayerY;
			}
			else {
				bDeltaY = ENEMY_SPEED;
				eDir = DIRECTION_SW;
			}
			wDistanceToPlayerX = -wDistanceToPlayerX;
		}
		else {
			bDeltaX = ENEMY_SPEED;
			if(wDistanceToPlayerY < 0) {
				bDeltaY = -ENEMY_SPEED;
				eDir = DIRECTION_NE;
				wDistanceToPlayerX = -wDistanceToPlayerX;
			}
			else {
				bDeltaY = ENEMY_SPEED;
				eDir = DIRECTION_SE;
			}
	}

		if(pEnemy->ubAttackCooldown == 0) {
			if((UWORD)wDistanceToPlayerX < 10 && (UWORD)wDistanceToPlayerY < 10) {
				s_sPlayer.wHealth -= 5;
				audioMixerPlaySfx(s_pSfxBite[0], SFX_CHANNEL_BITE, SFX_PRIORITY_BITE, 0);
				pEnemy->ubAttackCooldown = ENEMY_ATTACK_COOLDOWN;
			}
		}
		else {
			--pEnemy->ubAttackCooldown;
		}

		// if(0) {
			characterTryMoveBy(pEnemy, bDeltaX, bDeltaY);
		// }
		tFrameOffset *pOffset = &s_pEnemyFrameOffsets[eDir][pEnemy->eFrame];
		bobSetFrame(&pEnemy->sBob, pOffset->pPixels, pOffset->pMask);
		pEnemy->sBob.sPos.uwX = pEnemy->sPos.uwX - ENEMY_BOB_OFFSET_X;
		pEnemy->sBob.sPos.uwY = pEnemy->sPos.uwY - ENEMY_BOB_OFFSET_Y;
		bobPush(&pEnemy->sBob);
	}
	else {
		// Try respawn
		static UBYTE ubSpawnIndex = 0;
		s_pCollisionTiles[pEnemy->sPos.uwX / COLLISION_SIZE_X][pEnemy->sPos.uwY / COLLISION_SIZE_Y] = 0;
		for(UBYTE i = 0; i < RESPAWN_SLOTS_PER_POSITION; ++i) {
			tUwCoordYX sSpawn = s_pRespawnSlots[s_sPlayer.sPos.uwX / COLLISION_SIZE_X][s_sPlayer.sPos.uwY / COLLISION_SIZE_Y][ubSpawnIndex];
			ubSpawnIndex = (ubSpawnIndex + 1) & 3;
			if(!s_pCollisionTiles[sSpawn.uwX / COLLISION_SIZE_X][sSpawn.uwY / COLLISION_SIZE_Y]) {
				pEnemy->wHealth = ENEMY_HEALTH_MAX;
				s_pCollisionTiles[sSpawn.uwX / COLLISION_SIZE_X][sSpawn.uwY / COLLISION_SIZE_Y] = pEnemy;
				pEnemy->sPos = sSpawn;
				return;
			}
		}
		// Failsafe to prevent trashing collision map
		pEnemy->sPos.uwX = 0;
		pEnemy->sPos.uwY = 0;
	}
}

static void blitUnsafeCopyStain(
	UBYTE *pSrc, UBYTE *pMsk, WORD wDstX, WORD wDstY
) {
	// Blitter register values
	UBYTE ubDstDelta = wDstX & 0xF;
	UWORD uwBlitWidth = (STAIN_SIZE_X + ubDstDelta + 15) & 0xFFF0;
	UWORD uwBlitWords = uwBlitWidth >> 4;

	UWORD uwFirstMask = 0xFFFF;
	UWORD uwLastMask = 0xFFFF << (uwBlitWidth - STAIN_SIZE_X);

	UBYTE ubShift = ubDstDelta;
	UWORD uwBltCon1 = ubShift << BSHIFTSHIFT;

	ULONG ulDstOffs = s_pRowOffsetFromY[wDstY] + (wDstX >> 3);
	UBYTE *pCD = &s_pPristineBuffer->Planes[0][ulDstOffs];

	UWORD uwBltCon0 = uwBltCon1 |USEA|USEB|USEC|USED | MINTERM_COOKIE;
	WORD wSrcModulo = STAIN_BYTES_PER_BITPLANE_ROW - uwBlitWords * 2;
	WORD wDstModulo = BG_BYTES_PER_BITPLANE_ROW - uwBlitWords * 2;

	blitWait(); // Don't modify registers when other blit is in progress
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = uwBltCon1;
	g_pCustom->bltafwm = uwFirstMask;
	g_pCustom->bltalwm = uwLastMask;
	g_pCustom->bltamod = wSrcModulo;
	g_pCustom->bltbmod = wSrcModulo;
	g_pCustom->bltcmod = wDstModulo;
	g_pCustom->bltdmod = wDstModulo;
	g_pCustom->bltapt = pMsk;
	g_pCustom->bltbpt = pSrc;
	g_pCustom->bltcpt = pCD;
	g_pCustom->bltdpt = pCD;
	g_pCustom->bltsize = ((STAIN_SIZE_Y * GAME_BPP) << HSIZEBITS) | uwBlitWords;
}

static void onVblank(
	UNUSED_ARG REGARG(volatile tCustom *pCustom, "a0"),
	REGARG(volatile void *pData, "a1")
) {
	ULONG *pFrameCount = (ULONG*)pData;
	*pFrameCount += 1;
}

static void gameGsCreate(void) {
	logBlockBegin("gameGsCreate()");
	s_pTileset = bitmapCreateFromPath("data/tiles.bm", 0);
	s_pHudWeapons = bitmapCreateFromPath("data/weapons.bm", 0);

	ULONG ulRawCopSize = 16 + simpleBufferGetRawCopperlistInstructionCount(GAME_BPP) * 2;
	s_pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, ulRawCopSize,
	TAG_END);

	s_pVpHud = vPortCreate(0,
		TAG_VPORT_BPP, GAME_BPP,
		TAG_VPORT_HEIGHT, HUD_SIZE_Y,
		TAG_VPORT_VIEW, s_pView,
	TAG_END);

	ULONG ulCopOffset = 16;
	s_pBufferHud = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
		TAG_SIMPLEBUFFER_VPORT, s_pVpHud,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, ulCopOffset,
	TAG_END);

	s_pVpMain = vPortCreate(0,
		TAG_VPORT_VIEW, s_pView,
		TAG_VPORT_BPP, GAME_BPP,
	TAG_END);

	ulCopOffset += simpleBufferGetRawCopperlistInstructionCount(GAME_BPP);
	s_pBufferMain = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, MAP_TILES_X * MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, MAP_TILES_Y * MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, ulCopOffset,
	TAG_END);

	blitRect(s_pBufferHud->pBack, 0, 0, 320, HUD_SIZE_Y, COLOR_HUD_BG);

	s_pPristineBuffer = bitmapCreate(
		bitmapGetByteWidth(s_pBufferMain->pBack) * 8,
		s_pBufferMain->pBack->Rows, GAME_BPP, BMF_INTERLEAVED
	);
	s_pPristinePlanes = s_pPristineBuffer->Planes[0];

	paletteLoadFromPath("data/game.plt", s_pVpHud->pPalette, 1 << GAME_BPP);

	s_pMod = ptplayerModCreateFromPath("data/germz1.mod");

	randInit(&g_sRand, 2184, 1911);

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
		for(tCharacterFrame eFrame = 0; eFrame < PLAYER_FRAME_COUNT; ++eFrame) {
			s_pPlayerFrameOffsets[eDir][eFrame].pPixels = bobCalcFrameAddress(s_pPlayerFrames[eDir], eFrame * PLAYER_BOB_SIZE_Y);
			s_pPlayerFrameOffsets[eDir][eFrame].pMask = bobCalcFrameAddress(s_pPlayerMasks[eDir], eFrame * PLAYER_BOB_SIZE_Y);

#if defined(GAME_COLLISION_DEBUG)
			blitRect(
				s_pPlayerFrames[eDir], PLAYER_BOB_OFFSET_X,
				eFrame * PLAYER_BOB_SIZE_Y + PLAYER_BOB_OFFSET_Y,
				COLLISION_SIZE_X, COLLISION_SIZE_Y, 31
			);
#endif
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
		for(tCharacterFrame eFrame = 0; eFrame < PLAYER_FRAME_COUNT; ++eFrame) {
			s_pEnemyFrameOffsets[eDir][eFrame].pPixels = bobCalcFrameAddress(s_pEnemyFrames[eDir], eFrame * ENEMY_BOB_SIZE_Y);
			s_pEnemyFrameOffsets[eDir][eFrame].pMask = bobCalcFrameAddress(s_pEnemyMasks[eDir], eFrame * ENEMY_BOB_SIZE_Y);

#if defined(GAME_COLLISION_DEBUG)
			blitRect(
				s_pEnemyFrames[eDir], ENEMY_BOB_OFFSET_X,
				eFrame * ENEMY_BOB_SIZE_Y + ENEMY_BOB_OFFSET_Y,
				COLLISION_SIZE_X, COLLISION_SIZE_Y, 25
			);
#endif
		}
	}

	s_pBulletFrames = bitmapCreateFromPath("data/bullets.bm", 0);
	s_pBulletMasks = bitmapCreateFromPath("data/bullets_mask.bm", 0);
	for(UBYTE i = 0; i < WEAPON_MAX_BULLETS_IN_MAGAZINE; ++i) {
		s_pHudBulletOffsets[i] = (tUbCoordYX){
			.ubX = HUD_AMMO_FIELD_OFFSET_X + (i % HUD_AMMO_BULLET_COUNT_X) * (HUD_AMMO_BULLET_SIZE_X + 1),
			.ubY = HUD_AMMO_FIELD_OFFSET_Y + (i / HUD_AMMO_BULLET_COUNT_X) * (HUD_AMMO_BULLET_SIZE_Y + 1)
		};
	}

	s_pStainFrames = bitmapCreateFromPath("data/stains.bm", 0);
	s_pStainMasks = bitmapCreateFromPath("data/stains_mask.bm", 0);

	s_pSfxRifle[0] = ptplayerSfxCreateFromPath("data/sfx/rifle_shot_1.sfx", 1);
	s_pSfxRifle[1] = ptplayerSfxCreateFromPath("data/sfx/rifle_shot_2.sfx", 1);
	s_pSfxAssault[0] = ptplayerSfxCreateFromPath("data/sfx/assault_shot_1.sfx", 1);
	s_pSfxAssault[1] = ptplayerSfxCreateFromPath("data/sfx/assault_shot_2.sfx", 1);
	s_pSfxSmg[0] = ptplayerSfxCreateFromPath("data/sfx/smg_shot_1.sfx", 1);
	s_pSfxSmg[1] = ptplayerSfxCreateFromPath("data/sfx/smg_shot_2.sfx", 1);
	s_pSfxShotgun[0] = ptplayerSfxCreateFromPath("data/sfx/shotgun_shot_1.sfx", 1);
	s_pSfxShotgun[1] = ptplayerSfxCreateFromPath("data/sfx/shotgun_shot_2.sfx", 1);
	s_pSfxImpact[0] = ptplayerSfxCreateFromPath("data/sfx/impact_1.sfx", 1);
	s_pSfxImpact[1] = ptplayerSfxCreateFromPath("data/sfx/impact_2.sfx", 1);
	s_pSfxBite[0] = ptplayerSfxCreateFromPath("data/sfx/bite_1.sfx", 1);
	s_pSfxBite[1] = ptplayerSfxCreateFromPath("data/sfx/bite_2.sfx", 1);
	s_pSfxReload = ptplayerSfxCreateFromPath("data/sfx/reload_1.sfx", 1);

	for(UBYTE i = 0; i < SPREAD_SIDE_COUNT; ++i) {
		s_pSpreadSide1[i] = - 2/2 + randUwMax(&g_sRand, 2);
	}
	for(UBYTE i = 0; i < SPREAD_SIDE_COUNT; ++i) {
		s_pSpreadSide2[i] = - 4/2 + randUwMax(&g_sRand, 4);
	}
	for(UBYTE i = 0; i < SPREAD_SIDE_COUNT; ++i) {
		s_pSpreadSide3[i] = - 6/2 + randUwMax(&g_sRand, 6);
	}
	for(UBYTE i = 0; i < SPREAD_SIDE_COUNT; ++i) {
		s_pSpreadSide10[i] = - 20/2 + randUwMax(&g_sRand, 20);
	}

	bobManagerCreate(
		s_pBufferMain->pFront, s_pBufferMain->pBack,
		s_pPristineBuffer, MAP_TILES_Y * MAP_TILE_SIZE
	);

	for(UBYTE i = 0; i < STAIN_FRAME_PRESET_COUNT; ++i) {
		UWORD uwOffsY = STAIN_SIZE_Y * randUwMax(&g_sRand, STAIN_FRAME_COUNT - 1);
		s_pStainFrameOffsets[i].pPixels = bobCalcFrameAddress(s_pStainFrames, uwOffsY);
		s_pStainFrameOffsets[i].pMask = bobCalcFrameAddress(s_pStainMasks, uwOffsY);
	}
	s_pNextStainOffset = &s_pStainFrameOffsets[0];

	for(UBYTE i = 0; i < STAINS_MAX; ++i) {
		bobInit(&s_pStainBobs[i], STAIN_SIZE_X, STAIN_SIZE_Y, 1, 0, 0, 0, 0);
		s_pFreeStains[i] = &s_pStainBobs[i];
	}
	s_pNextFreeStain = &s_pFreeStains[STAINS_MAX];
	s_pNextPushStain = &s_pPushStains[0];
	s_pNextWaitStain = &s_pWaitStains[0];

	bobInit(&s_sPlayer.sBob, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, 1, s_pPlayerFrameOffsets[0][0].pPixels, s_pPlayerFrameOffsets[0][0].pMask, 32, 32);
	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		bobInit(&s_pEnemies[i].sBob, ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_Y, 1, s_pEnemyFrameOffsets[0][0].pPixels, s_pEnemyFrameOffsets[0][0].pMask, 32, 32);
	}

	bobReallocateBuffers();
	gameMathInit();
	for(UBYTE ubAngle = 0; ubAngle < GAME_MATH_ANGLE_COUNT; ++ubAngle) {
		s_pSin10p6[ubAngle] = csin(ubAngle) >> 10;
	}

	for(UBYTE ubX = 0; ubX < COLLISION_LOOKUP_SIZE_X; ++ubX) {
		for(UBYTE ubY = 0; ubY < COLLISION_LOOKUP_SIZE_Y; ++ubY) {
			ULONG ulCenterX = ubX * COLLISION_SIZE_X;
			ULONG ulCenterY = ubY * COLLISION_SIZE_Y;
			LONG lLeft = ulCenterX - MAIN_VPORT_SIZE_X / 2;
			LONG lTop = ulCenterY - (MAIN_VPORT_SIZE_Y - HUD_SIZE_Y) / 2;
			lLeft = CLAMP(lLeft, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_X - MAP_MARGIN_TILES) * MAP_TILE_SIZE - MAIN_VPORT_SIZE_X);
			lTop = CLAMP(lTop, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_Y - MAP_MARGIN_TILES) * MAP_TILE_SIZE - MAIN_VPORT_SIZE_Y);

			s_pRespawnSlots[ubX][ubY][0] = (tUwCoordYX) {
				.uwX = CLAMP(lLeft - ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_X, MAP_TILES_X * MAP_TILE_SIZE - ENEMY_BOB_SIZE_X),
				.uwY = ulCenterY,
			};
			s_pRespawnSlots[ubX][ubY][1] = (tUwCoordYX) {
				.uwX = CLAMP(lLeft + MAIN_VPORT_SIZE_X + ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_X, MAP_TILES_X * MAP_TILE_SIZE - ENEMY_BOB_SIZE_X),
				.uwY = ulCenterY,
			};
			s_pRespawnSlots[ubX][ubY][2] = (tUwCoordYX) {
				.uwX = ulCenterX,
				.uwY = CLAMP(lTop - ENEMY_BOB_SIZE_Y, ENEMY_BOB_SIZE_Y, MAP_TILES_Y * MAP_TILE_SIZE - ENEMY_BOB_SIZE_Y),
			};
			s_pRespawnSlots[ubX][ubY][3] = (tUwCoordYX) {
				.uwX = ulCenterX,
				.uwY = CLAMP(lTop + MAIN_VPORT_SIZE_Y + ENEMY_BOB_SIZE_Y, ENEMY_BOB_SIZE_Y, MAP_TILES_Y * MAP_TILE_SIZE - ENEMY_BOB_SIZE_Y),
			};
		}
	}

	s_pBmCrosshair = bitmapCreateFromPath("data/cursor_crosshair.bm", 0);
	spriteManagerCreate(s_pView, 0, 0);
	s_pSpriteCursor = spriteAdd(SPRITE_CHANNEL_CURSOR, s_pBmCrosshair);
	systemSetDmaBit(DMAB_SPRITE, 1);
	mouseSetBounds(MOUSE_PORT_1, 0, HUD_SIZE_Y, 320, 256);

	systemUnuse();
	systemSetInt(INTB_VERTB, onVblank, (void*)&s_ulFrameCount);
	gameStart();
	viewLoad(s_pView);
	viewProcessManagers(s_pView);
	viewProcessManagers(s_pView);
	logBlockEnd("gameGsCreate()");
	ptplayerLoadMod(s_pMod, 0, 0);
	ptplayerEnableMusic(1);
}

__attribute__((always_inline))
static inline void gameWaitForNextFrame(void) {
	systemIdleBegin();
	while(s_ulFrameCount < s_ulFrameWaitCount) continue;
	s_ulFrameWaitCount += 2;
	vPortWaitUntilEnd(s_pVpMain);
	systemIdleEnd();
}

static void gameGsLoop(void) {
	if(keyUse(KEY_ESCAPE)) {
		statePop(g_pGameStateManager);
		return;
	}
	if(keyUse(KEY_Q)) {
		bobDiscardUndraw();
		gameStart();
		return;
	}

	s_pBackPlanes = s_pBufferMain->pBack->Planes[0];
	s_pCurrentProjectile = &s_pProjectiles[0];
	bobBegin(s_pBufferMain->pBack);
	while(s_pCurrentProjectile != &s_pProjectiles[PROJECTILE_COUNT]) {
		projectileUndrawNext();
	}

	tCharacter **pPrev = &s_pSortedChars[0];
	for(UBYTE i = 0; i < SORTED_CHARS_COUNT; ++i) {
		tCharacter *pChar = s_pSortedChars[i];
		if(pChar == &s_sPlayer) {
			playerProcess();
		}
		else {
			enemyProcess(pChar);
		}

		if(pChar->sPos.ulYX < (*pPrev)->sPos.ulYX) {
			s_pSortedChars[i] = *pPrev;
			*pPrev = pChar;
		}
		pPrev = &s_pSortedChars[i];
	}

	s_pCurrentProjectile = &s_pProjectiles[0];
	bobEnd();
	while(s_pCurrentProjectile != &s_pProjectiles[PROJECTILE_COUNT]) {
		projectileDrawNext();
	}

	if(s_pNextWaitStain != &s_pWaitStains[0]) {
		tBob *pStain = *(--s_pNextWaitStain);
		*(s_pNextFreeStain++) = pStain;
	}

	if(s_pNextPushStain != &s_pPushStains[0]) {
		tBob *pStain = *(--s_pNextPushStain);
		blitUnsafeCopyStain(
			s_pNextStainOffset->pPixels, s_pNextStainOffset->pMask,
			pStain->sPos.uwX, pStain->sPos.uwY
		);
		bobForceUndraw(pStain);
		*(s_pNextWaitStain++) = pStain;
		if(++s_pNextStainOffset == &s_pStainFrameOffsets[STAIN_FRAME_PRESET_COUNT]) {
			s_pNextStainOffset = &s_pStainFrameOffsets[0];
		}
	}

	s_ubBufferCurr = !s_ubBufferCurr;

	hudProcess();

	simpleBufferProcess(s_pBufferMain);
	cameraProcess(s_pBufferMain->pCamera);
	copSwapBuffers();
	gameWaitForNextFrame();
}

static void gameGsDestroy(void) {
	viewLoad(0);
	ptplayerStop();
	systemUse();

	ptplayerSfxDestroy(s_pSfxRifle[0]);
	ptplayerSfxDestroy(s_pSfxRifle[1]);
	ptplayerSfxDestroy(s_pSfxAssault[0]);
	ptplayerSfxDestroy(s_pSfxAssault[1]);
	ptplayerSfxDestroy(s_pSfxSmg[0]);
	ptplayerSfxDestroy(s_pSfxSmg[1]);
	ptplayerSfxDestroy(s_pSfxShotgun[0]);
	ptplayerSfxDestroy(s_pSfxShotgun[1]);
	ptplayerSfxDestroy(s_pSfxImpact[0]);
	ptplayerSfxDestroy(s_pSfxImpact[1]);
	ptplayerSfxDestroy(s_pSfxBite[0]);
	ptplayerSfxDestroy(s_pSfxBite[1]);
	ptplayerSfxDestroy(s_pSfxReload);

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

	bitmapDestroy(s_pBulletFrames);
	bitmapDestroy(s_pBulletMasks);
	bitmapDestroy(s_pStainFrames);
	bitmapDestroy(s_pStainMasks);

	viewDestroy(s_pView);
	bitmapDestroy(s_pPristineBuffer);
	bitmapDestroy(s_pHudWeapons);
	bitmapDestroy(s_pTileset);
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy,
};
