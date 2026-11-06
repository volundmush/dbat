/* ************************************************************************
*   File: random.c                                      Part of CircleMUD *
*  Usage: pseudo-random number generator                                  *
************************************************************************ */

#include "dbat/random.h"
#include "dbat/game_world.h"

/*
 * I am bothered by the non-portablility of 'rand' and 'random' -- rand
 * is ANSI C, but on some systems such as Suns, rand has seriously tragic
 * spectral properties (the low bit alternates between 0 and 1!).  random
 * is better but isn't supported by all systems.  So, in my quest for Ultimate
 * CircleMUD Portability, I decided to include this code for a simple but
 * relatively effective random number generator.  It's not the best RNG code
 * around, but I like it because it's very short and simple, and for our
 * purposes it's "random enough".
 *               --Jeremy Elson  2/23/95
 *
 * Now that we're using GNU's autoconf, I've coded Circle to always use
 * random(), and automatically link in this object file if random() isn't
 * supported on the target system.  -JE 2/3/96
 *
 * Well, despite autoconf we're back to using this random all the
 * time.  Oh well, there's no harm in changing my mind on this one
 * from release to release...  -JE 10/28/97
 */

/***************************************************************************/

/*
 *
 * This program is public domain and was written by William S. England
 * (Oct 1988).  It is based on an article by:
 *
 * Stephen K. Park and Keith W. Miller. RANDOM NUMBER GENERATORS:
 * GOOD ONES ARE HARD TO FIND. Communications of the ACM,
 * New York, NY.,October 1988 p.1192

 The following is a portable c program for generating random numbers.
 The modulus and multipilier have been extensively tested and should
 not be changed except by someone who is a professional Lehmer generator
 writer.  THIS GENERATOR REPRESENTS THE MINIMUM STANDARD AGAINST WHICH
 OTHER GENERATORS SHOULD BE JUDGED. ("Quote from the referenced article's
 authors. WSE" )
*/

/* m, q, a and r below */
constexpr unsigned long PM_MODULUS   = 2147483647; /* m = 2^31 - 1 */
constexpr unsigned long PM_QUOTIENT  = 127773;     /* q = m / a */
constexpr unsigned int PM_MULTIPLIER = 16807;      /* a */
constexpr unsigned int PM_REMAINDER  = 2836;       /* r = m % a */

/*
** F(z)	= (az)%m
**	= az-m(az/m)
**
** F(z)  = G(z)+mT(z)
** G(z)  = a(z%q)- r(z/q)
** T(z)  = (z/q) - (az/m)
**
** F(z)  = a(z%q)- rz/q+ m((z/q) - a(z/m))
** 	 = a(z%q)- rz/q+ m(z/q) - az
*/


/* local functions */

void circle_srandom(GameWorld *g, unsigned long initial_seed)
{
  g->rng_seed = initial_seed;
}

unsigned long circle_random(GameWorld *g)
{
  int lo, hi, test;

  hi = g->rng_seed / PM_QUOTIENT;
  lo = g->rng_seed % PM_QUOTIENT;

  test = PM_MULTIPLIER * lo - PM_REMAINDER * hi;

  if (test > 0)
    g->rng_seed = test;
  else
    g->rng_seed = test + PM_MODULUS;

  return (g->rng_seed);
}

/* creates a random number in long long int */
int64_t large_rand(GameWorld *g, int64_t from, int64_t to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int64_t tmp = from;
    from        = to;
    to          = tmp;
  }

  /* This should always be of the form:
   *
   *    ((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.  */
  return ((circle_random(g) % (to - from + 1)) + from);
}

/* creates a random number in interval [from;to] */
int rand_number(GameWorld *g, int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from    = to;
    to      = tmp;
  }
  return large_rand(g, from, to);
}

/* Axion engine dice function */
int axion_dice(GameWorld *g, int adjust)
{

  int die1 = 0, die2 = 0, roll = 0;

  die1 = rand_number(g, 1, 60);
  die2 = rand_number(g, 1, 60);

  roll = (die1 + die2) + adjust;

  if (roll < 2)
    roll = 2;

  return (roll);
}

/* simulates dice roll */
int dice(GameWorld *g, int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(g, 1, size);

  return (sum);
}

/* Add should be set to the amount you want to add to whatever is rolled. */
int roll_aff_duration(GameWorld *g, int num, int add)
{
  int start   = num / 20;
  int finish  = num / 10;
  int outcome = add;

  outcome += rand_number(g, start, finish);

  return (outcome);
}