// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include "assets.h"

void assetsGlobalCreate(void) {
	g_pFont = fontCreateFromPath("data/uni54.fnt");
}

void assetsGlobalDestroy(void) {
	fontDestroy(g_pFont);
}

tFont *g_pFont;
