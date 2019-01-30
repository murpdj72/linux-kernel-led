====================================
Multi Color LED handling under Linux
====================================

Description
===========
The multi color class groups monochrome LEDs and allows controlling two
aspects of the final combined color: hue and lightness. The former is
controlled via <color>_intensity files and the latter is controlled
via brightness file.

For more details on hue and lightness notions please refer to
https://en.wikipedia.org/wiki/CIECAM02.

Note that intensity files only cache the written value and the actual
change of hardware state occurs upon writing brightness file. This
allows for changing many factors of the perceived color in a virtually
unnoticeable way for the human observer.

Multicolor Class Control
========================
The multicolor class presents the LED groups under a directory called "colors".
This directory is a child under the LED parent node created by the led_class
framework.  The led_class framework is documented in led-class.rst within this
documentation directory.

Each colored LED will have two files created under the colors directory
<color>_intensity and <color>_max_intensity. These files will contain
one of LED_COLOR_ID_* definitions from the header
include/dt-bindings/leds/common.h.

Directory Layout Example
========================
root:/sys/class/leds/rgb:grouped_leds# ls -lR colors/
-rw-rwxr-- 1 root root 4096 Jul 7 03:10 red_max_intensity
--w--wx-w- 1 root root 4096 Jul 7 03:10 red_intensity
-rw-rwxr-- 1 root root 4096 Jul 7 03:10 green_max_intensity
--w--wx-w- 1 root root 4096 Jul 7 03:10 green_intensity
-rw-rwxr-- 1 root root 4096 Jul 7 03:10 blue_max_intensity
--w--wx-w- 1 root root 4096 Jul 7 03:10 blue_intensity

Multicolor Class Brightness Control
===================================
The multiclor class framework will calculate each monochrome LEDs intensity.

The brightness level for each LED is calculated based on the color LED
intensity setting divided by the color LED max intensity setting multiplied by
the requested brightness.

led_brightness = brightness * <color>_intensity/<color>_max_intensity

Example:
Three LEDs are present in the group as defined in "Directory Layout Example"
within this document.

A user first writes the color LED brightness file with the brightness level that
is necessary to achieve a blueish violet output from the RGB LED group.

echo 138 > /sys/class/leds/rgb:grouped_leds/red_intensity
echo 43 > /sys/class/leds/rgb:grouped_leds/green_intensity
echo 226 > /sys/class/leds/rgb:grouped_leds/blue_intensity

red -
	intensity = 138
	max_intensity = 255
green -
	intensity = 43
	max_intensity = 255
blue -
	intensity = 226
	max_intensity = 255

The user can control the brightness of that RGB group by writing the parent
'brightness' control.  Assuming a parent max_brightness of 255 the user may want
to dim the LED color group to half.  The user would write a value of 128 to the
parent brightness file then the values written to each LED will be adjusted
base on this value

cat /sys/class/leds/rgb:grouped_leds/max_brightness
255
echo 128 > /sys/class/leds/rgb:grouped_leds/brightness

adjusted_red_value = 128 * 138/255 = 69
adjusted_green_value = 128 * 43/255 = 21
adjusted_blue_value = 128 * 226/255 = 113

Reading the parent brightness file will return the current brightness value of
the color LED group.

cat /sys/class/leds/rgb:grouped_leds/max_brightness
255

echo 128 > /sys/class/leds/rgb:grouped_leds/brightness

cat /sys/class/leds/rgb:grouped_leds/brightness
128
