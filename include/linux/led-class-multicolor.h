/* SPDX-License-Identifier: GPL-2.0 */
/* LED Multicolor class interface
 * Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/
 */

#ifndef __LINUX_MULTICOLOR_LEDS_H_INCLUDED
#define __LINUX_MULTICOLOR_LEDS_H_INCLUDED

#include <linux/leds.h>
#include <dt-bindings/leds/common.h>

struct led_classdev_mc;

struct led_mc_color_entry {
	struct led_classdev_mc *mcled_cdev;

	struct device_attribute max_intensity_attr;
	struct device_attribute intensity_attr;

	enum led_brightness max_intensity;
	enum led_brightness intensity;

	struct list_head list;

	int led_color_id;
};

struct led_classdev_mc {
	/* led class device */
	struct led_classdev *led_cdev;
	struct list_head color_list;

	unsigned long available_colors;
	int num_leds;
};

static inline struct led_classdev_mc *lcdev_to_mccdev(
						struct led_classdev *lcdev)
{
	return container_of(lcdev, struct led_classdev_mc, led_cdev);
}

/**
 * led_classdev_multicolor_register_ext - register a new object of led_classdev
 *				      class with support for multicolor LEDs
 * @parent: the multicolor LED to register
 * @mcled_cdev: the led_classdev_mc structure for this device
 * @init_data: the LED class Multi color device initialization data
 *
 * Returns: 0 on success or negative error value on failure
 */
int led_classdev_multicolor_register_ext(struct device *parent,
					    struct led_classdev_mc *mcled_cdev,
					    struct led_init_data *init_data);

#define led_classdev_multicolor_register(parent, mcled_cdev)		\
	led_classdev_multicolor_register_ext(parent, mcled_cdev, NULL)

/**
 * led_classdev_multicolor_unregister - unregisters an object of led_classdev
 *					class with support for multicolor LEDs
 * @mcled_cdev: the multicolor LED to unregister
 *
 * Unregister a previously registered via led_classdev_multicolor_register
 * object
 */
void led_classdev_multicolor_unregister(struct led_classdev_mc *mcled_cdev);

/* Calculate brightness for the monochrome LED cluster */
void led_mc_calc_brightness(struct led_classdev_mc *mcled_cdev,
			    enum led_brightness brightness,
			    int brightness_val[]);

#endif	/* __LINUX_MULTICOLOR_LEDS_H_INCLUDED */
