/***************************************************************************
               Sample.h  -  definition of the sample type
			     -------------------
    begin                : Feb 09 2001
    copyright            : (C) 2001 by Thomas Eschenbacher
    email                : Thomas Eschenbacher <thomas.eschenbacher@gmx.de>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SAMPLE_H_
#define _SAMPLE_H_

//***************************************************************************

#include <sys/types.h>

/** Currently a "sample" is defined as a 32 bit integer with 24 valid bits */
typedef int32_t sample_t;

/** number of significant bits per sample */
#define SAMPLE_BITS 24

/** number of bits used for storing samples in integer representation */
#define SAMPLE_STORAGE_BITS 32

/** lowest sample value */
#define SAMPLE_MIN (-(1<<(SAMPLE_BITS-1))+1)

/** highest sample value */
#define SAMPLE_MAX (+(1<<(SAMPLE_BITS-1))-1)

/**
 * Simple conversion from float to sample_t
 */
static inline sample_t float2sample(const float f) {
    return (sample_t)(f * (float)(1 << (SAMPLE_BITS-1)));
}

/**
 * Simple conversion from sample_t to float
 */
static inline float sample2float(const sample_t s) {
    return (float)((float)s / float(1 << (SAMPLE_BITS-1)));
}

#endif /* _SAMPLE_H_ */
