# LED Matrix Demo

This demo exercises the matrix framework, built on top of the Zephyr LED
driver API. It was tested with the IS31FL3733 LED driver, but should work
with any LED driver that indexes a matrix of LEDs in a row major style.

## LED Matrix framework
This framework is designed to abstract some of the LED management away from
the application, especially in cases where the LED matrix layout does not
align with the underlying matrix topology of the LED driver.


## Demo application
This demo application drives two 6x8 led matrices. It drives matrix 1 first,
then repeats the same test sequence for matrix 2.

The following steps are performed:
- led_blink test:
  - set all LEDs to blink with a long on time, and short off time
  - delay for 5ms so user can observe auto breathe mode functioning
- led_channel_write test:
  - set all leds to full brightness
  - set alternating leds to dim brightness
  - perform a partial update of the led array, disabling upper quadrant
- led_brightness test:
  - dim leds in sequence
  - restore led brightness in sequence
  - clear led brightness in sequence
- led_on_off test:
  - enable leds in sequence
  - disable leds in sequence
  - toggle each led in sequence