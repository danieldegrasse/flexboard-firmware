#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/param.h>

static uint32_t is31fl3733_pfs_time_scale[] = {
	210,
	420,
	70,
	840,
	168,
};

#define CONF_REG_PFS_MAX  0x4 /* PWM frequency scaling shift */

#define ABMX_CTRL1_T1_MAX 0x7 /* Timescale register 1 max value */
#define ABMX_CTRL1_T2_MAX 0x8 /* Timescale register 2 max value */
#define ABMX_CTRL2_T3_MAX 0x7 /* Timescale register 3 max value */
#define ABMX_CTRL2_T4_MAX 0xA /* Timescale register 4 max value */

/* Constant used in led breathe mode calculations */
#define LOG2_3_2 0.584962501f

/* Helper function to find best possible auto breathe mode setting
 * for requested ramp up, ramp down, on, and off times
 * returns cumulative error of best solution found.
 *
 * Sets t1_b, t2_b, t3_b, t4_b, and pfs_b to best values for
 * T1,T2,T3,T4, and PFS registers.
 */
static int is31fl3733_abm_solution(uint32_t ramp_up, uint32_t ramp_down,
	uint32_t delay_on, uint32_t delay_off, uint8_t *pfs_b, uint8_t *t1_b,
	uint8_t *t2_b, uint8_t *t3_b, uint8_t *t4_b)
{
	uint32_t error, cand, t1_err, t2_err, t3_err, t4_err, pfs_base;
	uint8_t t1, t2, t3, t4, pfs;
	/* Since the IS31FL3733 only supports discrete on and off times,
	 * we must search all possible settings of the PFS and time scale
	 * registers to find the one best approximating the ramp up, ramp
	 * down, time on, and time off values requested by the user.
	 *
	 * We will calculate the cumulative error of each PFS and timescale
	 * value, and compare it to the current best setting we've located.
	 * If the new solution wins, we'll keep it and continue searching
	 * Note: this process is rather compute intensive. If you'd like
	 * to reduce the impact, ensure the devicetree properties
	 * abm-ramp-up, abm-ramp-down, and the arguments delay_on and delay_off
	 * are exact values possible with the ABM control registers
	 * (see Tables 15 and 16 of the datasheet). The algorithm will always
	 * stop if it finds an exact solution for requested values.
	 */
	error = UINT32_MAX;
	for (pfs = 0; pfs <= CONF_REG_PFS_MAX; pfs++) {
		pfs_base = is31fl3733_pfs_time_scale[pfs];
		for (t1 = 0; t1 <= ABMX_CTRL1_T1_MAX; t1++) {
			/* T1 error */
			t1_err = abs(ramp_up - (pfs_base << t1));
			for (t2 = 0; t2 <= ABMX_CTRL1_T2_MAX; t2++) {
				/* T2 error */
				if (t2 == 0) {
					t2_err = delay_on;
				} else {
					t2_err = abs(delay_on - (pfs_base << (t2 - 1)));
				}
				for (t3 = 0; t3 <= ABMX_CTRL2_T3_MAX; t3++) {
					/* T3 error */
					t3_err = abs(ramp_down - (pfs_base << t3));
					for (t4 = 0; t4 <= ABMX_CTRL2_T4_MAX; t4++) {
						/* T4 error */
						if (t4 == 0) {
							t4_err = delay_off;
							if (pfs == 2) {
								/* Datasheet says this is
								 * an invalid setting
								 */
								continue;
							}
						} else {
							t4_err = abs(delay_off -
								(pfs_base << (t4 - 1)));
						}
						cand = (t1_err + t2_err + t3_err + t4_err);
						if (cand < error) {
							/* New best solution found */
							error = cand;
							*t1_b = t1;
							*t2_b = t2;
							*t3_b = t3;
							*t4_b = t4;
							*pfs_b = pfs;
							if (error == 0) {
								return 0;
							}
						}
					}
				}
			}
		}
	}
	return error;
}

/*
 * Helper function to find best possible auto breathe mode setting
 * for requested ramp up, ramp down, on, and off times
 * returns cumulative error of best solution found.
 *
 * Sets t1_b, t2_b, t3_b, t4_b, and pfs_b to best values for
 * T1,T2,T3,T4, and PFS registers.
 */
