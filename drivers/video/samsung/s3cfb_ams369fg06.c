/*
 * TL2796 LCD Panel Driver for the Samsung Universal board
 *
 * Copyright (c) 2008 Samsung Electronics
 * Author: InKi Dae  <inki.dae@samsung.com>
 *
 * Derived from drivers/video/omap/lcd-apollon.c
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <linux/wait.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>

#include <plat/gpio-cfg.h>
#include <plat/regs-fb.h>

#include "s3cfb.h"

#define SLEEPMSEC		0x1000
#define ENDDEF			0x2000
#define	DEFMASK			0xFF00
#define COMMAND_ONLY		0xFE
#define DATA_ONLY		0xFF

#define DEVICE_NAME		"ams369fg06"

#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED

//define dbg printk
#define dbg(fmt...)

static int locked = 0;
struct s5p_lcd{
	struct spi_device *g_spi;
	struct lcd_device *lcd_dev;
	struct backlight_device *bl_dev;

};
static struct s5p_lcd lcd;
#else
static struct spi_device *g_spi;
#endif //CONFIG_BACKLIGHT_TL2796_AMOLED

const unsigned short SEQ_DISPLAY_ON[] = {
	0x14, 0x03,

	ENDDEF, 0x0000
};

const unsigned short SEQ_DISPLAY_OFF[] = {
	0x14, 0x00,

	ENDDEF, 0x0000
};

const unsigned short SEQ_STANDBY_ON[] = {
	0x1D, 0xA1,
	SLEEPMSEC, 200,

	ENDDEF, 0x0000
};

const unsigned short SEQ_STANDBY_OFF[] = {
	0x1D, 0xA0,
	SLEEPMSEC, 250,

	ENDDEF, 0x0000
};

// aesop
const unsigned short SEQ_SETTING[] = {
	0x17, 0x00,
	0x18, 0x33,	//power AMP Medium
	0x19, 0x03,	//Gamma Amp Medium
	0x1a, 0x03,
	0x22, 0xa3,	//Vinternal = 0.65*VCI
	0x23, 0x07,	//VLOUT1 Setting = 0.98*VCI
	0x26, 0xa0,	//Display Condition LTPS signal generation : Reference= DOTCLK

        0xef, 0xd0, 	/* pentile key setting */
        DATA_ONLY, 0xe8,

	0x1d, 0xa1,
	SLEEPMSEC, 300,

	0x14, 0x03,


	0x12, 0x05, // bottom red line
	0x1b, 0x32, // bright by VGL
	0x1C, 0x05, // 
	0x21, 0x00,
	0x26, 0xA2,
	
	0x39, 0,
	0x46, 46,
	0x56, 52,
	0x66, 82,
	0x39, 0,

	ENDDEF, 0x0000
};


/* FIXME: will be moved to platform data */
static struct s3cfb_lcd tl2796 = {
	.width = 480,
	.height = 800,
	.bpp = 24,

	/* smdkc110, For Android */
	.freq = 60,
	.timing = {
		.h_fp = 9,
		.h_bp = 9,
		.h_sw = 2,
		.v_fp = 5,
		.v_fpe = 1,
		.v_bp = 5,
		.v_bpe = 1,
		.v_sw = 2,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 1,
	},
};


static int tl2796_spi_write_driver(int addr, int data)
{
	int ret;
	u16 buf[1];
	struct spi_message msg;

#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED
	int ret;
#endif
	struct spi_transfer xfer = {
		.len	= 2,
		.tx_buf	= buf,
	};

	buf[0] = (addr << 8) | data;

//	printk("LCD SPI Addr : %x, data %x\n", addr, data);

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED
	locked  = 1;
	ret = spi_sync(lcd.g_spi, &msg);
	locked = 0;
	return ret ;
#else
	ret = spi_sync(g_spi, &msg);
	return ret;
#endif //CONFIG_BACKLIGHT_TL2796_AMOLED
}


static void tl2796_spi_write(unsigned char address, unsigned char command)
{
	if (address != DATA_ONLY)
		tl2796_spi_write_driver(0x70, address);

	tl2796_spi_write_driver(0x72, command);
}

