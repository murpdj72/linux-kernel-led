// SPDX-License-Identifier: GPL-2.0
// LED Multi Color class interface
// Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/

#include <linux/device.h>
#include <linux/init.h>
#include <linux/led-class-multicolor.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "leds.h"

const char *led_colors[LED_COLOR_ID_MAX + 1] = {
	[LED_COLOR_ID_WHITE] = LED_COLOR_NAME_WHITE,
	[LED_COLOR_ID_RED] = LED_COLOR_NAME_RED,
	[LED_COLOR_ID_GREEN] = LED_COLOR_NAME_GREEN,
	[LED_COLOR_ID_BLUE] = LED_COLOR_NAME_BLUE,
	[LED_COLOR_ID_AMBER] = LED_COLOR_NAME_AMBER,
	[LED_COLOR_ID_VIOLET] = LED_COLOR_NAME_VIOLET,
	[LED_COLOR_ID_YELLOW] = LED_COLOR_NAME_YELLOW,
};

struct led_classdev_mc_data {
	struct led_classdev_mc *mcled_cdev;
	struct kobject *color_kobj;
	struct kobject *led_kobj;

	struct device_attribute sync_attr;
	struct device_attribute sync_enable_attr;

	struct list_head color_list;
};

struct led_classdev_mc_priv {
	struct led_classdev_mc *mcled_cdev;

	struct device_attribute max_brightness_attr;
	struct device_attribute brightness_attr;

	enum led_brightness max_brightness;
	enum led_brightness brightness;
	struct list_head list;

	int color_id;
};

static ssize_t sync_store(struct device *dev,
			  struct device_attribute *sync_attr,
			  const char *buf, size_t size)
{
	struct led_classdev_mc_data *data = container_of(sync_attr,
						      struct led_classdev_mc_data,
						      sync_attr);
	struct led_classdev_mc *mcled_cdev = data->mcled_cdev;
	struct led_classdev *led_cdev = &mcled_cdev->led_cdev;
	const struct led_multicolor_ops *ops = mcled_cdev->ops;
	struct led_classdev_mc_priv *priv;
	unsigned long sync_value;
	ssize_t ret = -EINVAL;

	mutex_lock(&led_cdev->led_access);

	if (!mcled_cdev->sync_enabled)
		goto unlock;

	ret = kstrtoul(buf, 0, &sync_value);
	if (ret)
		goto unlock;

	if (!sync_value) {
		ret = size;
		goto unlock;
	}

	list_for_each_entry(priv, &data->color_list, list) {
		ret = ops->set_color_brightness(priv->mcled_cdev,
						priv->color_id,
						priv->brightness);
		if (ret < 0)
			goto unlock;
	}

	ret = size;
unlock:
	mutex_unlock(&led_cdev->led_access);
	return ret;
}

static ssize_t sync_enable_store(struct device *dev,
				 struct device_attribute *sync_enable_attr,
				 const char *buf, size_t size)
{
	struct led_classdev_mc_data *data = container_of(sync_enable_attr,
						      struct led_classdev_mc_data,
						      sync_enable_attr);
	struct led_classdev_mc *mcled_cdev = data->mcled_cdev;
	struct led_classdev *led_cdev = &mcled_cdev->led_cdev;
	unsigned long sync_value;
	ssize_t ret = -EINVAL;

	mutex_lock(&led_cdev->led_access);

	ret = kstrtoul(buf, 0, &sync_value);
	if (ret)
		goto unlock;

	mcled_cdev->sync_enabled = sync_value;

	ret = size;
unlock:
	mutex_unlock(&led_cdev->led_access);
	return ret;
}

static ssize_t sync_enable_show(struct device *dev,
				struct device_attribute *sync_enable_attr,
				char *buf)
{
	struct led_classdev_mc_data *data = container_of(sync_enable_attr,
						      struct led_classdev_mc_data,
						      sync_enable_attr);
	struct led_classdev_mc *mcled_cdev = data->mcled_cdev;

	return sprintf(buf, "%d\n", mcled_cdev->sync_enabled);
}

