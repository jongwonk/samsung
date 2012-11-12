/* linux/arch/arm/mach-s5pv210/mach-smdkv210.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/sysdev.h>
#include <linux/fb.h>
//K #include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/pwm_backlight.h>

#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <linux/smsc911x.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/setup.h>
#include <asm/mach-types.h>

#include <video/platform_lcd.h>

#include <mach/map.h>
#include <mach/regs-clock.h>
#include <mach/regs-fb.h>

#include <plat/regs-serial.h>
#include <plat/regs-srom.h>
#include <plat/gpio-cfg.h>
#include <plat/s5pv210.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/adc.h>
#include <plat/ts.h>
#include <plat/ata.h>
#include <plat/iic.h>
#include <plat/keypad.h>
#include <plat/pm.h>
#include <plat/fb.h>
#include <plat/s5p-time.h>

#include <plat/media.h>
#include <mach/media.h>
#include <s3cfb.h>

#include <linux/gpio_keys.h>

#include <plat/s3c64xx-spi.h>

#include <mach/regs-gpio.h>

#include <mach/gpio.h>
#include <plat/gpio-core.h>
// extern struct platform_device s5p_device_smsc911x;

extern struct platform_device s5pv210_pd_g3d;
extern struct platform_device s5pv210_pd_lcd;
extern struct platform_device s5pv210_pd_audio;

#define S5P_PA_SMC9115 0xA0000000 

// UM page 689
#define SMC9115_Tacs	(0)	// 0clk		address set-up
#define SMC9115_Tcos	(4)	// 4clk		chip selection set-up
#define SMC9115_Tacc	(13)	// 14clk	access cycle
#define SMC9115_Tcoh	(1)	// 1clk		chip selection hold
#define SMC9115_Tah	    (4)	// 4clk		address holding time
#define SMC9115_Tacp	(6)	// 6clk		page mode access cycle
#define SMC9115_PMC	    (0)	// normal(1data)page mode configuration


/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDKV210_UCON_DEFAULT	(S3C2410_UCON_TXILEVEL |	\
				 S3C2410_UCON_RXILEVEL |	\
				 S3C2410_UCON_TXIRQMODE |	\
				 S3C2410_UCON_RXIRQMODE |	\
				 S3C2410_UCON_RXFIFO_TOI |	\
				 S3C2443_UCON_RXERR_IRQEN)

#define SMDKV210_ULCON_DEFAULT	S3C2410_LCON_CS8

#define SMDKV210_UFCON_DEFAULT	(S3C2410_UFCON_FIFOMODE |	\
				 S5PV210_UFCON_TXTRIG4 |	\
				 S5PV210_UFCON_RXTRIG4)


#define S5PV210_LCD_WIDTH 480
#define S5PV210_LCD_HEIGHT 800
#define NUM_BUFFER 4

#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0 (6144 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1 (9900 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2 (6144 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0 (36864 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1 (36864 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD (S5PV210_LCD_WIDTH * \
                                             S5PV210_LCD_HEIGHT * NUM_BUFFER * \
                                             (CONFIG_FB_S3C_NR_BUFFERS + \
                                                 (CONFIG_FB_S3C_NUM_OVLY_WIN * \
                                                  CONFIG_FB_S3C_NUM_BUF_OVLY_WIN)))
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG (8192 * SZ_1K)
#define  S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 (1800 * SZ_1K)
#define  S5PV210_VIDEO_SAMSUNG_MEMSIZE_G2D (8192 * SZ_1K)


static struct s5p_media_device s5pv210_media_devs[] = {
        [0] = {
                .id = S5P_MDEV_MFC,
                .name = "mfc",
                .bank = 0,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC0,
                .paddr = 0,
        },
        [1] = {
                .id = S5P_MDEV_MFC,
                .name = "mfc",
                .bank = 1,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_MFC1,
                .paddr = 0,
        },
        [2] = {
                .id = S5P_MDEV_FIMC0,
                .name = "fimc0",
                .bank = 1,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC0,
                .paddr = 0,
        },
        [3] = {
                .id = S5P_MDEV_FIMC1,
                .name = "fimc1",
                .bank = 1,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC1,
                .paddr = 0,
        },
        [4] = {
                .id = S5P_MDEV_FIMC2,
                .name = "fimc2",
                .bank = 1,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMC2,
                .paddr = 0,
        },
        [5] = {
                .id = S5P_MDEV_JPEG,
                .name = "jpeg",
                .bank = 0,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_JPEG,
                .paddr = 0,
        },
	[6] = {
                .id = S5P_MDEV_FIMD,
                .name = "fimd",
                .bank = 1,
                .memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_FIMD,
                .paddr = 0,
        },
	[7] = {
                .id = S5P_MDEV_PMEM_GPU1,
                .name = "pmem_gpu1",
                .bank = 0, /* OneDRAM */
                .memsize = S5PV210_ANDROID_PMEM_MEMSIZE_PMEM_GPU1,
                .paddr = 0,
        },
	[8] = {
		.id = S5P_MDEV_G2D,
		.name = "g2d",
		.bank = 0,
		.memsize = S5PV210_VIDEO_SAMSUNG_MEMSIZE_G2D,
		.paddr = 0,
	},
};

