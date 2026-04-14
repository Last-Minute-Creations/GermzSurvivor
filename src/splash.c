/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "splash.h"
#include <ace/managers/viewport/simplebuffer.h>
#include <ace/managers/game.h>
#include <ace/managers/system.h>
#include <ace/managers/joy.h>
#include <ace/managers/key.h>
#include <ace/managers/blit.h>
#include <ace/utils/palette.h>
#include <ace/managers/ptplayer.h>
#include "cutscene.h"
#include "game.h"
#include "fade.h"
#include "survivor.h"

#define FLASH_START_FRAME_A 1
#define FLASH_START_FRAME_C 10
#define FLASH_START_FRAME_E 30
#define FLASH_START_FRAME_PWR 50
#define FLASH_RATIO_INACTIVE -1

typedef enum tStateAce {
	STATE_ACE_FADE_IN,
	STATE_ACE_FADE_OUT
} tStateAce;

static tView *s_pView;
static tVPort *s_pVp;
static tSimpleBufferManager *s_pBfr;
static UBYTE s_ubWaitFrame = 0;

static UBYTE s_isAnyPressed = 0;
static UBYTE s_isEscapePressed = 0;
static tPtplayerSfx *s_pSfxLmc, *s_pSfxAce;

static tState s_sStateSplashLmc;
static tState s_sStateSplashAce;

static tStateManager *s_pStateMachineSplash;
static tFade *s_pFade;
static tStateAce s_eStateAce;

static tCopBlock *s_pAceBlocks[30];

static const UWORD s_pAceColors[] = {
  0xA00, 0xA00,
  0xC00, 0xC00, 0xC00,
  0xD40, 0xD40,
  0xE80, 0xE80, 0xE80,
  0xFB2, 0xFB2,
  0xFE5, 0xFE5, 0xFE5,
  0xCC2, 0xCC2,
  0x990, 0x990, 0x990,
  0x494, 0x494,
  0x099, 0x099, 0x099,
  0x077, 0x077,
  0x055, 0x055,
  0x055
};

static const UWORD s_uwColorPowered = 0x343;
static UWORD s_uwFlashFrame;
static BYTE s_bAceFadeoutRatio;
static BYTE s_bRatioFlashA;
static BYTE s_bRatioFlashC;
static BYTE s_bRatioFlashE;
static BYTE s_bRatioFlashPwr;
static tUwRect s_sSplashRect;

static void splashGsCreate(void) {
	logBlockBegin("splashGsCreate()");

	s_pView = viewCreate(0,
		TAG_VIEW_GLOBAL_PALETTE, 1,
	TAG_END);

	s_pVp = vPortCreate(0,
		TAG_VPORT_BPP, 3,
		TAG_VPORT_VIEW, s_pView,
	TAG_END);

	s_pBfr = simpleBufferCreate(0,
		TAG_SIMPLEBUFFER_BITMAP_FLAGS, BMF_CLEAR | BMF_INTERLEAVED,
		TAG_SIMPLEBUFFER_VPORT, s_pVp,
		TAG_SIMPLEBUFFER_IS_DBLBUF, 0,
		TAG_SIMPLEBUFFER_USE_X_SCROLLING, 0,
	TAG_END);

	s_pFade = fadeCreate(s_pView, 0, 0);
	s_pStateMachineSplash = stateManagerCreate();
	stateChange(s_pStateMachineSplash, &s_sStateSplashLmc);

	logBlockEnd("splashGsCreate()");
	systemUnuse();
	viewLoad(s_pView);
}

static void splashGsLoop(void) {
	s_isAnyPressed = (
		keyUse(KEY_RETURN) | keyUse(KEY_SPACE) |
		keyUse(KEY_LSHIFT) | keyUse(KEY_RSHIFT) |
		joyUse(JOY1 + JOY_FIRE) | joyUse(JOY2 + JOY_FIRE)
	);
	if(!s_isEscapePressed) {
		s_isEscapePressed = keyUse(KEY_ESCAPE);
	}

	stateProcess(s_pStateMachineSplash);
}

static void splashGsDestroy(void) {
	// this must be here or else state destruction will be called when os is alive and it shouldn't be
	statePopAll(s_pStateMachineSplash);

	systemUse();
	logBlockBegin("splashGsDestroy()");
	stateManagerDestroy(s_pStateMachineSplash);
	fadeDestroy(s_pFade);
	viewDestroy(s_pView);
	logBlockEnd("splashGsDestroy()");
}

//-------------------------------------------------------------------------- LMC

static void onLmcFadeOut(void)
{
	if(s_isEscapePressed) {
		ptplayerStop();
		ptplayerSetMasterVolume(PTPLAYER_MASTER_VOLUME);
		stateChange(g_pGameStateManager, &g_sStateGame);
	}
	else {
		stateChange(s_pStateMachineSplash, &s_sStateSplashAce);
	}
}

