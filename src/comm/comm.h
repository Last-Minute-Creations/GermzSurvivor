/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef COMM_COMM_H
#define COMM_COMM_H

#include <ace/utils/font.h>
// #include "direction.h"
// #include "../steer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define COMM_WIDTH 256
#define COMM_HEIGHT 192
// #define COMM_DEBUG

#define COMM_DISPLAY_X 23
#define COMM_DISPLAY_Y 29
#define COMM_DISPLAY_WIDTH 210
#define COMM_DISPLAY_HEIGHT 136
#define COMM_DISPLAY_COLOR_BG 6
#define COMM_DISPLAY_COLOR_TEXT_HOVER 10
#define COMM_DISPLAY_COLOR_TEXT 8
#define COMM_DISPLAY_COLOR_TEXT_DARK 7

typedef void (*tPageProcess)(void);

typedef void (*tPageCleanup)(void);

typedef enum _tBtnState {
	BTN_STATE_NACTIVE = 0,
	BTN_STATE_ACTIVE = 1,
	BTN_STATE_USED
} tBtnState;

void commCreate(void);

void commDestroy(void);

void commProcess(void);

void commShow(void);

void commHide(void);

UBYTE commIsShown(void);

tBitMap *commGetDisplayBuffer(void);

tUwCoordYX commGetOrigin(void);

tUwCoordYX commGetOriginDisplay(void);

void commDrawText(
	UWORD uwX, UWORD uwY, const char *szText, UBYTE ubFontFlags, UBYTE ubColor
);

/**
 * @brief Draws multiline text, dividing it where whitespace occurs.
 *
 * @param szText Text to display.
 * @param uwStartX Start X position of each line, relative to Commrade screen origin.
 * @param uwStartY Stary Y position of first line, relative to Commrade screen origin.
 * @return Number of lines written.
 */
UBYTE commDrawMultilineText(const char *szText, UWORD uwStartX, UWORD uwStartY);

void commDrawTitle(UWORD uwX, UWORD uwY, const char *szTitle);

void commErase(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight);

void commEraseAll(void);

// void commProgressInit(void);

// void commProgress(UBYTE ubPercent, const char *szDescription);

UBYTE commBreakTextToWidth(const char *szInput, UWORD uwMaxLineWidth);

UBYTE commGetLineHeight(void);

void commRegisterPage(tPageProcess cbProcess, tPageCleanup cbCleanup);

UBYTE commProcessPage(void);

#ifdef __cplusplus
}
#endif

#endif // COMM_COMM_H