static struct smsc911x_platform_config smsc911x_config = {
        .irq_polarity   = SMSC911X_IRQ_POLARITY_ACTIVE_LOW,
        .irq_type       = SMSC911X_IRQ_TYPE_PUSH_PULL,
        .flags          = SMSC911X_USE_32BIT,
        .phy_interface  = PHY_INTERFACE_MODE_MII,
};

static struct resource s5p_smsc911x_resources[] = {
        [0] = {
                .start = S5P_PA_SMC9115,
                //.end   = S5P_PA_SMC9115 + 0xff,
                .end   = S5P_PA_SMC9115 + SZ_1K -1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_EINT6,
                .end   = IRQ_EINT6,
                .flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL,
        }
};

struct platform_device s5p_device_smsc911x = {
        .name           = "smsc911x",
        .id             =  -1,
        .num_resources  = ARRAY_SIZE(s5p_smsc911x_resources),
        .resource       = s5p_smsc911x_resources,
        .dev = {
                .platform_data = &smsc911x_config,
        },
};

static struct s3c2410_uartcfg smdkv210_uartcfgs[] __initdata = {
	[0] = {
		.hwport		= 0,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		= 1,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		= 2,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		= 3,
		.flags		= 0,
		.ucon		= SMDKV210_UCON_DEFAULT,
		.ulcon		= SMDKV210_ULCON_DEFAULT,
		.ufcon		= SMDKV210_UFCON_DEFAULT,
	},
};

static struct gpio_keys_button aesop_gpio_keys_table[] = {
	{
		.code 	= KEY_DOWN,
		.gpio		= S5PV210_GPC1(0),
		.desc		= "KEY_DOWN",
		.type		= EV_KEY,
		.active_low	= 1,
		.wakeup		= 1,
		.debounce_interval = 5,
		
	},
	
	{
		.code 	= KEY_ENTER,
		.gpio		= S5PV210_GPC1(1),
		.desc		= "KEY_ENTER",
		.type		= EV_KEY,
		.active_low	= 1,
		.wakeup		= 0,
		.debounce_interval = 5,
	},
	{
		.code 	= KEY_RIGHT,
		.gpio		= S5PV210_GPC1(2),
		.desc		= "KEY_RIGHT",
		.type		= EV_KEY,
		.active_low	= 1,
		.wakeup		= 0,
		.debounce_interval = 5,
	},
	{
		.code 	= KEY_LEFT,
		.gpio		= S5PV210_GPA1(2),
		.desc		= "KEY_LEFT",
		.type		= EV_KEY,
		.active_low	= 1,
		.wakeup		= 0,
		.debounce_interval = 5,
	},
	{
		.code 	= KEY_UP,
		.gpio		= S5PV210_GPA1(3),
		.desc		= "KEY_UP",
		.type		= EV_KEY,
		.active_low	= 1,
		.wakeup		= 0,
		.debounce_interval = 5,
	},	

};


