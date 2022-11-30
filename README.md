# FlexBoard Firmware

This repository contains firmware for the Flexboard.

Currently, this repository only contains bringup tests and board
definitions for the FlexEval board, a simple low cost board intended to
verify the design prior to finalizing the larger Flexboard.

### Note: Zephyr Base
This repository is intended to be build against the downstream Zephyr branch
[frozen/flexeval-branch](https://github.com/danieldegrasse/zephyr/tree/frozen/flexeval-branch) based on Zephyr 3.2

## Directories
### Bringup

This directory contains bringup tests for the FlexEval
(and eventually Flexboard) development kits. It is intended to verify all
on device hardware is functioning correctly.

### Boards
This directory contains board definitions. Currently the only definition
present is the FlexEval board definition.