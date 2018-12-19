// SPDX-License-Identifier: GPL-2.0
// TI LP50XX LED chip family driver
// Copyright (C) 2018 Texas Instruments Incorporated - http://www.ti.com/

#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regmap.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <uapi/linux/uleds.h>

#include <linux/led-class-multicolor.h>

#define LP50XX_DEV_CFG0		0x00
#define LP50XX_DEV_CFG1		0x01
#define LP50XX_LED_CFG0		0x02

/* LP5009 and LP5012 registers */
#define LP5012_BNK_BRT		0x03
#define LP5012_BNKA_CLR		0x04
#define LP5012_BNKB_CLR		0x05
#define LP5012_BNKC_CLR		0x06
#define LP5012_LED0_BRT		0x07
#define LP5012_LED1_BRT		0x08
#define LP5012_LED2_BRT		0x09
#define LP5012_LED3_BRT		0x0a
#define LP5012_OUT0_CLR		0x0b
#define LP5012_OUT1_CLR		0x0c
#define LP5012_OUT2_CLR		0x0d
#define LP5012_OUT3_CLR		0x0e
#define LP5012_OUT4_CLR		0x0f
#define LP5012_OUT5_CLR		0x10
#define LP5012_OUT6_CLR		0x11
#define LP5012_OUT7_CLR		0x12
#define LP5012_OUT8_CLR		0x13
#define LP5012_OUT9_CLR		0x14
#define LP5012_OUT10_CLR	0x15
#define LP5012_OUT11_CLR	0x16
#define LP5012_RESET		0x17

/* LP5018 and LP5024 registers */
#define LP5024_BNK_BRT		0x03
#define LP5024_BNKA_CLR		0x04
#define LP5024_BNKB_CLR		0x05
#define LP5024_BNKC_CLR		0x06
#define LP5024_LED0_BRT		0x07
#define LP5024_LED1_BRT		0x08
#define LP5024_LED2_BRT		0x09
#define LP5024_LED3_BRT		0x0a
#define LP5024_LED4_BRT		0x0b
#define LP5024_LED5_BRT		0x0c
#define LP5024_LED6_BRT		0x0d
#define LP5024_LED7_BRT		0x0e

#define LP5024_OUT0_CLR		0x0f
#define LP5024_OUT1_CLR		0x10
#define LP5024_OUT2_CLR		0x11
#define LP5024_OUT3_CLR		0x12
#define LP5024_OUT4_CLR		0x13
#define LP5024_OUT5_CLR		0x14
#define LP5024_OUT6_CLR		0x15
#define LP5024_OUT7_CLR		0x16
#define LP5024_OUT8_CLR		0x17
#define LP5024_OUT9_CLR		0x18
#define LP5024_OUT10_CLR	0x19
#define LP5024_OUT11_CLR	0x1a
#define LP5024_OUT12_CLR	0x1b
#define LP5024_OUT13_CLR	0x1c
#define LP5024_OUT14_CLR	0x1d
#define LP5024_OUT15_CLR	0x1e
#define LP5024_OUT16_CLR	0x1f
#define LP5024_OUT17_CLR	0x20
#define LP5024_OUT18_CLR	0x21
#define LP5024_OUT19_CLR	0x22
#define LP5024_OUT20_CLR	0x23
#define LP5024_OUT21_CLR	0x24
#define LP5024_OUT22_CLR	0x25
#define LP5024_OUT23_CLR	0x26
#define LP5024_RESET		0x27

/* LP5030 and LP5036 registers */
#define LP5036_LED_CFG1		0x03
#define LP5036_BNK_BRT		0x04
#define LP5036_BNKA_CLR		0x05
#define LP5036_BNKB_CLR		0x06
#define LP5036_BNKC_CLR		0x07
#define LP5036_LED0_BRT		0x08
#define LP5036_LED1_BRT		0x09
#define LP5036_LED2_BRT		0x0a
#define LP5036_LED3_BRT		0x0b
#define LP5036_LED4_BRT		0x0c
#define LP5036_LED5_BRT		0x0d
#define LP5036_LED6_BRT		0x0e
#define LP5036_LED7_BRT		0x0f
#define LP5036_LED8_BRT		0x10
#define LP5036_LED9_BRT		0x11
#define LP5036_LED10_BRT	0x12
#define LP5036_LED11_BRT	0x13

