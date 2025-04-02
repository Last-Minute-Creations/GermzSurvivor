// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef HI_SCORE_H
#define HI_SCORE_H

#include <ace/types.h>

void hiScoreEnteringProcess(void);

UBYTE hiScoreIsEnteringNew(void);

void hiScoreSetup(ULONG ulScore, UBYTE isPostGame);

void hiScoreLoad(void);

void hiScoreDrawAll(void);

#endif // _HI_SCORE_H_