static int is31fl3733_abm_solution_2(uint32_t ramp_up, uint32_t ramp_down,
	uint32_t delay_on, uint32_t delay_off, uint8_t *pfs_b, uint8_t *t1_b,
	uint8_t *t2_b, uint8_t *t3_b, uint8_t *t4_b)
{
	float t1, t2, t3, t4;
	uint32_t pfs_base, err, cand;

	/*
	 * Since the IS31FL3733 only supports discrete delays,
	 * we must find the best fit for the PFS and T1-T4 values for
	 * a given ramp up/down time, and on/off time.
	 *
	 * This can be accomplished by minimizing a function calculating
	 * the difference between the desired ramp up/down time and on/off
	 * time, and the actual ramp up/down and on/off time for a given PFS
	 * and T1-T4.
	 * This function can be modeled with the following equation:
	 * e = (up - (f*2^t1))^2 + (on - (f*2^(t2 - 1)))^2 +
	 *	(down - (f*2^t3))^2 + (off - (f*2^(t4 - 1)))^2
	 * where e is error, and f is the base delay for a given PFS value
	 * We then calculate the gradient of this function, and find the
	 * gradient is zero at:
	 * <T1, T2, T3, T4> = <log2(up/f), log2(on/f) + 1, log2(down/f), log2(off/f)>
	 *
	 * Since the PFS base values are not easily differentiable as T1-T4
	 * values are, use the following algorithm:
	 * for each PFS base value:
	 *	calculate zero vector for gradient
	 *	round each T value to the nearest integer that minimizes error
	 *	calculate the error function with new Tx values
	 *	if error is better than current best error:
	 *		update best error
	 *		save new Tx and PFS values
	 *		if error is zero, exit
	 */


	err = UINT32_MAX;
	for (uint32_t pfs = 0; pfs <= CONF_REG_PFS_MAX; pfs++) {
		pfs_base = is31fl3733_pfs_time_scale[pfs];
		/* Calculate the optimal time values for this
		 * pfs value using the error function's gradient.
		 */
		t1 = log2f(((float)ramp_up) / ((float)pfs_base));
		t2 = log2f(((float)delay_on) / ((float)pfs_base)) + 1;
		t3 = log2f(((float)ramp_down) / ((float)pfs_base));
		t4 = log2f(((float)delay_off) / ((float)pfs_base)) + 1;
		/*
		 * Round each T value in order to minimize the error of
		 * the function 2^T. We use this rounding because we know
		 * that in the error function, 2 will be raised to the power
		 * of T, and traditional integer rounding will not always
		 * minimize this.
		 * For a given range [x, x+1], 2^T where log2(x) < T < log2(x+1)
		 * will be closer to x when: T- floor(T) < log2(3/2)
		 */
		if (t1 - floorf(t1) > LOG2_3_2) {
			t1 = ceilf(t1);
		} else {
			t1 = floorf(t1);
		}
		if (floorf(t2) == 0.0f) {
			/* For 0 < t2 < 1, the function does not follow the
			 * formula above. We should round any value between
			 * 0 and 1 to 1, since setting T2 gives a delay of 0,
			 * not f/2 as the function implies
			 */
			t2 = 1.0f;
		}
		if (t2 - floorf(t2) > LOG2_3_2) {
			t2 = ceilf(t2);
		} else {
			t2 = floorf(t2);
		}
		if (t3 - floorf(t3) > LOG2_3_2) {
			t3 = ceilf(t3);
		} else {
			t3 = floorf(t3);
		}
		if (floorf(t4) == 0.0f) {
			/* For 0 < t4 < 1, the function does not follow the
			 * formula above. We should round any value between
			 * 0 and 1 to 1, since setting T4 gives a delay of 0,
			 * not f/2 as the function implies
			 */
			t4 = 1.0f;
		}
		if (t4 - floorf(t4) > LOG2_3_2) {
			t4 = ceilf(t4);
		} else {
			t4 = floorf(t4);
		}
		/* T1-T4 may not be less than 0 */
		t1 = MAX(t1, 0);
		t2 = MAX(t2, 0);
		t3 = MAX(t3, 0);
		t4 = MAX(t4, 0);
		if ((t4 == 0) && (pfs == 2)) {
			/*
			 * Datasheet says this is an invalid setting.
			 * Test with T4=1
			 */
			t4 = 1;
		}
		/* Clamp T1-T4 to their maximum values */
                t1 = MIN(t1, ABMX_CTRL1_T1_MAX);
                t2 = MIN(t2, ABMX_CTRL1_T2_MAX);
                t3 = MIN(t3, ABMX_CTRL2_T3_MAX);
                t4 = MIN(t4, ABMX_CTRL2_T4_MAX);
		cand = abs(ramp_up - (pfs_base << ((uint8_t)t1)));
		cand += abs(delay_on - (pfs_base << (((uint8_t)t2) - 1)));
		cand += abs(ramp_down - (pfs_base << ((uint8_t)t3)));
		cand += abs(delay_off - (pfs_base << (((uint8_t)t4) - 1)));
		if (cand < err) {
			/* Save new best time values */
			*t1_b = (uint8_t)t1;
			*t2_b = (uint8_t)t2;
			*t3_b = (uint8_t)t3;
			*t4_b = (uint8_t)t4;
			*pfs_b = pfs;
			/* Update best error */
			err = cand;
			if (err == 0) {
				return 0;
			}
		}
	}
	return err;
}

