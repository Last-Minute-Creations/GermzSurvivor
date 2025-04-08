// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef PERKS_H
#define PERKS_H

#include <ace/managers/state.h>

typedef enum tPerk {
	PERK_GRIM_DEAL,
	PERK_FATAL_LOTTERY,
	PERK_INSTANT_WINNER,
	PERK_BANDAGE,
	PERK_THICK_SKINNED,
	PERK_DEATH_CLOCK,
	PERK_RETALIATION,
	PERK_AMMO_MANIAC,
	PERK_MY_FAVOURITE_WEAPON,
	PERK_ANXIOUS_LOADER,

	PERK_COUNT // also used for locked perk
} tPerk;

void perksReset(void);

void perksUnlock(tPerk ePerk);

void perksLock(tPerk ePerk);

extern tState g_sStatePerks;

#endif // PERKS_H
