/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*  Standard C Included Files */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <cr_section_macros.h>
#include "board.h"
#include "fsl_lcdc.h"

#include "pin_mux.h"
#include "fsl_sctimer.h"
#include "fsl_debug_console.h"
#include "navball.h"
#include "textureMap.h"

#define M_PI 3.14159265358979323846264338


extern textureMap_t defaultTextureMap;

//static navballImage_t navball;
 __NOINIT(BOARD_SDRAM) __attribute__ ((aligned)) navballImage_t navball;
//__SECTION(data, BOARD_SDRAM) __attribute__ ((aligned)) navballImage_t navball;


/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define APP_LCD LCD
#define LCD_PANEL_CLK 9000000
#define LCD_PPL 480
#define LCD_HSW 2
#define LCD_HFP 8
#define LCD_HBP 43
#define LCD_LPP 272
#define LCD_VSW 10
#define LCD_VFP 4
#define LCD_VBP 12
#define LCD_POL_FLAGS kLCDC_InvertVsyncPolarity | kLCDC_InvertHsyncPolarity
#define IMG_HEIGHT 272
#define IMG_WIDTH 480
#define LCD_INPUT_CLK_FREQ CLOCK_GetLcdClkFreq()
#define APP_LCD_IRQHandler LCD_IRQHandler
#define APP_LCD_IRQn LCD_IRQn
#define APP_PIXEL_PER_BYTE 8

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

#if (defined(__CC_ARM) || defined(__ARMCC_VERSION) || defined(__GNUC__))
__attribute__((aligned(8)))
#elif defined(__ICCARM__)
#pragma data_alignment = 8
#else
#error Toolchain not support.
#endif
// __NOINIT(BOARD_SDRAM) __attribute__ ((aligned)) uint8_t s_frameBufs[IMG_HEIGHT][IMG_WIDTH][3];
__attribute__(( section(".noinit.$RAM4"), aligned(8) ))
static uint16_t s_frameBufs[IMG_HEIGHT][IMG_WIDTH];
// static uint8_t s_frameBufs[IMG_HEIGHT][IMG_WIDTH][3];

//static uint8_t s_frameBufs[IMG_HEIGHT][IMG_WIDTH / APP_PIXEL_PER_BYTE];
//static uint8_t s_frameBufs[IMG_HEIGHT][IMG_WIDTH];


static volatile bool s_frameEndFlag;


/*
 * In this example, LCD controller works in 2 bpp mode, supports 4 colors,
 * the palette data format is:
 *
 * RGB format: (lcdConfig.swapRedBlue = false)
 * Bit(s)   Name     Description
 * 4:0      R[4:0]   Red palette data B[4:0] Blue palette data
 * 9:5      G[4:0]   Green palette data G[4:0] Green palette data
 * 14:10    B[4:0]   Blue palette data R[4:0] Red palette data
 * 15       I        Intensity / unused I Intensity / unused
 * 20:16    R[4:0]   Red palette data B[4:0] Blue palette data
 * 25:21    G[4:0]   Green palette data G[4:0] Green palette data
 * 30:26    B[4:0]   Blue palette data R[4:0] Red palette data
 * 31       I        Intensity / unused I Intensity / unused
 *
 * BGR format: (lcdConfig.swapRedBlue = true)
 * Bit(s)   Name     Description
 * 4:0      B[4:0]   Blue palette data
 * 9:5      G[4:0]   Green palette data
 * 14:10    R[4:0]   Red palette data
 * 15       I        Intensity / unused
 * 20:16    B[4:0]   Blue palette data
 * 25:21    G[4:0]   Green palette data
 * 30:26    R[4:0]   Red palette data
 * 31       I        Intensity / unused
 *
 * This example uses RGB format, the supported colors set in palette
 * are: black, red, green, and blue.
 */
//static const uint32_t palette[] = {0x00007FFF};
static const uint32_t palette[] = {0x001F0000U, 0x7C0003D0U };



/*******************************************************************************
 * Code
 ******************************************************************************/
static void BOARD_InitPWM(void)
{
    sctimer_config_t config;
    sctimer_pwm_signal_param_t pwmParam;
    uint32_t event;

    CLOCK_AttachClk(kMAIN_CLK_to_SCT_CLK);

    CLOCK_SetClkDiv(kCLOCK_DivSctClk, 2, true);

    SCTIMER_GetDefaultConfig(&config);

    SCTIMER_Init(SCT0, &config);

    pwmParam.output           = kSCTIMER_Out_5;
    pwmParam.level            = kSCTIMER_HighTrue;
    pwmParam.dutyCyclePercent = 5;

    SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 1000U, CLOCK_GetSctClkFreq(), &event);
}

