// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef SURVIVOR_MENNU_H
#define SURVIVOR_MENNU_H

#define MENU_BUTTON_MARGIN_Y 5
#define LOGO_SIZE_X 112
#define LOGO_SIZE_Y 48

#include <ace/managers/state.h>

extern tState g_sStateMenu;

void menuPush(UBYTE isDead);

#endif // SURVIVOR_MENNU_H
