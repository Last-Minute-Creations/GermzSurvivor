/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "game.h"
#include <ace/managers/bob.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/managers/system.h>
#include <ace/managers/sprite.h>
#include <ace/managers/ptplayer.h>
#include <ace/contrib/managers/audio_mixer.h>
#include <ace/utils/palette.h>
#include <ace/utils/chunky.h>
#include <ace/utils/string.h>
#include "comm/comm.h"
#include "survivor.h"
#include "game_math.h"
#include "assets.h"
#include "menu.h"
#include "hi_score.h"
#include "pause.h"

#define PERK_DEATH_CLOCK_COOLDOWN 5
#define PERK_DODGE_CHANCE_DODGER 10
#define PERK_DODGE_CHANCE_NINJA 25

#define EXPLOSION_HIT_RANGE 64
#define EXPLOSION_BOB_SIZE_X 64
#define EXPLOSION_BOB_SIZE_Y 64
#define EXPLOSION_FRAME_COUNT 6
#define EXPLOSION_COOLDOWN 5

#define GAME_PLAYER_DEATH_COOLDOWN 50
#define WEAPON_MAX_BULLETS_IN_MAGAZINE (((30 + 2) * 12 + 5) / 10)
#define STAIN_FRAME_COUNT 6
#define STAIN_FRAME_PRESET_COUNT 16
#define STAINS_MAX 20
#define STAIN_SIZE_X 16
#define STAIN_SIZE_Y 16
#define STAIN_BYTES_PER_BITPLANE_ROW (STAIN_SIZE_X / 8)
#define STAIN_BYTES_PER_PIXEL_ROW (STAIN_BYTES_PER_BITPLANE_ROW * GAME_BPP)

#define DIGIT_WIDTH_MAX 5
#define HUD_WEAPON_SIZE_X 48
#define HUD_WEAPON_SIZE_Y 15
#define HUD_AMMO_FIELD_OFFSET_X (HUD_WEAPON_SIZE_X + 2)
#define HUD_AMMO_FIELD_OFFSET_Y 2
#define HUD_AMMO_BULLETS_MAX_ONCE 5
#define HUD_AMMO_BULLET_COUNT_Y 2
#define HUD_AMMO_BULLET_COUNT_X ((WEAPON_MAX_BULLETS_IN_MAGAZINE + HUD_AMMO_BULLET_COUNT_Y - 1) / HUD_AMMO_BULLET_COUNT_Y)
#define HUD_AMMO_BULLET_SIZE_X 2
#define HUD_AMMO_BULLET_SIZE_Y 6
#define HUD_AMMO_FIELD_SIZE_X (HUD_AMMO_BULLET_COUNT_X * (HUD_AMMO_BULLET_SIZE_X + 1) - 1)
#define HUD_AMMO_FIELD_SIZE_Y (HUD_AMMO_BULLET_COUNT_Y * (HUD_AMMO_BULLET_SIZE_Y + 1) - 1)
#define HUD_AMMO_COUNT_FORCE_REDRAW 0xFF
#define HUD_LEVEL_UP_OFFSET_X CEIL_TO_FACTOR(HUD_AMMO_FIELD_OFFSET_X + HUD_AMMO_FIELD_SIZE_X, 16)
#define HUD_LEVEL_UP_OFFSET_Y 0
#define HUD_LEVEL_UP_SIZE_X 80
#define HUD_LEVEL_UP_SIZE_Y 15
#define HUD_HEALTH_BAR_OFFSET_X 215
#define HUD_HEALTH_BAR_OFFSET_Y 11
#define HUD_HEALTH_BAR_SIZE_X PLAYER_HEALTH_MAX
#define HUD_HEALTH_BAR_SIZE_Y 3
#define HUD_SCORE_DIGITS 10
#define HUD_SCORE_TEXT_X 215
#define HUD_SCORE_TEXT_Y 1
#define HUD_SCORE_TEXT_SIZE_X (HUD_SCORE_DIGITS * (DIGIT_WIDTH_MAX + 1) - 1)
#define HUD_SCORE_TEXT_SIZE_Y 5
#define HUD_SCORE_BAR_OFFSET_X 215
#define HUD_SCORE_BAR_OFFSET_Y 7
#define HUD_SCORE_BAR_SIZE_X 100
#define HUD_SCORE_BAR_SIZE_Y 3
#define HUD_LEVEL_DIGITS 3
#define HUD_LEVEL_TEXT_Y HUD_SCORE_TEXT_Y
#define HUD_LEVEL_TEXT_SIZE_X (HUD_LEVEL_DIGITS * (DIGIT_WIDTH_MAX + 1) - 1)
#define HUD_LEVEL_TEXT_SIZE_Y HUD_SCORE_TEXT_SIZE_Y
#define HUD_LEVEL_TEXT_X (HUD_SCORE_BAR_OFFSET_X + HUD_SCORE_BAR_SIZE_X - HUD_LEVEL_TEXT_SIZE_X)

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
#define MAP_TILE_SIZE (1 << MAP_TILE_SHIFT)
#define BG_TILE_SHIFT 5
#define BG_TILE_SIZE (1 << BG_TILE_SHIFT)
#define BG_TILES_X (MAP_TILES_X * MAP_TILE_SIZE / BG_TILE_SIZE)
#define BG_TILES_Y (MAP_TILES_Y * MAP_TILE_SIZE / BG_TILE_SIZE)
#define BG_TILE_COUNT 8
#define SPRITE_CHANNEL_CURSOR 4

#define COLOR_HURT 23
#define COLOR_LEVEL 27
#define COLOR_HUD_BG 28
#define COLOR_BAR_BG 24
#define COLOR_HUD_HP 10
#define COLOR_HUD_SCORE 25

#define COLLISION_SIZE_X 8
#define COLLISION_SIZE_Y 8
#define COLLISION_LOOKUP_SIZE_X (MAP_TILES_X * MAP_TILE_SIZE / COLLISION_SIZE_X)
#define COLLISION_LOOKUP_SIZE_Y (MAP_TILES_Y * MAP_TILE_SIZE / COLLISION_SIZE_Y)
#define RESPAWN_SLOTS_PER_POSITION 4

#define HEALTH_ENEMY_DEAD_AWAITING_RESPAWN (-32768)
#define HEALTH_ENEMY_OFFSCREENED (-32767)
#define HEALTH_ENEMY_DEATH_ANIM (-32764)
#define HEALTH_PICKUP_INACTIVE (-32766)
#define HEALTH_PICKUP_READY_TO_SPAWN (-32765)

#define PLAYER_BOB_SIZE_X 32
#define PLAYER_BOB_SIZE_Y 32
// From the top-left of the collision rectangle
#define PLAYER_BOB_OFFSET_X 12
#define PLAYER_BOB_OFFSET_Y 19
#define PLAYER_ATTACK_COOLDOWN 12
#define PLAYER_BLINK_COOLDOWN 4
#define PLAYER_HEALTH_MAX 100
#define PLAYER_RETALIATION_DAMAGE 30

#define ENEMY_BOB_SIZE_X 16
#define ENEMY_BOB_SIZE_Y 24
// From the top-left of the collision rectangle
#define ENEMY_BOB_OFFSET_X 4
#define ENEMY_BOB_OFFSET_Y 15
#define ENEMY_ATTACK_COOLDOWN 15
#define ENEMY_HEALTH_BASE 5
#define ENEMY_DAMAGE_BASE 5
#define ENEMY_HEALTH_ADD_PER_LEVEL 5
#define ENEMY_EXP 25
#define ENEMY_SPEEDY_CHANCE_MAX 127
#define ENEMY_SPEEDY_CHANCE_ADD_PER_LEVEL 10
#define ENEMY_PREFERRED_SPAWN_NONE 0xFF

#define ENEMY_COUNT 25
#define PROJECTILE_COUNT 20
#define PROJECTILE_LIFETIME GAME_FPS
#define PROJECTILE_SPEED 5
#define SPREAD_SIDE_COUNT 40

#define PICKUP_BOB_SIZE_X 16
#define PICKUP_BOB_SIZE_Y 12
#define PICKUP_BOB_OFFSET_X (PICKUP_BOB_SIZE_X / 2)
#define PICKUP_BOB_OFFSET_Y (PICKUP_BOB_SIZE_Y / 2)
#define PICKUP_SPAWN_CHANCE_MAX 128
#define PICKUP_SPAWN_CHANCE ((10 * PICKUP_SPAWN_CHANCE_MAX) / 100)
#define PICKUP_LIFE_SECONDS 7

// + player + pickup
#define SORTED_ENTITIES_COUNT (ENEMY_COUNT + 1 + 1)

#define BG_BYTES_PER_BITPLANE_ROW (MAP_TILES_X * MAP_TILE_SIZE / 8)
#define BG_BYTES_PER_PIXEL_ROW (BG_BYTES_PER_BITPLANE_ROW * GAME_BPP)

#define CURSOR_SPRITE_SIZE_X 16
#define CURSOR_SPRITE_SIZE_Y (CURSOR_SIZE+2)

typedef UWORD tFix10p6;

typedef enum tBlinkKind {
	BLINK_KIND_HURT,
	BLINK_KIND_LEVEL,
	BLINK_KIND_COUNT
} tBlinkKind;

typedef enum tPickupKind {
	PICKUP_KIND_RIFLE,
	PICKUP_KIND_SMG,
	PICKUP_KIND_ASSAULT_RIFLE,
	PICKUP_KIND_SHOTGUN,
	PICKUP_KIND_SAWOFF,

	PICKUP_KIND_EXP_400,
	PICKUP_KIND_EXP_800,

	PICKUP_KIND_BOMB,

	PICKUP_KIND_COUNT,
	PICKUP_KIND_WEAPON_LAST = PICKUP_KIND_SAWOFF,
} tPickupKind;

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
	ENTITY_FRAME_WALK_1,
	ENTITY_FRAME_WALK_2,
	ENTITY_FRAME_WALK_3,
	ENTITY_FRAME_WALK_4,
	ENTITY_FRAME_WALK_5,
	ENTITY_FRAME_WALK_6,
	ENTITY_FRAME_WALK_7,
	ENTITY_FRAME_WALK_8,

	ENTITY_FRAME_DIE_1,
	ENTITY_FRAME_DIE_2,
	ENTITY_FRAME_DIE_3,
	ENTITY_FRAME_DIE_4,
	ENTITY_FRAME_DIE_5,
	ENTITY_FRAME_DIE_6,
	ENTITY_FRAME_DIE_7,
	ENTITY_FRAME_DIE_8,

	ENTITY_FRAME_SHOOT_1,
	ENTITY_FRAME_SHOOT_2,
	ENTITY_FRAME_SHOOT_3,
	ENTITY_FRAME_SHOOT_4,
	ENTITY_FRAME_SHOOT_5,
	ENTITY_FRAME_SHOOT_6,
	ENTITY_FRAME_SHOOT_7,
	ENTITY_FRAME_SHOOT_8,

	ENTITY_FRAME_COUNT,
	ENEMY_FRAME_COUNT = ENTITY_FRAME_DIE_4
} tCharacterFrame;

typedef enum tWeaponKind {
	WEAPON_KIND_STOCK_RIFLE,
	WEAPON_KIND_SMG,
	WEAPON_KIND_ASSAULT_RIFLE,
	WEAPON_KIND_SHOTGUN,
	WEAPON_KIND_SAWOFF,
} tWeaponKind;

typedef struct tFrameOffset {
	UBYTE *pPixels;
	UBYTE *pMask;
} tFrameOffset;

typedef enum tEntityKind {
	ENTITY_KIND_PLAYER,
	ENTITY_KIND_ENEMY,
	ENTITY_KIND_PICKUP,
} tEntityKind;