static void tl2796_panel_send_sequence(const unsigned short *wbuf)
{
	int i = 0;

	while ((wbuf[i] & DEFMASK) != ENDDEF) {
		if ((wbuf[i] & DEFMASK) != SLEEPMSEC)
			tl2796_spi_write(wbuf[i], wbuf[i+1]);
		else
			msleep(wbuf[i+1]);
			//mdelay(wbuf[i+1]);
		i += 2;
	}
}

void tl2796_ldi_init(void)
{
	tl2796_panel_send_sequence(SEQ_SETTING);
	tl2796_panel_send_sequence(SEQ_STANDBY_OFF);
}

void tl2796_ldi_enable(void)
{
	tl2796_panel_send_sequence(SEQ_DISPLAY_ON);
	/* Added by james */
	tl2796_panel_send_sequence(SEQ_STANDBY_OFF);
	
}

void tl2796_ldi_disable(void)
{
#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED
	tl2796_panel_send_sequence(SEQ_DISPLAY_OFF);
	/* Added by james for fixing LCD suspend problem */
	tl2796_panel_send_sequence(SEQ_STANDBY_ON);
#endif
}

void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	tl2796.init_ldi = NULL;
	ctrl->lcd = &tl2796;
}

//backlight operations and functions
#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED
static int s5p_bl_update_status(struct backlight_device* bd)
{
	int bl = bd->props.brightness;
	int level = 0;	
	dbg("\nUpdate brightness=%d\n",bd->props.brightness);
	
	if(!locked)
        {
		if((bl >= 0) && (bl <= 80))
                        level = 1;
                else if((bl > 80) && (bl <= 100))
                        level = 2;
                else if((bl > 100) && (bl <= 150))
                        level = 3;
                else if((bl > 150) && (bl <= 180))
			level = 4;
                else if((bl > 180) && (bl <= 200))
                        level = 5;
                else if((bl > 200) && (bl <= 255))
                        level = 6;
		
		if(level){
			tl2796_spi_write(0x39,((bl/64 +1)<<4)&(bl/64+1));	

			switch (level){
				case 1:			
					tl2796_spi_write(0x46, (bl/2)+6); //R
        		        	tl2796_spi_write(0x56, (bl/2)+4); //G
                			tl2796_spi_write(0x66, (bl/2)+30); //B
					break;
				case 2:			
					tl2796_spi_write(0x46, (bl/2)+4); //R
        		        	tl2796_spi_write(0x56, (bl/2)+2); //G
                			tl2796_spi_write(0x66, (bl/2)+28); //B
					break;
				case 3:			
					tl2796_spi_write(0x46, (bl/2)+6); //R
        		        	tl2796_spi_write(0x56, (bl/2)+1); //G
                			tl2796_spi_write(0x66, (bl/2)+32); //B
					break;
				case 4:			
					tl2796_spi_write(0x46, (bl/2)+6); //R
        		        	tl2796_spi_write(0x56, (bl/2)+1); //G
                			tl2796_spi_write(0x66, (bl/2)+38); //B
					break;
				case 5:
					tl2796_spi_write(0x46, (bl/2)+7); //R
        	        		tl2796_spi_write(0x56, (bl/2)); //G
                			tl2796_spi_write(0x66, (bl/2)+40); //B
					break;
				case 6:
					tl2796_spi_write(0x46, (bl/2)+10); //R
        	        		tl2796_spi_write(0x56, (bl/2)); //G
                			tl2796_spi_write(0x66, (bl/2)+48); //B
					break;
				default:
					break;
			}
		}//level check over
	}else{
		dbg("\nLOCKED!!!Brightness cannot be changed now! locked=%d", locked);
	}
	return 0;
}

static int s5p_bl_get_brightness(struct backlight_device *bd)
{
	dbg("\n reading brightness \n");
	return 0;
}

static struct backlight_ops s5p_bl_ops = {
	.update_status = s5p_bl_update_status,
	.get_brightness = s5p_bl_get_brightness,	
};
#endif //CONFIG_BACKLIGHT_TL2796_AMOLED

