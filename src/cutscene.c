/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/system.h>
#include <ace/managers/key.h>
#include <ace/managers/mouse.h>
#include <ace/utils/disk_file.h>
#include <ace/utils/palette.h>
#include <ace/generic/screen.h>
#include "fade.h"
#include "assets.h"
// #include "music.h"
#include "game.h"
#include "assets.h"
#include "survivor.h"

#define SLIDES_MAX 10
#define LINES_PER_SLIDE_MAX 6
#define SLIDE_SIZE 128
#define SLIDE_POS_X ((SCREEN_PAL_WIDTH - SLIDE_SIZE) / 2)
#define SLIDE_POS_Y ((SCREEN_PAL_HEIGHT - SLIDE_SIZE) / 4)
#define TEXT_POS_X (SCREEN_PAL_WIDTH / 2)
#define TEXT_POS_Y (SLIDE_POS_Y + SLIDE_SIZE + 16)
#define TEXT_FINALE_POS_Y (SCREEN_PAL_HEIGHT / 2)
#define TEXT_LINE_HEIGHT (10 + 2)
#define SLIDE_ANIM_COLS 2
#define COLOR_TEXT 18

#define GRAYSCALE(ubValue) ((ubValue) << 8 | (ubValue) << 4 | (ubValue))

typedef struct tSlide {
	tBitMap *pBitmap;
	UWORD pPalette[32];
} tSlide;

static const char *s_pLinesIntro[][LINES_PER_SLIDE_MAX] = {
	[0] = {
		"The world ended when that damned rock",
		"hit the Earth and brought us the virus",
		"from gods knows what dimension...",
		0,
	},
	[1] = {
		"Scientists wanted to study it of course,",
		"but they only \"accidentally\" released",
		"it from the lab.",
		"Just like in the movies...",
		0,
	},
	[2] = {
		"That damned virus spreaded like",
		"a wildfire throughout the entire planet",
		"and nobody could do a damn thing about it.",
		"Not that they didn't try.",
		0,
	},
	[3] = {
		"The virus got into people's minds",
		"and messed them up real good.",
		"Made them fuckin' cultists or somethin'.",
		"My friends, family... all gone,",
		"mumblin' foul chants, eatin' people alive...",
		0,
	},
	[4] = {
		"I'm not gonna make it either, I know that.",
		"But I'm goin' to take as many of those",
		"fuckers as I can with me to hell.",
		"Ellen, one of the last survivors",
		"on Earth, signin' out.",
		0,
	},
};

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBuffer;
static tFade *s_pFade;
static UBYTE s_ubSlideCount, s_ubCurrentSlide, s_ubCurrentLine;
static UWORD s_uwFontColorVal;
static UBYTE s_ubFadeStep;

static tSlide s_pSlides[SLIDES_MAX];

static tCopBlock *s_pBlockAboveLine, *s_pBlockBelowLine, *s_pBlockAfterLines;
static tPtplayerMod *s_pModIntro;

static const char *getLine(UBYTE ubSlide, UBYTE ubLine) {
	const char *pLine = s_pLinesIntro[ubSlide][ubLine];
	return pLine;
}

static void drawSlide(void) {
	// Draw slide
	blitCopyAligned(
		s_pSlides[s_ubCurrentSlide].pBitmap, 0, 0, s_pBuffer->pBack,
		SLIDE_POS_X, SLIDE_POS_Y, SLIDE_SIZE, SLIDE_SIZE
	);

	// Erase old text
	blitRect(
		s_pBuffer->pBack, 0, TEXT_POS_Y,
		SCREEN_PAL_WIDTH, LINES_PER_SLIDE_MAX * TEXT_LINE_HEIGHT, 0
	);

	// Load new palette
	fadeChangeRefPalette(s_pFade, s_pSlides[s_ubCurrentSlide].pPalette, 32);

	// Update the color used by copper to match the palette
	s_pBlockAfterLines->pCmds[0].sMove.bfValue = s_pSlides[s_ubCurrentSlide].pPalette[COLOR_TEXT];
}

static void initSlideText(void) {
	// Reset copblocks
	s_ubCurrentLine = 0;
	copBlockWait(
		s_pView->pCopList, s_pBlockAboveLine, 0,
		s_pView->ubPosY + TEXT_POS_Y + TEXT_LINE_HEIGHT * s_ubCurrentLine
	);
	copBlockWait(
		s_pView->pCopList, s_pBlockBelowLine, 0,
		s_pView->ubPosY + TEXT_POS_Y + TEXT_LINE_HEIGHT * (s_ubCurrentLine + 1) - 1
	);
	s_pBlockAboveLine->uwCurrCount = 0;
	copMove(
		s_pView->pCopList, s_pBlockAboveLine,
		&g_pCustom->color[COLOR_TEXT], 0x000
	);
	copProcessBlocks();
	vPortWaitForEnd(s_pVp);
	s_ubFadeStep = 0;

	// Draw text
	while(getLine(s_ubCurrentSlide, s_ubCurrentLine)) {
		const char *szLine = getLine(s_ubCurrentSlide, s_ubCurrentLine);
		// Draw next portion of text
		fontDrawStr(
			g_pFont, s_pBuffer->pBack, TEXT_POS_X,
			TEXT_POS_Y + TEXT_LINE_HEIGHT * s_ubCurrentLine, szLine,
			COLOR_TEXT, FONT_LAZY | FONT_HCENTER, g_pLineBuffer
		);
		++s_ubCurrentLine;
	}

	s_ubCurrentLine = 0;
}

