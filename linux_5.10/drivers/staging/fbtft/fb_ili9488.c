// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the ILI9488 LCD Controller
 *
 * Copyright (C) 2014 Noralf Tronnes
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME		"fb_ili9488"
#define WIDTH		320
#define HEIGHT		480

// /* this init sequence matches PiScreen */
// static const s16 default_init_sequence[] = {
// 	/* Interface Mode Control */
// 	-1, 0xb0, 0x0,
// 	-1, MIPI_DCS_EXIT_SLEEP_MODE,
// 	-2, 250,
// 	/* Interface Pixel Format */
// 	-1, MIPI_DCS_SET_PIXEL_FORMAT, 0x55,
// 	/* Power Control 3 */
// 	-1, 0xC2, 0x44,
// 	/* VCOM Control 1 */
// 	-1, 0xC5, 0x00, 0x00, 0x00, 0x00,
// 	/* PGAMCTRL(Positive Gamma Control) */
// 	-1, 0xE0, 0x0F, 0x1F, 0x1C, 0x0C, 0x0F, 0x08, 0x48, 0x98,
// 		  0x37, 0x0A, 0x13, 0x04, 0x11, 0x0D, 0x00,
// 	/* NGAMCTRL(Negative Gamma Control) */
	// -1, 0xE1, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75,
	// 	  0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
// 	/* Digital Gamma Control 1 */
// 	-1, 0xE2, 0x0F, 0x32, 0x2E, 0x0B, 0x0D, 0x05, 0x47, 0x75,
// 		  0x37, 0x06, 0x10, 0x03, 0x24, 0x20, 0x00,
// 	-1, MIPI_DCS_EXIT_SLEEP_MODE,
// 	-1, MIPI_DCS_SET_DISPLAY_ON,
// 	/* end marker */
// 	-3
// };

static const s16 default_init_sequence[] = {
	-1, 0x11,
	-2, 150,
	
	-1, 0XFF,
	-1, 0XF7, 0xA9, 0x51, 0x2C, 0x82,

	/* Power Control 1 (C0h)  */
	-1, 0xC0, 0x11, 0x09,

	/* Power Control 2 (C1h) */
	-1, 0xC1, 0x41,

	/* VCOM Control (C5h)  */
	-1, 0XC5, 0x00, 0x0A, 0x80,

	/* Frame Rate Control (In Normal Mode/Full Colors) (B1h) */
	-1, 0xB1, 0xB0, 0x11,

	/* Display Inversion Control (B4h) */
	-1, 0xB4, 0x02,

	/* Display Function Control (B6h)  */
	-1, 0xB6, 0x02, 0x22,

	/* Entry Mode Set (B7h)  */
	-1, 0xB7, 0xc6,

	/* HS Lanes Control (BEh) */
	-1, 0xBE, 0x00, 0x04,

	/* Set Image Function (E9h)  */
	-1, 0xE9, 0x00,

	/* Set MADCTL - MY MX MV ML RGB MH - - (36h)  */
	-1, 0x36, 0x60,
	//LCD_WR_DATA(0x08,

	/* Interface Pixel Format (3Ah) */ /* 0x55 : 16 bits/pixel  */
	-1, 0x3A, 0x55,

	/* PGAMCTRL (Positive Gamma Control) (E0h) */
	-1, 0xE0, 0x00, 0x07, 0x10, 0x09, 0x17, 0x0B, 0x41, 0x89, 
			0x4B, 0x0A, 0x0C, 0x0E, 0x18, 0x1B, 0x0F,

	/* NGAMCTRL (Negative Gamma Control) (E1h)  */
	-1, 0XE1, 0x00, 0x17, 0x1A, 0x04, 0x0E, 0x06, 0x2F, 0x45, 
			0x43, 0x02, 0x0A, 0x09, 0x32, 0x36, 0x0F,

	/* Sleep Out11h */
	-1, 0x11,	//MIPI_DCS_EXIT_SLEEP_MODE

	-2, 120,

	/* Show */
	-1, 0x29,	//MIPI_DCS_SET_DISPLAY_ON
	-3
};


static void set_addr_win(struct fbtft_par *par, int xs, int ys, int xe, int ye)
{
	write_reg(par, MIPI_DCS_SET_COLUMN_ADDRESS,
		  xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);

	write_reg(par, MIPI_DCS_SET_PAGE_ADDRESS,
		  ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);

	write_reg(par, MIPI_DCS_WRITE_MEMORY_START);
}

static int set_var(struct fbtft_par *par)
{
	switch (par->info->var.rotate) {
	case 0:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  0x80 | (par->bgr << 3));
		break;
	case 90:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  0x20 | (par->bgr << 3));
		break;
	case 180:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  0x40 | (par->bgr << 3));
		break;
	case 270:
		write_reg(par, MIPI_DCS_SET_ADDRESS_MODE,
			  0xE0 | (par->bgr << 3));
		break;
	default:
		break;
	}

	return 0;
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = WIDTH,
	.height = HEIGHT,
	.init_sequence = default_init_sequence,
	.fbtftops = {
		.set_addr_win = set_addr_win,
		.set_var = set_var,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "ilitek,ili9488", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:ili9488");
MODULE_ALIAS("platform:ili9488");

MODULE_DESCRIPTION("FB driver for the ILI9488 LCD Controller");
MODULE_AUTHOR("Noralf Tronnes");
MODULE_LICENSE("GPL");
