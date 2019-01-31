// SPDX-License-Identifier: GPL-2.0
// LED RGB class interface
// Copyright (C) 2019 Texas Instruments Incorporated - http://www.ti.com/

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/led-class-multicolor.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <uapi/linux/uleds.h>

struct multicolor_fw_test_led {
	struct led_classdev_mc mcled_cdev;
	struct i2c_client *client;

	u32 white_setting[3];

	u32 led_strings[3];

	char led_name[LED_MAX_NAME_SIZE];
};

static struct multicolor_fw_test_led *mcled_cdev_to_led(struct led_classdev_mc *mcled_cdev)
{
	return container_of(mcled_cdev, struct multicolor_fw_test_led, mcled_cdev);
}

static int multicolor_fw_set_color(struct led_classdev_mc *mcled_cdev, int color, int value)
{
	printk("%s: Would set 0x%X to %d\n", __func__, color, value);

	return 0;
}

static struct led_multicolor_ops multicolor_test_ops = {
	.set_color_brightness = multicolor_fw_set_color,
/* Need to do get functions */
//	.get_color = multicolor_fw_get_color,
};
static int multicolor_fw_test_brightness_set(struct led_classdev *cdev,
					enum led_brightness brightness)
{

	printk("%s: Brightness is 0x%X\n", __func__, brightness);
	return 0;
}

static int multicolor_fw_test_register_leds(struct multicolor_fw_test_led *led,
					    int num_of_colors)
{
	struct led_classdev *led_cdev;

	led->mcled_cdev.ops = &multicolor_test_ops;
	led->mcled_cdev.num_of_leds = num_of_colors;

	led_cdev = &led->mcled_cdev.led_cdev;
	led_cdev->brightness_set_blocking = multicolor_fw_test_brightness_set;
	led_cdev->name = led->led_name;

	return devm_led_classdev_multicolor_register(&led->client->dev, &led->mcled_cdev);
}

static int multicolor_fw_test_probe_node(struct multicolor_fw_test_led *led)
{

	struct fwnode_handle *child = NULL;
	const char *name;
	int ret;
	int num_of_colors;
printk("%s: Enter\n", __func__);
/* Need to set this up for multiple nodes this is not working since we reuse
the same led node.  Need to fix that to continue */
	device_for_each_child_node(&led->client->dev, child) {
		ret = fwnode_property_read_string(child, "label", &name);

		snprintf(led->led_name, sizeof(led->led_name),
			 "%s:%s", led->client->name, name);

		num_of_colors = fwnode_property_read_u32_array(child, "led-colors",
						     NULL, 0);
printk("%s: Num of colors %i\n", __func__, num_of_colors);
		ret = fwnode_property_read_u32_array(child, "led-colors",
						     led->mcled_cdev.available_colors,
						     num_of_colors);

printk("%s: Register\n", __func__);
		ret = multicolor_fw_test_register_leds(led, num_of_colors);
	}
	return 0;
}

static int multicolor_fw_test_probe(struct i2c_client *client)
{
	struct multicolor_fw_test_led *led;
	int ret;

printk("%s: Enter\n", __func__);
	led = devm_kzalloc(&client->dev, sizeof(*led), GFP_KERNEL);
	if (!led)
		return -ENOMEM;

	led->client = client;
	i2c_set_clientdata(client, led);

	ret = multicolor_fw_test_probe_node(led);
	if (ret)
		return -ENODEV;

	return ret;
}

static int multicolor_fw_test_remove(struct i2c_client *client)
{
	struct multicolor_fw_test_led *led = i2c_get_clientdata(client);

	led_classdev_multicolor_unregister(&led->mcled_cdev);

	return 0;
}

static const struct i2c_device_id multicolor_fw_test_id[] = {
	{ "MCNONE", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lm3601x_id);

static const struct of_device_id of_multicolor_fw_test_leds_match[] = {
	{ .compatible = "multicolor,framework_test", },
	{ }
};
MODULE_DEVICE_TABLE(of, of_multicolor_fw_test_leds_match);

static struct i2c_driver multicolor_fw_test_i2c_driver = {
	.driver = {
		.name = "multicolor_framework_test",
		.of_match_table = of_multicolor_fw_test_leds_match,
	},
	.probe_new = multicolor_fw_test_probe,
	.remove = multicolor_fw_test_remove,
	.id_table = multicolor_fw_test_id,
};
module_i2c_driver(multicolor_fw_test_i2c_driver);

MODULE_AUTHOR("Dan Murphy <dmurphy@ti.com>");
MODULE_DESCRIPTION("Multi Color LED class interface");
MODULE_LICENSE("GPL v2");