#define LP5036_OUT0_CLR		0x14
#define LP5036_OUT1_CLR		0x15
#define LP5036_OUT2_CLR		0x16
#define LP5036_OUT3_CLR		0x17
#define LP5036_OUT4_CLR		0x18
#define LP5036_OUT5_CLR		0x19
#define LP5036_OUT6_CLR		0x1a
#define LP5036_OUT7_CLR		0x1b
#define LP5036_OUT8_CLR		0x1c
#define LP5036_OUT9_CLR		0x1d
#define LP5036_OUT10_CLR	0x1e
#define LP5036_OUT11_CLR	0x1f
#define LP5036_OUT12_CLR	0x20
#define LP5036_OUT13_CLR	0x21
#define LP5036_OUT14_CLR	0x22
#define LP5036_OUT15_CLR	0x23
#define LP5036_OUT16_CLR	0x24
#define LP5036_OUT17_CLR	0x25
#define LP5036_OUT18_CLR	0x26
#define LP5036_OUT19_CLR	0x27
#define LP5036_OUT20_CLR	0x28
#define LP5036_OUT21_CLR	0x29
#define LP5036_OUT22_CLR	0x2a
#define LP5036_OUT23_CLR	0x2b
#define LP5036_OUT24_CLR	0x2c
#define LP5036_OUT25_CLR	0x2d
#define LP5036_OUT26_CLR	0x2e
#define LP5036_OUT27_CLR	0x2f
#define LP5036_OUT28_CLR	0x30
#define LP5036_OUT29_CLR	0x31
#define LP5036_OUT30_CLR	0x32
#define LP5036_OUT31_CLR	0x33
#define LP5036_OUT32_CLR	0x34
#define LP5036_OUT33_CLR	0x35
#define LP5036_OUT34_CLR	0x36
#define LP5036_OUT35_CLR	0x37
#define LP5036_RESET		0x38

#define LP50XX_SW_RESET		0xff
#define LP50XX_CHIP_EN		BIT(6)

/* There are 3 LED outputs per bank */
#define LP50XX_LEDS_PER_MODULE	3

#define LP5009_MAX_LED_MODULES	2
#define LP5012_MAX_LED_MODULES	4
#define LP5018_MAX_LED_MODULES	6
#define LP5024_MAX_LED_MODULES	8
#define LP5030_MAX_LED_MODULES	10
#define LP5036_MAX_LED_MODULES	12

#define LP5009_MAX_LEDS	(LP5009_MAX_LED_MODULES * LP50XX_LEDS_PER_MODULE)
#define LP5012_MAX_LEDS	(LP5012_MAX_LED_MODULES * LP50XX_LEDS_PER_MODULE)
#define LP5018_MAX_LEDS	(LP5018_MAX_LED_MODULES * LP50XX_LEDS_PER_MODULE)
#define LP5024_MAX_LEDS	(LP5024_MAX_LED_MODULES * LP50XX_LEDS_PER_MODULE)
#define LP5030_MAX_LEDS	(LP5030_MAX_LED_MODULES * LP50XX_LEDS_PER_MODULE)
#define LP5036_MAX_LEDS	(LP5036_MAX_LED_MODULES * LP50XX_LEDS_PER_MODULE)

struct lp50xx_led {
	struct led_classdev led_dev;
	struct led_classdev_mc mc_cdev;
	struct lp50xx *priv;
	unsigned long bank_modules;
	int led_intensity[LP50XX_LEDS_PER_MODULE];
	u8 ctrl_bank_enabled;
	int led_number;
};

/**
 * struct lp50xx -
 * @enable_gpio: hardware enable gpio
 * @regulator: LED supply regulator pointer
 * @client: pointer to the I2C client
 * @regmap: device register map
 * @dev: pointer to the devices device struct
 * @lock: lock for reading/writing the device
 * @chip_info: chip specific information (ie num_leds)
 * @num_of_banked_leds: holds the number of banked LEDs
 * @leds: array of LED strings
 */
struct lp50xx {
	struct gpio_desc *enable_gpio;
	struct regulator *regulator;
	struct i2c_client *client;
	struct regmap *regmap;
	struct device *dev;
	struct mutex lock;
	const struct lp50xx_chip_info *chip_info;
	int num_of_banked_leds;

