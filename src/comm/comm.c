/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <comm/comm.h>
#include <ace/managers/ptplayer.h>
#include <ace/managers/rand.h>
#include <ace/managers/system.h>
#include <ace/managers/key.h>
#include <ace/utils/chunky.h>
#include <ace/contrib/managers/audio_mixer.h>
#include "../game.h"
#include "../assets.h"

#define SFX_CHANNEL_KEY 0
#define COMM_BUTTON_LABEL_Y 170
#define COMM_BUTTON_LABEL_OFFICE_X 39
#define COMM_BUTTON_LABEL_WORKSHOP_X 115
#define COMM_BUTTON_LABEL_WAREHOUSE_X 201
#define COMM_BUTON_LABEL_COLOR_TEXT 9
#define COMM_BUTON_LABEL_COLOR_SHADOW 6
#define COMM_BUTON_LABEL_COLOR_BG 5
#define COMM_BUTON_LABEL_COLOR_CORNER 6
#define COMM_PROGRESS_BAR_WIDTH (COMM_DISPLAY_WIDTH - 20)
#define COMM_PROGRESS_BAR_HEIGHT 5
#define COMM_PROGRESS_BAR_BORDER_DISTANCE 2

static tBitMap *s_pBmEdgesMask;
static tBitMap *s_pBg;
static tBitMap *s_pBmDraw;
static UBYTE s_isCommShown = 0;
// static const char *s_szPrevProgressText;
static tPageProcess s_pPageProcess = 0;
static tPageCleanup s_pPageCleanup = 0;

tBitMap *g_pCommBmFaces;
tBitMap *g_pCommBmSelection;
tBitMap *g_pCommWorkshopIcons;

//------------------------------------------------------------ PRIVATE FUNCTIONS

static void fixNextBackgroundEdge(UWORD uwX, UWORD uwY, UWORD uwHeight) {
	tUwCoordYX sOrigin = commGetOrigin();
	UWORD uwBlitWidthInWords = 1;
	ULONG ulOffsBCD = s_pBmDraw->BytesPerRow * (uwY + sOrigin.uwY) + ((uwX + sOrigin.uwX) / 8);
	blitWait();
	g_pCustom->bltbpt = (UBYTE*)((ULONG)g_pGamePristineBuffer->Planes[0] + ulOffsBCD);
	g_pCustom->bltcpt = (UBYTE*)((ULONG)s_pBmDraw->Planes[0] + ulOffsBCD);
	g_pCustom->bltdpt = (UBYTE*)((ULONG)s_pBmDraw->Planes[0] + ulOffsBCD);
	g_pCustom->bltsize = ((uwHeight * GAME_BPP) << HSIZEBITS) | (uwBlitWidthInWords);
}

static void fixBackgroundEdges(void) {
	// A: edge mask, B: restore buffer, C/D: display buffer
	// Use C channel instead of A - same speed, less regs to set up
	UWORD uwBltCon0 = USEA|USEB|USEC|USED | MINTERM_COOKIE;
	UWORD uwBlitWidthInWords = 1;
	WORD wModuloBCD = bitmapGetByteWidth(s_pBmDraw) - (uwBlitWidthInWords << 1);

	blitWait();
	g_pCustom->bltafwm = 0xFFFF;
	g_pCustom->bltalwm = 0xFFFF;
	g_pCustom->bltcon0 = uwBltCon0;
	g_pCustom->bltcon1 = 0;

	g_pCustom->bltapt = (UBYTE*)((ULONG)s_pBmEdgesMask->Planes[0]);
	g_pCustom->bltamod = 0;
	g_pCustom->bltbmod = wModuloBCD;
	g_pCustom->bltcmod = wModuloBCD;
	g_pCustom->bltdmod = wModuloBCD;

	fixNextBackgroundEdge(0, 0, 5);
	fixNextBackgroundEdge(COMM_WIDTH - 16, 0, 4);
	fixNextBackgroundEdge(0, COMM_HEIGHT - 4, 4);
	fixNextBackgroundEdge(COMM_WIDTH - 16, COMM_HEIGHT - 5, 5);
}

//------------------------------------------------------------- PUBLIC FUNCTIONS

void commCreate(void) {
	systemUse();
	s_pBg = bitmapCreateFromPath("data/killfriend.bm", 0);
	s_pBmEdgesMask = bitmapCreateFromPath("data/killfriend_edges.bm", 0);
	systemUnuse();

	s_isCommShown = 0;
}

void commDestroy(void) {
	systemUse();

	bitmapDestroy(s_pBg);
	bitmapDestroy(s_pBmEdgesMask);

	bitmapDestroy(g_pCommBmFaces);
	bitmapDestroy(g_pCommBmSelection);
	bitmapDestroy(g_pCommWorkshopIcons);

	systemUnuse();
}

