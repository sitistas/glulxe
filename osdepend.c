/* osdepend.c: Glulxe platform-dependent code.
    Designed by Andrew Plotkin <erkyrath@eblong.com>
    http://eblong.com/zarf/glulx/index.html
*/

#include "glk.h"
#include "glulxe.h"

/* This file contains definitions for platform-dependent code. Since
   Glk takes care of I/O, this is a short list -- memory allocation
   and random numbers.

   The Makefile (or whatever) should define OS_UNIX, or some other
   symbol. Code contributions welcome. 
*/


/* We have a slightly baroque random-number scheme. If the Glulx
   @setrandom opcode is given seed 0, we use "true" randomness, from a
   platform native RNG if possible. If @setrandom is given a nonzero
   seed, we use a simple xoshiro128** RNG (provided below). The
   use of a provided algorithm aids cross-platform testing and debugging.
   (Those being the cases where you'd set a nonzero seed.)

   To define a native RNG, define the macros RAND_SET_SEED() (seed the
   RNG with the clock or some other truly random source) and RAND_GET()
   (grab a number). Note that RAND_SET_SEED() does not take an argument;
   it is only called when seed=0.

   If RAND_SET_SEED/RAND_GET are not provided, we call back to the same
   xoshiro128** RNG as before, but seeded from the system clock.
*/

static glui32 mt_random(void);
static void mt_seed_random(glui32 seed);

#ifdef OS_STDC

#include <time.h>
#include <stdlib.h>

/* Allocate a chunk of memory. */
void *glulx_malloc(glui32 len)
{
  return malloc(len);
}

/* Resize a chunk of memory. This must follow ANSI rules: if the
   size-change fails, this must return NULL, but the original chunk
   must remain unchanged. */
void *glulx_realloc(void *ptr, glui32 len)
{
  return realloc(ptr, len);
}

/* Deallocate a chunk of memory. */
void glulx_free(void *ptr)
{
  free(ptr);
}

/* Use our xoshiro128** as the native RNG, seeded from the clock. */
#define RAND_SET_SEED() (mt_seed_random(time(NULL)))
#define RAND_GET() (mt_random())

#endif /* OS_STDC */

#ifdef OS_UNIX

#include <time.h>
#include <stdlib.h>

/* Allocate a chunk of memory. */
void *glulx_malloc(glui32 len)
{
  return malloc(len);
}

/* Resize a chunk of memory. This must follow ANSI rules: if the
   size-change fails, this must return NULL, but the original chunk
   must remain unchanged. */
void *glulx_realloc(void *ptr, glui32 len)
{
  return realloc(ptr, len);
}

/* Deallocate a chunk of memory. */
void glulx_free(void *ptr)
{
  free(ptr);
}

/* Use POSIX random() as the native RNG, seeded from the POSIX clock. */
#define RAND_SET_SEED() (srandom(time(NULL)))
#define RAND_GET() (random())

#endif /* OS_UNIX */

#ifdef OS_WINDOWS

#ifdef _MSC_VER /* For Visual C++, get rand_s() */
#define _CRT_RAND_S
#endif

#include <time.h>
#include <stdlib.h>

/* Allocate a chunk of memory. */
void *glulx_malloc(glui32 len)
{
  return malloc(len);
}

/* Resize a chunk of memory. This must follow ANSI rules: if the
   size-change fails, this must return NULL, but the original chunk
   must remain unchanged. */
void *glulx_realloc(void *ptr, glui32 len)
{
  return realloc(ptr, len);
}

/* Deallocate a chunk of memory. */
void glulx_free(void *ptr)
{
  free(ptr);
}

#ifdef _MSC_VER /* Visual C++ */

/* Do nothing, as rand_s() has no seed. */
static void msc_srandom()
{
}

/* Use the Visual C++ function rand_s() as the native RNG.
   This calls the OS function RtlGetRandom(). */
static glui32 msc_random()
{
  glui32 value;
  rand_s(&value);
  return value;
}