static int __init tl2796_probe(struct spi_device *spi)
{
	int ret;

	spi->bits_per_word = 16;
	ret = spi_setup(spi);
#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED
	printk("\n\ntest\n\n");
	lcd.g_spi = spi;
#else
	g_spi = spi;
#endif //CONFIG_BACKLIGHT_TL2796_AMOLED

	tl2796_ldi_init();
	tl2796_ldi_enable();

	printk("Samsung AMS369FG06 AMOLED Initialized. \n");

	if (ret < 0)
		return 0;

	return ret;
}
#ifdef CONFIG_PM // add by ksoo (2009.09.07)
int tl2796_suspend(struct platform_device *pdev, pm_message_t state)
{
#ifdef CONFIG_BACKLIGHT_TL2796_AMOLED
	printk("tl2796_suspend\n");
	tl2796_ldi_disable();
#endif
	return 0;
}

int tl2796_resume(struct platform_device *pdev, pm_message_t state)
{
	printk("tl2796_resume\n");
	tl2796_ldi_init();
	tl2796_ldi_enable();

	return 0;
}
#endif

static struct spi_driver tl2796_driver = {
	.driver = {
		.name	=  DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= tl2796_probe,
	.remove		= __exit_p(tl2796_remove),
	.suspend	= NULL,
	.resume		= NULL,
};

static int __init tl2796_init(void)
{
	return spi_register_driver(&tl2796_driver);
}

static void __exit tl2796_exit(void)
{
	spi_unregister_driver(&tl2796_driver);
}

module_init(tl2796_init);
module_exit(tl2796_exit);


// aesop

#include <linux/platform_device.h>
#include <linux/sysfs.h>

//#define	DEVICE_NAME	"tl2796"

int str2int(const char *buf, size_t count)
{
	int i;
	if (buf[0] == '0' && (buf[1] == 'x' || buf[1] == 'X')) {
	  i = simple_strtoul(&buf[2], NULL, 16);
	} else {
	  i = simple_strtoul(buf, NULL, 10);
	}
	if (i <   0) i = 0;
	if (i > 255) i = 255;
	printk("input=%d(0x%x)\n", i, i);
	return i;
}

static ssize_t tl2796_11_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x11, color);  return count; } static DEVICE_ATTR(11, (S_IWUGO), NULL, tl2796_11_write);
static ssize_t tl2796_12_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x12, color);  return count; } static DEVICE_ATTR(12, (S_IWUGO), NULL, tl2796_12_write);
static ssize_t tl2796_13_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x13, color);  return count; } static DEVICE_ATTR(13, (S_IWUGO), NULL, tl2796_13_write);
static ssize_t tl2796_14_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x14, color);  return count; } static DEVICE_ATTR(14, (S_IWUGO), NULL, tl2796_14_write);
static ssize_t tl2796_15_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x15, color);  return count; } static DEVICE_ATTR(15, (S_IWUGO), NULL, tl2796_15_write);
static ssize_t tl2796_16_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x16, color);  return count; } static DEVICE_ATTR(16, (S_IWUGO), NULL, tl2796_16_write);
static ssize_t tl2796_17_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x17, color);  return count; } static DEVICE_ATTR(17, (S_IWUGO), NULL, tl2796_17_write);
static ssize_t tl2796_18_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x18, color);  return count; } static DEVICE_ATTR(18, (S_IWUGO), NULL, tl2796_18_write);
static ssize_t tl2796_19_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x19, color);  return count; } static DEVICE_ATTR(19, (S_IWUGO), NULL, tl2796_19_write);
static ssize_t tl2796_1A_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x1A, color);  return count; } static DEVICE_ATTR(1A, (S_IWUGO), NULL, tl2796_1A_write);
static ssize_t tl2796_1B_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x1B, color);  return count; } static DEVICE_ATTR(1B, (S_IWUGO), NULL, tl2796_1B_write);
static ssize_t tl2796_1C_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x1C, color);  return count; } static DEVICE_ATTR(1C, (S_IWUGO), NULL, tl2796_1C_write);
static ssize_t tl2796_1D_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x1D, color);  return count; } static DEVICE_ATTR(1D, (S_IWUGO), NULL, tl2796_1D_write);
static ssize_t tl2796_1E_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x1E, color);  return count; } static DEVICE_ATTR(1E, (S_IWUGO), NULL, tl2796_1E_write);
static ssize_t tl2796_1F_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x1F, color);  return count; } static DEVICE_ATTR(1F, (S_IWUGO), NULL, tl2796_1F_write);
static ssize_t tl2796_20_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x20, color);  return count; } static DEVICE_ATTR(20, (S_IWUGO), NULL, tl2796_20_write);
static ssize_t tl2796_21_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x21, color);  return count; } static DEVICE_ATTR(21, (S_IWUGO), NULL, tl2796_21_write);
static ssize_t tl2796_22_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x22, color);  return count; } static DEVICE_ATTR(22, (S_IWUGO), NULL, tl2796_22_write);
static ssize_t tl2796_23_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x23, color);  return count; } static DEVICE_ATTR(23, (S_IWUGO), NULL, tl2796_23_write);
static ssize_t tl2796_24_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x24, color);  return count; } static DEVICE_ATTR(24, (S_IWUGO), NULL, tl2796_24_write);
static ssize_t tl2796_25_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x25, color);  return count; } static DEVICE_ATTR(25, (S_IWUGO), NULL, tl2796_25_write);
static ssize_t tl2796_26_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x26, color);  return count; } static DEVICE_ATTR(26, (S_IWUGO), NULL, tl2796_26_write);
static ssize_t tl2796_27_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x27, color);  return count; } static DEVICE_ATTR(27, (S_IWUGO), NULL, tl2796_27_write);
static ssize_t tl2796_28_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x28, color);  return count; } static DEVICE_ATTR(28, (S_IWUGO), NULL, tl2796_28_write);
static ssize_t tl2796_29_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x29, color);  return count; } static DEVICE_ATTR(29, (S_IWUGO), NULL, tl2796_29_write);
static ssize_t tl2796_2A_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x2A, color);  return count; } static DEVICE_ATTR(2A, (S_IWUGO), NULL, tl2796_2A_write);
static ssize_t tl2796_2B_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x2B, color);  return count; } static DEVICE_ATTR(2B, (S_IWUGO), NULL, tl2796_2B_write);
static ssize_t tl2796_2C_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x2C, color);  return count; } static DEVICE_ATTR(2C, (S_IWUGO), NULL, tl2796_2C_write);
static ssize_t tl2796_2D_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x2D, color);  return count; } static DEVICE_ATTR(2D, (S_IWUGO), NULL, tl2796_2D_write);
static ssize_t tl2796_2E_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x2E, color);  return count; } static DEVICE_ATTR(2E, (S_IWUGO), NULL, tl2796_2E_write);
static ssize_t tl2796_2F_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x2F, color);  return count; } static DEVICE_ATTR(2F, (S_IWUGO), NULL, tl2796_2F_write);
static ssize_t tl2796_30_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x30, color);  return count; } static DEVICE_ATTR(30, (S_IWUGO), NULL, tl2796_30_write);
static ssize_t tl2796_31_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x31, color);  return count; } static DEVICE_ATTR(31, (S_IWUGO), NULL, tl2796_31_write);
static ssize_t tl2796_32_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x32, color);  return count; } static DEVICE_ATTR(32, (S_IWUGO), NULL, tl2796_32_write);
static ssize_t tl2796_33_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x33, color);  return count; } static DEVICE_ATTR(33, (S_IWUGO), NULL, tl2796_33_write);
static ssize_t tl2796_34_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x34, color);  return count; } static DEVICE_ATTR(34, (S_IWUGO), NULL, tl2796_34_write);
static ssize_t tl2796_35_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x35, color);  return count; } static DEVICE_ATTR(35, (S_IWUGO), NULL, tl2796_35_write);
static ssize_t tl2796_36_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x36, color);  return count; } static DEVICE_ATTR(36, (S_IWUGO), NULL, tl2796_36_write);
static ssize_t tl2796_37_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x37, color);  return count; } static DEVICE_ATTR(37, (S_IWUGO), NULL, tl2796_37_write);
static ssize_t tl2796_38_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x38, color);  return count; } static DEVICE_ATTR(38, (S_IWUGO), NULL, tl2796_38_write);
static ssize_t tl2796_39_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x39, color);  return count; } static DEVICE_ATTR(39, (S_IWUGO), NULL, tl2796_39_write);
static ssize_t tl2796_3A_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x3A, color);  return count; } static DEVICE_ATTR(3A, (S_IWUGO), NULL, tl2796_3A_write);
static ssize_t tl2796_3B_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x3B, color);  return count; } static DEVICE_ATTR(3B, (S_IWUGO), NULL, tl2796_3B_write);
static ssize_t tl2796_3C_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x3C, color);  return count; } static DEVICE_ATTR(3C, (S_IWUGO), NULL, tl2796_3C_write);
static ssize_t tl2796_3D_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x3D, color);  return count; } static DEVICE_ATTR(3D, (S_IWUGO), NULL, tl2796_3D_write);
static ssize_t tl2796_3E_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x3E, color);  return count; } static DEVICE_ATTR(3E, (S_IWUGO), NULL, tl2796_3E_write);
static ssize_t tl2796_3F_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x3F, color);  return count; } static DEVICE_ATTR(3F, (S_IWUGO), NULL, tl2796_3F_write);
static ssize_t tl2796_40_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x40, color);  return count; } static DEVICE_ATTR(40, (S_IWUGO), NULL, tl2796_40_write);
static ssize_t tl2796_41_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x41, color);  return count; } static DEVICE_ATTR(41, (S_IWUGO), NULL, tl2796_41_write);
static ssize_t tl2796_42_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x42, color);  return count; } static DEVICE_ATTR(42, (S_IWUGO), NULL, tl2796_42_write);
static ssize_t tl2796_43_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x43, color);  return count; } static DEVICE_ATTR(43, (S_IWUGO), NULL, tl2796_43_write);
static ssize_t tl2796_44_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x44, color);  return count; } static DEVICE_ATTR(44, (S_IWUGO), NULL, tl2796_44_write);
static ssize_t tl2796_45_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x45, color);  return count; } static DEVICE_ATTR(45, (S_IWUGO), NULL, tl2796_45_write);
static ssize_t tl2796_46_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x46, color);  return count; } static DEVICE_ATTR(46, (S_IWUGO), NULL, tl2796_46_write);
static ssize_t tl2796_47_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x47, color);  return count; } static DEVICE_ATTR(47, (S_IWUGO), NULL, tl2796_47_write);
static ssize_t tl2796_48_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x48, color);  return count; } static DEVICE_ATTR(48, (S_IWUGO), NULL, tl2796_48_write);
static ssize_t tl2796_49_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x49, color);  return count; } static DEVICE_ATTR(49, (S_IWUGO), NULL, tl2796_49_write);
static ssize_t tl2796_4A_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x4A, color);  return count; } static DEVICE_ATTR(4A, (S_IWUGO), NULL, tl2796_4A_write);
static ssize_t tl2796_4B_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x4B, color);  return count; } static DEVICE_ATTR(4B, (S_IWUGO), NULL, tl2796_4B_write);
static ssize_t tl2796_4C_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x4C, color);  return count; } static DEVICE_ATTR(4C, (S_IWUGO), NULL, tl2796_4C_write);
static ssize_t tl2796_4D_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x4D, color);  return count; } static DEVICE_ATTR(4D, (S_IWUGO), NULL, tl2796_4D_write);
static ssize_t tl2796_4E_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x4E, color);  return count; } static DEVICE_ATTR(4E, (S_IWUGO), NULL, tl2796_4E_write);
static ssize_t tl2796_4F_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x4F, color);  return count; } static DEVICE_ATTR(4F, (S_IWUGO), NULL, tl2796_4F_write);
static ssize_t tl2796_50_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x50, color);  return count; } static DEVICE_ATTR(50, (S_IWUGO), NULL, tl2796_50_write);
static ssize_t tl2796_51_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x51, color);  return count; } static DEVICE_ATTR(51, (S_IWUGO), NULL, tl2796_51_write);
static ssize_t tl2796_52_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x52, color);  return count; } static DEVICE_ATTR(52, (S_IWUGO), NULL, tl2796_52_write);
static ssize_t tl2796_53_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x53, color);  return count; } static DEVICE_ATTR(53, (S_IWUGO), NULL, tl2796_53_write);
static ssize_t tl2796_54_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x54, color);  return count; } static DEVICE_ATTR(54, (S_IWUGO), NULL, tl2796_54_write);
static ssize_t tl2796_55_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x55, color);  return count; } static DEVICE_ATTR(55, (S_IWUGO), NULL, tl2796_55_write);
static ssize_t tl2796_56_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x56, color);  return count; } static DEVICE_ATTR(56, (S_IWUGO), NULL, tl2796_56_write);
static ssize_t tl2796_57_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x57, color);  return count; } static DEVICE_ATTR(57, (S_IWUGO), NULL, tl2796_57_write);
static ssize_t tl2796_58_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x58, color);  return count; } static DEVICE_ATTR(58, (S_IWUGO), NULL, tl2796_58_write);
static ssize_t tl2796_59_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x59, color);  return count; } static DEVICE_ATTR(59, (S_IWUGO), NULL, tl2796_59_write);
static ssize_t tl2796_5A_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x5A, color);  return count; } static DEVICE_ATTR(5A, (S_IWUGO), NULL, tl2796_5A_write);
static ssize_t tl2796_5B_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x5B, color);  return count; } static DEVICE_ATTR(5B, (S_IWUGO), NULL, tl2796_5B_write);
static ssize_t tl2796_5C_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x5C, color);  return count; } static DEVICE_ATTR(5C, (S_IWUGO), NULL, tl2796_5C_write);
static ssize_t tl2796_5D_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x5D, color);  return count; } static DEVICE_ATTR(5D, (S_IWUGO), NULL, tl2796_5D_write);
static ssize_t tl2796_5E_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x5E, color);  return count; } static DEVICE_ATTR(5E, (S_IWUGO), NULL, tl2796_5E_write);
static ssize_t tl2796_5F_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x5F, color);  return count; } static DEVICE_ATTR(5F, (S_IWUGO), NULL, tl2796_5F_write);
static ssize_t tl2796_60_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x60, color);  return count; } static DEVICE_ATTR(60, (S_IWUGO), NULL, tl2796_60_write);
static ssize_t tl2796_61_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x61, color);  return count; } static DEVICE_ATTR(61, (S_IWUGO), NULL, tl2796_61_write);
static ssize_t tl2796_62_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x62, color);  return count; } static DEVICE_ATTR(62, (S_IWUGO), NULL, tl2796_62_write);
static ssize_t tl2796_63_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x63, color);  return count; } static DEVICE_ATTR(63, (S_IWUGO), NULL, tl2796_63_write);
static ssize_t tl2796_64_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x64, color);  return count; } static DEVICE_ATTR(64, (S_IWUGO), NULL, tl2796_64_write);
static ssize_t tl2796_65_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x65, color);  return count; } static DEVICE_ATTR(65, (S_IWUGO), NULL, tl2796_65_write);
static ssize_t tl2796_66_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count) {  int color = str2int(buf, count);  tl2796_spi_write(0x66, color);  return count; } static DEVICE_ATTR(66, (S_IWUGO), NULL, tl2796_66_write);