static struct gpio_keys_platform_data aesop_gpio_keys_data = {
	.buttons	= aesop_gpio_keys_table,
	.nbuttons	= ARRAY_SIZE(aesop_gpio_keys_table),
};

static struct platform_device aesop_device_gpiokeys = {
	.name = "gpio-keys",
	.id = -1,
	.dev = {
		.platform_data = &aesop_gpio_keys_data,
	},
};

static void s5pv210_fb_cfg_gpios(unsigned int base, unsigned int nr)
{
	s3c_gpio_cfgrange_nopull(base, nr, S3C_GPIO_SFN(2));

	for (; nr > 0; nr--, base++)
		s5p_gpio_set_drvstr(base, S5P_GPIO_DRVSTR_LV4);
}

static void smdkv210_cfg_gpio(struct platform_device *dev)
{
	s5pv210_fb_cfg_gpios(S5PV210_GPF0(0), 8);
	s5pv210_fb_cfg_gpios(S5PV210_GPF1(0), 8);
	s5pv210_fb_cfg_gpios(S5PV210_GPF2(0), 8);
	s5pv210_fb_cfg_gpios(S5PV210_GPF3(0), 4);
	
	writel(0x2, S5P_MDNIE_SEL);

}

static void smdkv210_cfg_gpio_spi1(void)
{
	s5pv210_fb_cfg_gpios(S5PV210_GPB(4), 4);
}

#define S5PV210_GPD_0_0_TOUT_0  (0x2)
#define S5PV210_GPD_0_1_TOUT_1  (0x2 << 4)
#define S5PV210_GPD_0_2_TOUT_2  (0x2 << 8)
#define S5PV210_GPD_0_3_TOUT_3  (0x2 << 12)

static int  smdkv210_backlight_on(struct platform_device *dev)
{
	return 0;
}

static int smdkv210_reset_lcd(struct platform_device *dev)
{
  
	int err;
	err = gpio_request(S5PV210_GPD0(3), "GPD0_3");
	if (err) {
		printk(KERN_ERR "failed to request GPD0_3 for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5PV210_GPD0(3), 1);
	mdelay(10);

	gpio_set_value(S5PV210_GPD0(3), 0);
	mdelay(10);

	gpio_set_value(S5PV210_GPD0(3), 1);
	mdelay(1);

	gpio_free(S5PV210_GPD0(3));
	
	return 0;
	
}

#define S5PV210_LCD_WIDTH 480
#define S5PV210_LCD_HEIGHT 800

static struct s3cfb_lcd aesop_ams369fg06 = {
	.width = S5PV210_LCD_WIDTH,
	.height = S5PV210_LCD_HEIGHT,
	.bpp = 24,
	.freq = 60,

	.timing = {
		.h_fp = 8,
		.h_bp = 13,
		.h_sw = 3,
		.v_fp = 5,
		.v_fpe = 0,
		.v_bp = 7,
		.v_bpe = 0,
		.v_sw = 1,
	},
	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 0,
		.inv_vsync = 0,
		.inv_vden = 1,
	},
};
static struct s3c_platform_fb aesop_lcd_data __initdata = {
	.hw_ver		= 0x62,
	.clk_name	= "sclk_fimd",
	.nr_wins	= 5,
	.default_win	= CONFIG_FB_S3C_DEFAULT_WINDOW,
	.swap		= FB_SWAP_HWORD | FB_SWAP_WORD,

	.lcd = &aesop_ams369fg06,
	.cfg_gpio	= smdkv210_cfg_gpio,
	.backlight_on	= smdkv210_backlight_on,
	.reset_lcd	= smdkv210_reset_lcd,
};


static int smdkv210_backlight_init(struct device *dev)
{
	int ret;

	ret = gpio_request(S5PV210_GPD0(3), "Backlight");
	if (ret) {
		printk(KERN_ERR "failed to request GPD for PWM-OUT 3\n");
		return ret;
	}

	/* Configure GPIO pin with S5PV210_GPD_0_3_TOUT_3 */
	s3c_gpio_cfgpin(S5PV210_GPD0(3), S3C_GPIO_SFN(2));

	return 0;
}

static void smdkv210_backlight_exit(struct device *dev)
{
	s3c_gpio_cfgpin(S5PV210_GPD0(3), S3C_GPIO_OUTPUT);
	gpio_free(S5PV210_GPD0(3));
}


static struct platform_pwm_backlight_data smdkv210_backlight_data = {
	.pwm_id		= 3,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 78770,
	.init		= smdkv210_backlight_init,
	.exit		= smdkv210_backlight_exit,
};

static struct platform_device smdkv210_backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.parent		= &s3c_device_timer[3].dev,
		.platform_data	= &smdkv210_backlight_data,
	},
};

