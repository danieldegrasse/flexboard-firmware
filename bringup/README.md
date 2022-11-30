# FlexEval Bringup

This repository contains a series of basic Zephyr applications, intended to
verify that the FlexEval board can be booted, and run a basic application.
The tests are ordered by complexity alphabetically.

Note that all tests implement a loop in `z_arm_platform_init`. This loop
can be exited using a debugger on later platform tests. The intent is to
insure no failing tests leave the SOC unrecoverable.

## Tests
### 00_blinky_spin
This test sets `CONFIG_PLATFORM_SPECIFIC_INIT=y`, and installs a
`z_arm_platform_init` function that will endlessly blink an LED. This function
will be called very early in platform boot, and will endlessly blink an LED.
due to the stage in the boot process where this code is implemented, only
NXP HAL APIs can be used, although devicetree is still used to determine which
LED to use.

Note, this sample seems to only be stable under a debugger, probably due to
invalid clocks configuration. Since it is only intended to verify code will
load on the board, this will be left for future development.

### 01_blinky
This test implements a basic loop in `z_arm_platform_init`, which can be
exited with a debugger. It then runs the standard Zephyr blinky application.

### 02_shell
This test implements a basic loop in `z_arm_platform_init`, which can be
exited with a debugger. It then runs a barebones Zephyr shell

### 03_i2c_shell
This test implements a basic loop in `z_arm_platform_init`, which can be
exited with a debugger. It then runs a Zephyr shell, with I2C and GPIO shell
modules available to probe devices on the board. This sample allows the
debug loop to be disabled, once the device is confirmed to be stable.

### 04_usb_cdc_acm
This test implements a basic loop in `z_arm_platform_init`, which can be
exited with a debugger. It then runs the Zephyr USB CDC ACM sample, which
will register a USB CDC Virtual COM port with the connected host. When
working as expected, a USB serial device should enumerate on the host
when a USB-C cable is pulled into the board, and the USB serial device
should echo characters.