static struct attribute *tl2796_sysfs_attributes[] = {
	&dev_attr_11.attr,
	&dev_attr_12.attr,
	&dev_attr_13.attr,
	&dev_attr_14.attr,
	&dev_attr_15.attr,
	&dev_attr_16.attr,
	&dev_attr_17.attr,
	&dev_attr_18.attr,
	&dev_attr_19.attr,
	&dev_attr_1A.attr,
	&dev_attr_1B.attr,
	&dev_attr_1C.attr,
	&dev_attr_1D.attr,
	&dev_attr_1E.attr,
	&dev_attr_1F.attr,
	&dev_attr_20.attr,
	&dev_attr_21.attr,
	&dev_attr_22.attr,
	&dev_attr_23.attr,
	&dev_attr_24.attr,
	&dev_attr_25.attr,
	&dev_attr_26.attr,
	&dev_attr_27.attr,
	&dev_attr_28.attr,
	&dev_attr_29.attr,
	&dev_attr_2A.attr,
	&dev_attr_2B.attr,
	&dev_attr_2C.attr,
	&dev_attr_2D.attr,
	&dev_attr_2E.attr,
	&dev_attr_2F.attr,
	&dev_attr_30.attr,
	&dev_attr_31.attr,
	&dev_attr_32.attr,
	&dev_attr_33.attr,
	&dev_attr_34.attr,
	&dev_attr_35.attr,
	&dev_attr_36.attr,
	&dev_attr_37.attr,
	&dev_attr_38.attr,
	&dev_attr_39.attr,
	&dev_attr_3A.attr,
	&dev_attr_3B.attr,
	&dev_attr_3C.attr,
	&dev_attr_3D.attr,
	&dev_attr_3E.attr,
	&dev_attr_3F.attr,
	&dev_attr_40.attr,
	&dev_attr_41.attr,
	&dev_attr_42.attr,
	&dev_attr_43.attr,
	&dev_attr_44.attr,
	&dev_attr_45.attr,
	&dev_attr_46.attr,
	&dev_attr_47.attr,
	&dev_attr_48.attr,
	&dev_attr_49.attr,
	&dev_attr_4A.attr,
	&dev_attr_4B.attr,
	&dev_attr_4C.attr,
	&dev_attr_4D.attr,
	&dev_attr_4E.attr,
	&dev_attr_4F.attr,
	&dev_attr_50.attr,
	&dev_attr_51.attr,
	&dev_attr_52.attr,
	&dev_attr_53.attr,
	&dev_attr_54.attr,
	&dev_attr_55.attr,
	&dev_attr_56.attr,
	&dev_attr_57.attr,
	&dev_attr_58.attr,
	&dev_attr_59.attr,
	&dev_attr_5A.attr,
	&dev_attr_5B.attr,
	&dev_attr_5C.attr,
	&dev_attr_5D.attr,
	&dev_attr_5E.attr,
	&dev_attr_5F.attr,
	&dev_attr_60.attr,
	&dev_attr_61.attr,
	&dev_attr_62.attr,
	&dev_attr_63.attr,
	&dev_attr_64.attr,
	&dev_attr_65.attr,
	&dev_attr_66.attr,
	0
};