static void splashLmcCreate(void) {
	systemUse();
	UWORD pPaletteRef[32];
	paletteLoadFromPath("data/splash/lmc.plt", pPaletteRef, 1 << s_pVp->ubBpp);
	tBitMap *pSplash = bitmapCreateFromPath("data/splash/lmc.bm", 0);
	s_pSfxLmc = ptplayerSfxCreateFromPath("data/splash/lmc.sfx", 0);
	systemUnuse();

	s_sSplashRect.uwWidth = bitmapGetByteWidth(pSplash) * 8;
	s_sSplashRect.uwHeight = pSplash->Rows;
	blitCopy(
		pSplash, 0, 0, s_pBfr->pBack,
		(s_pVp->uwWidth - s_sSplashRect.uwWidth) / 2,
		(s_pVp->uwHeight - s_sSplashRect.uwHeight) / 2,
		s_sSplashRect.uwWidth, s_sSplashRect.uwHeight, MINTERM_COOKIE
	);

	systemUse();
	bitmapDestroy(pSplash);
	systemUnuse();
	fadeChangeRefPalette(s_pFade, pPaletteRef, 1 << s_pVp->ubBpp);
	fadeSet(s_pFade, FADE_STATE_IN, 50, 0, 0);
}

static void splashLmcLoop(void) {
	tFadeState eFadeState = fadeProcess(s_pFade);
	if(eFadeState == FADE_STATE_EVENT_FIRED) {
		// state may have quit here
		return;
	}

	if(
		(eFadeState == FADE_STATE_IN || eFadeState == FADE_STATE_IDLE) &&
		(s_isAnyPressed || s_isEscapePressed)
	) {
		fadeSet(s_pFade, FADE_STATE_OUT, 50, 1, onLmcFadeOut);
	}
	else if(eFadeState == FADE_STATE_IDLE) {
		++s_ubWaitFrame;

		if(s_ubWaitFrame >= 100 || s_isAnyPressed || s_isEscapePressed) {
			fadeSet(s_pFade, FADE_STATE_OUT, 50, 1, onLmcFadeOut);
		}
		else if(s_ubWaitFrame == 1){
			ptplayerSfxPlay(s_pSfxLmc, 0, PTPLAYER_VOLUME_MAX, 1);
			// fadeSet(s_pFade, FADE_STATE_OUT, 50, 1, onLmcFadeOut); // FOR DEBUGGING SFX GLITCHES
		}
	}

	vPortWaitForEnd(s_pVp);
}

static void splashLmcDestroy(void) {
	// Clear after LMC logo display
	blitRect(
		s_pBfr->pBack,
		(s_pVp->uwWidth - s_sSplashRect.uwWidth) / 2,
		(s_pVp->uwHeight - s_sSplashRect.uwHeight) / 2,
		s_sSplashRect.uwWidth, s_sSplashRect.uwHeight, 0
	);

	ptplayerWaitForSfx();
	ptplayerSfxDestroy(s_pSfxLmc);
}

//-------------------------------------------------------------------------- ACE

static UWORD blendColors(UWORD uwColorSrc, UWORD uwColorDst, UBYTE ubRatio)
{
  BYTE bSrcR = (uwColorSrc >> 8);
  BYTE bSrcG = ((uwColorSrc >> 4) & 0xF);
  BYTE bSrcB = ((uwColorSrc >> 0) & 0xF);
  BYTE bDstR = (uwColorDst >> 8);
  BYTE bDstG = ((uwColorDst >> 4) & 0xF);
  BYTE bDstB = ((uwColorDst >> 0) & 0xF);
  UBYTE ubCurrentR = bSrcR + ((bDstR - bSrcR) * ubRatio) / 15;
  UBYTE ubCurrentG = bSrcG + ((bDstG - bSrcG) * ubRatio) / 15;
  UBYTE ubCurrentB = bSrcB + ((bDstB - bSrcB) * ubRatio) / 15;
  UWORD uwColorOut = (ubCurrentR << 8) | (ubCurrentG << 4) | ubCurrentB;
  return uwColorOut;
}

static void splashAceCreate(void) {
	systemUse();

	tBitMap *pSplashAce = bitmapCreateFromPath("data/splash/ace.bm", 0);
	s_sSplashRect.uwWidth = bitmapGetByteWidth(pSplashAce) * 8;
	s_sSplashRect.uwHeight = pSplashAce->Rows;
	UWORD uwSplashOffsY = (256 - s_sSplashRect.uwHeight) / 2;

	for(UBYTE i = 0; i < 30; ++i) {
    s_pAceBlocks[i] = copBlockCreate(s_pView->pCopList, 3, 0, s_pView->ubPosY + uwSplashOffsY + i * 2);
    copMove(s_pView->pCopList, s_pAceBlocks[i], &g_pCustom->color[1], 0x000);
    copMove(s_pView->pCopList, s_pAceBlocks[i], &g_pCustom->color[2], 0x000);
    copMove(s_pView->pCopList, s_pAceBlocks[i], &g_pCustom->color[3], 0x000);
  }

	s_uwFlashFrame = 1;
	s_bRatioFlashA = FLASH_RATIO_INACTIVE;
	s_bRatioFlashC = FLASH_RATIO_INACTIVE;
	s_bRatioFlashE = FLASH_RATIO_INACTIVE;
	s_bRatioFlashPwr = FLASH_RATIO_INACTIVE;

	s_pSfxAce = ptplayerSfxCreateFromPath("data/splash/ace.sfx", 0);
	systemUnuse();

	blitCopy(
		pSplashAce, 0, 0, s_pBfr->pBack, (320 - s_sSplashRect.uwWidth) / 2, uwSplashOffsY,
		s_sSplashRect.uwWidth, s_sSplashRect.uwHeight, MINTERM_COOKIE
	);

	systemUse();
	bitmapDestroy(pSplashAce);
	systemUnuse();

	s_eStateAce = STATE_ACE_FADE_IN;
	ptplayerSfxPlay(s_pSfxAce, 0, 64, 100);
	s_bAceFadeoutRatio = 15;
}