	/* This needs to be at the end of the struct */
	struct lp50xx_led leds[];
};

static const struct reg_default lp5012_reg_defs[] = {
	{LP50XX_DEV_CFG0, 0x0},
	{LP50XX_DEV_CFG1, 0x3c},
	{LP50XX_LED_CFG0, 0x0},
	{LP5012_BNK_BRT, 0xff},
	{LP5012_BNKA_CLR, 0x0f},
	{LP5012_BNKB_CLR, 0x0f},
	{LP5012_BNKC_CLR, 0x0f},
	{LP5012_LED0_BRT, 0x0f},
	{LP5012_LED1_BRT, 0xff},
	{LP5012_LED2_BRT, 0xff},
	{LP5012_LED3_BRT, 0xff},
	{LP5012_OUT0_CLR, 0x0f},
	{LP5012_OUT1_CLR, 0x00},
	{LP5012_OUT2_CLR, 0x00},
	{LP5012_OUT3_CLR, 0x00},
	{LP5012_OUT4_CLR, 0x00},
	{LP5012_OUT5_CLR, 0x00},
	{LP5012_OUT6_CLR, 0x00},
	{LP5012_OUT7_CLR, 0x00},
	{LP5012_OUT8_CLR, 0x00},
	{LP5012_OUT9_CLR, 0x00},
	{LP5012_OUT10_CLR, 0x00},
	{LP5012_OUT11_CLR, 0x00},
	{LP5012_RESET, 0x00}
};

static const struct reg_default lp5024_reg_defs[] = {
	{LP50XX_DEV_CFG0, 0x0},
	{LP50XX_DEV_CFG1, 0x3c},
	{LP50XX_LED_CFG0, 0x0},
	{LP5024_BNK_BRT, 0xff},
	{LP5024_BNKA_CLR, 0x0f},
	{LP5024_BNKB_CLR, 0x0f},
	{LP5024_BNKC_CLR, 0x0f},
	{LP5024_LED0_BRT, 0x0f},
	{LP5024_LED1_BRT, 0xff},
	{LP5024_LED2_BRT, 0xff},
	{LP5024_LED3_BRT, 0xff},
	{LP5024_LED4_BRT, 0xff},
	{LP5024_LED5_BRT, 0xff},
	{LP5024_LED6_BRT, 0xff},
	{LP5024_LED7_BRT, 0xff},
	{LP5024_OUT0_CLR, 0x0f},
	{LP5024_OUT1_CLR, 0x00},
	{LP5024_OUT2_CLR, 0x00},
	{LP5024_OUT3_CLR, 0x00},
	{LP5024_OUT4_CLR, 0x00},
	{LP5024_OUT5_CLR, 0x00},
	{LP5024_OUT6_CLR, 0x00},
	{LP5024_OUT7_CLR, 0x00},
	{LP5024_OUT8_CLR, 0x00},
	{LP5024_OUT9_CLR, 0x00},
	{LP5024_OUT10_CLR, 0x00},
	{LP5024_OUT11_CLR, 0x00},
	{LP5024_OUT12_CLR, 0x00},
	{LP5024_OUT13_CLR, 0x00},
	{LP5024_OUT14_CLR, 0x00},
	{LP5024_OUT15_CLR, 0x00},
	{LP5024_OUT16_CLR, 0x00},
	{LP5024_OUT17_CLR, 0x00},
	{LP5024_OUT18_CLR, 0x00},
	{LP5024_OUT19_CLR, 0x00},
	{LP5024_OUT20_CLR, 0x00},
	{LP5024_OUT21_CLR, 0x00},
	{LP5024_OUT22_CLR, 0x00},
	{LP5024_OUT23_CLR, 0x00},
	{LP5024_RESET, 0x00}
};

