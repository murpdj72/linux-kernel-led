// SPDX-License-Identifier: GPL-2.0
// LED Multi Color class interface
// Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/

#include <linux/device.h>
#include <linux/init.h>
#include <linux/led-class-multicolor.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "leds.h"

#define INTENSITY_NAME		"_intensity"
#define MAX_INTENSITY_NAME	"_max_intensity"

void led_mc_calc_brightness(struct led_classdev_mc *mcled_cdev,
			    enum led_brightness brightness,
			    int brightness_val[])
{
	struct led_mc_color_entry *priv;
	int i = 0;

	list_for_each_entry(priv, &mcled_cdev->color_list, list) {
		brightness_val[i] = brightness *
				    priv->intensity / priv->max_intensity;
		i++;
	}
}
EXPORT_SYMBOL_GPL(led_mc_calc_brightness);

static ssize_t intensity_store(struct device *dev,
				struct device_attribute *intensity_attr,
				const char *buf, size_t size)
{
	struct led_mc_color_entry *priv = container_of(intensity_attr,
						    struct led_mc_color_entry,
						      intensity_attr);
	struct led_classdev *led_cdev = priv->mcled_cdev->led_cdev;
	unsigned long value;
	ssize_t ret;

	mutex_lock(&led_cdev->led_access);

	ret = kstrtoul(buf, 10, &value);
	if (ret)
		goto unlock;

	if (value > priv->max_intensity) {
		ret = -EINVAL;
		goto unlock;
	}

	priv->intensity = value;
	ret = size;

unlock:
	mutex_unlock(&led_cdev->led_access);
	return ret;
}

static ssize_t intensity_show(struct device *dev,
			      struct device_attribute *intensity_attr,
			      char *buf)
{
	struct led_mc_color_entry *priv = container_of(intensity_attr,
						    struct led_mc_color_entry,
						      intensity_attr);

	return sprintf(buf, "%d\n", priv->intensity);
}

static ssize_t max_intensity_show(struct device *dev,
				   struct device_attribute *max_intensity_attr,
				   char *buf)
{
	struct led_mc_color_entry *priv = container_of(max_intensity_attr,
						    struct led_mc_color_entry,
						      max_intensity_attr);

	return sprintf(buf, "%d\n", priv->max_intensity);
}

static struct attribute *led_color_attrs[] = {
	NULL,
};

static struct attribute_group led_color_group = {
	.name = "colors",
	.attrs = led_color_attrs,
};

static int led_multicolor_init_color(struct led_classdev_mc *mcled_cdev,
				     int color_id, int color_index)
{
	struct led_classdev *led_cdev = mcled_cdev->led_cdev;
	struct led_mc_color_entry *mc_priv;
	char *intensity_file_name;
	char *max_intensity_file_name;
	size_t len;
	int ret;

	mc_priv = devm_kzalloc(led_cdev->dev, sizeof(*mc_priv), GFP_KERNEL);
	if (!mc_priv)
		return -ENOMEM;

	mc_priv->led_color_id = color_id;
	mc_priv->mcled_cdev = mcled_cdev;

	sysfs_attr_init(&mc_priv->intensity_attr.attr);
	len = strlen(led_colors[color_id]) + strlen(INTENSITY_NAME) + 1;
	intensity_file_name = kzalloc(len, GFP_KERNEL);
	if (!intensity_file_name)
		return -ENOMEM;

	snprintf(intensity_file_name, len, "%s%s",
		 led_colors[color_id], INTENSITY_NAME);
	mc_priv->intensity_attr.attr.name = intensity_file_name;
	mc_priv->intensity_attr.attr.mode = 644;
	mc_priv->intensity_attr.store = intensity_store;
	mc_priv->intensity_attr.show = intensity_show;
	ret = sysfs_add_file_to_group(&led_cdev->dev->kobj,
				      &mc_priv->intensity_attr.attr,
				      led_color_group.name);
	if (ret)
		goto intensity_err_out;

	sysfs_attr_init(&mc_priv->max_intensity_attr.attr);
	len = strlen(led_colors[color_id]) + strlen(MAX_INTENSITY_NAME) + 1;
	max_intensity_file_name = kzalloc(len, GFP_KERNEL);
	if (!max_intensity_file_name) {
		ret = -ENOMEM;
		goto intensity_err_out;
	}

	snprintf(max_intensity_file_name, len, "%s%s",
		 led_colors[color_id], MAX_INTENSITY_NAME);
	mc_priv->max_intensity_attr.attr.name = max_intensity_file_name;
	mc_priv->max_intensity_attr.attr.mode = 444;
	mc_priv->max_intensity_attr.show = max_intensity_show;
	ret = sysfs_add_file_to_group(&led_cdev->dev->kobj,
				      &mc_priv->max_intensity_attr.attr,
				      led_color_group.name);
	if (ret)
		goto max_intensity_err_out;

	mc_priv->max_intensity = LED_FULL;
	list_add_tail(&mc_priv->list, &mcled_cdev->color_list);

max_intensity_err_out:
	kfree(max_intensity_file_name);
intensity_err_out:
	kfree(intensity_file_name);
	return ret;
}

static int led_multicolor_init_color_dir(struct led_classdev_mc *mcled_cdev)
{
	struct led_classdev *led_cdev = mcled_cdev->led_cdev;
	int ret;
	int i, color_index = 0;

	ret = sysfs_create_group(&led_cdev->dev->kobj, &led_color_group);
	if (ret)
		return ret;

	for (i = 0; i < LED_COLOR_ID_MAX; i++) {
		if (test_bit(i, &mcled_cdev->available_colors)) {
			ret = led_multicolor_init_color(mcled_cdev, i,
							color_index);
			if (ret)
				break;

			color_index++;
		}
	}

	return ret;
}

int led_classdev_multicolor_register_ext(struct device *parent,
				     struct led_classdev_mc *mcled_cdev,
				     struct led_init_data *init_data)
{
	struct led_classdev *led_cdev;
	int ret;

	if (!mcled_cdev)
		return -EINVAL;

	led_cdev = mcled_cdev->led_cdev;
	INIT_LIST_HEAD(&mcled_cdev->color_list);

	/* Register led class device */
	ret = led_classdev_register_ext(parent, led_cdev, init_data);
	if (ret)
		return ret;

	return led_multicolor_init_color_dir(mcled_cdev);
}
EXPORT_SYMBOL_GPL(led_classdev_multicolor_register_ext);

void led_classdev_multicolor_unregister(struct led_classdev_mc *mcled_cdev)
{
	struct led_mc_color_entry *priv, *next;

	if (!mcled_cdev)
		return;

	list_for_each_entry_safe(priv, next, &mcled_cdev->color_list, list)
		list_del(&priv->list);

	sysfs_remove_group(&mcled_cdev->led_cdev->dev->kobj, &led_color_group);
	led_classdev_unregister(mcled_cdev->led_cdev);
}
EXPORT_SYMBOL_GPL(led_classdev_multicolor_unregister);

MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com>");
MODULE_DESCRIPTION("Multi Color LED class interface");
MODULE_LICENSE("GPL v2");