typedef struct tEntity {
	tEntityKind eKind;
	tUwCoordYX sPos; ///< Top-left coordinate of collision box.
	tBob sBob;
	tCharacterFrame eFrame;
	WORD wHealth;
	union {
		struct {
			tDirection eDirection;
			tWeaponKind eWeaponKind;
			UBYTE ubFrameCooldown;
			UBYTE ubAttackCooldown;
			UBYTE ubAmmo;
			UBYTE ubMaxAmmo;
			BYTE bReloadCooldown;
			UBYTE ubWeaponCooldown;
			UBYTE ubBlinkCooldown;
		} sPlayer;
		struct {
			tDirection eDirection;
			UBYTE ubFrameCooldown;
			UBYTE ubAttackCooldown;
			UBYTE ubSpeed;
			UBYTE ubPreferredSpawn;
		} sEnemy;
		struct {
			tPickupKind ePickupKind;
			WORD wBlinkCooldown;
			UBYTE isDisplayed;
		} sPickup;
	};
} tEntity;

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
	HUD_STATE_DRAW_LEVEL_UP,
	HUD_STATE_DRAW_HEALTH_BAR,
	HUD_STATE_DRAW_WEAPON,
	HUD_STATE_DRAW_BULLETS,
	HUD_STATE_PREPARE_EXP_POINTS,
	HUD_STATE_DRAW_EXP_POINTS,
	HUD_STATE_DRAW_EXP_BAR,
	HUD_STATE_PREPARE_LEVEL_NUM,
	HUD_STATE_DRAW_LEVEL_NUM,
} tHudState;

typedef enum tCursorKind {
	CURSOR_KIND_EMPTY,
	CURSOR_KIND_HALF_FULL,
	CURSOR_KIND_FULL,
	CURSOR_KIND_COUNT
} tCursorKind;

static tView *s_pView;
static tVPort *s_pVpMain;
static tVPort *s_pVpHud;
static UBYTE *s_pPristinePlanes;
static UBYTE *s_pBackPlanes;
static tSimpleBufferManager *s_pBufferHud;
static tBitMap *s_pTileset;
static ULONG *s_pCursorData;
static tBitMap *s_pBmCursor;
static tBitMap *s_pBmCursorFrames;
static ULONG *s_pCursorOffsets[CURSOR_KIND_COUNT];
static tBitMap *s_pHudWeapons;
static tBitMap *s_pHudLevelUp;
static tSprite *s_pSpriteCursor;
static volatile ULONG s_ulFrameCount;
static ULONG s_ulFrameWaitCount;

static tBitMap *s_pPlayerFrames[DIRECTION_COUNT];
static tBitMap *s_pPlayerMasks[DIRECTION_COUNT];
static tBitMap *s_pPlayerBlinkBitmaps[BLINK_KIND_COUNT];
static UBYTE *s_pPlayerBlinkData[BLINK_KIND_COUNT];
static tFrameOffset s_pPlayerFrameOffsets[DIRECTION_COUNT][ENTITY_FRAME_COUNT];
static tEntity s_sPlayer;
static ULONG s_ulScore;
static UBYTE s_ubPendingPerks;
static ULONG s_ulKills;
static ULONG s_ulPrevLevelScore;
static ULONG s_ulNextLevelScore;
static UBYTE s_ubScoreLevel;
static UBYTE s_ubHiSpeedChance;
static UWORD s_uwEnemySpawnHealth;
static UBYTE s_ubEnemyDamage;

// Perks
static UBYTE s_isDeathClock;
static UBYTE s_ubDeathClockCooldown;
static UBYTE s_isRetaliation;
static UBYTE s_isAmmoManiac;
static UBYTE s_isFavouriteWeapon;
static UBYTE s_isAnxiousLoader;
static UBYTE s_isBloodyAmmo;
static UBYTE s_isDeathDance;
static UBYTE s_ubDodgeChance;
static UBYTE s_isFinalRevenge;
static UBYTE s_isStationaryReloader;
static UBYTE s_isToughReloader;
static UBYTE s_isSwiftLearner;
static UBYTE s_isFastShot;
static UBYTE s_isImmortal;
static UBYTE s_isBonusLearner;

static tBitMap *s_pEnemyFrames[DIRECTION_COUNT];
static tBitMap *s_pEnemyMasks[DIRECTION_COUNT];
static tFrameOffset s_pEnemyFrameOffsets[DIRECTION_COUNT][ENEMY_FRAME_COUNT];
static tEntity s_pEnemies[ENEMY_COUNT];

static tEntity s_sPickup;
static tBitMap *s_pPickupFrames;
static tBitMap *s_pPickupMasks;
static tFrameOffset s_pPickupFrameOffsets[PICKUP_KIND_COUNT];

static tEntity *s_pSortedEntities[SORTED_ENTITIES_COUNT];

static tBitMap *s_pBulletFrames;
static tBitMap *s_pBulletMasks;

static tBitMap *s_pExplosionFrames;
static tBitMap *s_pExplosionMasks;
static tFrameOffset s_pExplosionFrameOffsets[EXPLOSION_FRAME_COUNT];
static UBYTE s_ubExplosionFrame;
static UBYTE s_ubExplosionCooldown;

static tBitMap *s_pStainFrames;
static tBitMap *s_pStainMasks;
static tFrameOffset s_pStainFrameOffsets[STAIN_FRAME_PRESET_COUNT];
static tFrameOffset *s_pNextStainOffset;

static UBYTE s_ubBufferCurr;
static tProjectile *s_pCurrentProjectile;
static tFix10p6 s_pSin10p6[GAME_MATH_ANGLE_COUNT];
static UBYTE s_ubDeathCooldown;

static const UBYTE s_pBulletMaskFromX[] = {
	BV(7), BV(6), BV(5), BV(4), BV(3), BV(2), BV(1), BV(0)
};
static const UBYTE s_pWeaponAmmo[] = {
	[WEAPON_KIND_STOCK_RIFLE] = 10,
	[WEAPON_KIND_SMG] = 30,
	[WEAPON_KIND_ASSAULT_RIFLE] = 25,
	[WEAPON_KIND_SHOTGUN] = 12,
	[WEAPON_KIND_SAWOFF] = 12,
};
static const UBYTE s_pWeaponReloadCooldowns[] = {
	[WEAPON_KIND_STOCK_RIFLE] = 30,
	[WEAPON_KIND_SMG] = 40,
	[WEAPON_KIND_ASSAULT_RIFLE] = 40,
	[WEAPON_KIND_SHOTGUN] = 80,
	[WEAPON_KIND_SAWOFF] = 80,
};
static const UBYTE s_pWeaponDamages[] = {
	[WEAPON_KIND_STOCK_RIFLE] = 6,
	[WEAPON_KIND_SMG] = 5,
	[WEAPON_KIND_ASSAULT_RIFLE] = 7,
	[WEAPON_KIND_SHOTGUN] = 7,
	[WEAPON_KIND_SAWOFF] = 9,
};
static const UBYTE s_pWeaponFireCooldowns[] = {
	[WEAPON_KIND_STOCK_RIFLE] =  15,
	[WEAPON_KIND_SMG] =  3,
	[WEAPON_KIND_ASSAULT_RIFLE] =  4,
	[WEAPON_KIND_SHOTGUN] =  18,
	[WEAPON_KIND_SAWOFF] =  18,
};

typedef struct tHudBulletDef {
	tUbCoordYX sOffs;
	UBYTE ubMaxDrawDelta;
} tHudBulletDef;

static ULONG s_pRowOffsetFromY[MAP_TILES_Y * MAP_TILE_SIZE];

static tEntity *s_pCollisionTiles[COLLISION_LOOKUP_SIZE_X][COLLISION_LOOKUP_SIZE_Y];
static tUwCoordYX s_pRespawnSlots[COLLISION_LOOKUP_SIZE_X][COLLISION_LOOKUP_SIZE_Y][RESPAWN_SLOTS_PER_POSITION]; // left, right, up, down

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
static ULONG s_ulHudScore;
static ULONG s_ubHudBarPixel;
static UBYTE s_ubHudLevel;
static UBYTE s_ubHudPendingPerksDrawn;
static tHudBulletDef s_pHudBulletDefs[WEAPON_MAX_BULLETS_IN_MAGAZINE];

static tBob s_sExplosionBob;
static tBob s_pStainBobs[STAINS_MAX];
static tBob *s_pFreeStains[STAINS_MAX];
static tBob *s_pPushStains[STAINS_MAX];
static tBob *s_pWaitStains[STAINS_MAX];
static tBob **s_pNextFreeStain;
static tBob **s_pNextPushStain;
static tBob **s_pNextWaitStain;

static const tBCoordYX s_pPlayerFrameDeathOffset[DIRECTION_COUNT][8] = {
	[DIRECTION_S] = {
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 1 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 2 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 6 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 6 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 9 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 9 },
	},
	[DIRECTION_SW] = {
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X - 2, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X - 5, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X - 7, .bY = -PLAYER_BOB_OFFSET_Y + 1 },
		{.bX = -PLAYER_BOB_OFFSET_X - 7, .bY = -PLAYER_BOB_OFFSET_Y + 1 },
	},
	[DIRECTION_NW] = {
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X - 4, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X - 4, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
	},
	[DIRECTION_N] = {
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
	},
	[DIRECTION_NE] = {
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 2, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 4, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 6, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 7, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 8, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 8, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
	},
	[DIRECTION_SE] = {
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 0 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 5 },
		{.bX = -PLAYER_BOB_OFFSET_X + 0, .bY = -PLAYER_BOB_OFFSET_Y + 4 },
		{.bX = -PLAYER_BOB_OFFSET_X + 1, .bY = -PLAYER_BOB_OFFSET_Y + 5 },
		{.bX = -PLAYER_BOB_OFFSET_X + 1, .bY = -PLAYER_BOB_OFFSET_Y + 5 },
	},
};

tSimpleBufferManager *g_pGameBufferMain;
tBitMap *g_pGamePristineBuffer;
tRandManager g_sRand;

//------------------------------------------------------------------ PRIVATE FNS

static inline tFix10p6 fix10p6Add(tFix10p6 a, tFix10p6 b) {return a + b; }
static inline tFix10p6 fix10p6FromUword(UWORD x) {return x << 6; }
static inline tFix10p6 fix10p6ToUword(UWORD x) {return x >> 6; }
#define fix10p6Sin(x) s_pSin10p6[x]
#define fix10p6Cos(x) (((x) < 3 * ANGLE_90) ? fix10p6Sin(ANGLE_90 + (x)) : fix10p6Sin((x) - (3 * ANGLE_90)))

__attribute__((always_inline))
static inline void gameSetCursor(tCursorKind eKind) {
	ULONG *pSrc = s_pCursorOffsets[eKind];
	ULONG *pDest = s_pCursorData;
	for(UBYTE i = 0; i < CURSOR_SIZE; ++i) {
		*(pDest++) = *(pSrc++);
	}
}

__attribute__((always_inline))
static inline void playerSetBlink(tBlinkKind eBlinkKind) {
	s_sPlayer.sBob.pFrameData = s_pPlayerBlinkData[eBlinkKind];
	s_sPlayer.sPlayer.ubBlinkCooldown = PLAYER_BLINK_COOLDOWN;
}

__attribute__((always_inline))
static inline void scoreLevelUp(void) {
	s_ulPrevLevelScore = s_ulNextLevelScore;
	s_ulNextLevelScore *= 2;
	if(s_isSwiftLearner) {
		s_ulNextLevelScore -= s_ulPrevLevelScore / 5;
	}
	++s_ubScoreLevel;
	++s_ubPendingPerks;
	s_ubHiSpeedChance = MIN(s_ubHiSpeedChance + ENEMY_SPEEDY_CHANCE_ADD_PER_LEVEL, ENEMY_SPEEDY_CHANCE_MAX);
	s_uwEnemySpawnHealth += ENEMY_HEALTH_ADD_PER_LEVEL;
	playerSetBlink(BLINK_KIND_LEVEL);
}

__attribute__((always_inline))
static inline void scoreAddSmall(ULONG ulScore) {
	s_ulScore += ulScore;
	if(s_ulScore >= s_ulNextLevelScore) {
		scoreLevelUp();
	}
}

// Can add multiple levels at once
static void scoreAddLarge(ULONG ulScore) {
	s_ulScore += ulScore;
	while(s_ulScore >= s_ulNextLevelScore) {
		scoreLevelUp();
	}
}

static void gameSetTile(UBYTE ubTileIndex, UWORD uwBgTileX, UWORD uwBgTileY) {
	blitCopyAligned(
		s_pTileset, 0, ubTileIndex * BG_TILE_SIZE,
		g_pGameBufferMain->pBack, uwBgTileX * BG_TILE_SIZE, uwBgTileY * BG_TILE_SIZE,
		BG_TILE_SIZE, BG_TILE_SIZE
	);
	blitCopyAligned(
		s_pTileset, 0, ubTileIndex * BG_TILE_SIZE,
		g_pGameBufferMain->pFront, uwBgTileX * BG_TILE_SIZE, uwBgTileY * BG_TILE_SIZE,
		BG_TILE_SIZE, BG_TILE_SIZE
	);
	blitCopyAligned(
		s_pTileset, 0, ubTileIndex * BG_TILE_SIZE,
		g_pGamePristineBuffer, uwBgTileX * BG_TILE_SIZE, uwBgTileY * BG_TILE_SIZE,
		BG_TILE_SIZE, BG_TILE_SIZE
	);
}

