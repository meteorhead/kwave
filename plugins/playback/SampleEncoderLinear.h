/*************************************************************************
    SampleEncoderLinear.h  -  encoder for all non-compressed linear formats
                             -------------------
    begin                : Tue Apr 18 2006
    copyright            : (C) 2006 by Thomas Eschenbacher
    email                : Thomas.Eschenbacher@gmx.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _SAMPLE_ENCODER_LINEAR_H_
#define _SAMPLE_ENCODER_LINEAR_H_

#include "config.h"
#include "libkwave/ByteOrder.h"
#include "libkwave/SampleFormat.h"
#include "SampleEncoder.h"

class SampleEncoderLinear: public SampleEncoder
{
public:

    /**
     * Constructor
     * @param sample_format index of the sample format (signed/unsigned)
     * @param bits_per_sample number of bits per sample in the raw data
     * @param endianness either SOURCE_LITTLE_ENDIAN or SOURCE_BIG_ENDIAN
     */
    SampleEncoderLinear(SampleFormat sample_format,
                        unsigned int bits_per_sample,
                        byte_order_t endianness);

    /** Destructor */
    virtual ~SampleEncoderLinear();

    /**
     * Encodes a buffer with samples into a buffer with raw data.
     * @param samples array with samples
     * @param count number of samples
     * @param raw_data array with raw encoded audio data
     */
    virtual void encode(const QMemArray<sample_t> &samples,
                        unsigned int count,
                        QByteArray &raw_data);

    /** Returns the number of bytes per sample in raw (encoded) form */
    virtual unsigned int rawBytesPerSample();

private:

    /** number of bytes per raw sample */
    unsigned int m_bytes_per_sample;

    /** optimized function used for encoding the given format */
    void(*m_encoder)(const sample_t *, u_int8_t *, unsigned int);

};

#endif /* _SAMPLE_ENCODER_LINEAR_H_ */