static void onFadeIn(void) {
	copBlockEnable(s_pView->pCopList, s_pBlockAboveLine);
	copBlockEnable(s_pView->pCopList, s_pBlockBelowLine);
	copBlockEnable(s_pView->pCopList, s_pBlockAfterLines);
	initSlideText();
}

static void cutsceneGsCreate(void) {
	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
	TAG_END);

	s_pVp = vPortCreate(0,
		TAG_VPORT_BPP, 5,
		TAG_VPORT_VIEW, s_pView,
	TAG_END);

	s_pBuffer = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_VPORT, s_pVp,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
	TAG_END);

	s_ubCurrentSlide = 0;
	s_ubCurrentLine = 0;
	s_pModIntro = ptplayerModCreateFromPath("data/intro.mod");

	// Load palette
	UWORD pPalette[32];
	paletteLoadFromPath("data/intro.plt", pPalette, 32);
	s_pFade = fadeCreate(s_pView, pPalette, 32);

	s_uwFontColorVal = pPalette[COLOR_TEXT];
	s_pBlockAboveLine = copBlockCreate(s_pView->pCopList, 1, 0, 0);
	s_pBlockBelowLine = copBlockCreate(s_pView->pCopList, 1, 0, 0);
	s_pBlockAfterLines = copBlockCreate(
		s_pView->pCopList, 1, 0,
		s_pView->ubPosY + TEXT_POS_Y + (LINES_PER_SLIDE_MAX - 1) * TEXT_LINE_HEIGHT
	);
	copMove(
		s_pView->pCopList, s_pBlockBelowLine,
		&g_pCustom->color[COLOR_TEXT], 0x000
	);
	copMove(
		s_pView->pCopList, s_pBlockAfterLines,
		&g_pCustom->color[COLOR_TEXT], s_uwFontColorVal
	);
	copBlockDisable(s_pView->pCopList, s_pBlockAboveLine);
	copBlockDisable(s_pView->pCopList, s_pBlockBelowLine);
	copBlockDisable(s_pView->pCopList, s_pBlockAfterLines);

	// Load slides
	char szPath[30];
	s_ubSlideCount = 0;
	for(s_ubSlideCount = 0; s_ubSlideCount < 10; ++s_ubSlideCount) {
		sprintf(szPath, "data/intro/%hhu.bm", s_ubSlideCount);
		if(!diskFileExists(szPath)) {
			break;
		}
		s_pSlides[s_ubSlideCount].pBitmap = bitmapCreateFromPath(szPath, 0);
		sprintf(szPath, "data/intro/%hhu.plt", s_ubSlideCount);
		paletteLoadFromPath(szPath, s_pSlides[s_ubSlideCount].pPalette, 32);
		s_pSlides[s_ubSlideCount].pPalette[COLOR_TEXT] = s_uwFontColorVal;
	}

	systemUnuse();
	ptplayerLoadMod(s_pModIntro, 0, 0); // g_pModSamples
	ptplayerEnableMusic(1);

	// Draw first slide
	drawSlide();
	fadeSet(s_pFade, FADE_STATE_IN, 50, 1, onFadeIn);
	viewLoad(s_pView);
}

static void onCutsceneFadeOut(void) {
	ptplayerStop();
	ptplayerSetMasterVolume(32);
	stateChange(g_pGameStateManager, &g_sStateGame);
}

static void onFadeOutSlide(void) {
	drawSlide();
	fadeSet(s_pFade, FADE_STATE_IN, 15, 0, onFadeIn);
}

