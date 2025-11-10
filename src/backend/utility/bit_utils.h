/* bit_utils.h
 * Thomas J. Armytage 2025 ( https://github.com/tommojphillips/ )
 * bit utilities
 */

#ifndef BIT_UTILS_H
#define BIT_UTILS_H

/* Returns 1 if the bit is on the rising edge. (pv != bit && v == bit) */
#define IS_RISING_EDGE(mask, pvalue, value)  (!((pvalue) & (mask)) && ((value) & (mask)))

/* Returns 1 if the bit is on the falling edge. (pv == bit && v != bit) */
#define IS_FALLING_EDGE(mask, pvalue, value) (((pvalue) & (mask)) && !((value) & (mask)))

/* Returns 1 if any bits have changed (pv != v) */
#define HAS_BITS_CHANGED(mask, pvalue, value) (((pvalue) & (mask)) != ((value) & (mask)))

/* set bit/s in flags to value by mask */
#define SET_BIT(flags, value, mask) (flags = (flags & ~mask) | (value & mask))

#endif
