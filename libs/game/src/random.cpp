/* ************************************************************************
*   File: random.c                                      Part of CircleMUD *
*  Usage: pseudo-random number generator                                  *
************************************************************************ */

#include "dbat/game/random.h"
#include <effolkronium/random.hpp>

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

#define	m  (unsigned long)2147483647
#define	q  (unsigned long)127773

#define	a (unsigned int)16807
#define	r (unsigned int)2836

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

static unsigned long seed;

/* local functions */



void circle_srandom(unsigned long initial_seed)
{
    seed = initial_seed; 
}


unsigned long circle_random(void)
{
   int lo, hi, test;

    hi   = seed/q;
    lo   = seed%q;

    test = a*lo - r*hi;

    if (test > 0)
	seed = test;
    else
	seed = test+ m;

    return (seed);
}


/* creates a random number in long long int */
int64_t large_rand(int64_t from, int64_t to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int64_t tmp = from;
    from = to;
    to = tmp;
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
  return ((circle_random() % (to - from + 1)) + from);
}

/* creates a random number in interval [from;to] */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
  }
  return effolkronium::random_static::get(from, to);
}

/* Axion engine dice function */
int axion_dice(int adjust)
{

 int die1 = 0, die2 = 0, roll = 0;

 die1 = rand_number(1, 60);
 die2 = rand_number(1, 60);

 roll = (die1 + die2) + adjust;

 if (roll < 2)
  roll = 2;

 return (roll);
}

/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}

/* Add should be set to the amount you want to add to whatever is rolled. */
int roll_aff_duration(int num, int add)
{
 int start = num / 20;
 int finish = num / 10;
 int outcome = add;


 outcome += rand_number(start, finish);

 return (outcome);
}