void APP_LCD_IRQHandler(void)
{
    uint32_t intStatus = LCDC_GetEnabledInterruptsPendingStatus(APP_LCD);

    LCDC_ClearInterruptsStatus(APP_LCD, intStatus);

    if (intStatus & kLCDC_VerticalCompareInterrupt)
    {
        s_frameEndFlag = true;
    }
    __DSB();
/* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
  exception return operation might vector to incorrect interrupt */
#if defined __CORTEX_M && (__CORTEX_M == 4U)
    __DSB();
#endif
}


int main(void)
{
    lcdc_config_t lcdConfig;


    /* Route Main clock to LCD. */
    CLOCK_AttachClk(kMAIN_CLK_to_LCD_CLK);

    CLOCK_SetClkDiv(kCLOCK_DivLcdClk, 1, true);

    BOARD_InitPins();
    BOARD_BootClockPLL180M();

    BOARD_InitSDRAM();

    /* Set the back light PWM. */
    BOARD_InitPWM();

//    APP_FillBuffer((void *)(s_frameBufAddr[0]));

    LCDC_GetDefaultConfig(&lcdConfig);

    lcdConfig.panelClock_Hz  = LCD_PANEL_CLK;
    lcdConfig.ppl            = LCD_PPL;
    lcdConfig.hsw            = LCD_HSW;
    lcdConfig.hfp            = LCD_HFP;
    lcdConfig.hbp            = LCD_HBP;
    lcdConfig.lpp            = LCD_LPP;
    lcdConfig.vsw            = LCD_VSW;
    lcdConfig.vfp            = LCD_VFP;
    lcdConfig.vbp            = LCD_VBP;
    lcdConfig.polarityFlags  = LCD_POL_FLAGS;
    lcdConfig.upperPanelAddr = (uint32_t)s_frameBufs;
    lcdConfig.bpp            = kLCDC_16BPP565; // kLCDC_1BPP / kLCDC_2BPP kLCDC_24BPP kLCDC_16BPP565
    lcdConfig.display        = kLCDC_DisplayTFT;
    lcdConfig.swapRedBlue    = false;

    LCDC_Init(APP_LCD, &lcdConfig, LCD_INPUT_CLK_FREQ);

    LCDC_SetPalette(APP_LCD, palette, ARRAY_SIZE(palette));

    LCDC_EnableInterrupts(APP_LCD, kLCDC_VerticalCompareInterrupt);
    NVIC_EnableIRQ(APP_LCD_IRQn);

    LCDC_Start(APP_LCD);
    LCDC_PowerUp(APP_LCD);

    float pitch = 45.0 * M_PI / 180.0;
	float roll = 30.0 * M_PI / 180.0;
	float yaw = 10.0 * M_PI / 180.0;
    float pitch_rad = 0.0;
    float roll_rad = 0.0;
    float yaw_rad = 0.0;

//	LCDC_SetPanelAddr(APP_LCD, kLCDC_UpperPanel, (uint32_t)(s_frameBufAddr[0]));
	uint8_t r, g, b;

    while (1)
    {
        pitch += 1.0;
        roll += 1.0;
        yaw += 1.0;
        pitch_rad = pitch * M_PI / 180.0;
        roll_rad = roll * M_PI / 180.0;
        yaw_rad = yaw * M_PI / 180.0;
        if (pitch > 360.0) {
        	pitch = 0.0;
        }
        if (roll > 360.0) {
        	roll = 0.0;
        }
        if (yaw > 360.0) {
        	yaw = 0.0;
        }
    	generateNavBall(&defaultTextureMap, &navball, pitch_rad, roll_rad, yaw_rad);
        // pass through the frame buffer {IMG_HEIGHT][IMG_WIDTH / APP_PIXEL_PER_BYTE];
        for (int i = 0; i < IMG_HEIGHT; i++) {
        	for (int j = 0; j < IMG_WIDTH; j++) {
//        		s_frameBufs[i][j] = 0x001F;
                if(i < SIZE_NAVBALL && j < SIZE_NAVBALL) {
                	r = navball.data[i][j][0] >> 3;
                	g = navball.data[i][j][1] >> 2;
                	b = navball.data[i][j][2] >> 3;
                } else {
                	r = 0;
                	g = 0;
                	b = 0;
                }
                s_frameBufs[i][j] = (r << 11) | (g << 5) | b;
        	}
        }

        while (!s_frameEndFlag)
        {
        }



        s_frameEndFlag = false;
    }
}