void commShow(void) {
	g_pGameBufferMain->pCamera->uPos.uwX &= 0xFFF0;
	tUwCoordYX sOrigin = commGetOrigin();
	s_isCommShown = 1;

	s_pBmDraw = g_pGameBufferMain->pBack;

	// Draw commrade background
	blitCopyAligned(
		s_pBg, 0, 0, s_pBmDraw, sOrigin.uwX, sOrigin.uwY,
		COMM_WIDTH, COMM_HEIGHT
	);

	fixBackgroundEdges();
}

void commProcess(void) {
}

void commHide(void) {
	if(!s_isCommShown) {
		return;
	}
	s_isCommShown = 0;

	commRegisterPage(0, 0);

	// Restore content beneath commrade
	tUwCoordYX sOrigin = commGetOrigin();
	blitCopyAligned(
		g_pGamePristineBuffer, sOrigin.uwX, sOrigin.uwY,
		s_pBmDraw, sOrigin.uwX, sOrigin.uwY,
		COMM_WIDTH, COMM_HEIGHT
	);
}

UBYTE commIsShown(void) {
	return s_isCommShown;
}

tBitMap *commGetDisplayBuffer(void) {
	return s_pBmDraw;
}

tUwCoordYX commGetOrigin(void) {
	UWORD uwScrollX = g_pGameBufferMain->pCamera->uPos.uwX;
	UWORD uwScrollY = g_pGameBufferMain->pCamera->uPos.uwY;

	tUwCoordYX sOrigin = {
		.uwX = uwScrollX + (GAME_MAIN_VPORT_SIZE_X - COMM_WIDTH) / 2,
		.uwY = uwScrollY + (GAME_MAIN_VPORT_SIZE_Y - COMM_HEIGHT) / 2
	};

	return sOrigin;
}

tUwCoordYX commGetOriginDisplay(void) {
	tUwCoordYX sOrigin = commGetOrigin();
	sOrigin.uwX += COMM_DISPLAY_X;
	sOrigin.uwY += COMM_DISPLAY_Y;
	return sOrigin;
}

void commDrawText(
	UWORD uwX, UWORD uwY, const char *szText, UBYTE ubFontFlags, UBYTE ubColor
) {
	const tUwCoordYX sOrigin = commGetOriginDisplay();
	fontDrawStr(
		g_pFont, s_pBmDraw, sOrigin.uwX + uwX, sOrigin.uwY + uwY,
		szText, ubColor, ubFontFlags, g_pGameLineBuffer
	);
}

UBYTE commDrawMultilineText(
	const char *szText, UWORD uwStartX, UWORD uwStartY
) {
	UWORD uwTextOffs = 0;
	UBYTE ubLinesWritten = 0;
	UWORD uwCurrY = uwStartY;
	UBYTE ubLineHeight = commGetLineHeight();
	char szLineBfr[50];
	do {
		// Measure chars in line
		UBYTE ubCharsInLine = commBreakTextToWidth(
			&szText[uwTextOffs], COMM_DISPLAY_WIDTH - uwStartX
		);

		// Copy to line buffer and draw
		memcpy(szLineBfr, &szText[uwTextOffs], ubCharsInLine);
		szLineBfr[ubCharsInLine] = '\0';
		if(szLineBfr[ubCharsInLine - 1] == '\n') {
			szLineBfr[ubCharsInLine - 1] = ' ';
		}
		commDrawText(uwStartX, uwCurrY, szLineBfr, FONT_COOKIE, COMM_DISPLAY_COLOR_TEXT);

		// Advance
		uwTextOffs += ubCharsInLine;
		++ubLinesWritten;
		uwCurrY += ubLineHeight;
	} while(szText[uwTextOffs] != '\0');

	return ubLinesWritten;
}

void commDrawTitle(UWORD uwX, UWORD uwY, const char *szTitle) {
	// Clear old contents
	if(g_pGameLineBuffer->uwActualWidth) {
		blitRect(
			g_pGameLineBuffer->pBitMap, 0, 0,
			g_pGameLineBuffer->uwActualWidth, g_pGameLineBuffer->pBitMap->Rows, 0
		);
	}

	char szChar[2] = {0};
	UWORD uwEndX = 0;
	for(const char *pNextChar = szTitle; *pNextChar != '\0'; ++pNextChar) {
		// Prepare "bold" letter
		szChar[0] = *pNextChar;
		uwEndX = fontDrawStr1bpp(g_pFont, g_pGameLineBuffer->pBitMap, uwEndX, 0, szChar).uwX + 1;
	}
	g_pGameLineBuffer->uwActualWidth = uwEndX - 1;

	const tUwCoordYX sOrigin = commGetOriginDisplay();
	fontDrawTextBitMap(
		s_pBmDraw, g_pGameLineBuffer, sOrigin.uwX + uwX, sOrigin.uwY + uwY,
		COMM_DISPLAY_COLOR_TEXT, FONT_COOKIE
	);
	fontDrawTextBitMap(
		s_pBmDraw, g_pGameLineBuffer, sOrigin.uwX + uwX + 1, sOrigin.uwY + uwY,
		COMM_DISPLAY_COLOR_TEXT, FONT_COOKIE
	);
}