static ssize_t brightness_store(struct device *dev,
				struct device_attribute *brightness_attr,
				const char *buf, size_t size)
{
	struct led_classdev_mc_priv *priv = container_of(brightness_attr,
						      struct led_classdev_mc_priv,
						      brightness_attr);
	struct led_multicolor_ops *ops = priv->mcled_cdev->ops;
	struct led_classdev *led_cdev = &priv->mcled_cdev->led_cdev;

	int old_brightness;
	unsigned long value;
	ssize_t ret = -EINVAL;

	mutex_lock(&led_cdev->led_access);

	ret = kstrtoul(buf, 10, &value);
	if (ret)
		goto unlock;

	if (value > priv->max_brightness) {
		ret = -EINVAL;
		goto unlock;
	}

	/* Retain the current brightness in case writing the LED fails */
	old_brightness = priv->brightness;
	priv->brightness = value;

	if (priv->mcled_cdev->sync_enabled) {
		ret = size;
		goto unlock;
	}

	ret = ops->set_color_brightness(priv->mcled_cdev,
					priv->color_id, value);
	if (ret < 0) {
		priv->brightness = old_brightness;
		goto unlock;
	}

	ret = size;
unlock:
	mutex_unlock(&led_cdev->led_access);
	return ret;
}

static ssize_t brightness_show(struct device *dev,
			       struct device_attribute *brightness_attr, char *buf)
{
	struct led_classdev_mc_priv *priv = container_of(brightness_attr,
						      struct led_classdev_mc_priv,
						      brightness_attr);
	const struct led_multicolor_ops *ops = priv->mcled_cdev->ops;
	int value = 0;

	if (priv->mcled_cdev->sync_enabled) {
		value = priv->brightness;
		goto sync_enabled;
	}

	if (ops->get_color_brightness) {
		value = ops->get_color_brightness(priv->mcled_cdev,
						  priv->color_id);
		priv->brightness = value;
	} else {
		value = priv->brightness;
	}

sync_enabled:
	return sprintf(buf, "%d\n", value);
}

static ssize_t max_brightness_show(struct device *dev,
				   struct device_attribute *max_brightness_attr,
				   char *buf)
{
	struct led_classdev_mc_priv *priv = container_of(max_brightness_attr,
						      struct led_classdev_mc_priv,
						      max_brightness_attr);

	return sprintf(buf, "%d\n", priv->max_brightness);
}

static int led_multicolor_init_color(struct led_classdev_mc_data *data,
				     struct led_classdev_mc *mcled_cdev,
				     int color_id)
{
	struct led_classdev *led_cdev = &mcled_cdev->led_cdev;
	struct led_classdev_mc_priv *mc_priv;
	int ret;

	mc_priv = devm_kzalloc(led_cdev->dev, sizeof(*mc_priv), GFP_KERNEL);
	if (!mc_priv)
		return -ENOMEM;

	mc_priv->color_id = color_id;
	mc_priv->mcled_cdev = mcled_cdev;

	data->led_kobj = kobject_create_and_add(led_colors[color_id],
						data->color_kobj);
	if (!data->led_kobj)
		return -EINVAL;

	sysfs_attr_init(&mc_priv->brightness_attr.attr);
	mc_priv->brightness_attr.attr.name = "brightness";
	mc_priv->brightness_attr.attr.mode = S_IRUSR | S_IWUSR;
	mc_priv->brightness_attr.show = brightness_show;
	mc_priv->brightness_attr.store = brightness_store;
	ret = sysfs_create_file(data->led_kobj,
				&mc_priv->brightness_attr.attr);
	if (ret)
		goto err_out;

	sysfs_attr_init(&mc_priv->max_brightness_attr.attr);
	mc_priv->max_brightness_attr.attr.name = "max_brightness";
	mc_priv->max_brightness_attr.attr.mode = S_IRUSR;
	mc_priv->max_brightness_attr.show = max_brightness_show;
	ret = sysfs_create_file(data->led_kobj,
				&mc_priv->max_brightness_attr.attr);
	if (ret)
		goto err_out;

	mc_priv->max_brightness = LED_FULL;
	list_add_tail(&mc_priv->list, &data->color_list);

err_out:
	return ret;
}

static int led_multicolor_init_color_dir(struct led_classdev_mc_data *data,
					 struct led_classdev_mc *mcled_cdev)
{
	struct led_classdev *led_cdev = &mcled_cdev->led_cdev;
	int ret;

	data->color_kobj = kobject_create_and_add("colors",
						  &led_cdev->dev->kobj);
	if (!data->color_kobj)
		return -EINVAL;

