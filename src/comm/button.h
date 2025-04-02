#ifndef BUTTON_H
#define BUTTON_H

#include <ace/types.h>
#include <ace/utils/font.h>

#define BUTTON_NAME_MAX 20
#define BUTTON_INVALID 0xFF

typedef struct _tButton {
	char szName[BUTTON_NAME_MAX];
	UWORD uwWidth;
	tUwCoordYX sPos; ///< Top-left of the button
} tButton;

void buttonReset(void);

UBYTE buttonAdd(const char *szName, UWORD uwX, UWORD uwY);

void buttonDraw(UBYTE ubIdx, tBitMap *pBfr);

void buttonDrawAll(tBitMap *pBfr);

UBYTE buttonSelect(UBYTE ubIdx);

void buttonDeselectAll(void);

UBYTE buttonGetCount(void);

UBYTE buttonGetSelected(void);

UWORD buttonGetWidth(UBYTE ubIndex);

UBYTE buttonGetHeight(void);

void buttonProcess(UWORD uwCursorX, UWORD uwCursorY, tBitMap *pBfr);

#endif // BUTTON_H
