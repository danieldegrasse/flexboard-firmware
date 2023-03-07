# IS31FL3733 LED blink algorithm test

This folder contains simple test code for the IS31FL3733 auto breathe mode
algorithm. Due to the nature of Zephyr's `led_blink` API, the best fit
set of discrete PFS, T1, T2, T3, and T4 values must be calculated
for a given led on/off time, and ramp up/down time.

## Test design
Two algorithms are tested. One algorithm is the "naive" approach, which
simply searches all possible solutions for PFS, T1, T2, T3, and T4, and
returns the one that minimizes error from the requested time delays. The
naive algorithm should be O(n<sup>5</sup>)

The second algorithm uses logarithms to calculate the minimum T1,T2,T3,T4 for
each possible PFS value, and takes the PFS and T1-T4 that minimize error.
This algorithm should be O(n).

The test involves running the algorithm for a range of ramp on/off and on/off
times, and verifying that both algorithms always return the same values.