static struct attribute_group tl2796_sysfs_attribute_group = {
	.name   = NULL,
	.attrs  = tl2796_sysfs_attributes,
};

static int __devinit tl2796_sysfs_probe(struct platform_device *pdev)
{
	int ret;
	ret = sysfs_create_group(&pdev->dev.kobj, &tl2796_sysfs_attribute_group);
	if (ret)
	{
		printk("%s: sysfs_create_group failed. error=%d\n", __FUNCTION__, ret);
		return ret;
	}
//	printk("%s: sysfs_create_group succeed.\n", __FUNCTION__);
	return 0;
}

//-----------------------------------------------------------------------------

static int __devexit tl2796_sysfs_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &tl2796_sysfs_attribute_group);
	return 0;
}

static struct platform_device tl2796_sysfs_device = {
	.name = DEVICE_NAME,
	.id = -1,
};

static struct platform_driver tl2796_sysfs_driver = {
	.probe = tl2796_sysfs_probe,
	.remove = tl2796_sysfs_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
	},
};

static int __init tl2796_sysfs_init(void)
{
	int ret;

	// device
	ret = platform_device_register(&tl2796_sysfs_device);
	if (ret)
	{
		printk("%s: failed to add platform device %s (%d) \n", __FUNCTION__, tl2796_sysfs_device.name, ret);
		return ret;
	}
//	printk("%s: %s device registered\n", __FUNCTION__, tl2796_sysfs_device.name);

	// driver
	ret = platform_driver_register(&tl2796_sysfs_driver);
	if (ret)
	{
		printk("%s: failed to add platform driver %s (%d) \n", __FUNCTION__, tl2796_sysfs_driver.driver.name, ret);
		return ret;
	}
//	printk("%s: %s driver registered\n", __FUNCTION__, tl2796_sysfs_driver.driver.name);

	printk("[%s] driver initialized\n", DEVICE_NAME);
	return 0;
}

module_init(tl2796_sysfs_init);

static void __exit tl2796_sysfs_exit(void)
{
	// driver
	platform_driver_unregister(&tl2796_sysfs_driver);
//	printk("%s: %s driver unregistered\n", __FUNCTION__, tl2796_sysfs_driver.driver.name);

	// device
	platform_device_unregister(&tl2796_sysfs_device);
//	printk("%s: %s device unregistered\n", __FUNCTION__, tl2796_sysfs_device.name);

	printk("[%s] driver released\n", DEVICE_NAME);
}

module_exit(tl2796_sysfs_exit);