static struct s3c64xx_spi_csinfo smdk_spi1_csi = {
	      .line = S5PV210_GPB(5),
	      .set_level = gpio_set_value,
	      .fb_delay = 0x2,
};

static struct spi_board_info s3c_spi_devs[] __initdata = {
	{
		.modalias        = "ams369fg06",
		.mode            = SPI_MODE_0,
		.max_speed_hz    = 500000,
		.bus_num         = 1,
		.chip_select     = 0,
		.controller_data = &smdk_spi1_csi,		
	},
};

struct platform_device sec_device_battery = {
        .name   = "sec-fake-battery",
        .id = -1,
};

static struct platform_device *smdkv210_devices[] __initdata = {
//	&s3c_device_adc,
//	&s3c_device_cfcon,


//	&s5pv210_device_spi0,

//	&s3c_device_hsmmc0,
//	&s3c_device_hsmmc1,
//	&s3c_device_hsmmc2,
//	&s3c_device_hsmmc3,
	&s3c_device_i2c0,
	&s3c_device_i2c1,
	&s3c_device_i2c2,
	&s3c_device_rtc,
	&s5p_device_smsc911x,	
	
	&sec_device_battery,
	
//	&s3c_device_ts,
//	&s3c_device_wdt,
//	&s5pv210_device_ac97,
//	&s5pv210_device_iis0,
//	&s5pv210_device_spdif,
//	&samsung_asoc_dma,

	&s5pv210_pd_g3d,
	&s5pv210_pd_lcd,
	&s5pv210_pd_audio,
	&s3c_device_g3d,
	&s3c_device_lcd,
	
	&s5pv210_device_ac97,
	&s5pv210_device_iis0,
	&s5pv210_device_spdif,
	&samsung_asoc_dma,
	
	&s3c_device_fb,

	&s5pv210_device_spi1,

//	&samsung_device_keypad,
	&s3c_device_timer[3],
#if 0	
	&smdkv210_backlight_device,
#endif	
	&aesop_device_gpiokeys,
};

/* I2C0 */
static struct i2c_board_info smdkv210_i2c_devs0[] __initdata = {
	{I2C_BOARD_INFO("hidis-ts", 0x46),},
	{ I2C_BOARD_INFO("max98088", 0x10), },
/*
{ 	I2C_BOARD_INFO("accel", 0x0F),
 		.platform_data = &mapphone_kxtf9_data,
 	},
*/ 	
};

static struct i2c_board_info smdkv210_i2c_devs1[] __initdata = {
	/* To Be Updated */
};

static struct i2c_board_info smdkv210_i2c_devs2[] __initdata = {
	/* To Be Updated */
};