	sysfs_attr_init(&data->sync_enable_attr.attr);
	data->sync_enable_attr.attr.name = "sync_enable";
	data->sync_enable_attr.attr.mode = S_IRUSR | S_IWUSR;
	data->sync_enable_attr.show = sync_enable_show;
	data->sync_enable_attr.store = sync_enable_store;
	ret = sysfs_create_file(data->color_kobj,
				&data->sync_enable_attr.attr);
	if (ret)
		goto err_out;

	sysfs_attr_init(&data->sync_attr.attr);
	data->sync_attr.attr.name = "sync";
	data->sync_attr.attr.mode = S_IWUSR;
	data->sync_attr.store = sync_store;
	ret = sysfs_create_file(data->color_kobj,
				&data->sync_attr.attr);
	if (ret)
		goto err_out;

	data->mcled_cdev = mcled_cdev;

err_out:
	return ret;
}

int led_classdev_multicolor_register(struct device *parent,
				     struct led_classdev_mc *mcled_cdev)
{
	struct led_classdev *led_cdev;
	struct led_multicolor_ops *ops;
	struct led_classdev_mc_data *data;
	int ret;
	int i;

	if (!mcled_cdev)
		return -EINVAL;

	ops = mcled_cdev->ops;
	if (!ops || !ops->set_color_brightness)
		return -EINVAL;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	led_cdev = &mcled_cdev->led_cdev;

	INIT_LIST_HEAD(&data->color_list);

	/* Register led class device */
	ret = led_classdev_register(parent, led_cdev);
	if (ret)
		return ret;

	ret = led_multicolor_init_color_dir(data, mcled_cdev);
	if (ret)
		return ret;


	/* Select the sysfs attributes to be created for the device */
	for (i = 0; i < mcled_cdev->num_of_leds; i++) {
		ret = led_multicolor_init_color(data, mcled_cdev,
						mcled_cdev->available_colors[i]);
		if (ret)
			break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(led_classdev_multicolor_register);

void led_classdev_multicolor_unregister(struct led_classdev_mc *mcled_cdev)
{
	if (!mcled_cdev)
		return;

	led_classdev_unregister(&mcled_cdev->led_cdev);
}
EXPORT_SYMBOL_GPL(led_classdev_multicolor_unregister);

static void devm_led_classdev_multicolor_release(struct device *dev, void *res)
{
	led_classdev_multicolor_unregister(*(struct led_classdev_mc **)res);
}

/**
 * devm_of_led_classdev_register - resource managed led_classdev_register()
 *
 * @parent: parent of LED device
 * @led_cdev: the led_classdev structure for this device.
 */
int devm_led_classdev_multicolor_register(struct device *parent,
					  struct led_classdev_mc *mcled_cdev)
{
	struct led_classdev_mc **dr;
	int ret;

	dr = devres_alloc(devm_led_classdev_multicolor_release,
			  sizeof(*dr), GFP_KERNEL);
	if (!dr)
		return -ENOMEM;

	ret = led_classdev_multicolor_register(parent, mcled_cdev);
	if (ret) {
		devres_free(dr);
		return ret;
	}

	*dr = mcled_cdev;
	devres_add(parent, dr);

	return 0;
}
EXPORT_SYMBOL_GPL(devm_led_classdev_multicolor_register);

static int devm_led_classdev_multicolor_match(struct device *dev,
					      void *res, void *data)
{
	struct mcled_cdev **p = res;

	if (WARN_ON(!p || !*p))
		return 0;

	return *p == data;
}

/**
 * devm_led_classdev_multicolor_unregister() - resource managed
 *					led_classdev_multicolor_unregister()
 * @parent: The device to unregister.
 * @mcled_cdev: the led_classdev_mc structure for this device.
 */
void devm_led_classdev_multicolor_unregister(struct device *dev,
				  struct led_classdev_mc *mcled_cdev)
{
	WARN_ON(devres_release(dev,
			       devm_led_classdev_multicolor_release,
			       devm_led_classdev_multicolor_match, mcled_cdev));
}
EXPORT_SYMBOL_GPL(devm_led_classdev_multicolor_unregister);

MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com>");
MODULE_DESCRIPTION("Multi Color LED class interface");
MODULE_LICENSE("GPL v2");
