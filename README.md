# FlexBoard Firmware

This repository contains firmware for the Flexboard and FlexEval kits

Bringup code and some driver demos are included. The main firmware
application (ZMK) is managed via the west manifest

### Note: Zephyr Base
This repository is intended to be build against the downstream Zephyr branch
[frozen/flexeval-branch](https://github.com/danieldegrasse/zephyr/tree/frozen/flexeval-branch)
based on Zephyr 3.3

## Directories
### Bringup

This directory contains bringup tests for the FlexEval and Flexboard
development kits. It is intended to verify all
on device hardware is functioning correctly.

### Boards
This directory contains board definitions for the FlexEval and Flexboard
boards

### Demos
This directory contains any driver demos for the FlexEval and Flexboard that
have not been upstreamed into Zephyr.
