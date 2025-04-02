#include "button.h"
#include <string.h>
#include <ace/managers/mouse.h>
#include <ace/utils/font.h>
#include <ace/utils/string.h>
#include <comm/comm.h>
#include "../assets.h"
#include "../game.h"

#define BUTTON_COUNT_MAX 5

// Chars used in buttons don't take up most of the vertical font space
#define BORDER_LR_DEPTH 2
#define TEXT_PADDING_X 1
#define TEXT_PADDING_Y 1
#define TEXT_HEIGHT_DECREMENT 3
#define TEXT_BASELINE_POS 8
#define TEXT_CAP_POS 3
#define TEXT_HEIGHT (TEXT_BASELINE_POS - TEXT_CAP_POS)

static tButton s_pButtons[BUTTON_COUNT_MAX];
static UBYTE s_ubButtonCount;
static UBYTE s_ubSelected;

void buttonReset(void) {
	for(UBYTE i = BUTTON_COUNT_MAX; i--;) {
		s_pButtons[i].szName[0] = '\0';
	}
	s_ubButtonCount = 0;
	buttonDeselectAll();
}

UBYTE buttonAdd(const char *szName, UWORD uwX, UWORD uwY) {
	if(s_ubButtonCount >= BUTTON_COUNT_MAX) {
		return BUTTON_INVALID;
	}

	UBYTE ubButtonIndex = s_ubButtonCount++;
	tButton *pButton = &s_pButtons[ubButtonIndex];
	stringCopy(szName, pButton->szName);
	const tUwCoordYX sOrigin = commGetOriginDisplay();
	pButton->uwWidth = buttonGetWidth(ubButtonIndex);
	pButton->sPos.uwX = sOrigin.uwX + uwX - pButton->uwWidth / 2;
	pButton->sPos.uwY = sOrigin.uwY + uwY - buttonGetHeight() / 2;
	return ubButtonIndex;
}

void buttonDraw(UBYTE ubIdx, tBitMap *pBfr) {
	if(ubIdx >= s_ubButtonCount) {
		return;
	}

	UBYTE ubColor = (
		ubIdx == s_ubSelected ? COMM_DISPLAY_COLOR_TEXT_HOVER : COMM_DISPLAY_COLOR_TEXT_DARK
	);
	const tButton *pButton = &s_pButtons[ubIdx];
	tUwCoordYX sSize = {.uwX = pButton->uwWidth, .uwY = buttonGetHeight()};
	UWORD uwBtnX = pButton->sPos.uwX;
	UWORD uwBtnY = pButton->sPos.uwY - TEXT_PADDING_Y;
	tUwCoordYX sOrigin = commGetOriginDisplay();
	blitRect( // left line
		pBfr, uwBtnX, uwBtnY, 1, sSize.uwY, ubColor
	);
	blitRect( // top-left line
		pBfr, uwBtnX, uwBtnY, BORDER_LR_DEPTH, 1, ubColor
	);
	blitRect( // bottom-left line
		pBfr, uwBtnX, uwBtnY + sSize.uwY - 1, BORDER_LR_DEPTH, 1, ubColor
	);
	blitRect( // right line
		pBfr, uwBtnX + sSize.uwX - 1, uwBtnY, 1, sSize.uwY, ubColor
	);
	blitRect( // top-right line
		pBfr, uwBtnX + sSize.uwX - BORDER_LR_DEPTH, uwBtnY,
		BORDER_LR_DEPTH, 1, ubColor
	);
	blitRect( // bottom-right line
		pBfr, uwBtnX + sSize.uwX - BORDER_LR_DEPTH, uwBtnY + sSize.uwY - 1,
		BORDER_LR_DEPTH, 1, ubColor
	);
	commDrawText(
		uwBtnX - sOrigin.uwX + 3, uwBtnY - sOrigin.uwY - TEXT_CAP_POS + TEXT_PADDING_Y,
		pButton->szName, FONT_COOKIE, ubColor
	);
}

void buttonDrawAll(tBitMap *pBfr) {
	for(UBYTE i = 0; i < s_ubButtonCount; ++i) {
		buttonDraw(i, pBfr);
	}
}

void buttonDeselectAll(void) {
	s_ubSelected = BUTTON_INVALID;
}

UBYTE buttonSelect(UBYTE ubIdx) {
	if(ubIdx >= s_ubButtonCount) {
		return 0;
	}
	s_ubSelected = ubIdx;
	return 1;
}

UBYTE buttonGetCount(void) {
	return s_ubButtonCount;
}

UBYTE buttonGetSelected(void) {
	return s_ubSelected;
}

UWORD buttonGetWidth(UBYTE ubIndex) {
	const tButton *pButton = &s_pButtons[ubIndex];
	UWORD uwWidth = fontMeasureText(g_pFont, pButton->szName).uwX + (BORDER_LR_DEPTH + TEXT_PADDING_X) * 2;
	return uwWidth;
}

UBYTE buttonGetHeight(void) {
	return TEXT_HEIGHT + 2 * TEXT_PADDING_Y;
}

void buttonProcess(UWORD uwCursorX, UWORD uwCursorY, tBitMap *pBfr) {
	UBYTE ubPrevSelected = s_ubSelected;
	s_ubSelected = BUTTON_INVALID;
	uwCursorX += g_pGameBufferMain->pCamera->uPos.uwX;
	uwCursorY += g_pGameBufferMain->pCamera->uPos.uwY;
	for(UBYTE i = 0; i < s_ubButtonCount; ++i) {
		tButton *pButton = &s_pButtons[i];
		if(
			pButton->sPos.uwX <= uwCursorX && uwCursorX <= pButton->sPos.uwX + pButton->uwWidth &&
			pButton->sPos.uwY <= uwCursorY && uwCursorY <= pButton->sPos.uwY + buttonGetHeight()
		) {
			s_ubSelected = i;
		}
	}

	if(s_ubSelected != ubPrevSelected) {
		buttonDraw(ubPrevSelected, pBfr);
		buttonDraw(s_ubSelected, pBfr);
	}
}