#define RAND_SET_SEED() (msc_srandom())
#define RAND_GET() (msc_random())

#else /* Other Windows compilers */

/* Use our xoshiro128** as the native RNG, seeded from the clock. */
#define RAND_SET_SEED() (mt_seed_random(time(NULL)))
#define RAND_GET() (mt_random())

#endif

#endif /* OS_WINDOWS */


/* If no native RNG is defined above, use the xoshiro128** implementation. */
#ifndef RAND_SET_SEED
#define RAND_SET_SEED() (mt_seed_random(time(NULL)))
#define RAND_GET() (mt_random())
#endif /* RAND_SET_SEED */

static int rand_use_native = TRUE;

/* Set the random-number seed, and also select which RNG to use.
*/
void glulx_setrandom(glui32 seed)
{
    if (seed == 0) {
        rand_use_native = TRUE;
        RAND_SET_SEED();
    }
    else {
        rand_use_native = FALSE;
        mt_seed_random(seed);
    }
}

/* Return a random number in the range 0 to 2^32-1. */
glui32 glulx_random()
{
    if (rand_use_native) {
        return RAND_GET();
    }
    else {
        return mt_random();
    }
}


/* This is the "xoshiro128**" random-number generator and seed function.
   Adapted from: https://prng.di.unimi.it/xoshiro128starstar.c
   About this algorithm: https://prng.di.unimi.it/
*/
static glui32 mt_random(void);
static void mt_seed_random(glui32 seed);

static uint32_t mt_table[4];

static void mt_seed_random(glui32 seed)
{
    int ix;
    /* We set up the 128-bit state using a different RNG, SplitMix32.
       This isn't high-quality, but we just need to get a bunch of
       bits into mt_table. */
    for (ix=0; ix<4; ix++) {
        seed += 0x9E3779B9;
        glui32 s = seed;
        s ^= s >> 15;
        s *= 0x85EBCA6B;
        s ^= s >> 13;
        s *= 0xC2B2AE35;
        s ^= s >> 16;
        mt_table[ix] = s;
    }
}

static uint32_t rotl(const uint32_t x, int k) {
	return (x << k) | (x >> (32 - k));
}

uint32_t mt_random(void)
{
	const uint32_t result = rotl(mt_table[1] * 5, 7) * 9;

	const uint32_t t = mt_table[1] << 9;

	mt_table[2] ^= mt_table[0];
	mt_table[3] ^= mt_table[1];
	mt_table[1] ^= mt_table[2];
	mt_table[0] ^= mt_table[3];

	mt_table[2] ^= t;

	mt_table[3] = rotl(mt_table[3], 11);

	return result;
}


#include <stdlib.h>

/* I'm putting a wrapper for qsort() here, in case I ever have to
   worry about a platform without it. But I am not worrying at
   present. */
void glulx_sort(void *addr, int count, int size, 
  int (*comparefunc)(void *p1, void *p2))
{
  qsort(addr, count, size, (int (*)(const void *, const void *))comparefunc);
}

#ifdef FLOAT_SUPPORT
#include <math.h>

#ifdef FLOAT_COMPILE_SAFER_POWF

/* This wrapper handles all special cases, even if the underlying
   powf() function doesn't. */
gfloat32 glulx_powf(gfloat32 val1, gfloat32 val2)
{
  if (val1 == 1.0f)
    return 1.0f;
  else if ((val2 == 0.0f) || (val2 == -0.0f))
    return 1.0f;
  else if ((val1 == -1.0f) && isinf(val2))
    return 1.0f;
  return powf(val1, val2);
}

#else /* FLOAT_COMPILE_SAFER_POWF */

/* This is the standard powf() function, unaltered. */
gfloat32 glulx_powf(gfloat32 val1, gfloat32 val2)
{
  return powf(val1, val2);
}

#endif /* FLOAT_COMPILE_SAFER_POWF */

#endif /* FLOAT_SUPPORT */