/*
 * This helper returns the result of log2(num/denom), rounded to the nearest
 * positive integer to minimize the error of 2^log2(num/denom)
 */
int log2_helper(uint32_t num, uint32_t denom)
{
	uint32_t round_div, res = 0;

	/* First, divide the numerator and denominator. There are three
	 * cases we must handle here:
	 * - num * 2 < demon: special case, return -1
	 * - num < denom * 2: round to nearest integer.
	 * - num > denom * 2: truncate. This eliminates the possible error
	 *   of (demon/2) introduced by rounding up to the division, to a value
	 *   that would result in a higher base 2 log value
	 */
	if ((num << 1) < denom) {
		/* For some T calculations, T=0 results in an output time
		 * of 0. Any case where num is less than half of denom
		 * will use T=0, so return -1 here. For the T calculations
		 * that add 1, this will result in 0, and other calculations
		 * will simply force the final T value to be 0.
		 */
		return -1;
	} else if (num < (denom << 1)) {
		round_div = (num + (denom >> 1)) / denom;
	} else {
		round_div = num / denom;
	}
	/* Now, approximate log2(). We check the two highest MSBs to determine
	 * rounding direction
	 */
	while (round_div > 3) {
		res++;
		round_div = round_div >> 1;
	}
	/* If MSB and LSB of remaining value are set, log2() should round up */
	if (round_div == 3) {
		res += 2;
	} else if (round_div == 2) {
		res += 1;
	}
	return res;
}


/*
 * Helper function to find best possible auto breathe mode setting
 * for requested ramp up, ramp down, on, and off times
 * returns cumulative error of best solution found.
 *
 * Sets t1_b, t2_b, t3_b, t4_b, and pfs_b to best values for
 * T1,T2,T3,T4, and PFS registers.
 */
static int is31fl3733_abm_solution_3(uint32_t ramp_up, uint32_t ramp_down,
	uint32_t delay_on, uint32_t delay_off, uint8_t *pfs_b, uint8_t *t1_b,
	uint8_t *t2_b, uint8_t *t3_b, uint8_t *t4_b)
{
	float t1, t2, t3, t4;
	uint32_t pfs_base, err, cand;

	/*
	 * Since the IS31FL3733 only supports discrete delays,
	 * we must find the best fit for the PFS and T1-T4 values for
	 * a given ramp up/down time, and on/off time.
	 *
	 * This can be accomplished by minimizing a function calculating
	 * the difference between the desired ramp up/down time and on/off
	 * time, and the actual ramp up/down and on/off time for a given PFS
	 * and T1-T4.
	 * This function can be modeled with the following equation:
	 * e = (up - (f*2^t1))^2 + (on - (f*2^(t2 - 1)))^2 +
	 *	(down - (f*2^t3))^2 + (off - (f*2^(t4 - 1)))^2
	 * where e is error, and f is the base delay for a given PFS value
	 * We then calculate the gradient of this function, and find the
	 * gradient is zero at:
	 * <T1, T2, T3, T4> = <log2(up/f), log2(on/f) + 1, log2(down/f), log2(off/f)>
	 *
	 * Since the PFS base values are not easily differentiable as T1-T4
	 * values are, use the following algorithm:
	 * for each PFS base value:
	 *	calculate zero vector for gradient
	 *	round each T value to the nearest integer that minimizes error
	 *	calculate the error function with new Tx values
	 *	if error is better than current best error:
	 *		update best error
	 *		save new Tx and PFS values
	 *		if error is zero, exit
	 */


	err = UINT32_MAX;
	for (uint32_t pfs = 0; pfs <= CONF_REG_PFS_MAX; pfs++) {
		pfs_base = is31fl3733_pfs_time_scale[pfs];
		/* Calculate the optimal time values for this
		 * pfs value using the log2 helper function
		 */
		t1 = MAX(log2_helper(ramp_up, pfs_base), 0);
		t2 = MAX(log2_helper(delay_on, pfs_base) + 1, 0);
		t3 = MAX(log2_helper(ramp_down, pfs_base), 0);
		t4 = MAX(log2_helper(delay_off, pfs_base) + 1, 0);

		/* Clamp T1-T4 to their maximum values */
                t1 = MIN(t1, ABMX_CTRL1_T1_MAX);
                t2 = MIN(t2, ABMX_CTRL1_T2_MAX);
                t3 = MIN(t3, ABMX_CTRL2_T3_MAX);
                t4 = MIN(t4, ABMX_CTRL2_T4_MAX);

		if ((t4 == 0) && (pfs == 2)) {
			/*
			 * Datasheet says this is an invalid setting.
			 * Test with T4=1
			 */
			t4 = 1;
		}
		cand = abs(ramp_up - (pfs_base << ((uint8_t)t1)));
		cand += abs(delay_on - (pfs_base << (((uint8_t)t2) - 1)));
		cand += abs(ramp_down - (pfs_base << ((uint8_t)t3)));
		cand += abs(delay_off - (pfs_base << (((uint8_t)t4) - 1)));
		if (cand < err) {
			/* Save new best time values */
			*t1_b = (uint8_t)t1;
			*t2_b = (uint8_t)t2;
			*t3_b = (uint8_t)t3;
			*t4_b = (uint8_t)t4;
			*pfs_b = pfs;
			/* Update best error */
			err = cand;
			if (err == 0) {
				return 0;
			}
		}
	}
	return err;
}