UBYTE commGetLineHeight(void) {
	return g_pFont->uwHeight + 1;
}

void commErase(UWORD uwX, UWORD uwY, UWORD uwWidth, UWORD uwHeight) {
#if defined(COMM_DEBUG)
	static UBYTE ubDebugColor = 0;
	const UBYTE ubClearColor = ubDebugColor;
	ubDebugColor = (ubDebugColor + 1) & 31;
#else
	const UBYTE ubClearColor = COMM_DISPLAY_COLOR_BG;
#endif
	const tUwCoordYX sPosDisplay = commGetOriginDisplay();
	blitRect(
		s_pBmDraw, sPosDisplay.uwX + uwX, sPosDisplay.uwY + uwY,
		uwWidth, uwHeight, ubClearColor
	);
}

void commEraseAll(void) {
	commErase(0, 0, COMM_DISPLAY_WIDTH, COMM_DISPLAY_HEIGHT);
}

// void commProgressInit(void) {
// 	if(!commIsShown()) {
// 		return;
// 	}

// 	s_szPrevProgressText = 0;

// 	tUwCoordYX sBarPos = commGetOriginDisplay();
// 	sBarPos.uwX += (COMM_DISPLAY_WIDTH - COMM_PROGRESS_BAR_WIDTH) / 2;
// 	sBarPos.uwY += (COMM_DISPLAY_HEIGHT + g_pFont->uwHeight) / 2 + 2 * COMM_PROGRESS_BAR_BORDER_DISTANCE;
// 	s_sProgressBarConfig.sBarPos = sBarPos;
// 	progressBarInit(&s_sProgressBarConfig, s_pBmDraw);
// }

// void commProgress(UBYTE ubPercent, const char *szDescription) {
// 	if(!commIsShown()) {
// 		return;
// 	}

// 	logWrite("Comm Progress: %hhu (%s)\n", ubPercent, szDescription ? szDescription : "???");

// 	if(szDescription != s_szPrevProgressText) {
// 		UWORD uwFontPosY = (COMM_DISPLAY_HEIGHT - g_pFont->uwHeight) / 2;
// 		commErase(0, uwFontPosY, COMM_DISPLAY_WIDTH, g_pFont->uwHeight);
// 		commDrawText(
// 			COMM_DISPLAY_WIDTH / 2, uwFontPosY, szDescription,
// 			FONT_HCENTER | FONT_COOKIE | FONT_LAZY,
// 			COMM_DISPLAY_COLOR_TEXT
// 		);
// 		s_szPrevProgressText = szDescription;
// 	}

// 	progressBarAdvance(&s_sProgressBarConfig, s_pBmDraw, ubPercent);
// }

UBYTE commBreakTextToWidth(const char *szInput, UWORD uwMaxLineWidth) {
	UWORD uwLineWidth = 0;
	UBYTE ubCharsInLine = 0;
	UBYTE ubLastSpace = 0xFF; // next char, actually

	while(*szInput != '\0') {
		UBYTE ubCharWidth = fontGlyphWidth(g_pFont, *szInput) + 1;

		if(uwLineWidth + ubCharWidth >= uwMaxLineWidth) {
			if(ubLastSpace != 0xFF) {
				ubCharsInLine = ubLastSpace;
			}

			break;
		}

		uwLineWidth += ubCharWidth;
		ubCharsInLine++;

		if(*szInput == ' ') {
			ubLastSpace = ubCharsInLine;
		}
		else if(*szInput == '\r') {
			logWrite("ERR: CR character detected - use LF line endings!");
			break;
		}
		else if(*szInput == '\n') {
			break;
		}

		++szInput;
	}
	return ubCharsInLine;
}

void commRegisterPage(tPageProcess cbProcess, tPageCleanup cbCleanup) {
	logBlockBegin(
		"commRegisterPage(cbProcess: %p, cbCleanup: %p)", cbProcess, cbCleanup
	);
	if(s_pPageCleanup) {
		s_pPageCleanup();
		s_pPageCleanup = 0;
	}
	commEraseAll();

	s_pPageProcess = cbProcess;
	s_pPageCleanup = cbCleanup;
	logBlockEnd("commRegisterPage()");
}

UBYTE commProcessPage(void) {
	if(s_pPageProcess) {
		s_pPageProcess();
		return 1;
	}
	return 0;
}
