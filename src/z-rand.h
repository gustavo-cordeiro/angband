/**
 * \file z-rand.h
 * \brief A Random Number Generator for Angband
 *
 * Copyright (c) 1997 Ben Harrison, Randy Hutson
 * Copyright (c) 2010 Erik Osheim
 * 
 * See below for copyright on the WELL random number generator.
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */

#ifndef INCLUDED_Z_RAND_H
#define INCLUDED_Z_RAND_H

#include "h-basic.h"

/**
 * Assumed maximum dungeon level.  This value is used for various 
 * calculations involving object and monster creation.  It must be at least 
 * 100. Setting it below 128 may prevent the creation of some objects.
 */
#define MAX_RAND_DEPTH	128

/**
 * A struct representing a strategy for making a dice roll.
 *
 * The result will be base + XdY + BONUS, where m_bonus is used in a
 * tricky way to determine BONUS.
 */
typedef struct random {
	int base;
	int dice;
	int sides;
	int m_bonus;
} random_value;

/**
 * The number of 32-bit integers worth of seed state.
 */
#define RAND_DEG 32

/**
 * Random aspects used by damcalc, m_bonus_calc, and ranvals
 */
typedef enum {
	MINIMISE,
	AVERAGE,
	MAXIMISE,
	EXTREMIFY,
	RANDOMISE
} aspect;


/**
 * Generates a random signed long integer X where "0 <= X < M" holds.
 *
 * The integer X falls along a uniform distribution.
 */
#define randint0(M) ((s32b) Rand_div(M))


/**
 * Generates a random signed long integer X where "1 <= X <= M" holds.
 *
 * The integer X falls along a uniform distribution.
 */
#define randint1(M) ((s32b) Rand_div(M) + 1)

/**
 * Generate a random signed long integer X where "A - D <= X <= A + D" holds.
 * Note that "rand_spread(A, D)" == "rand_range(A - D, A + D)"
 *
 * The integer X falls along a uniform distribution.
 */
#define rand_spread(A, D) ((A) + (randint0(1 + (D) + (D))) - (D))

/**
 * Return TRUE one time in `x`.
 */
#define one_in_(x) (!randint0(x))

/**
 * Whether we are currently using the "quick" method or not.
 */
extern bool Rand_quick;

/**
 * The state used by the "quick" RNG.
 */
extern u32b Rand_value;

/**
 * The state used by the "complex" RNG.
 */
extern u32b state_i;
extern u32b STATE[RAND_DEG];
extern u32b z0;
extern u32b z1;
extern u32b z2;


/**
 * Initialise the RNG state with the given seed.
 */
void Rand_state_init(u32b seed);

/**
 * Initialise the RNG
 */
void Rand_init(void);

/**
 * Generates a random unsigned long integer X where "0 <= X < M" holds.
 *
 * The integer X falls along a uniform distribution.
 */
u32b Rand_div(u32b m);

/**
 * Generate a signed random integer within `stand` standard deviations of
 * `mean`, following a normal distribution.
 */
s16b Rand_normal(int mean, int stand);

/**
 * Generate a semi-random number from 0 to m-1, in a way that doesn't affect
 * gameplay.  This is intended for use by external program parts like the
 * main-*.c files.
 */
u32b Rand_simple(u32b m);

/**
 * Emulate a number `num` of dice rolls of dice with `sides` sides.
 */
int damroll(int num, int sides);

/**
 * Calculation helper function for damroll
 */
int damcalc(int num, int sides, aspect dam_aspect);

/**
 * Generates a random signed long integer X where "A <= X <= B"
 * Note that "rand_range(0, N-1)" == "randint0(N)".
 *
 * The integer X falls along a uniform distribution.
 */
int rand_range(int A, int B);

/**
 * Function used to determine enchantment bonuses, see function header for
 * a more complete description.
 */
s16b m_bonus(int max, int level);

/**
 * Calculation helper function for m_bonus.
 */
s16b m_bonus_calc(int max, int level, aspect bonus_aspect);

/**
 * Calculation helper function for random_value structs.
 */
int randcalc(random_value v, int level, aspect rand_aspect);

/**
 * Test to see if a value is within a random_value's range.
 */
bool randcalc_valid(random_value v, int test);

/**
 * Test to see if a random_value actually varies.
 */
bool randcalc_varies(random_value v);

extern void rand_fix(u32b val);

#endif /* INCLUDED_Z_RAND_H */