/* Test up to 100 ms worth of delay with algorithm */
#define TEST_RANGE 100

int main(void)
{
        uint8_t pfs_1, t1_1, t2_1, t3_1, t4_1;
        uint8_t pfs_2, t1_2, t2_2, t3_2, t4_2;
        int err_1, err_2;

	/* Run checks against log2_helper function */
	for (uint32_t i = 0; i < 1000; i++) {
		for (uint32_t j = 1; j < 1000; j++) {
			uint32_t res = log2_helper(i, j) + 1;
			float check = log2f((float)i/(float)j) + 1;
			if (floorf(check) == 0.0f) {
				/* For 0 < check < 1, We should round any value
				 * between 0 and 1 to 1, since setting check to 0
				 * gives a delay of 0, not f/2 as the function
				 * implies
				 */
				check = 1.0f;
			}
			if (check - floorf(check) > LOG2_3_2- 0.0001f) {
				check = ceilf(check);
			} else {
				check = floorf(check);
			}
			check = MAX(check, 0);
			if (((uint32_t)check) != res) {
				printf("%d/%d: %d vs %f\n", i, j, res, check);
				return 255;
			}
		}
	}

        for (uint32_t on = 0; on < TEST_RANGE; on++) {
		for (uint32_t off = 0; off < TEST_RANGE; off++) {
                        printf(".", off);
			fflush(stdout);
			for (uint32_t rise = 0; rise < TEST_RANGE; rise++) {
				for (uint32_t fall = 0; fall < TEST_RANGE; fall++) {
                                        err_1 = is31fl3733_abm_solution(rise, fall, on, off,
                                                &pfs_1, &t1_1, &t2_1, &t3_1, &t4_1);
                                        err_2 = is31fl3733_abm_solution_3(rise, fall, on, off,
                                                &pfs_2, &t1_2, &t2_2, &t3_2, &t4_2);
                                        if ((err_1 != err_2)) {
                                                printf("Ramp rise: %d, Ramp fall: %d,\n"
                                                        "Time on: %d, Time off: %d\n",
                                                        rise, fall, on, off);
                                                printf("Values do not match:\n"
                                                        "t1: %d vs %d\n"
                                                        "t2: %d vs %d\n"
                                                        "t3: %d vs %d\n"
                                                        "t4: %d vs %d\n"
                                                        "pfs: %d vs %d\n"
                                                        "err: %d vs %d\n",
                                                        t1_1, t1_2, t2_1, t2_2,
                                                        t3_1, t3_2, t4_1, t4_2,
                                                        pfs_1, pfs_2,
                                                        err_1, err_2);
                                                return 255;
                                        }
                                        if (err_1 == 0) {
                                                printf("Ramp rise: %d, Ramp fall: %d,\n"
                                                        "Time on: %d, Time off: %d\n",
                                                        rise, fall, on, off);
                                                printf("Found solution:\n"
                                                        "t1: %d vs %d\n"
                                                        "t2: %d vs %d\n"
                                                        "t3: %d vs %d\n"
                                                        "t4: %d vs %d\n"
                                                        "pfs: %d vs %d\n"
                                                        "err: %d vs %d\n",
                                                        t1_1, t1_2, t2_1, t2_2,
                                                        t3_1, t3_2, t4_1, t4_2,
                                                        pfs_1, pfs_2,
                                                        err_1, err_2);
                                        }
                                }
                        }
                }
        }
        printf("Test complete. All checks passed\n");
        return 0;
}