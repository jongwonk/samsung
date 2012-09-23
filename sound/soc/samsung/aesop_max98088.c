/*
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Author: Chanwoo Choi <cw00.choi@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>

#include <sound/soc.h>
#include <sound/jack.h>

#include <asm/mach-types.h>
#include <mach/gpio.h>
#include <linux/clk.h>

#include "../codecs/max98088.h"

#define MACHINE_NAME	0
#define CPU_VOICE_DAI	1

static struct platform_device *aesop_snd_device;

/* 3.5 pie jack
// static struct snd_soc_jack jack;

// 3.5 pie jack detection DAPM pins
static struct snd_soc_jack_pin jack_pins[] = {
	{
		.pin = "Headset Mic",
		.mask = SND_JACK_MICROPHONE,
	}, {
		.pin = "Headset Stereophone",
		.mask = SND_JACK_HEADPHONE | SND_JACK_MECHANICAL |
			SND_JACK_AVOUT,
	},
};

// 3.5 pie jack detection gpios
static struct snd_soc_jack_gpio jack_gpios[] = {
	{
		.gpio = S5PV210_GPH0(6),
		.name = "DET_3.5",
		.report = SND_JACK_HEADSET | SND_JACK_MECHANICAL |
			SND_JACK_AVOUT,
		.debounce_time = 200,
	},
};

*/

static const struct snd_soc_dapm_widget aesop_dapm_widgets[] = {
	SND_SOC_DAPM_LINE("Headset Out", NULL),
	SND_SOC_DAPM_LINE("Speaker Out", NULL),
	SND_SOC_DAPM_MIC("FrontMic In", NULL),
	SND_SOC_DAPM_MIC("HeadsetMic In", NULL),
	
};

static const struct snd_soc_dapm_route aesop_dapm_routes[] = {
	/* Connections to Headset */

	{"Headset Out", NULL, "HP Left Out"},
	{"Headset Out", NULL, "HP Right Out"},

	/* Connections to Speaker */
//	{"Speaker Out", NULL, "Speaker Left"},
//	{"Speaker Out", NULL, "Speaker Right"},


	{"HeadsetMic In", NULL, "INB1 Input"},
	{"HeadsetMic In", NULL, "INB2 Input"},


};

static int set_mclk_rate(unsigned long rate)
{
	struct clk *fout_epll;

	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return -ENOENT;
	}

	if (rate == clk_get_rate(fout_epll))
		goto out;

	clk_disable(fout_epll);
	clk_set_rate(fout_epll, rate);
	clk_enable(fout_epll);

out:
	clk_put(fout_epll);

	return 0;
}

static int clk_init(void)
{
	struct clk *a, *b, *c, *d,*e;

	a = clk_get(NULL, "fout_epll");
	b = clk_get(NULL, "mout_epll");
	c = clk_get(NULL, "mout_audss");
	d = clk_get(NULL, "audio-bus");
	e = clk_get(NULL, "dout_audio_bus_clk_i2s"); 
	
	clk_set_parent(b,a);
	clk_set_parent(c,a);
	clk_set_parent(d,c);
	clk_set_parent(e,c);
	
	clk_enable(a);
	clk_enable(b);
	clk_enable(c);
	clk_enable(d);

	return 0;
}

static int aesop_max98088_init(struct snd_soc_pcm_runtime *rtd)
{
	struct snd_soc_codec *codec = rtd->codec;
	struct snd_soc_dapm_context *dapm = &codec->dapm;
	int  i ;

	/* add aesop specific widgets */
	snd_soc_dapm_new_controls(dapm, aesop_dapm_widgets,
			ARRAY_SIZE(aesop_dapm_widgets));

	/* set up aesop specific audio routes */
	snd_soc_dapm_add_routes(dapm, aesop_dapm_routes,
			ARRAY_SIZE(aesop_dapm_routes));

	/* No jack detect - mark all jacks as enabled */
	for(i=0; i<ARRAY_SIZE(aesop_dapm_widgets);i++)
	{
	  snd_soc_dapm_enable_pin(dapm, aesop_dapm_widgets[i].name);
	}

	
	/* Set up default in/out pins */
	
	snd_soc_dapm_enable_pin(dapm, "Headset Out");
	snd_soc_dapm_enable_pin(dapm, "HeadsetMic In");
		
	snd_soc_dapm_disable_pin(dapm, "FrontMic In");
	snd_soc_dapm_disable_pin(dapm, "Speaker Out");


	snd_soc_dapm_sync(dapm);

	return 0;
}