static void cutsceneLoopFinale(void) {
	tFadeState eFadeState = fadeProcess(s_pFade);
	if(eFadeState != FADE_STATE_IDLE) {
		return;
	}

	vPortWaitForEnd(s_pVp);
	if(s_ubFadeStep <= 0x10) {
		// Process text fade-in
		// Increment color
		s_pBlockAboveLine->uwCurrCount = 0;
		copMove(
			s_pView->pCopList, s_pBlockAboveLine, &g_pCustom->color[COLOR_TEXT],
			s_ubFadeStep < 0x10 ? GRAYSCALE(s_ubFadeStep) : s_uwFontColorVal
		);
		++s_ubFadeStep;

		// Refresh copperlist
		copProcessBlocks();
	}
	else if(s_ubCurrentLine < 3) {
		// Start fade-in for next line
		++s_ubCurrentLine;
		if(s_ubCurrentLine < 3) {
			// Draw next portion of text - move copBlocks and reset fadeStep
			copBlockWait(
				s_pView->pCopList, s_pBlockAboveLine, 0,
				s_pView->ubPosY + TEXT_FINALE_POS_Y + TEXT_LINE_HEIGHT * 2 * (s_ubCurrentLine - 1)
			);
			copBlockWait(
				s_pView->pCopList, s_pBlockBelowLine, 0,
				s_pView->ubPosY + TEXT_FINALE_POS_Y + TEXT_LINE_HEIGHT * 2 * (s_ubCurrentLine - 1 + 1) - 1
			);
			s_pBlockAboveLine->uwCurrCount = 0;
			copMove(
				s_pView->pCopList, s_pBlockAboveLine,
				&g_pCustom->color[COLOR_TEXT], 0x000
			);
			copProcessBlocks();
			s_ubFadeStep = 0;
			for(UBYTE i = 0; i < 50; ++i) {
				vPortWaitForEnd(s_pVp);
			}
		}
		else {
			// Last line was displayed - copblocks are no longer needed
			copBlockDisable(s_pView->pCopList, s_pBlockAboveLine);
			copBlockDisable(s_pView->pCopList, s_pBlockBelowLine);
			copBlockDisable(s_pView->pCopList, s_pBlockAfterLines);
			copProcessBlocks();
		}
	}
	else if(
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		mouseUse(MOUSE_PORT_1, MOUSE_LMB)
	) {
		// Quit the cutscene
		fadeSet(s_pFade, FADE_STATE_OUT, 50, 1, onCutsceneFadeOut);
	}
}

static void cutsceneGsLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFade);
	if(eFadeState != FADE_STATE_IDLE) {
		return;
	}

	vPortWaitForEnd(s_pVp);
	if(s_ubFadeStep <= 0x10) {
		// Process text fade-in
		// Increment color
		s_pBlockAboveLine->uwCurrCount = 0;
		copMove(
			s_pView->pCopList, s_pBlockAboveLine, &g_pCustom->color[COLOR_TEXT],
			s_ubFadeStep < 0x10 ? GRAYSCALE(s_ubFadeStep) : s_uwFontColorVal
		);
		++s_ubFadeStep;

		// Refresh copperlist
		copProcessBlocks();
	}
	else if(getLine(s_ubCurrentSlide, s_ubCurrentLine)) {
		// Start fade-in for next line
		++s_ubCurrentLine;
		if(getLine(s_ubCurrentSlide, s_ubCurrentLine)) {
			// Draw next portion of text - move copBlocks and reset fadeStep
			copBlockWait(
				s_pView->pCopList, s_pBlockAboveLine, 0,
				s_pView->ubPosY + TEXT_POS_Y + TEXT_LINE_HEIGHT * s_ubCurrentLine
			);
			copBlockWait(
				s_pView->pCopList, s_pBlockBelowLine, 0,
				s_pView->ubPosY + TEXT_POS_Y + TEXT_LINE_HEIGHT * (s_ubCurrentLine + 1) - 1
			);
			s_pBlockAboveLine->uwCurrCount = 0;
			copMove(
				s_pView->pCopList, s_pBlockAboveLine,
				&g_pCustom->color[COLOR_TEXT], 0x000
			);
			copProcessBlocks();
			s_ubFadeStep = 0;
		}
		else {
			// Last line was displayed - copblocks are no longer needed
			copBlockDisable(s_pView->pCopList, s_pBlockAboveLine);
			copBlockDisable(s_pView->pCopList, s_pBlockBelowLine);
			copBlockDisable(s_pView->pCopList, s_pBlockAfterLines);
			copProcessBlocks();
		}
	}
	else if(
		keyUse(KEY_RETURN) || keyUse(KEY_LSHIFT) || keyUse(KEY_RSHIFT) ||
		mouseUse(MOUSE_PORT_1, MOUSE_LMB)
	) {
		if(++s_ubCurrentSlide < s_ubSlideCount) {
			// Draw next slide
			fadeSet(s_pFade, FADE_STATE_OUT, 15, 0, onFadeOutSlide);
		}
		else {
			// Quit the cutscene
			fadeSet(s_pFade, FADE_STATE_OUT, 50, 1, onCutsceneFadeOut);
		}
	}
}

static void cutsceneGsDestroy(void) {
	viewLoad(0);
	systemUse();
	viewDestroy(s_pView);
	fadeDestroy(s_pFade);

	// Destroy slides
	for(UBYTE i = 0; i < s_ubSlideCount; ++i) {
		bitmapDestroy(s_pSlides[i].pBitmap);
	}

	ptplayerModDestroy(s_pModIntro);
}

tState g_sStateCutscene = {
	.cbCreate = cutsceneGsCreate, .cbLoop = cutsceneGsLoop,
	.cbDestroy = cutsceneGsDestroy
};