static const struct reg_default lp5036_reg_defs[] = {
	{LP50XX_DEV_CFG0, 0x0},
	{LP50XX_DEV_CFG1, 0x3c},
	{LP50XX_LED_CFG0, 0x0},
	{LP5036_LED_CFG1, 0x0},
	{LP5036_BNK_BRT, 0xff},
	{LP5036_BNKA_CLR, 0x0f},
	{LP5036_BNKB_CLR, 0x0f},
	{LP5036_BNKC_CLR, 0x0f},
	{LP5036_LED0_BRT, 0x0f},
	{LP5036_LED1_BRT, 0xff},
	{LP5036_LED2_BRT, 0xff},
	{LP5036_LED3_BRT, 0xff},
	{LP5036_LED4_BRT, 0xff},
	{LP5036_LED5_BRT, 0xff},
	{LP5036_LED6_BRT, 0xff},
	{LP5036_LED7_BRT, 0xff},
	{LP5036_OUT0_CLR, 0x0f},
	{LP5036_OUT1_CLR, 0x00},
	{LP5036_OUT2_CLR, 0x00},
	{LP5036_OUT3_CLR, 0x00},
	{LP5036_OUT4_CLR, 0x00},
	{LP5036_OUT5_CLR, 0x00},
	{LP5036_OUT6_CLR, 0x00},
	{LP5036_OUT7_CLR, 0x00},
	{LP5036_OUT8_CLR, 0x00},
	{LP5036_OUT9_CLR, 0x00},
	{LP5036_OUT10_CLR, 0x00},
	{LP5036_OUT11_CLR, 0x00},
	{LP5036_OUT12_CLR, 0x00},
	{LP5036_OUT13_CLR, 0x00},
	{LP5036_OUT14_CLR, 0x00},
	{LP5036_OUT15_CLR, 0x00},
	{LP5036_OUT16_CLR, 0x00},
	{LP5036_OUT17_CLR, 0x00},
	{LP5036_OUT18_CLR, 0x00},
	{LP5036_OUT19_CLR, 0x00},
	{LP5036_OUT20_CLR, 0x00},
	{LP5036_OUT21_CLR, 0x00},
	{LP5036_OUT22_CLR, 0x00},
	{LP5036_OUT23_CLR, 0x00},
	{LP5036_OUT24_CLR, 0x00},
	{LP5036_OUT25_CLR, 0x00},
	{LP5036_OUT26_CLR, 0x00},
	{LP5036_OUT27_CLR, 0x00},
	{LP5036_OUT28_CLR, 0x00},
	{LP5036_OUT29_CLR, 0x00},
	{LP5036_OUT30_CLR, 0x00},
	{LP5036_OUT31_CLR, 0x00},
	{LP5036_OUT32_CLR, 0x00},
	{LP5036_OUT33_CLR, 0x00},
	{LP5036_OUT34_CLR, 0x00},
	{LP5036_OUT35_CLR, 0x00},
	{LP5036_RESET, 0x00}
};

static const struct regmap_config lp5012_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = LP5012_RESET,
	.reg_defaults = lp5012_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(lp5012_reg_defs),
	.cache_type = REGCACHE_RBTREE,
};

static const struct regmap_config lp5024_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = LP5024_RESET,
	.reg_defaults = lp5024_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(lp5024_reg_defs),
	.cache_type = REGCACHE_RBTREE,
};

static const struct regmap_config lp5036_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,

	.max_register = LP5036_RESET,
	.reg_defaults = lp5036_reg_defs,
	.num_reg_defaults = ARRAY_SIZE(lp5036_reg_defs),
	.cache_type = REGCACHE_RBTREE,
};

enum lp50xx_model {
	LP5009,
	LP5012,
	LP5018,
	LP5024,
	LP5030,
	LP5036,
};

/*
 * struct lp50xx_chip_info -
 * @num_leds: number of LED outputs available on the device
 * @led_brightness0_reg: first brightness register of the device
 * @mix_out0_reg: first color mix register of the device
 * @bank_brt_reg: bank brightness register
 * @bank_mix_reg: color mix register
 * @reset_reg: device reset register
 */
struct lp50xx_chip_info {
	struct regmap_config lp50xx_regmap_config;
	u8 num_leds;
	u8 led_brightness0_reg;
	u8 mix_out0_reg;
	u8 bank_brt_reg;
	u8 bank_mix_reg;
	u8 reset_reg;
};

