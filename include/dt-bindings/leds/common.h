/* SPDX-License-Identifier: GPL-2.0 */
/*
 * This header provides macros for the common LEDs device tree bindings.
 *
 * Copyright (C) 2015, Samsung Electronics Co., Ltd.
 *
 * Author: Jacek Anaszewski <j.anaszewski@samsung.com>
 */

#ifndef __DT_BINDINGS_LEDS_H
#define __DT_BINDINGS_LEDS_H

/* External trigger type */
#define LEDS_TRIG_TYPE_EDGE	0
#define LEDS_TRIG_TYPE_LEVEL	1

/* Boost modes */
#define LEDS_BOOST_OFF		0
#define LEDS_BOOST_ADAPTIVE	1
#define LEDS_BOOST_FIXED	2

#define LED_COLOR_ID_WHITE	 0
#define LED_COLOR_ID_RED	 1
#define LED_COLOR_ID_GREEN	 2
#define LED_COLOR_ID_BLUE	 3
#define LED_COLOR_ID_AMBER	 4
#define LED_COLOR_ID_VIOLET	 5
#define LED_COLOR_ID_YELLOW	 6
#define LED_COLOR_ID_MAX	 LED_COLOR_ID_YELLOW

#define LED_COLOR_NAME_WHITE	"white"
#define LED_COLOR_NAME_RED	"red"
#define LED_COLOR_NAME_GREEN	"green"
#define LED_COLOR_NAME_BLUE	"blue"
#define LED_COLOR_NAME_AMBER	"amber"
#define LED_COLOR_NAME_VIOLET	"violet"
#define LED_COLOR_NAME_YELLOW	"yellow"

#define LED_COLOR_NAME_MAX_SZ	6

#endif /* __DT_BINDINGS_LEDS_H */
