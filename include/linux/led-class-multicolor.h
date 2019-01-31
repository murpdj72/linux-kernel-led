// SPDX-License-Identifier: GPL-2.0
/* LED Multicolor class interface
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 */

#ifndef __LINUX_MULTICOLOR_LEDS_H_INCLUDED
#define __LINUX_MULTICOLOR_LEDS_H_INCLUDED

#include <linux/leds.h>
#include <dt-bindings/leds/common.h>

struct led_classdev_mc;

struct led_multicolor_ops {
	/* Set brightness for a specific color id */
	int (*set_color_brightness)(struct led_classdev_mc *mcled_cdev,
				    int color_id, int value);
	/* Read current color setting */
	int (*get_color_brightness)(struct led_classdev_mc *mcled_cdev,
				    int color_id);
};

struct led_classdev_mc {
	/* led class device */
	struct led_classdev led_cdev;

	/* multicolor led specific ops */
	struct led_multicolor_ops *ops;

	u32 available_colors[LED_COLOR_ID_MAX];
	int num_of_leds;

	bool sync_enabled;
};

static inline struct led_classdev_mc *lcdev_to_mccdev(
						struct led_classdev *lcdev)
{
	return container_of(lcdev, struct led_classdev_mc, led_cdev);
}

/**
 * led_classdev_multicolor_register - register a new object of led_classdev
 *				      class with support for multicolor LEDs
 * @parent: the multicolor LED to register
 * @mcled_cdev: the led_classdev_mc structure for this device
 *
 * Returns: 0 on success or negative error value on failure
 */
extern int led_classdev_multicolor_register(struct device *parent,
					    struct led_classdev_mc *mcled_cdev);

/**
 * led_classdev_multicolor_unregister - unregisters an object of led_classdev
 *					class with support for multicolor LEDs
 * @mcled_cdev: the multicolor LED to unregister
 *
 * Unregister a previously registered via led_classdev_multicolor_register
 * object
 */
extern void led_classdev_multicolor_unregister(struct led_classdev_mc *mcled_cdev);

extern int devm_led_classdev_multicolor_register(struct device *parent,
						 struct led_classdev_mc *mcled_cdev);

extern void devm_led_classdev_multicolor_unregister(struct device *parent,
						    struct led_classdev_mc *mcled_cdev);

#endif	/* __LINUX_MULTICOLOR_LEDS_H_INCLUDED */