static struct s3c2410_ts_mach_info s3c_ts_platform __initdata = {
	.delay			= 10000,
	.presc			= 49,
	.oversampling_shift	= 2,
};

static void __init smdkv210_map_io(void)
{
	s5p_init_io(NULL, 0, S5P_VA_CHIPID);
	s3c24xx_init_clocks(24000000);
	s3c24xx_init_uarts(smdkv210_uartcfgs, ARRAY_SIZE(smdkv210_uartcfgs));
	s5p_set_timer_source(S5P_PWM2, S5P_PWM4);
	
	s5p_reserve_bootmem(s5pv210_media_devs,
			ARRAY_SIZE(s5pv210_media_devs), S5P_RANGE_MFC);
			
}

static void __init smdkc110_smc911x_set(void)
{
	unsigned int readVar;
        unsigned int tmp;

        tmp = ((SMC9115_Tacs<<28)|(SMC9115_Tcos<<24)|(SMC9115_Tacc<<16)|(SMC9115_Tcoh<<12)|(SMC9115_Tah<<8)|(SMC9115_Tacp<<4)|(SMC9115_PMC));
        __raw_writel(tmp, (S5P_SROM_BW+0x14));

        tmp = __raw_readl(S5P_SROM_BW);
        tmp &= ~(0xf<<16);

        tmp |= (0x1<<16);
        tmp |= (0x1<<17);

        __raw_writel(tmp, S5P_SROM_BW);

        tmp = __raw_readl(S5PV210_MP01CON);
        tmp &= ~(0xf<<16);
        tmp |=(2<<16);

        __raw_writel(tmp,(S5PV210_MP01CON));
	
	readVar = readl(S5PV210_GPH0_BASE);
	writel(readVar|(0x0F<<24), S5PV210_GPH0_BASE);	
	    
//	s3c_gpio_cfgpin(S5PV210_GPH0(6),EINT_MODE);
	
}

static void __init aesop_navi_init(void)
{
	int gpio;

	gpio = S5PV210_GPC1(0);		
	 s5p_register_gpio_interrupt(gpio);

	gpio = S5PV210_GPC1(1);		
	s5p_register_gpio_interrupt(gpio);

	gpio = S5PV210_GPC1(2);		
	s5p_register_gpio_interrupt(gpio);

	gpio = S5PV210_GPA1(2);		
	s5p_register_gpio_interrupt(gpio);

	gpio = S5PV210_GPA1(3);		
	s5p_register_gpio_interrupt(gpio);
		
}	

static void __init smdkv210_machine_init(void)
{
	s3c_pm_init();

	s5pv210_gpiolib_init();
	
	//samsung_keypad_set_platdata(&smdkv210_keypad_data);
	
	s3c24xx_ts_set_platdata(&s3c_ts_platform);
	
	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);
	s3c_i2c2_set_platdata(NULL);
	i2c_register_board_info(0, smdkv210_i2c_devs0,
			ARRAY_SIZE(smdkv210_i2c_devs0));
	i2c_register_board_info(1, smdkv210_i2c_devs1,
			ARRAY_SIZE(smdkv210_i2c_devs1));
	i2c_register_board_info(2, smdkv210_i2c_devs2,
			ARRAY_SIZE(smdkv210_i2c_devs2));
	
	spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));
	
	s5pv210_spi_set_info(1,0,0);
	
	s3cfb_set_platdata(&aesop_lcd_data);

	smdkc110_smc911x_set();
	
	platform_add_devices(smdkv210_devices, ARRAY_SIZE(smdkv210_devices));
	
	aesop_navi_init();
	
	//smdkv210_cfg_gpio_spi1();
}

MACHINE_START(SMDKV210, "SMDKV210")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.init_irq	= s5pv210_init_irq,
	.map_io		= smdkv210_map_io,
	.init_machine	= smdkv210_machine_init,
	.timer		= &s5p_timer,
MACHINE_END