static void splashAceLoop(void) {
	if(s_eStateAce == STATE_ACE_FADE_IN) {
		++s_uwFlashFrame;

		if(s_uwFlashFrame >= FLASH_START_FRAME_A) {
			s_bRatioFlashA = MIN(16, s_bRatioFlashA + 1);
		}
		if(s_uwFlashFrame >= FLASH_START_FRAME_C) {
			s_bRatioFlashC = MIN(16, s_bRatioFlashC + 1);
		}
		if(s_uwFlashFrame >= FLASH_START_FRAME_E) {
			s_bRatioFlashE = MIN(16, s_bRatioFlashE + 1);
		}
		if(s_uwFlashFrame >= FLASH_START_FRAME_PWR) {
			s_bRatioFlashPwr = MIN(16, s_bRatioFlashPwr + 1);
		}
		if (s_uwFlashFrame >= 215 || s_isAnyPressed || s_isEscapePressed) {
			s_eStateAce = STATE_ACE_FADE_OUT;
		}

		for(UBYTE i = 0; i < 30; ++i) {
			if(s_bRatioFlashA != FLASH_RATIO_INACTIVE && s_bRatioFlashA < 16) {
				s_pAceBlocks[i]->pCmds[0].sMove.bfValue = blendColors(0xFFF, s_pAceColors[i], s_bRatioFlashA);
			}

			if(s_bRatioFlashC != FLASH_RATIO_INACTIVE && s_bRatioFlashC < 16) {
				s_pAceBlocks[i]->pCmds[1].sMove.bfValue = blendColors(0xFFF, s_pAceColors[i], s_bRatioFlashC);
			}

			if(i < 9) {
				// Watch out for "E" flash interfering here
				UWORD uwColor = (
					s_bRatioFlashPwr != FLASH_RATIO_INACTIVE ?
					blendColors(0xFFF, s_uwColorPowered, s_bRatioFlashPwr) : 0
				);
				s_pAceBlocks[i]->pCmds[2].sMove.bfValue = uwColor;
			}
			else {
				if(s_bRatioFlashE != FLASH_RATIO_INACTIVE && s_bRatioFlashE < 16) {
					s_pAceBlocks[i]->pCmds[2].sMove.bfValue = blendColors(0xFFF, s_pAceColors[i], s_bRatioFlashE);
				}
			}

			s_pAceBlocks[i]->ubUpdated = 2;
			s_pView->pCopList->ubStatus |= STATUS_UPDATE;
		}
	}
	else if(s_eStateAce == STATE_ACE_FADE_OUT) {
		for(UBYTE i = 0; i < 30; ++i) {
			s_pAceBlocks[i]->uwCurrCount = 0;
			copMove(
				s_pView->pCopList, s_pAceBlocks[i], &g_pCustom->color[1],
				blendColors(0x000, s_pAceColors[i], s_bAceFadeoutRatio)
			);
			copMove(
				s_pView->pCopList, s_pAceBlocks[i], &g_pCustom->color[2],
				blendColors(0x000, s_pAceColors[i], s_bAceFadeoutRatio)
			);
			copMove(
				s_pView->pCopList, s_pAceBlocks[i], &g_pCustom->color[3],
				blendColors(0x000, i < 9 ? s_uwColorPowered : s_pAceColors[i], s_bAceFadeoutRatio)
			);
		}

		--s_bAceFadeoutRatio;
	}

	copProcessBlocks();
	vPortWaitForEnd(s_pVp);

	if (s_bAceFadeoutRatio <= 0)
	{
		if(s_isEscapePressed) {
			ptplayerStop();
			ptplayerSetMasterVolume(20);
			stateChange(g_pGameStateManager, &g_sStateGame);
		}
		else {
			stateChange(g_pGameStateManager, &g_sStateCutscene);
		}

		return;
	}
}

static void splashAceDestroy(void) {
	ptplayerWaitForSfx();
	ptplayerSfxDestroy(s_pSfxAce);
}

tState g_sStateSplash = {
	.cbCreate = splashGsCreate, .cbLoop = splashGsLoop, .cbDestroy = splashGsDestroy
};

static tState s_sStateSplashLmc = {
	.cbCreate = splashLmcCreate, .cbLoop = splashLmcLoop, .cbDestroy = splashLmcDestroy
};

static tState s_sStateSplashAce = {
	.cbCreate = splashAceCreate, .cbLoop = splashAceLoop, .cbDestroy = splashAceDestroy
};