static const struct lp50xx_chip_info lp50xx_chip_info_tbl[] = {
	[LP5009] = {
		.num_leds = LP5009_MAX_LEDS,
		.led_brightness0_reg = LP5012_LED0_BRT,
		.mix_out0_reg = LP5012_OUT0_CLR,
		.bank_brt_reg = LP5012_BNK_BRT,
		.bank_mix_reg = LP5012_BNKA_CLR,
		.reset_reg = LP5012_RESET,
		.lp50xx_regmap_config = lp5012_regmap_config,
	},
	[LP5012] = {
		.num_leds = LP5012_MAX_LEDS,
		.led_brightness0_reg = LP5012_LED0_BRT,
		.mix_out0_reg = LP5012_OUT0_CLR,
		.bank_brt_reg = LP5012_BNK_BRT,
		.bank_mix_reg = LP5012_BNKA_CLR,
		.reset_reg = LP5012_RESET,
		.lp50xx_regmap_config = lp5012_regmap_config,
	},
	[LP5018] = {
		.num_leds = LP5018_MAX_LEDS,
		.led_brightness0_reg = LP5024_LED0_BRT,
		.mix_out0_reg = LP5024_OUT0_CLR,
		.bank_brt_reg = LP5024_BNK_BRT,
		.bank_mix_reg = LP5024_BNKA_CLR,
		.reset_reg = LP5024_RESET,
		.lp50xx_regmap_config = lp5024_regmap_config,
	},
	[LP5024] = {
		.num_leds = LP5024_MAX_LEDS,
		.led_brightness0_reg = LP5024_LED0_BRT,
		.mix_out0_reg = LP5024_OUT0_CLR,
		.bank_brt_reg = LP5024_BNK_BRT,
		.bank_mix_reg = LP5024_BNKA_CLR,
		.reset_reg = LP5024_RESET,
		.lp50xx_regmap_config = lp5024_regmap_config,
	},
	[LP5030] = {
		.num_leds = LP5030_MAX_LEDS,
		.led_brightness0_reg = LP5036_LED0_BRT,
		.mix_out0_reg = LP5036_OUT0_CLR,
		.bank_brt_reg = LP5036_BNK_BRT,
		.bank_mix_reg = LP5036_BNKA_CLR,
		.reset_reg = LP5036_RESET,
		.lp50xx_regmap_config = lp5036_regmap_config,
	},
	[LP5036] = {
		.num_leds = LP5036_MAX_LEDS,
		.led_brightness0_reg = LP5036_LED0_BRT,
		.mix_out0_reg = LP5036_OUT0_CLR,
		.bank_brt_reg = LP5036_BNK_BRT,
		.bank_mix_reg = LP5036_BNKA_CLR,
		.reset_reg = LP5036_RESET,
		.lp50xx_regmap_config = lp5036_regmap_config,
	},
};

static int lp50xx_brightness_set(struct led_classdev *cdev,
			     enum led_brightness brightness)
{
	struct lp50xx_led *led = container_of(cdev, struct lp50xx_led, led_dev);
	const struct lp50xx_chip_info *led_chip = led->priv->chip_info;
	struct led_mc_color_entry *color_data;
	u8 led_offset, reg_val, reg_color_offset;
	int ret = 0;

	mutex_lock(&led->priv->lock);

	if (led->ctrl_bank_enabled)
		reg_val = led_chip->bank_brt_reg;
	else
		reg_val = led_chip->led_brightness0_reg +
			  led->led_number;

	ret = regmap_write(led->priv->regmap, reg_val, brightness);
	if (ret) {
		dev_err(&led->priv->client->dev,
			"Cannot write brightness value %d\n", ret);
		goto out;
	}

	list_for_each_entry(color_data, &led->mc_cdev.color_list, list) {
		if (color_data->led_color_id == LED_COLOR_ID_RED)
			reg_color_offset = 0;
		else if (color_data->led_color_id == LED_COLOR_ID_GREEN)
			reg_color_offset = 1;
		else if (color_data->led_color_id == LED_COLOR_ID_BLUE)
			reg_color_offset = 2;
		else
			continue;

		if (led->ctrl_bank_enabled) {
			reg_val = led_chip->bank_mix_reg + reg_color_offset;
		} else {
			led_offset = (led->led_number * 3)  + reg_color_offset;
			reg_val = led_chip->mix_out0_reg + led_offset;
		}

		ret = regmap_write(led->priv->regmap, reg_val,
				   color_data->intensity);
		if (ret) {
			dev_err(&led->priv->client->dev,
				"Cannot write intensity value %d\n", ret);
			goto out;
		}
	}
out:
	mutex_unlock(&led->priv->lock);
	return ret;
}