__attribute__((always_inline))
static inline UBYTE isPositionCollidingWithEntity(
	tUwCoordYX sPos, const tEntity *pEntity
) {
	WORD Dx = sPos.uwX - pEntity->sPos.uwX;
	WORD Dy = sPos.uwY - pEntity->sPos.uwY;
	return (
		-COLLISION_SIZE_X <= Dx && Dx <= COLLISION_SIZE_X &&
		-COLLISION_SIZE_Y <= Dy && Dy <= COLLISION_SIZE_Y
	);
}

__attribute__((always_inline))
static inline tEntity *entityGetNearPos(
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
static inline UBYTE enemyTryMoveBy(tEntity *pEnemy, LONG lDeltaX, LONG lDeltaY) {
	tUwCoordYX sGoodPos = pEnemy->sPos;
	UBYTE isMoved = 0;

	if (lDeltaX) {
		tUwCoordYX sNewPos = sGoodPos;
		sNewPos.uwX += lDeltaX;
		UWORD uwTestX = sNewPos.uwX;
		if(lDeltaX < 0) {
			uwTestX -= ENEMY_BOB_OFFSET_X;
		}
		UBYTE isColliding = 0;

		// collision with upper corner
		tEntity *pUp = entityGetNearPos(sGoodPos.uwX, SGN(lDeltaX), sGoodPos.uwY, 0);
		if(pUp && pUp != pEnemy) {
			isColliding = isPositionCollidingWithEntity(sNewPos, pUp);
		}

		// collision with lower corner
		if (!isColliding && (sGoodPos.uwY & (COLLISION_SIZE_Y - 1))) {
			tEntity *pDown = entityGetNearPos(sGoodPos.uwX, SGN(lDeltaX), sGoodPos.uwY, +1);
			if(pDown && pDown != pEnemy) {
				isColliding = isPositionCollidingWithEntity(sNewPos, pDown);
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
			uwTestY -= ENEMY_BOB_OFFSET_Y;
		}
		UBYTE isColliding = 0;

		// collision with left corner
		tEntity *pLeft = entityGetNearPos(sGoodPos.uwX, 0, sGoodPos.uwY, SGN(lDeltaY));
		if(pLeft && pLeft != pEnemy) {
			isColliding = isPositionCollidingWithEntity(sNewPos, pLeft);
		}

		// collision with right corner
		if (!isColliding && (sGoodPos.uwX & (COLLISION_SIZE_X - 1))) {
			tEntity *pRight = entityGetNearPos(sGoodPos.uwX, +1, sGoodPos.uwY, SGN(lDeltaY));
			if(pRight && pRight != pEnemy) {
				isColliding = isPositionCollidingWithEntity(sNewPos, pRight);
			}
		}

		if(!isColliding) {
			isMoved = 1;
			sGoodPos = sNewPos;
		}
	}

	if(isMoved) {
		ULONG ubOldLookupX = pEnemy->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubOldLookupY = pEnemy->sPos.uwY / COLLISION_SIZE_Y;
		pEnemy->sPos = sGoodPos;
		// Update lookup
		if(
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY] &&
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY] != pEnemy
		) {
			logWrite(
				"ERR: Erasing other entity %p\n",
				s_pCollisionTiles[ubOldLookupX][ubOldLookupY]
			);
		}
		s_pCollisionTiles[ubOldLookupX][ubOldLookupY] = 0;

		ULONG ubNewLookupX = pEnemy->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubNewLookupY = pEnemy->sPos.uwY / COLLISION_SIZE_Y;
		if(s_pCollisionTiles[ubNewLookupX][ubNewLookupY]) {
			logWrite(
				"ERR: Overwriting other entity %p in lookup with %p\n",
				s_pCollisionTiles[ubNewLookupX][ubNewLookupY], pEnemy
			);
		}
		s_pCollisionTiles[ubNewLookupX][ubNewLookupY] = pEnemy;
	}

	return isMoved;
}

__attribute__((always_inline))
static inline UBYTE playerTryMoveBy(tEntity *pPlayer, LONG lDeltaX, LONG lDeltaY) {
	tUwCoordYX sGoodPos = pPlayer->sPos;
	UBYTE isMoved = 0;

	if (lDeltaX) {
		tUwCoordYX sNewPos = sGoodPos;
		sNewPos.uwX += lDeltaX;
		UWORD uwTestX = sNewPos.uwX;
		if(lDeltaX < 0) {
			uwTestX -= MAP_MARGIN_TILES * MAP_TILE_SIZE;
		}
		UBYTE isColliding = (
			uwTestX >= (MAP_TILES_X - MAP_MARGIN_TILES) * MAP_TILE_SIZE - PLAYER_BOB_OFFSET_X
		);

		// collision with upper corner
		if(!isColliding) {
			tEntity *pUp = entityGetNearPos(sGoodPos.uwX, SGN(lDeltaX), sGoodPos.uwY, 0);
			if(pUp && pUp->eKind == ENTITY_KIND_ENEMY) {
				isColliding = isPositionCollidingWithEntity(sNewPos, pUp);
			}
		}

		// collision with lower corner
		if (!isColliding && (sGoodPos.uwY & (COLLISION_SIZE_Y - 1))) {
			tEntity *pDown = entityGetNearPos(sGoodPos.uwX, SGN(lDeltaX), sGoodPos.uwY, +1);
			if(pDown && pDown->eKind == ENTITY_KIND_ENEMY) {
				isColliding = isPositionCollidingWithEntity(sNewPos, pDown);
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
			uwTestY -= MAP_MARGIN_TILES * MAP_TILE_SIZE + PLAYER_BOB_SIZE_Y - PLAYER_BOB_OFFSET_Y;
		}
		UBYTE isColliding = (
			uwTestY >= (MAP_TILES_Y - MAP_MARGIN_TILES) * MAP_TILE_SIZE - 6
		);

		// collision with left corner
		if(!isColliding) {
			tEntity *pLeft = entityGetNearPos(sGoodPos.uwX, 0, sGoodPos.uwY, SGN(lDeltaY));
			if(pLeft && pLeft->eKind == ENTITY_KIND_ENEMY) {
				isColliding = isPositionCollidingWithEntity(sNewPos, pLeft);
			}
		}

		// collision with right corner
		if (!isColliding && (sGoodPos.uwX & (COLLISION_SIZE_X - 1))) {
			tEntity *pRight = entityGetNearPos(sGoodPos.uwX, +1, sGoodPos.uwY, SGN(lDeltaY));
			if(pRight && pRight->eKind == ENTITY_KIND_ENEMY) {
				isColliding = isPositionCollidingWithEntity(sNewPos, pRight);
			}
		}

		if(!isColliding) {
			isMoved = 1;
			sGoodPos = sNewPos;
		}
	}

	if(isMoved) {
		ULONG ubOldLookupX = pPlayer->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubOldLookupY = pPlayer->sPos.uwY / COLLISION_SIZE_Y;
		pPlayer->sPos = sGoodPos;
		// Update lookup
		if(
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY] &&
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY] != pPlayer &&
			s_pCollisionTiles[ubOldLookupX][ubOldLookupY]->eKind != ENTITY_KIND_PICKUP
		) {
			logWrite(
				"ERR: Erasing other entity %p\n",
				s_pCollisionTiles[ubOldLookupX][ubOldLookupY]
			);
		}
		s_pCollisionTiles[ubOldLookupX][ubOldLookupY] = 0;

		ULONG ubNewLookupX = pPlayer->sPos.uwX / COLLISION_SIZE_X;
		ULONG ubNewLookupY = pPlayer->sPos.uwY / COLLISION_SIZE_Y;
		if(
			s_pCollisionTiles[ubNewLookupX][ubNewLookupY] &&
			s_pCollisionTiles[ubNewLookupX][ubNewLookupY]->eKind != ENTITY_KIND_PICKUP
		) {
			logWrite(
				"ERR: Overwriting other entity %p in lookup with %p\n",
				s_pCollisionTiles[ubNewLookupX][ubNewLookupY], pPlayer
			);
		}
		s_pCollisionTiles[ubNewLookupX][ubNewLookupY] = pPlayer;
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
		tEntity *pEnemy;
		if(uwProjectileX >= MAP_TILES_X * MAP_TILE_SIZE || uwProjectileY >= MAP_TILES_Y * MAP_TILE_SIZE) {
			// TODO: Remove in favor of dummy entries in collision tiles at the edges
			s_pCurrentProjectile->ubLife = 1; // so that it will be undrawn on both buffers
		}
		else if(
			((pEnemy = entityGetNearPos(uwProjectileX, 0, uwProjectileY, 0)) && pEnemy->eKind == ENTITY_KIND_ENEMY) ||
			((pEnemy = entityGetNearPos(uwProjectileX, -1, uwProjectileY, 0)) && pEnemy->eKind == ENTITY_KIND_ENEMY) ||
			((pEnemy = entityGetNearPos(uwProjectileX, 0, uwProjectileY, -1)) && pEnemy->eKind == ENTITY_KIND_ENEMY) ||
			((pEnemy = entityGetNearPos(uwProjectileX, -1, uwProjectileY, -1)) && pEnemy->eKind == ENTITY_KIND_ENEMY)
		) {
			if(s_pNextFreeStain != &s_pFreeStains[0]) {
				tBob *pStain = *(--s_pNextFreeStain);
				pStain->sPos.uwX = uwProjectileX;
				pStain->sPos.uwY = uwProjectileY;
				*(s_pNextPushStain++) = pStain;
			}

			s_pCurrentProjectile->ubLife = 1; // so that it will be undrawn on both buffers
			pEnemy->wHealth -= s_pCurrentProjectile->ubDamage;
			audioMixerPlaySfx(g_pSfxImpact, SFX_CHANNEL_IMPACT, SFX_PRIORITY_IMPACT, 0);
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
	LONG lTop = ulCenterX - GAME_MAIN_VPORT_SIZE_X / 2;
	LONG lLeft = ulCenterY - (GAME_MAIN_VPORT_SIZE_Y - GAME_HUD_VPORT_SIZE_Y) / 2;
	pManager->uPos.uwX = CLAMP(lTop, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_X - MAP_MARGIN_TILES) * MAP_TILE_SIZE - GAME_MAIN_VPORT_SIZE_X);
	pManager->uPos.uwY = CLAMP(lLeft, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_Y - MAP_MARGIN_TILES) * MAP_TILE_SIZE - GAME_MAIN_VPORT_SIZE_Y);
}