static int aesop_hifi_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	struct snd_soc_dai *cpu_dai = rtd->cpu_dai;
	unsigned int fmclk;
	int ret = 0;

	fmclk = set_mclk_rate(params_rate(params) * 1536); // 16K*1536 <= 24.576MHz
	
	// set the cpu DAI configuration 
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM); // master
	if (ret < 0)
		return ret;

	// set codec DAI configuration 
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_I2S |
			SND_SOC_DAIFMT_NB_NF | SND_SOC_DAIFMT_CBM_CFM); // master
	if (ret < 0)
		return ret;

	// set the codec system clock
	ret = snd_soc_dai_set_sysclk(codec_dai, 0,fmclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	return 0;
}

static struct snd_soc_ops aesop_hifi_ops = {
	.hw_params =  aesop_hifi_hw_params,
};

static int aesop_voice_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *codec_dai = rtd->codec_dai;
	unsigned int fmclk;
	int ret = 0;

	fmclk = set_mclk_rate(params_rate(params) * 1536); // 16K*1536  <= 24.576MHz
	
	// set codec DAI configuration 
	ret = snd_soc_dai_set_fmt(codec_dai, SND_SOC_DAIFMT_LEFT_J |
			SND_SOC_DAIFMT_IB_IF | SND_SOC_DAIFMT_CBM_CFM); 
	if (ret < 0)
		return ret;

	// set the codec system clock 
	ret = snd_soc_dai_set_sysclk(codec_dai, 0,fmclk, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

	
	return 0;
}
/*
static struct snd_soc_ops aesop_voice_ops = {
	.hw_params = aesop_voice_hw_params,
};
*/

/*

{
	.name = "MAX98088 Voice",
	.stream_name = "Voice",
	.cpu_dai_name = "samsung-i2s.4",
	.codec_dai_name = "max98088_Aux",
	.platform_name = "samsung-audio",
	.codec_name = "max98088-codec.0-0010",
	.ops = &aesop_voice_ops,
},
*/

/*
struct snd_soc_card aesop = {
	.name = "aesop",
	.dai_link = aesop_dai_link,
	.num_links = ARRAY_SIZE(aesop_dai_link),
};
*/

struct snd_soc_dai_link aesop_dai_link[] = {
{
	.name = "MAX98088",
	.stream_name = "HiFi Playback",
	.cpu_dai_name = "samsung-i2s.0",
	.codec_dai_name = "max98088_HiFi",
	.platform_name = "samsung-idma", //"samsung-pcm", //
	.codec_name = "max98088.0-0010",
	.init = aesop_max98088_init,
	.ops = &aesop_hifi_ops,
},

};

struct snd_soc_card aesop = {
	.name = "aesop",
	.dai_link = aesop_dai_link,
	.num_links = ARRAY_SIZE(aesop_dai_link),
};

static int __init aesop_init(void)
{
	int ret;

	aesop_snd_device = platform_device_alloc("soc-audio",-1);
	
	if (!aesop_snd_device)
	{
		printk("\nerror - 1\n\n");
		return -ENOMEM;
	}
	
	printk("\npass - 1\n\n");
	
	platform_set_drvdata(aesop_snd_device, &aesop);
	ret = platform_device_add(aesop_snd_device);

	
	if (ret) {
		printk("\npass - 2\n\n");
		platform_device_put(aesop_snd_device);
	}

	printk("\npass - 3\n\n");
	clk_init();
	
	printk("\npass - 4\n\n");
	return ret;
}

static void __exit aesop_exit(void)
{
	platform_device_unregister(aesop_snd_device);
}

module_init(aesop_init);
module_exit(aesop_exit);

/* Module information */
MODULE_DESCRIPTION("ALSA SoC MAX98088 AESOP(S5PV210)");
MODULE_AUTHOR("noname <noname@samsung.com>");
MODULE_LICENSE("GPL");