static enum led_brightness lp50xx_brightness_get(struct led_classdev *cdev)
{
	struct lp50xx_led *led = container_of(cdev, struct lp50xx_led, led_dev);
	const struct lp50xx_chip_info *led_chip = led->priv->chip_info;
	unsigned int brt_val;
	u8 reg_val;
	int ret;

	mutex_lock(&led->priv->lock);

	if (led->ctrl_bank_enabled)
		reg_val = led_chip->bank_brt_reg;
	else
		reg_val = led_chip->led_brightness0_reg + led->led_number;

	ret = regmap_read(led->priv->regmap, reg_val, &brt_val);

	mutex_unlock(&led->priv->lock);

	return brt_val;
}

static int lp50xx_set_banks(struct lp50xx *priv, u32 led_banks[])
{
	u8 led_config_lo, led_config_hi;
	u32 bank_enable_mask = 0;
	int ret;
	int i;

	for (i = 0; i < priv->chip_info->num_leds; i++)
		bank_enable_mask |= (1 << led_banks[i]);

	led_config_lo = (u8)(bank_enable_mask & 0xff);
	led_config_hi = (u8)(bank_enable_mask >> 8) & 0xff;

	ret = regmap_write(priv->regmap, LP50XX_LED_CFG0, led_config_lo);
	if (ret)
		return ret;

	if (led_config_hi)
		ret = regmap_write(priv->regmap, LP5036_LED_CFG1,
				   led_config_hi);

	return ret;
}

static int lp50xx_reset(struct lp50xx *priv)
{
	if (priv->enable_gpio)
		return gpiod_direction_output(priv->enable_gpio, 1);
	else
		return regmap_write(priv->regmap, priv->chip_info->reset_reg,
				    LP50XX_SW_RESET);
}

static int lp50xx_enable_disable(struct lp50xx *priv, u8 enable_disable)
{
	return regmap_write(priv->regmap, LP50XX_DEV_CFG0, enable_disable);
}

static int lp50xx_probe_dt(struct lp50xx *priv)
{
	u32 led_banks[LP5036_MAX_LED_MODULES];
	struct fwnode_handle *child = NULL;
	struct fwnode_handle *led_node = NULL;
	struct led_init_data init_data;
	struct lp50xx_led *led;
	int num_colors;
	u32 color_id;
	int led_number;
	size_t i = 0;
	int ret;

	priv->enable_gpio = devm_gpiod_get_optional(&priv->client->dev,
						   "enable", GPIOD_OUT_LOW);
	if (IS_ERR(priv->enable_gpio)) {
		ret = PTR_ERR(priv->enable_gpio);
		dev_err(&priv->client->dev, "Failed to get enable gpio: %d\n",
			ret);
		return ret;
	}

	priv->regulator = devm_regulator_get(&priv->client->dev, "vled");
	if (IS_ERR(priv->regulator))
		priv->regulator = NULL;

	device_for_each_child_node(&priv->client->dev, child) {
		led = &priv->leds[i];
		if (fwnode_property_present(child, "ti,led-bank")) {
			ret = fwnode_property_read_u32_array(child,
							     "ti,led-bank",
							     NULL, 0);
			ret = fwnode_property_read_u32_array(child,
							     "ti,led-bank",
							     led_banks,
							     ret);

			priv->num_of_banked_leds = ARRAY_SIZE(led_banks);

			ret = lp50xx_set_banks(priv, led_banks);
			if (ret) {
				dev_err(&priv->client->dev,
					"Cannot setup banked LEDs\n");
				fwnode_handle_put(child);
				goto child_out;
			}
			led->ctrl_bank_enabled = 1;

		} else {
			ret = fwnode_property_read_u32(child, "reg",
					       &led_number);

			led->led_number = led_number;
		}
		if (ret) {
			dev_err(&priv->client->dev,
				"led sourcing property missing\n");
			fwnode_handle_put(child);
			goto child_out;
		}

		if (led_number > priv->chip_info->num_leds) {
			dev_err(&priv->client->dev,
				"led-sources property is invalid\n");
			ret = -EINVAL;
			fwnode_handle_put(child);
			goto child_out;
		}

		init_data.fwnode = child;
		fwnode_property_read_string(child, "linux,default-trigger",
				    &led->led_dev.default_trigger);
		num_colors = 0;

		fwnode_for_each_child_node(child, led_node) {
			ret = fwnode_property_read_u32(led_node, "color",
						       &color_id);
			if (ret)
				dev_err(&priv->client->dev,
				"Cannot read color\n");

			set_bit(color_id, &led->mc_cdev.available_colors);
			num_colors++;

		}

		led->priv = priv;
		led->mc_cdev.num_leds = num_colors;
		led->mc_cdev.led_cdev = &led->led_dev;
		led->led_dev.brightness_set_blocking = lp50xx_brightness_set;
		led->led_dev.brightness_get = lp50xx_brightness_get;
		ret = led_classdev_multicolor_register_ext(&priv->client->dev,
						       &led->mc_cdev,
						       &init_data);
		if (ret) {
			dev_err(&priv->client->dev, "led register err: %d\n",
				ret);
			fwnode_handle_put(child);
			goto child_out;
		}
		i++;
	}

child_out:
	return ret;
}