static void hudProcess(void) {
	switch(s_eHudState) {
		case HUD_STATE_DRAW_LEVEL_UP:
			++s_eHudState;
			if(s_ubHudPendingPerksDrawn) {
				if(!s_ubPendingPerks) {
					s_ubHudPendingPerksDrawn = 0;
					blitRect(
						s_pBufferHud->pBack, HUD_LEVEL_UP_OFFSET_X, HUD_LEVEL_UP_OFFSET_Y,
						HUD_LEVEL_UP_SIZE_X, HUD_LEVEL_UP_SIZE_Y, COLOR_HUD_BG
					);
					break;
				}
			}
			else {
				if(s_ubPendingPerks) {
					s_ubHudPendingPerksDrawn = 1;
					blitCopyAligned(
						s_pHudLevelUp, 0, 0, s_pBufferHud->pBack,
						HUD_LEVEL_UP_OFFSET_X, HUD_LEVEL_UP_OFFSET_Y,
						HUD_LEVEL_UP_SIZE_X, HUD_LEVEL_UP_SIZE_Y
					);
					break;
				}
			}
			// fallthrough
		case HUD_STATE_DRAW_HEALTH_BAR:
			++s_eHudState;
			UWORD uwCurrentHealth = s_sPlayer.wHealth;
			if(s_uwHudHealth != uwCurrentHealth) {
				if(uwCurrentHealth > s_uwHudHealth) {
					blitRect(s_pBufferHud->pBack, HUD_HEALTH_BAR_OFFSET_X + s_uwHudHealth, HUD_HEALTH_BAR_OFFSET_Y, uwCurrentHealth - s_uwHudHealth, HUD_HEALTH_BAR_SIZE_Y, COLOR_HUD_HP);
				}
				else {
					blitRect(s_pBufferHud->pBack, HUD_HEALTH_BAR_OFFSET_X + uwCurrentHealth, HUD_HEALTH_BAR_OFFSET_Y, s_uwHudHealth - uwCurrentHealth, HUD_HEALTH_BAR_SIZE_Y, COLOR_BAR_BG);
				}
				s_uwHudHealth = uwCurrentHealth;
				break;
			}
			// fallthrough
		case HUD_STATE_DRAW_WEAPON:
			++s_eHudState;
			blitCopyAligned(
				s_pHudWeapons, 0, s_sPlayer.sPlayer.eWeaponKind * HUD_WEAPON_SIZE_Y,
				s_pBufferHud->pBack, 0, 0, HUD_WEAPON_SIZE_X, HUD_WEAPON_SIZE_Y
			);
			// fallthrough
		case HUD_STATE_DRAW_BULLETS:
			++s_eHudState;
			if(s_ubHudAmmoCount != s_sPlayer.sPlayer.ubAmmo) {
				if(s_ubHudAmmoCount == HUD_AMMO_COUNT_FORCE_REDRAW) {
					s_ubHudAmmoCount = 0;
					blitRect(
						s_pBufferHud->pBack, HUD_AMMO_FIELD_OFFSET_X, HUD_AMMO_FIELD_OFFSET_Y,
						HUD_AMMO_FIELD_SIZE_X, HUD_AMMO_FIELD_SIZE_Y, COLOR_HUD_BG
					);
				}
				else if(s_ubHudAmmoCount < s_sPlayer.sPlayer.ubAmmo) {
					UBYTE ubDelta = s_sPlayer.sPlayer.ubAmmo - s_ubHudAmmoCount;
					UBYTE ubDrawBulletCount = MIN(s_pHudBulletDefs[s_ubHudAmmoCount].ubMaxDrawDelta, ubDelta);
					UWORD uwSrcY;
					switch(s_sPlayer.sPlayer.eWeaponKind) {
						case WEAPON_KIND_SAWOFF:
						case WEAPON_KIND_SHOTGUN:
							uwSrcY = BULLET_OFFSET_Y_SHELL;
							break;
						default:
							uwSrcY = BULLET_OFFSET_Y_BULLET;
					}
					blitCopyMask(
						s_pBulletFrames, 0, uwSrcY, s_pBufferHud->pBack,
						s_pHudBulletDefs[s_ubHudAmmoCount].sOffs.ubX,
						s_pHudBulletDefs[s_ubHudAmmoCount].sOffs.ubY,
						ubDrawBulletCount * (HUD_AMMO_BULLET_SIZE_X + 1), HUD_AMMO_BULLET_SIZE_Y, s_pBulletMasks->Planes[0]
					);
					s_ubHudAmmoCount += ubDrawBulletCount;
				}
				else {
					--s_ubHudAmmoCount;
					blitRect(
						s_pBufferHud->pBack,
						s_pHudBulletDefs[s_ubHudAmmoCount].sOffs.ubX,
						s_pHudBulletDefs[s_ubHudAmmoCount].sOffs.ubY,
						HUD_AMMO_BULLET_SIZE_X, HUD_AMMO_BULLET_SIZE_Y, COLOR_HUD_BG
					);
				}
				break;
			}
			// fallthrough
		case HUD_STATE_PREPARE_EXP_POINTS:
			if(s_ulHudScore != s_ulScore) {
				s_ulHudScore = s_ulScore;
				++s_eHudState;
				char szScore[sizeof("4294967295")];
				stringDecimalFromULong(s_ulScore, szScore);
				fontFillTextBitMap(g_pFontSmall, g_pLineBuffer, szScore);
				break;
			}
			else {
				s_eHudState = 0; // skip to beginning
				break;
			}
			// fallthrough
		case HUD_STATE_DRAW_EXP_POINTS:
			++s_eHudState;
			blitRect(
				s_pBufferHud->pBack,
				HUD_SCORE_TEXT_X, HUD_SCORE_TEXT_Y,
				HUD_SCORE_TEXT_SIZE_X, HUD_SCORE_TEXT_SIZE_Y, COLOR_HUD_BG
			);
			fontDrawTextBitMap(
				s_pBufferHud->pBack, g_pLineBuffer, HUD_SCORE_TEXT_X, HUD_SCORE_TEXT_Y,
				COLOR_HUD_SCORE, FONT_COOKIE
			);
			break;
		case HUD_STATE_DRAW_EXP_BAR:
			++s_eHudState;
			UBYTE ubNewHudBarPixel = (s_ulScore - s_ulPrevLevelScore) * HUD_SCORE_BAR_SIZE_X / (s_ulNextLevelScore - s_ulPrevLevelScore);
			if(ubNewHudBarPixel > s_ubHudBarPixel) {
				blitRect(
					s_pBufferHud->pBack,
					HUD_SCORE_BAR_OFFSET_X + s_ubHudBarPixel, HUD_SCORE_BAR_OFFSET_Y,
					ubNewHudBarPixel - s_ubHudBarPixel, HUD_SCORE_BAR_SIZE_Y, COLOR_HUD_SCORE
				);
			}
			else if(ubNewHudBarPixel < s_ubHudBarPixel) {
				blitRect(
					s_pBufferHud->pBack,
					HUD_SCORE_BAR_OFFSET_X + ubNewHudBarPixel, HUD_SCORE_BAR_OFFSET_Y,
					s_ubHudBarPixel - ubNewHudBarPixel, HUD_SCORE_BAR_SIZE_Y, COLOR_BAR_BG
				);
			}
			s_ubHudBarPixel = ubNewHudBarPixel;
			break;
		case HUD_STATE_PREPARE_LEVEL_NUM:
			if(s_ubHudLevel != s_ubScoreLevel) {
				++s_eHudState;
				s_ubHudLevel = s_ubScoreLevel;
				char szScore[sizeof("255")];
				stringDecimalFromULong(s_ubScoreLevel, szScore);
				fontFillTextBitMap(g_pFontSmall, g_pLineBuffer, szScore);
			}
			else {
				s_eHudState = 0; // skip to beginning
			}
			break;
		case HUD_STATE_DRAW_LEVEL_NUM: {
			s_eHudState = 0; // HUD_STATE_END
			blitRect(
				s_pBufferHud->pBack,
				HUD_LEVEL_TEXT_X, HUD_LEVEL_TEXT_Y,
				HUD_LEVEL_TEXT_SIZE_X, HUD_LEVEL_TEXT_SIZE_Y, COLOR_HUD_BG
			);
			fontDrawTextBitMap(
				s_pBufferHud->pBack, g_pLineBuffer, HUD_LEVEL_TEXT_X, HUD_LEVEL_TEXT_Y,
				COLOR_HUD_SCORE, FONT_COOKIE
			);
		}
	}
}

static void hudReset(void) {
	s_uwHudHealth = 0;
	s_ubHudAmmoCount = HUD_AMMO_COUNT_FORCE_REDRAW;
	s_ulHudScore = 1;
	s_ubHudLevel = 255;
	s_ubHudBarPixel = 0;
	s_eHudState = 0;
	s_ubHudPendingPerksDrawn = !s_ubPendingPerks;

	blitRect(
		s_pBufferHud->pBack,
		HUD_SCORE_BAR_OFFSET_X, HUD_SCORE_BAR_OFFSET_Y,
		HUD_SCORE_BAR_SIZE_X, HUD_SCORE_BAR_SIZE_Y, COLOR_BAR_BG
	);
	blitRect(
		s_pBufferHud->pBack,
		HUD_HEALTH_BAR_OFFSET_X, HUD_HEALTH_BAR_OFFSET_Y,
		HUD_HEALTH_BAR_SIZE_X, HUD_HEALTH_BAR_SIZE_Y, COLOR_BAR_BG
	);
	do {
		hudProcess();
	} while(s_eHudState != 0);
}

static void playerCalculateMaxAmmo(void) {
	UBYTE ubMaxAmmo = s_pWeaponAmmo[s_sPlayer.sPlayer.eWeaponKind];
	if(s_isFavouriteWeapon) {
		ubMaxAmmo += 2;
	}
	if(s_isAmmoManiac) {
		ubMaxAmmo += (2 * ubMaxAmmo + 5) / 10;
	}

	s_sPlayer.sPlayer.ubMaxAmmo = ubMaxAmmo;
}

static void playerSetWeapon(tWeaponKind eWeaponKind) {
	s_sPlayer.sPlayer.eWeaponKind = eWeaponKind;
	playerCalculateMaxAmmo();
	s_sPlayer.sPlayer.ubAmmo = s_sPlayer.sPlayer.ubMaxAmmo;
	s_sPlayer.sPlayer.bReloadCooldown = 0;
	s_sPlayer.sPlayer.ubWeaponCooldown = s_pWeaponFireCooldowns[eWeaponKind];
	if(s_isFastShot) {
		s_sPlayer.sPlayer.ubWeaponCooldown -= 2;
	}

	s_ubHudAmmoCount = HUD_AMMO_COUNT_FORCE_REDRAW;
	gameSetCursor(CURSOR_KIND_FULL);
	audioMixerPlaySfx(g_pSfxReload, SFX_CHANNEL_RELOAD, SFX_PRIORITY_RELOAD, 0);
}

