// SPDX-License-Identifier: GPL-2.0+
/*
 * FB driver for the GC9306 LCD Controller
 *
 * Copyright (C) 2015 Dennis Menschel
 */

#include <linux/bitops.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <video/mipi_display.h>

#include "fbtft.h"

#define DRVNAME "fb_gc9306"

#define GC9306_IPS_GAMMA \
    "02 00 00 1b 1f 0b\n" \
    "01 03 00 28 2b 0e\n" \
    "0b 08 3b 04 03 4c\n" \
    "0e 07 46 04 05 51\n" \
    "08 15 15 1f 22 0F\n" \
    "0b 13 11 1f 21 0F"

/**
 * init_display() - initialize the display controller
 *
 * @par: FBTFT parameter object
 *
 * Most of the commands in this init function set their parameters to the
 * same default values which are already in place after the display has been
 * powered up. (The main exception to this rule is the pixel format which
 * would default to 18 instead of 16 bit per pixel.)
 * Nonetheless, this sequence can be used as a template for concrete
 * displays which usually need some adjustments.
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int init_display(struct fbtft_par *par)
{
	par->fbtftops.reset(par);//硬复位

	mdelay(50);

    //display control setting
    write_reg(par, 0xfe);
    write_reg(par, 0xef);
    write_reg(par, MIPI_DCS_SET_ADDRESS_MODE, 0x48);//MX, MY, RGB mode 刷新方向 48竖屏
    write_reg(par, MIPI_DCS_SET_PIXEL_FORMAT, MIPI_DCS_PIXEL_FMT_16BIT);//65k mode
    write_reg(par, 0xad,0x33);
    write_reg(par, 0xaf,0x55);
    write_reg(par, 0xae,0x2b);

    //GC9306 Power Sequence
    write_reg(par, 0xa4,0x44,0x44);
    write_reg(par, 0xa5,0x42,0x42);
    write_reg(par, 0xaa,0x88,0x88);
    write_reg(par, 0xae,0x2b);
    write_reg(par, 0xe8,0x11,0x0b);
    write_reg(par, 0xe3,0x01,0x10);
    write_reg(par, 0xff,0x61);
    write_reg(par, 0xac,0x00);
    write_reg(par, 0xaf,0x67);
    write_reg(par, 0xa6,0x2a,0x2a);
    write_reg(par, 0xa7,0x2b,0x2b);
    write_reg(par, 0xa8,0x18,0x18);
    write_reg(par, 0xa9,0x2a,0x2a);

    //display window 240X320 匹配mode
    write_reg(par, 0x2a,0x00,0x00,0x00,0xef);   //MIPI_DCS_SET_COLUMN_ADDRESS - 240
    write_reg(par, 0x2b,0x00,0x00,0x01,0x3f);   //MIPI_DCS_SET_PAGE_ADDRESS - 320
    write_reg(par, 0x2c);                       //MIPI_DCS_WRITE_MEMORY_START

    //GC9306 Gamma Sequence
    write_reg(par, 0xF0,0x02,0x00,0x00,0x1b,0x1f,0x0b);
    write_reg(par, 0xF1,0x01,0x03,0x00,0x28,0x2b,0x0e);
    write_reg(par, 0xF2,0x0b,0x08,0x3b,0x04,0x03,0x4c);
    write_reg(par, 0xF3,0x0e,0x07,0x46,0x04,0x05,0x51);
    write_reg(par, 0xF4,0x08,0x15,0x15,0x1f,0x22,0x0F);
    write_reg(par, 0xF5,0x0b,0x13,0x11,0x1f,0x21,0x0F);

    /* Sleep Out */
    write_reg(par, 0x11);                       //MIPI_DCS_EXIT_SLEEP_MODE
    mdelay(100);

    write_reg(par, 0x2c);                       //MIPI_DCS_WRITE_MEMORY_START

    // luat_lcd_clear(par, BLACK);

    /* display on */
	write_reg(par, 0x29);                       //MIPI_DCS_SET_DISPLAY_ON - 29
	mdelay(100);

    return 0;
}

/**
 * set_var() - apply LCD properties like rotation and BGR mode
 *
 * @par: FBTFT parameter object
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int set_var(struct fbtft_par *par)
{
	u8 madctl_par = 0;

	if (par->bgr)
		madctl_par =0x48;
	switch (par->info->var.rotate) {
	case 0:
		madctl_par = 0x48;
		break; //48
	case 90:
		madctl_par = 0xE8;
		break;
	case 180:
		madctl_par =0x28;
		break;
	case 270:
		madctl_par =0xF8;
		break;
	default:
		return -EINVAL;
	}
	write_reg(par, MIPI_DCS_SET_ADDRESS_MODE, madctl_par);
	return 0;
}


/**
 * set_gamma() - set gamma curves
 *
 * @par: FBTFT parameter object
 * @curves: gamma curves
 *
 * Before the gamma curves are applied, they are preprocessed with a bitmask
 * to ensure syntactically correct input for the display controller.
 * This implies that the curves input parameter might be changed by this
 * function and that illegal gamma values are auto-corrected and not
 * reported as errors.
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int set_gamma(struct fbtft_par *par, u32 *curves)
{
    //GC9306 Gamma Sequence
    write_reg(par, 0xF0,0x02,0x00,0x00,0x1b,0x1f,0x0b);
    write_reg(par, 0xF1,0x01,0x03,0x00,0x28,0x2b,0x0e);
    write_reg(par, 0xF2,0x0b,0x08,0x3b,0x04,0x03,0x4c);
    write_reg(par, 0xF3,0x0e,0x07,0x46,0x04,0x05,0x51);
    write_reg(par, 0xF4,0x08,0x15,0x15,0x1f,0x22,0x0F);
    write_reg(par, 0xF5,0x0b,0x13,0x11,0x1f,0x21,0x0F);

	return 0;
}

/**
 * blank() - blank the display
 *
 * @par: FBTFT parameter object
 * @on: whether to enable or disable blanking the display
 *
 * Return: 0 on success, < 0 if error occurred.
 */
static int blank(struct fbtft_par *par, bool on)
{
	if (on)
		write_reg(par, MIPI_DCS_SET_DISPLAY_OFF);
	else
		write_reg(par, MIPI_DCS_SET_DISPLAY_ON);
	return 0;
}

static struct fbtft_display display = {
	.regwidth = 8,
	.width = 240,
	.height = 320,
	.gamma_num = 6,
	.gamma_len = 6,
	.gamma = GC9306_IPS_GAMMA,
	.fbtftops = {
		.init_display = init_display,
		.set_var = set_var,
		.set_gamma = set_gamma,
		.blank = blank,
	},
};

FBTFT_REGISTER_DRIVER(DRVNAME, "sitronix,gc9306", &display);

MODULE_ALIAS("spi:" DRVNAME);
MODULE_ALIAS("platform:" DRVNAME);
MODULE_ALIAS("spi:gc9306");
MODULE_ALIAS("platform:gc9306");

MODULE_DESCRIPTION("FB driver for the GC9306 LCD Controller");
MODULE_AUTHOR("Dennis Menschel & Youkai");
MODULE_LICENSE("GPL");