static int lp50xx_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct lp50xx *led;
	int count;
	int ret;

	count = device_get_child_node_count(&client->dev);
	if (!count) {
		dev_err(&client->dev, "LEDs are not defined in device tree!");
		return -ENODEV;
	}

	led = devm_kzalloc(&client->dev, struct_size(led, leds, count),
			   GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	mutex_init(&led->lock);
	led->client = client;
	led->dev = &client->dev;
	led->chip_info = &lp50xx_chip_info_tbl[id->driver_data];
	i2c_set_clientdata(client, led);

	led->regmap = devm_regmap_init_i2c(client,
					&led->chip_info->lp50xx_regmap_config);
	if (IS_ERR(led->regmap)) {
		ret = PTR_ERR(led->regmap);
		dev_err(&client->dev, "Failed to allocate register map: %d\n",
			ret);
		return ret;
	}

	ret = lp50xx_reset(led);
	if (ret)
		return ret;

	ret = lp50xx_probe_dt(led);
	if (ret)
		return ret;

	return lp50xx_enable_disable(led, LP50XX_CHIP_EN);
}

static int lp50xx_remove(struct i2c_client *client)
{
	struct lp50xx *led = i2c_get_clientdata(client);
	int ret;

	ret = lp50xx_enable_disable(led, LP50XX_CHIP_EN);
	if (ret) {
		dev_err(&led->client->dev, "Failed to disable regulator\n");
		return ret;
	}

	if (led->enable_gpio)
		gpiod_direction_output(led->enable_gpio, 0);

	if (led->regulator) {
		ret = regulator_disable(led->regulator);
		if (ret)
			dev_err(&led->client->dev,
				"Failed to disable regulator\n");
	}

	mutex_destroy(&led->lock);

	return 0;
}

static const struct i2c_device_id lp50xx_id[] = {
	{ "lp5009", LP5009 },
	{ "lp5012", LP5012 },
	{ "lp5018", LP5018 },
	{ "lp5024", LP5024 },
	{ "lp5030", LP5030 },
	{ "lp5036", LP5036 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lp50xx_id);

static const struct of_device_id of_lp50xx_leds_match[] = {
	{ .compatible = "ti,lp5009", .data = (void *)LP5009 },
	{ .compatible = "ti,lp5012", .data = (void *)LP5012 },
	{ .compatible = "ti,lp5018", .data = (void *)LP5018 },
	{ .compatible = "ti,lp5024", .data = (void *)LP5024 },
	{ .compatible = "ti,lp5030", .data = (void *)LP5030 },
	{ .compatible = "ti,lp5036", .data = (void *)LP5036 },
	{},
};
MODULE_DEVICE_TABLE(of, of_lp50xx_leds_match);

static struct i2c_driver lp50xx_driver = {
	.driver = {
		.name	= "lp50xx",
		.of_match_table = of_lp50xx_leds_match,
	},
	.probe		= lp50xx_probe,
	.remove		= lp50xx_remove,
	.id_table	= lp50xx_id,
};
module_i2c_driver(lp50xx_driver);

MODULE_DESCRIPTION("Texas Instruments LP50XX LED driver");
MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com>");
MODULE_LICENSE("GPL v2");