__attribute__((always_inline))
static inline void playerStartReloadWeapon(void) {
	s_sPlayer.sPlayer.bReloadCooldown = s_pWeaponReloadCooldowns[s_sPlayer.sPlayer.eWeaponKind];
	// s_sPlayer.sPlayer.bReloadCooldown = 1;
	gameSetCursor(CURSOR_KIND_EMPTY);
	audioMixerPlaySfx(g_pSfxReload, SFX_CHANNEL_RELOAD, SFX_PRIORITY_RELOAD, 0);
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
	tWeaponKind eWeaponKind = s_sPlayer.sPlayer.eWeaponKind;
	s_sPlayer.sPlayer.ubAttackCooldown = s_sPlayer.sPlayer.ubWeaponCooldown;
	switch(eWeaponKind) {
		case WEAPON_KIND_STOCK_RIFLE:
			playerShootProjectile(ubAimAngle, s_pSpreadSide1, s_pWeaponDamages[WEAPON_KIND_STOCK_RIFLE]);
			audioMixerPlaySfx(g_pSfxRifle, SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			break;
		case WEAPON_KIND_SMG:
			playerShootProjectile(ubAimAngle, s_pSpreadSide3, s_pWeaponDamages[WEAPON_KIND_SMG]);
			audioMixerPlaySfx(g_pSfxSmg, SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			break;
		case WEAPON_KIND_ASSAULT_RIFLE:
			playerShootProjectile(ubAimAngle, s_pSpreadSide2, s_pWeaponDamages[WEAPON_KIND_ASSAULT_RIFLE]);
			audioMixerPlaySfx(g_pSfxAssault, SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			break;
		case WEAPON_KIND_SHOTGUN:
			for(UBYTE i = 0; i < 10; ++i) {
				playerShootProjectile(ubAimAngle, s_pSpreadSide3, s_pWeaponDamages[WEAPON_KIND_SHOTGUN]);
			}
			audioMixerPlaySfx(g_pSfxShotgun, SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			break;
		case WEAPON_KIND_SAWOFF:
			for(UBYTE i = 0; i < 10; ++i) {
				playerShootProjectile(ubAimAngle, s_pSpreadSide10, s_pWeaponDamages[WEAPON_KIND_SAWOFF]);
			}
			audioMixerPlaySfx(g_pSfxShotgun, SFX_CHANNEL_SHOOT, SFX_PRIORITY_SHOOT, 0);
			break;
	}
}

__attribute__((always_inline))
static inline void enemyProcess(tEntity *pEnemy) {
	BYTE bDeltaX, bDeltaY;
	tDirection eDir;
	if(pEnemy->wHealth > 0) {
		// Despawn if enemy is too far
		if(pEnemy->sPos.uwX < g_pGameBufferMain->pCamera->uPos.uwX - 32) {
			pEnemy->wHealth = HEALTH_ENEMY_OFFSCREENED;
			pEnemy->sEnemy.ubPreferredSpawn = 1;
			return;
		}
		else if(g_pGameBufferMain->pCamera->uPos.uwX + GAME_MAIN_VPORT_SIZE_X + 32 < pEnemy->sPos.uwX) {
			pEnemy->wHealth = HEALTH_ENEMY_OFFSCREENED;
			pEnemy->sEnemy.ubPreferredSpawn = 0;
			return;
		}
		else if(pEnemy->sPos.uwY < g_pGameBufferMain->pCamera->uPos.uwY - 32) {
			pEnemy->wHealth = HEALTH_ENEMY_OFFSCREENED;
			pEnemy->sEnemy.ubPreferredSpawn = 3;
			return;
		}
		else if(g_pGameBufferMain->pCamera->uPos.uwY + GAME_MAIN_VPORT_SIZE_Y + 32 < pEnemy->sPos.uwY) {
			pEnemy->sEnemy.ubPreferredSpawn = 2;
			pEnemy->wHealth = HEALTH_ENEMY_OFFSCREENED;
			return;
		}
		WORD wDistanceToPlayerX = s_sPlayer.sPos.uwX - pEnemy->sPos.uwX;
		WORD wDistanceToPlayerY = s_sPlayer.sPos.uwY - pEnemy->sPos.uwY;
		if(wDistanceToPlayerX < 0) {
			bDeltaX = -pEnemy->sEnemy.ubSpeed;
			if(wDistanceToPlayerY < 0) {
				bDeltaY = -pEnemy->sEnemy.ubSpeed;
				eDir = DIRECTION_NW;
				wDistanceToPlayerY = -wDistanceToPlayerY;
			}
			else {
				bDeltaY = pEnemy->sEnemy.ubSpeed;
				eDir = DIRECTION_SW;
			}
			wDistanceToPlayerX = -wDistanceToPlayerX;
		}
		else {
			bDeltaX = pEnemy->sEnemy.ubSpeed;
			if(wDistanceToPlayerY < 0) {
				bDeltaY = -pEnemy->sEnemy.ubSpeed;
				eDir = DIRECTION_NE;
				wDistanceToPlayerX = -wDistanceToPlayerX;
			}
			else {
				bDeltaY = pEnemy->sEnemy.ubSpeed;
				eDir = DIRECTION_SE;
			}
		}

		if(pEnemy->sEnemy.ubAttackCooldown == 0) {
			if((UWORD)wDistanceToPlayerX < 10 && (UWORD)wDistanceToPlayerY < 10) {
				if(randUwMax(&g_sRand, 99) >= s_ubDodgeChance) {
					playerSetBlink(BLINK_KIND_HURT);
					if(s_isDeathDance && randUwMax(&g_sRand, 99) < 5) {
						s_sPlayer.wHealth = 0;
					}
					if(!s_isImmortal) {
						UBYTE ubDamage = s_ubEnemyDamage;
						if(s_isToughReloader && s_sPlayer.sPlayer.bReloadCooldown) {
							--ubDamage;
						}
						s_sPlayer.wHealth -= ubDamage;
					}
					if(s_isRetaliation) {
						pEnemy->wHealth -= PLAYER_RETALIATION_DAMAGE;
					}
				}
				audioMixerPlaySfx(g_pSfxBite, SFX_CHANNEL_BITE, SFX_PRIORITY_BITE, 0);
				pEnemy->sEnemy.ubAttackCooldown = ENEMY_ATTACK_COOLDOWN;
			}
		}
		else {
			--pEnemy->sEnemy.ubAttackCooldown;
		}

		// if(0) {
			enemyTryMoveBy(pEnemy, bDeltaX, bDeltaY);
		// }
		if(!pEnemy->sEnemy.ubFrameCooldown) {
			pEnemy->sEnemy.ubFrameCooldown = 1;
		}
		else {
			pEnemy->sEnemy.ubFrameCooldown = 0;
			pEnemy->eFrame = (pEnemy->eFrame + 1);
			if(pEnemy->eFrame > ENTITY_FRAME_WALK_8) {
				pEnemy->eFrame = ENTITY_FRAME_WALK_1;
			}
		}

		tFrameOffset *pOffset = &s_pEnemyFrameOffsets[eDir][pEnemy->eFrame];
		pEnemy->sEnemy.eDirection = eDir;
		bobSetFrame(&pEnemy->sBob, pOffset->pPixels, pOffset->pMask);
		pEnemy->sBob.sPos.uwX = pEnemy->sPos.uwX - ENEMY_BOB_OFFSET_X;
		pEnemy->sBob.sPos.uwY = pEnemy->sPos.uwY - ENEMY_BOB_OFFSET_Y;
		bobPush(&pEnemy->sBob);
	}
	else {
		if(pEnemy->wHealth == HEALTH_ENEMY_DEAD_AWAITING_RESPAWN) {
			// Try respawn
			if(pEnemy->sEnemy.ubPreferredSpawn == ENEMY_PREFERRED_SPAWN_NONE) {
				tUwCoordYX sClosest;
				UWORD uwClosestDistance = 0xFFFF;
				for(UBYTE i = 0; i < RESPAWN_SLOTS_PER_POSITION; ++i) {
					tUwCoordYX sSpawn = s_pRespawnSlots[s_sPlayer.sPos.uwX / COLLISION_SIZE_X][s_sPlayer.sPos.uwY / COLLISION_SIZE_Y][i];
					WORD wDx = sSpawn.uwX - s_sPlayer.sPos.uwX;
					WORD wDy = sSpawn.uwY - s_sPlayer.sPos.uwY;

					UWORD uwDistance = fastMagnitude(ABS(wDx), ABS(wDy));
					if(uwDistance < uwClosestDistance) {
						uwClosestDistance = uwDistance;
						sClosest = sSpawn;
					}
				}
				if(!s_pCollisionTiles[sClosest.uwX / COLLISION_SIZE_X][sClosest.uwY / COLLISION_SIZE_Y]) {
					pEnemy->wHealth = s_uwEnemySpawnHealth;
					s_pCollisionTiles[sClosest.uwX / COLLISION_SIZE_X][sClosest.uwY / COLLISION_SIZE_Y] = pEnemy;
					pEnemy->sPos = sClosest;
					pEnemy->sEnemy.ubSpeed = (randUwMax(&g_sRand, ENEMY_SPEEDY_CHANCE_MAX) <= s_ubHiSpeedChance) ? 2 : 1;
					return;
				}
			}
			else {
				tUwCoordYX sSpawn = s_pRespawnSlots[s_sPlayer.sPos.uwX / COLLISION_SIZE_X][s_sPlayer.sPos.uwY / COLLISION_SIZE_Y][pEnemy->sEnemy.ubPreferredSpawn];
				if(!s_pCollisionTiles[sSpawn.uwX / COLLISION_SIZE_X][sSpawn.uwY / COLLISION_SIZE_Y]) {
					pEnemy->wHealth = s_uwEnemySpawnHealth;
					s_pCollisionTiles[sSpawn.uwX / COLLISION_SIZE_X][sSpawn.uwY / COLLISION_SIZE_Y] = pEnemy;
					pEnemy->sPos = sSpawn;
					pEnemy->sEnemy.ubSpeed = (randUwMax(&g_sRand, ENEMY_SPEEDY_CHANCE_MAX) <= s_ubHiSpeedChance) ? 2 : 1;
					return;
				}
			}
		}
		else if(pEnemy->wHealth == HEALTH_ENEMY_DEATH_ANIM) {
			if(!pEnemy->sEnemy.ubFrameCooldown) {
				pEnemy->sEnemy.ubFrameCooldown = 1;
			}
			else {
				pEnemy->sEnemy.ubFrameCooldown = 0;
				pEnemy->eFrame = (pEnemy->eFrame + 1);
				if(pEnemy->eFrame > ENTITY_FRAME_DIE_3) {
					pEnemy->eFrame = ENTITY_FRAME_DIE_3;
					pEnemy->wHealth = HEALTH_ENEMY_DEAD_AWAITING_RESPAWN;
				}
			}
			tFrameOffset *pOffset = &s_pEnemyFrameOffsets[pEnemy->sEnemy.eDirection][pEnemy->eFrame];
			bobSetFrame(&pEnemy->sBob, pOffset->pPixels, pOffset->pMask);
			bobPush(&pEnemy->sBob);
		}
		else {
			if(pEnemy->wHealth == HEALTH_ENEMY_OFFSCREENED) {
				pEnemy->wHealth = HEALTH_ENEMY_DEAD_AWAITING_RESPAWN;
			}
			else {
				scoreAddSmall(ENEMY_EXP);
				++s_ulKills;
				if(s_sPickup.wHealth == HEALTH_PICKUP_INACTIVE) {
					s_sPickup.wHealth = HEALTH_PICKUP_READY_TO_SPAWN;
					s_sPickup.sPos = pEnemy->sPos;
				}
				pEnemy->sEnemy.ubPreferredSpawn = ENEMY_PREFERRED_SPAWN_NONE;
				pEnemy->wHealth = HEALTH_ENEMY_DEATH_ANIM;
				pEnemy->sEnemy.ubFrameCooldown = 0;
				pEnemy->eFrame = ENTITY_FRAME_DIE_1;
			}
			s_pCollisionTiles[pEnemy->sPos.uwX / COLLISION_SIZE_X][pEnemy->sPos.uwY / COLLISION_SIZE_Y] = 0;
			// Failsafe to prevent trashing collision map
			pEnemy->sPos.ulYX = 0;
			// Display as-is to prevent flicker between alive and dead anim
			bobPush(&pEnemy->sBob);
		}
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
	UBYTE *pCD = &g_pGamePristineBuffer->Planes[0][ulDstOffs];

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

//------------------------------------------------------------------- PUBLIC FNS

ULONG gameGetSurviveTime(void) {
	return s_ulFrameCount;
}

ULONG gameGetKills(void) {
	return s_ulKills;
}

UBYTE gameGetLevel(void) {
	return s_ubScoreLevel;
}

ULONG gameGetExp(void) {
	return s_ulScore;
}

void gameResume(void) {
	systemSetInt(INTB_VERTB, onVblank, (void*)&s_ulFrameCount);
	hudReset();
}

void gameDiscardUndraw(void) {
	bobDiscardUndraw();
}

void gameProcessCursor(UWORD uwMouseX, UWORD uwMouseY) {
	s_pSpriteCursor->wX = uwMouseX - GAME_CURSOR_OFFSET_X;
	s_pSpriteCursor->wY = uwMouseY - GAME_CURSOR_OFFSET_Y;
	spriteProcess(s_pSpriteCursor);
}

void gameApplyPerk(tPerk ePerk) {
	--s_ubPendingPerks;
	perksLock(ePerk);
	switch(ePerk) {
		case PERK_GRIM_DEAL:
			scoreAddLarge((s_ulScore * 2) / 10);
			s_sPlayer.wHealth = 0;
			break;
		case PERK_FATAL_LOTTERY:
			if(randUwMax(&g_sRand, 99) < 50) {
				s_sPlayer.wHealth = 0;
			}
			else {
				scoreAddLarge(20000);
			}
			break;
		case PERK_INSTANT_WINNER:
			perksUnlock(ePerk); // multi-use
			scoreAddLarge(2000);
			break;
		case PERK_THICK_SKINNED:
			s_sPlayer.wHealth = MAX(1, s_sPlayer.wHealth - 25);
			--s_ubEnemyDamage;
			break;
		case PERK_BANDAGE:
			perksUnlock(ePerk); // multi-use
			s_sPlayer.wHealth = MIN(PLAYER_HEALTH_MAX, s_sPlayer.wHealth + (PLAYER_HEALTH_MAX / 10));
			break;
		case PERK_DEATH_CLOCK:
			s_sPlayer.wHealth = PLAYER_HEALTH_MAX;
			s_isDeathClock = 1;
			s_isImmortal = 1;
			s_ubDeathClockCooldown = PERK_DEATH_CLOCK_COOLDOWN;
			perksLock(PERK_FATAL_LOTTERY);
			perksLock(PERK_GRIM_DEAL);
			perksLock(PERK_BANDAGE);
			perksLock(PERK_BLOODY_AMMO);
			break;
		case PERK_RETALIATION:
			s_isRetaliation = 1;
			break;
		case PERK_AMMO_MANIAC:
			s_isAmmoManiac = 1;
			playerCalculateMaxAmmo();
			break;
		case PERK_MY_FAVOURITE_WEAPON:
			s_isFavouriteWeapon = 1;
			playerCalculateMaxAmmo();
			break;
		case PERK_ANXIOUS_LOADER:
			s_isAnxiousLoader = 1;
			break;
		case PERK_BLOODY_AMMO:
			s_isBloodyAmmo = 1;
			perksLock(PERK_DEATH_CLOCK);
			perksLock(PERK_DEATH_DANCE);
			break;
		case PERK_DEATH_DANCE:
			s_isDeathDance = 1;
			s_isImmortal = 1;
			perksLock(PERK_DEATH_CLOCK);
			perksLock(PERK_BANDAGE);
			perksLock(PERK_BLOODY_AMMO);
			break;
		case PERK_DODGER:
			s_ubDodgeChance = PERK_DODGE_CHANCE_DODGER;
			perksUnlock(PERK_NINJA);
			break;
		case PERK_NINJA:
			s_ubDodgeChance = PERK_DODGE_CHANCE_NINJA;
			break;
		case PERK_FINAL_REVENGE:
			s_isFinalRevenge = 1;
			break;
		case PERK_STATIONARY_RELOADER:
			s_isStationaryReloader = 1;
			break;
		case PERK_TOUGH_RELOADER:
			s_isToughReloader = 1;
			break;
		case PERK_SWIFT_LEARNER:
			s_isSwiftLearner = 1;
			s_ulNextLevelScore -= s_ulPrevLevelScore / 5;
			break;
		case PERK_FAST_SHOT:
			s_isFastShot = 1;
			s_sPlayer.sPlayer.ubWeaponCooldown -= 2;
			break;
		case PERK_BONUS_LEARNER:
			s_isBonusLearner = 1;
			break;
		case PERK_COUNT:
			__builtin_unreachable();
	}
}

void gameStart(void) {
	for(UBYTE ubX = 0; ubX < BG_TILES_X; ++ubX) {
		for(UBYTE ubY = 0; ubY < BG_TILES_Y; ++ubY) {
			UWORD uwRand = randUwMinMax(&g_sRand, 0, 99);
			if(uwRand < 5) {
				gameSetTile(randUwMinMax(&g_sRand, 3, 5), ubX, ubY);
			}
			else if(uwRand < 15) {
				gameSetTile(randUwMinMax(&g_sRand, 6, 7), ubX, ubY);
			}
			else {
				gameSetTile(randUwMinMax(&g_sRand, 0, 2), ubX, ubY);
			}
			// gameSetTile(randUwMinMax(&g_sRand, 0, 4), ubX, ubY);
		}
	}

	s_ubDeathCooldown = GAME_PLAYER_DEATH_COOLDOWN;
	gameSetCursor(CURSOR_KIND_FULL);
	perksReset();
	perksUnlock(PERK_BANDAGE);
	perksUnlock(PERK_GRIM_DEAL);
	perksUnlock(PERK_INSTANT_WINNER);
	perksUnlock(PERK_THICK_SKINNED);
	perksUnlock(PERK_DEATH_CLOCK);
	perksUnlock(PERK_RETALIATION);
	perksUnlock(PERK_AMMO_MANIAC);
	perksUnlock(PERK_MY_FAVOURITE_WEAPON);
	perksUnlock(PERK_ANXIOUS_LOADER);
	perksUnlock(PERK_BLOODY_AMMO);
	perksUnlock(PERK_DEATH_DANCE);
	perksUnlock(PERK_DODGER);
	perksUnlock(PERK_FINAL_REVENGE);
	perksUnlock(PERK_STATIONARY_RELOADER);
	perksUnlock(PERK_TOUGH_RELOADER);
	perksUnlock(PERK_SWIFT_LEARNER);
	perksUnlock(PERK_FAST_SHOT);
	perksUnlock(PERK_BONUS_LEARNER);
	s_isDeathClock = 0;
	s_isRetaliation = 0;
	s_isAmmoManiac = 0;
	s_isFavouriteWeapon = 0;
	s_isAnxiousLoader = 0;
	s_isBloodyAmmo = 0;
	s_isDeathDance = 0;
	s_ubDodgeChance = 0;
	s_isFinalRevenge = 0;
	s_isStationaryReloader = 0;
	s_isToughReloader = 0;
	s_isSwiftLearner = 0;
	s_isFastShot = 0;
	s_isBonusLearner = 0;
	s_isImmortal = 0;

	s_ulKills = 0;
	s_ulScore = 0;
	s_ulPrevLevelScore = 0;
	s_ulNextLevelScore = 1024;
	s_ubScoreLevel = 1;
	s_ubPendingPerks = 0;
	s_ubHiSpeedChance = 0;

	fontDrawStr(
		g_pFontSmall, s_pBufferHud->pBack,
		HUD_SCORE_TEXT_X - 20, HUD_SCORE_TEXT_Y, "EXP",
		COLOR_HUD_SCORE, FONT_COOKIE, g_pLineBuffer
	);
	fontDrawStr(
		g_pFontSmall, s_pBufferHud->pBack,
		HUD_LEVEL_TEXT_X - 20, HUD_LEVEL_TEXT_Y, "LVL",
		COLOR_HUD_SCORE, FONT_COOKIE, g_pLineBuffer
	);

	hudReset();

	for(UBYTE ubX = 0; ubX < COLLISION_LOOKUP_SIZE_X; ++ubX) {
		for(UBYTE ubY = 0; ubY < COLLISION_LOOKUP_SIZE_Y; ++ubY) {
			s_pCollisionTiles[ubX][ubY] = 0;
		}
	}

	UBYTE ubSorted = 0;
	s_ubEnemyDamage = ENEMY_DAMAGE_BASE;
	s_uwEnemySpawnHealth = ENEMY_HEALTH_BASE;
	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		s_pEnemies[i].eKind = ENTITY_KIND_ENEMY;
		s_pEnemies[i].wHealth = s_uwEnemySpawnHealth;
		s_pEnemies[i].sPos.uwX = 32 + (i % 8) * 32;
		s_pEnemies[i].sPos.uwY = 32 + (i / 8) * 32;
		s_pEnemies[i].eFrame = 0;
		s_pEnemies[i].sEnemy.ubFrameCooldown = 0;
		s_pEnemies[i].sEnemy.ubAttackCooldown = 0;
		s_pEnemies[i].sEnemy.ubSpeed = 1;
		s_pCollisionTiles[s_pEnemies[i].sPos.uwX / COLLISION_SIZE_X][s_pEnemies[i].sPos.uwY / COLLISION_SIZE_Y] = &s_pEnemies[i];
		s_pSortedEntities[ubSorted++] = &s_pEnemies[i];
	}

	s_sPlayer.eKind = ENTITY_KIND_PLAYER;
	s_sPlayer.wHealth = PLAYER_HEALTH_MAX;
	s_sPlayer.sPos.uwX = (MAP_TILES_X * MAP_TILE_SIZE) / 2;
	s_sPlayer.sPos.uwY = (MAP_TILES_Y * MAP_TILE_SIZE) / 2;
	s_sPlayer.eFrame = 0;
	s_sPlayer.sEnemy.ubFrameCooldown = 0;
	s_sPlayer.sPlayer.ubAttackCooldown = PLAYER_ATTACK_COOLDOWN;
	s_sPlayer.sPlayer.ubBlinkCooldown = PLAYER_BLINK_COOLDOWN;
	playerSetWeapon(WEAPON_KIND_STOCK_RIFLE);
	s_pCollisionTiles[s_sPlayer.sPos.uwX / COLLISION_SIZE_X][s_sPlayer.sPos.uwY / COLLISION_SIZE_Y] = &s_sPlayer;
	s_pSortedEntities[ubSorted++] = &s_sPlayer;

	s_sPickup.eKind = ENTITY_KIND_PICKUP;
	s_sPickup.wHealth = 0;
	s_sPickup.sPos.ulYX = 0;
	s_pSortedEntities[ubSorted++] = &s_sPickup;

	s_ubExplosionCooldown = 0;
	s_ubExplosionFrame = EXPLOSION_FRAME_COUNT;

	for(UBYTE i = 0; i < PROJECTILE_COUNT; ++i) {
		s_pProjectiles[i].ubLife = 0;
		s_pFreeProjectiles[i] = &s_pProjectiles[i];
	}
	s_ubFreeProjectileCount = PROJECTILE_COUNT;
	s_ulFrameCount = 0;
	s_ulFrameWaitCount = 1;
	gameResume();

	ptplayerLoadMod(g_pModGame, 0, 0);
	ptplayerEnableMusic(1);
}

__attribute__((always_inline))
static inline void detonateBombAtPlayer(void) {
	s_sExplosionBob.sPos.uwX = s_sPlayer.sPos.uwX - EXPLOSION_BOB_SIZE_X / 2;
	s_sExplosionBob.sPos.uwY = s_sPlayer.sPos.uwY - EXPLOSION_BOB_SIZE_Y / 2;
	s_ubExplosionCooldown = 1;
	s_ubExplosionFrame = -1;
	for(UBYTE i = 0; i < SORTED_ENTITIES_COUNT; ++i) {
		tEntity *pChar = s_pSortedEntities[i];
		if(pChar->eKind == ENTITY_KIND_ENEMY) {
			WORD wDistanceToPlayerX = s_sPlayer.sPos.uwX - pChar->sPos.uwX;
			WORD wDistanceToPlayerY = s_sPlayer.sPos.uwY - pChar->sPos.uwY;
			if(
				-EXPLOSION_HIT_RANGE < wDistanceToPlayerX && wDistanceToPlayerX < EXPLOSION_HIT_RANGE &&
				-EXPLOSION_HIT_RANGE < wDistanceToPlayerY && wDistanceToPlayerY < EXPLOSION_HIT_RANGE
			) {
				pChar->wHealth -= 200;
			}
		}
	}
}

__attribute__((always_inline))
static inline void playerApplyPickup(tPickupKind ePickupKind) {
	if(s_isBonusLearner) {
		scoreAddSmall(100);
	}

	switch(ePickupKind) {
		case PICKUP_KIND_RIFLE:
			playerSetWeapon(WEAPON_KIND_STOCK_RIFLE);
			break;
		case PICKUP_KIND_SMG:
			playerSetWeapon(WEAPON_KIND_SMG);
			break;
		case PICKUP_KIND_ASSAULT_RIFLE:
			playerSetWeapon(WEAPON_KIND_ASSAULT_RIFLE);
			break;
		case PICKUP_KIND_SHOTGUN:
			playerSetWeapon(WEAPON_KIND_SHOTGUN);
			break;
		case PICKUP_KIND_SAWOFF:
			playerSetWeapon(WEAPON_KIND_SAWOFF);
			break;
		case PICKUP_KIND_EXP_400:
			scoreAddSmall(400);
			break;
		case PICKUP_KIND_EXP_800:
			scoreAddSmall(800);
			break;
		case PICKUP_KIND_BOMB:
			detonateBombAtPlayer();
			break;
		case PICKUP_KIND_COUNT:
			__builtin_unreachable();
	}
}

__attribute__((always_inline))
static inline UBYTE playerProcess(void) {
	if(mouseUse(MOUSE_PORT_1, MOUSE_RMB) && s_ubPendingPerks) {
		systemSetInt(INTB_VERTB, 0, 0);
		gameSetCursor(CURSOR_KIND_FULL);
		statePush(g_pGameStateManager, &g_sStatePerks);
		return 1;
	}
	UWORD uwMouseX = mouseGetX(MOUSE_PORT_1);
	UWORD uwMouseY = mouseGetY(MOUSE_PORT_1);

	if(s_sPlayer.wHealth > 0) {
#if defined(GAME_DEBUG)
		if(keyUse(KEY_1)) {
			playerSetWeapon(WEAPON_KIND_STOCK_RIFLE);
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
#endif

		UBYTE ubAimAngle = getAngleBetweenPoints( // 0 is right, going clockwise
			s_sPlayer.sPos.uwX - g_pGameBufferMain->pCamera->uPos.uwX,
			s_sPlayer.sPos.uwY - g_pGameBufferMain->pCamera->uPos.uwY,
			uwMouseX, uwMouseY - GAME_HUD_VPORT_SIZE_Y
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
			playerTryMoveBy(&s_sPlayer, bDeltaX, bDeltaY);
			if(s_sPlayer.sPlayer.ubFrameCooldown >= 1) {
				s_sPlayer.eFrame = (s_sPlayer.eFrame + 1);
				if(s_sPlayer.eFrame > ENTITY_FRAME_WALK_8) {
					s_sPlayer.eFrame = ENTITY_FRAME_WALK_1;
				}
				s_sPlayer.sPlayer.ubFrameCooldown = 0;
			}
			else {
				++s_sPlayer.sPlayer.ubFrameCooldown;
			}
		}
		else {
			s_sPlayer.eFrame = ENTITY_FRAME_WALK_1;
			s_sPlayer.sPlayer.ubFrameCooldown = 0;
		}

		if(s_sPlayer.sPlayer.bReloadCooldown) {
			--s_sPlayer.sPlayer.bReloadCooldown;
			if(s_isAnxiousLoader && mouseUse(MOUSE_PORT_1, MOUSE_LMB)) {
				--s_sPlayer.sPlayer.bReloadCooldown;
			}
			if(s_isStationaryReloader && bDeltaX == 0 && bDeltaY == 0) {
				--s_sPlayer.sPlayer.bReloadCooldown;
			}
			if(s_sPlayer.sPlayer.bReloadCooldown <= 0) {
				s_sPlayer.sPlayer.bReloadCooldown = 0;
				s_sPlayer.sPlayer.ubAmmo = s_sPlayer.sPlayer.ubMaxAmmo;
				gameSetCursor(CURSOR_KIND_FULL);
			}
		}
		if(!s_sPlayer.sPlayer.ubAttackCooldown) {
			if(!s_sPlayer.sPlayer.bReloadCooldown) {
				if(!s_sPlayer.sPlayer.ubAmmo) {
					playerStartReloadWeapon();
				}
				else if(keyUse(KEY_R) && s_sPlayer.sPlayer.ubAmmo < s_sPlayer.sPlayer.ubMaxAmmo) {
					playerStartReloadWeapon();
				}
			}
			if(mouseCheck(MOUSE_PORT_1, MOUSE_LMB) && (s_sPlayer.sPlayer.ubAmmo || s_isBloodyAmmo)) {
				if(s_sPlayer.sPlayer.ubAmmo) {
					--s_sPlayer.sPlayer.ubAmmo;
				}
				else if(!s_isImmortal) {
					--s_sPlayer.wHealth;
				}
				playerShootWeapon(ubAimAngle);
				s_sPlayer.eFrame += ENTITY_FRAME_SHOOT_1 - ENTITY_FRAME_WALK_1;
			}
		}
		else {
			--s_sPlayer.sPlayer.ubAttackCooldown;
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
		s_sPlayer.sPlayer.eDirection = eDir;
		tFrameOffset *pOffset = &s_pPlayerFrameOffsets[eDir][s_sPlayer.eFrame];
		if(s_sPlayer.sPlayer.ubBlinkCooldown) {
			--s_sPlayer.sPlayer.ubBlinkCooldown;
		}
		else {
			s_sPlayer.sBob.pFrameData = pOffset->pPixels;
		}
		s_sPlayer.sBob.pMaskData = pOffset->pMask;
		s_sPlayer.sBob.sPos.uwX = s_sPlayer.sPos.uwX - PLAYER_BOB_OFFSET_X;
		s_sPlayer.sBob.sPos.uwY = s_sPlayer.sPos.uwY - PLAYER_BOB_OFFSET_Y;
		cameraCenterAtOptimized(g_pGameBufferMain->pCamera, s_sPlayer.sPos.uwX, s_sPlayer.sPos.uwY);

		if(s_isDeathClock) {
			if(s_ubDeathClockCooldown) {
				--s_ubDeathClockCooldown;
			}
			else {
				s_ubDeathClockCooldown = PERK_DEATH_CLOCK_COOLDOWN;
				--s_sPlayer.wHealth;
			}
		}
	}
	else {
		s_sPlayer.wHealth = 0; // Get rid of negative value for HUD etc
		if(s_ubDeathCooldown == GAME_PLAYER_DEATH_COOLDOWN) {
			// Create a different move target for zombies
			// s_sPlayer.sPos.uwX = (MAP_TILES_X * MAP_TILE_SIZE) - s_sPlayer.sPos.uwX;
			// s_sPlayer.sPos.uwY = (MAP_TILES_X * MAP_TILE_SIZE) - s_sPlayer.sPos.uwY;
			s_sPlayer.eFrame = ENTITY_FRAME_DIE_1;
			s_sPlayer.sPlayer.ubFrameCooldown = 0;
			if(s_isFinalRevenge) {
				detonateBombAtPlayer();
			}
		}
		if(s_ubDeathCooldown) {
			--s_ubDeathCooldown;

			if(s_sPlayer.sPlayer.ubFrameCooldown >= 1) {
				s_sPlayer.eFrame = (s_sPlayer.eFrame + 1);
				if(s_sPlayer.eFrame > ENTITY_FRAME_DIE_8) {
					s_sPlayer.eFrame = ENTITY_FRAME_DIE_8;
				}
				s_sPlayer.sPlayer.ubFrameCooldown = 0;
			}
			else {
				++s_sPlayer.sPlayer.ubFrameCooldown;
			}
			tFrameOffset *pOffset = &s_pPlayerFrameOffsets[s_sPlayer.sPlayer.eDirection][s_sPlayer.eFrame];
			bobSetFrame(&s_sPlayer.sBob, pOffset->pPixels, pOffset->pMask);
			s_sPlayer.sBob.sPos.uwX = s_sPlayer.sPos.uwX + s_pPlayerFrameDeathOffset[s_sPlayer.sPlayer.eDirection][s_sPlayer.eFrame - ENTITY_FRAME_DIE_1].bX;
			s_sPlayer.sBob.sPos.uwY = s_sPlayer.sPos.uwY + s_pPlayerFrameDeathOffset[s_sPlayer.sPlayer.eDirection][s_sPlayer.eFrame - ENTITY_FRAME_DIE_1].bY;
			}
		else {
			gameSetCursor(CURSOR_KIND_FULL);
			menuPush(1);
			return 1;
		}
	}
	bobPush(&s_sPlayer.sBob);

	gameProcessCursor(uwMouseX, uwMouseY);
	return 0;
}

__attribute__((always_inline))
static inline void pickupSpawnRandom(void) {
	tPickupKind ePickupKind = randUwMax(&g_sRand, PICKUP_KIND_COUNT - 1);
	if(s_isFavouriteWeapon && ePickupKind <= PICKUP_KIND_WEAPON_LAST) {
		s_sPickup.wHealth = HEALTH_PICKUP_INACTIVE;
		return;
	}
	s_sPickup.sPickup.ePickupKind = ePickupKind;
	s_sPickup.wHealth = PICKUP_LIFE_SECONDS * GAME_FPS;
	s_sPickup.sPickup.wBlinkCooldown = (PICKUP_LIFE_SECONDS - 3) * GAME_FPS;
	s_sPickup.sPickup.isDisplayed = 1;
	bobSetFrame(
		&s_sPickup.sBob,
		s_pPickupFrameOffsets[s_sPickup.sPickup.ePickupKind].pPixels,
		s_pPickupFrameOffsets[s_sPickup.sPickup.ePickupKind].pMask
	);
	s_sPickup.sBob.sPos.uwX = s_sPickup.sPos.uwX - PICKUP_BOB_OFFSET_X;
	s_sPickup.sBob.sPos.uwY = s_sPickup.sPos.uwY - PICKUP_BOB_OFFSET_Y;
	s_pCollisionTiles[s_sPickup.sPos.uwX / COLLISION_SIZE_X][s_sPickup.sPos.uwY / COLLISION_SIZE_Y] = &s_sPickup;
}

__attribute__((always_inline))
static inline void pickupProcss(tEntity *pPickup) {
	if(pPickup->wHealth > 0) {
		--pPickup->wHealth;
		if(isPositionCollidingWithEntity(pPickup->sPos, &s_sPlayer)) {
			pPickup->wHealth = 0;
			playerApplyPickup(pPickup->sPickup.ePickupKind);
		}
		else {
			if(--s_sPickup.sPickup.wBlinkCooldown == 0) {
				s_sPickup.sPickup.wBlinkCooldown = GAME_FPS / 5;
				s_sPickup.sPickup.isDisplayed = !s_sPickup.sPickup.isDisplayed;
			}
			if(s_sPickup.sPickup.isDisplayed) {
				bobPush(&pPickup->sBob);
			}
		}
	}
	else {
		if(pPickup->wHealth == HEALTH_PICKUP_READY_TO_SPAWN) {
			if(
				s_sPlayer.sPlayer.eWeaponKind == WEAPON_KIND_STOCK_RIFLE ||
				randUwMax(&g_sRand, PICKUP_SPAWN_CHANCE_MAX) < PICKUP_SPAWN_CHANCE
			) {
				pickupSpawnRandom();
			}
			else {
				pPickup->wHealth = HEALTH_PICKUP_INACTIVE;
			}
		}
		else if(pPickup->wHealth == 0) {
			s_pCollisionTiles[pPickup->sPos.uwX / COLLISION_SIZE_X][pPickup->sPos.uwY / COLLISION_SIZE_Y] = 0;
			pPickup->wHealth = HEALTH_PICKUP_INACTIVE;
		}
	}
}

__attribute__((always_inline))
static inline void explosionProcess(void) {
	if(s_ubExplosionFrame != EXPLOSION_FRAME_COUNT) {
		if(--s_ubExplosionCooldown == 0) {
			s_ubExplosionCooldown = EXPLOSION_COOLDOWN;
			if(++s_ubExplosionFrame == EXPLOSION_FRAME_COUNT) {
				return;
			}
			bobSetFrame(
				&s_sExplosionBob,
				s_pExplosionFrameOffsets[s_ubExplosionFrame].pPixels,
				s_pExplosionFrameOffsets[s_ubExplosionFrame].pMask
			);
		}
		bobPush(&s_sExplosionBob);
	}
}

//-------------------------------------------------------------------- GAMESTATE

static void gameGsCreate(void) {
	logBlockBegin("gameGsCreate()");
	s_pTileset = bitmapCreateFromPath("data/tiles.bm", 0);
	s_pHudWeapons = bitmapCreateFromPath("data/weapons.bm", 0);
	s_pHudLevelUp = bitmapCreateFromPath("data/level_up.bm", 0);
	assetsGameCreate();

	ULONG ulRawCopSize = 16 + simpleBufferGetRawCopperlistInstructionCount(GAME_BPP) * 2;
	s_pView = viewCreate(0,
		TAG_VIEW_COPLIST_MODE, VIEW_COPLIST_MODE_RAW,
		TAG_VIEW_COPLIST_RAW_COUNT, ulRawCopSize,
	TAG_END);

	s_pVpHud = vPortCreate(0,
		TAG_VPORT_BPP, GAME_BPP,
		TAG_VPORT_HEIGHT, GAME_HUD_VPORT_SIZE_Y,
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
	g_pGameBufferMain = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_BOUND_WIDTH, MAP_TILES_X * MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_BOUND_HEIGHT, MAP_TILES_Y * MAP_TILE_SIZE,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 1,
		TAG_SIMPLEBUFFER_VPORT, s_pVpMain,
		TAG_SIMPLEBUFFER_COPLIST_OFFSET, ulCopOffset,
	TAG_END);
	cameraCenterAtOptimized(
		g_pGameBufferMain->pCamera,
		MAP_TILES_X * MAP_TILE_SIZE / 2,
		MAP_TILES_Y * MAP_TILE_SIZE / 2
	);

	blitRect(s_pBufferHud->pBack, 0, 0, 320, GAME_HUD_VPORT_SIZE_Y, COLOR_HUD_BG);

	g_pGamePristineBuffer = bitmapCreate(
		bitmapGetByteWidth(g_pGameBufferMain->pBack) * 8,
		g_pGameBufferMain->pBack->Rows, GAME_BPP, BMF_INTERLEAVED
	);
	s_pPristinePlanes = g_pGamePristineBuffer->Planes[0];

	paletteLoadFromPath("data/game.plt", s_pVpHud->pPalette, 1 << GAME_BPP);


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

	s_pPlayerBlinkBitmaps[BLINK_KIND_HURT] = bitmapCreate(PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, GAME_BPP, BMF_INTERLEAVED);
	s_pPlayerBlinkBitmaps[BLINK_KIND_LEVEL] = bitmapCreate(PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, GAME_BPP, BMF_INTERLEAVED);
	blitRect(s_pPlayerBlinkBitmaps[BLINK_KIND_HURT], 0, 0, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, COLOR_HURT);
	blitRect(s_pPlayerBlinkBitmaps[BLINK_KIND_LEVEL], 0, 0, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, COLOR_LEVEL);
	s_pPlayerBlinkData[BLINK_KIND_HURT] = s_pPlayerBlinkBitmaps[BLINK_KIND_HURT]->Planes[0];
	s_pPlayerBlinkData[BLINK_KIND_LEVEL] = s_pPlayerBlinkBitmaps[BLINK_KIND_LEVEL]->Planes[0];

	for(tDirection eDir = 0; eDir < DIRECTION_COUNT; ++eDir) {
		for(tCharacterFrame eFrame = 0; eFrame < ENTITY_FRAME_COUNT; ++eFrame) {
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
		for(tCharacterFrame eFrame = 0; eFrame < ENEMY_FRAME_COUNT; ++eFrame) {
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

	s_pPickupFrames = bitmapCreateFromPath("data/pickups.bm", 0);
	s_pPickupMasks = bitmapCreateFromPath("data/pickups_mask.bm", 0);
	for(UBYTE i = 0; i < PICKUP_KIND_COUNT; ++i) {
		s_pPickupFrameOffsets[i].pPixels = bobCalcFrameAddress(s_pPickupFrames, i * PICKUP_BOB_SIZE_Y);
		s_pPickupFrameOffsets[i].pMask = bobCalcFrameAddress(s_pPickupMasks, i * PICKUP_BOB_SIZE_Y);
	}

	s_pBulletFrames = bitmapCreateFromPath("data/bullets.bm", 0);
	s_pBulletMasks = bitmapCreateFromPath("data/bullets_mask.bm", 0);
	for(UBYTE i = 0; i < WEAPON_MAX_BULLETS_IN_MAGAZINE; ++i) {
		s_pHudBulletDefs[i] = (tHudBulletDef){
			.sOffs = {
				.ubX = HUD_AMMO_FIELD_OFFSET_X + (i % HUD_AMMO_BULLET_COUNT_X) * (HUD_AMMO_BULLET_SIZE_X + 1),
				.ubY = HUD_AMMO_FIELD_OFFSET_Y + (i / HUD_AMMO_BULLET_COUNT_X) * (HUD_AMMO_BULLET_SIZE_Y + 1)
			},
			.ubMaxDrawDelta = MIN(HUD_AMMO_BULLET_COUNT_X - (i % HUD_AMMO_BULLET_COUNT_X), HUD_AMMO_BULLETS_MAX_ONCE)
		};
	}

	s_pExplosionFrames = bitmapCreateFromPath("data/explosion.bm", 0);
	s_pExplosionMasks = bitmapCreateFromPath("data/explosion_mask.bm", 0);
	for(UBYTE i = 0; i < EXPLOSION_FRAME_COUNT; ++i) {
		s_pExplosionFrameOffsets[i].pPixels = bobCalcFrameAddress(s_pExplosionFrames, i * EXPLOSION_BOB_SIZE_Y);
		s_pExplosionFrameOffsets[i].pMask = bobCalcFrameAddress(s_pExplosionMasks, i * EXPLOSION_BOB_SIZE_Y);
	}

	s_pStainFrames = bitmapCreateFromPath("data/stains.bm", 0);
	s_pStainMasks = bitmapCreateFromPath("data/stains_mask.bm", 0);

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
		g_pGameBufferMain->pFront, g_pGameBufferMain->pBack,
		g_pGamePristineBuffer, MAP_TILES_Y * MAP_TILE_SIZE
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
	bobInit(&s_sExplosionBob, EXPLOSION_BOB_SIZE_X, EXPLOSION_BOB_SIZE_Y, 1, 0, 0, 0, 0);
	s_pNextFreeStain = &s_pFreeStains[STAINS_MAX];
	s_pNextPushStain = &s_pPushStains[0];
	s_pNextWaitStain = &s_pWaitStains[0];

	bobInit(&s_sPlayer.sBob, PLAYER_BOB_SIZE_X, PLAYER_BOB_SIZE_Y, 1, s_pPlayerFrameOffsets[0][0].pPixels, s_pPlayerFrameOffsets[0][0].pMask, 32, 32);
	for(UBYTE i = 0; i < ENEMY_COUNT; ++i) {
		bobInit(&s_pEnemies[i].sBob, ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_Y, 1, s_pEnemyFrameOffsets[0][0].pPixels, s_pEnemyFrameOffsets[0][0].pMask, 32, 32);
	}

	bobInit(&s_sPickup.sBob, PICKUP_BOB_SIZE_X, PICKUP_BOB_SIZE_Y, 1, 0, 0, 0, 0);

	bobReallocateBuffers();
	gameMathInit();
	for(UBYTE ubAngle = 0; ubAngle < GAME_MATH_ANGLE_COUNT; ++ubAngle) {
		s_pSin10p6[ubAngle] = csin(ubAngle) >> 10;
	}

	for(UBYTE ubX = 0; ubX < COLLISION_LOOKUP_SIZE_X; ++ubX) {
		for(UBYTE ubY = 0; ubY < COLLISION_LOOKUP_SIZE_Y; ++ubY) {
			ULONG ulCenterX = ubX * COLLISION_SIZE_X;
			ULONG ulCenterY = ubY * COLLISION_SIZE_Y;
			LONG lLeft = ulCenterX - GAME_MAIN_VPORT_SIZE_X / 2;
			LONG lTop = ulCenterY - (GAME_MAIN_VPORT_SIZE_Y - GAME_HUD_VPORT_SIZE_Y) / 2;
			lLeft = CLAMP(lLeft, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_X - MAP_MARGIN_TILES) * MAP_TILE_SIZE - GAME_MAIN_VPORT_SIZE_X);
			lTop = CLAMP(lTop, MAP_MARGIN_TILES * MAP_TILE_SIZE, (MAP_TILES_Y - MAP_MARGIN_TILES) * MAP_TILE_SIZE - GAME_MAIN_VPORT_SIZE_Y);

			s_pRespawnSlots[ubX][ubY][0] = (tUwCoordYX) {
				.uwX = CLAMP(lLeft - ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_X, MAP_TILES_X * MAP_TILE_SIZE - ENEMY_BOB_SIZE_X),
				.uwY = ulCenterY,
			};
			s_pRespawnSlots[ubX][ubY][1] = (tUwCoordYX) {
				.uwX = CLAMP(lLeft + GAME_MAIN_VPORT_SIZE_X + ENEMY_BOB_SIZE_X, ENEMY_BOB_SIZE_X, MAP_TILES_X * MAP_TILE_SIZE - ENEMY_BOB_SIZE_X),
				.uwY = ulCenterY,
			};
			s_pRespawnSlots[ubX][ubY][2] = (tUwCoordYX) {
				.uwX = ulCenterX,
				.uwY = CLAMP(lTop - ENEMY_BOB_SIZE_Y, ENEMY_BOB_SIZE_Y, MAP_TILES_Y * MAP_TILE_SIZE - ENEMY_BOB_SIZE_Y),
			};
			s_pRespawnSlots[ubX][ubY][3] = (tUwCoordYX) {
				.uwX = ulCenterX,
				.uwY = CLAMP(lTop + GAME_MAIN_VPORT_SIZE_Y + ENEMY_BOB_SIZE_Y, ENEMY_BOB_SIZE_Y, MAP_TILES_Y * MAP_TILE_SIZE - ENEMY_BOB_SIZE_Y),
			};
		}
	}

	s_pBmCursor = bitmapCreate(CURSOR_SPRITE_SIZE_X, CURSOR_SPRITE_SIZE_Y, 2, BMF_INTERLEAVED | BMF_CLEAR);
	s_pBmCursorFrames = bitmapCreateFromPath("data/cursors.bm", 0);
	for(tCursorKind i = 0; i < CURSOR_KIND_COUNT; ++i) {
		s_pCursorOffsets[i] = (ULONG*)bobCalcFrameAddress(s_pBmCursorFrames, i * CURSOR_SIZE);
	}
	s_pCursorData = &((ULONG*)s_pBmCursor->Planes[0])[1];
	gameSetCursor(CURSOR_KIND_FULL);
	spriteManagerCreate(s_pView, 0, 0);
	s_pSpriteCursor = spriteAdd(SPRITE_CHANNEL_CURSOR, s_pBmCursor);
	systemSetDmaBit(DMAB_SPRITE, 1);
	mouseSetBounds(MOUSE_PORT_1, 0, GAME_HUD_VPORT_SIZE_Y, 320, 256);

	hiScoreLoad();
	commCreate();

	systemUnuse();
	gameStart();

	viewLoad(s_pView);
	spriteProcessChannel(SPRITE_CHANNEL_CURSOR);
	viewProcessManagers(s_pView);
	copSwapBuffers();

	spriteProcessChannel(SPRITE_CHANNEL_CURSOR);
	viewProcessManagers(s_pView);
	copSwapBuffers();

	logBlockEnd("gameGsCreate()");
	menuPush(0);
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
		systemSetInt(INTB_VERTB, 0, 0);
		gameSetCursor(CURSOR_KIND_FULL);
		statePush(g_pGameStateManager, &g_sStatePause);
		return;
	}
#if defined(GAME_DEBUG)
	if(keyUse(KEY_0)) {
		scoreAddSmall(500);
	}
#endif

	s_pBackPlanes = g_pGameBufferMain->pBack->Planes[0];
	s_pCurrentProjectile = &s_pProjectiles[0];
	bobBegin(g_pGameBufferMain->pBack);
	while(s_pCurrentProjectile != &s_pProjectiles[PROJECTILE_COUNT]) {
		projectileUndrawNext();
	}

	tEntity **pPrev = &s_pSortedEntities[0];
	for(UBYTE i = 0; i < SORTED_ENTITIES_COUNT; ++i) {
		tEntity *pChar = s_pSortedEntities[i];
		switch(pChar->eKind) {
			case ENTITY_KIND_PLAYER:
				if(playerProcess()) {
					return;
				}
				break;
			case ENTITY_KIND_ENEMY:
				enemyProcess(pChar);
				break;
			case ENTITY_KIND_PICKUP:
				pickupProcss(pChar);
				break;
		}

		if(pChar->sPos.ulYX < (*pPrev)->sPos.ulYX) {
			s_pSortedEntities[i] = *pPrev;
			*pPrev = pChar;
		}
		pPrev = &s_pSortedEntities[i];
	}

	explosionProcess();

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

	simpleBufferProcess(g_pGameBufferMain);
	cameraProcess(g_pGameBufferMain->pCamera);
	copSwapBuffers();
	gameWaitForNextFrame();
}

static void gameGsDestroy(void) {
	viewLoad(0);
	ptplayerStop();
	systemUse();

	commDestroy();

	spriteManagerDestroy();
	bitmapDestroy(s_pBmCursorFrames);
	bitmapDestroy(s_pBmCursor);

	bobManagerDestroy();
	for(tDirection eDir = 0; eDir < DIRECTION_COUNT; ++eDir) {
		bitmapDestroy(s_pPlayerFrames[eDir]);
		bitmapDestroy(s_pPlayerMasks[eDir]);

		bitmapDestroy(s_pEnemyFrames[eDir]);
		bitmapDestroy(s_pEnemyMasks[eDir]);
	}

	bitmapDestroy(s_pPlayerBlinkBitmaps[0]);
	bitmapDestroy(s_pPlayerBlinkBitmaps[1]);

	bitmapDestroy(s_pPickupFrames);
	bitmapDestroy(s_pPickupMasks);
	bitmapDestroy(s_pBulletFrames);
	bitmapDestroy(s_pBulletMasks);
	bitmapDestroy(s_pExplosionFrames);
	bitmapDestroy(s_pExplosionMasks);
	bitmapDestroy(s_pStainFrames);
	bitmapDestroy(s_pStainMasks);

	viewDestroy(s_pView);
	bitmapDestroy(g_pGamePristineBuffer);
	bitmapDestroy(s_pHudWeapons);
	bitmapDestroy(s_pHudLevelUp);
	bitmapDestroy(s_pTileset);
	assetsGameDestroy();
}

tState g_sStateGame = {
	.cbCreate = gameGsCreate, .cbLoop = gameGsLoop, .cbDestroy = gameGsDestroy,
};
