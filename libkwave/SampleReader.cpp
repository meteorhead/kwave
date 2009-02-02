/***************************************************************************
       SampleReader.cpp  -  stream for reading samples from a track
			     -------------------
    begin                : Apr 25 2001
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

#include "config.h"

#include "libkwave/memcpy.h"
#include "libkwave/Sample.h"
#include "libkwave/SampleReader.h"
#include "libkwave/Stripe.h"
#include "libkwave/Track.h"

#include <QList>

// define this for using only slow Qt array functions
// #define STRICTLY_QT

//***************************************************************************
SampleReader::SampleReader(Track &track,
                           unsigned int left, unsigned int right)
    :m_track(track),
    m_src_position(left), m_first(left), m_last(right),
    m_buffer(blockSize()),
    m_buffer_used(0), m_buffer_position(0)
{
    m_track.use();
}

//***************************************************************************
SampleReader::~SampleReader()
{
    m_track.release();
}

//***************************************************************************
void SampleReader::reset()
{
    m_src_position = m_first;
    m_buffer_used = 0;
    m_buffer_position = 0;

    emit proceeded();
}

//***************************************************************************
static inline void padBuffer(Kwave::SampleArray &buffer,
                             unsigned int offset, unsigned int len)
{
#ifdef STRICTLY_QT
    while (len--)
	buffer[offset++] = 0;
#else
    memset((&buffer[offset]), 0x00, len * sizeof(buffer[0]));
#endif
}

//***************************************************************************
void SampleReader::fillBuffer()
{
    Q_ASSERT(m_buffer_position >= m_buffer_used);
    m_buffer_used = 0;
    m_buffer_position = 0;
    if (eof()) return;

    unsigned int rest = m_buffer.size();/* - m_buffer_used (is 0) */
    if (m_src_position + rest > m_last) rest = (m_last - m_src_position + 1);
    Q_ASSERT(rest <= m_buffer.size());
    if (rest > m_buffer.size()) rest = m_buffer.size();
    Q_ASSERT(rest);

    unsigned int len = m_track.readSamples(m_src_position, m_buffer, 0, rest);
    Q_ASSERT(len == rest);
    m_buffer_used  += len;
    m_src_position += len;
    rest           -= len;

    emit proceeded();
}

//***************************************************************************
void SampleReader::minMax(unsigned int first, unsigned int last,
                          sample_t &min, sample_t &max)
{
    m_track.minMax(first, last, min, max);
}

//***************************************************************************
unsigned int SampleReader::read(Kwave::SampleArray &buffer,
                                unsigned int dstoff, unsigned int length)
{
    if (eof() || !length) return 0; // already done or nothing to do

    // just a sanity check
    Q_ASSERT(buffer.size());
    Q_ASSERT(dstoff < buffer.size());
    if (dstoff >= buffer.size()) return 0;

    unsigned int count = 0;
    unsigned int rest = length;
    if (dstoff + rest > buffer.size()) rest = buffer.size() - dstoff;
    Q_ASSERT(rest);
    if (!rest) return 0;

    // first try to read from the current buffer
    if (m_buffer_position < m_buffer_used) {
	unsigned int cnt = rest;
	unsigned int src = m_buffer_position;
	unsigned int dst = dstoff;

	if (m_buffer_position + cnt > m_buffer_used)
	    cnt = m_buffer_used - m_buffer_position;

	m_buffer_position += cnt;
	count  += cnt;
	rest   -= cnt;
	dstoff += cnt;
	qDebug("filling from buffer dstoff=%u, cnt=%u",dstoff,cnt);
#ifdef STRICTLY_QT
	while (cnt--) {
	    buffer[dst++] = m_buffer[src++];
	}
#else
	MEMCPY(&(buffer[dst]), &(m_buffer[src]), cnt * sizeof(sample_t));
#endif

	if (m_buffer_position >= m_buffer_used) {
	    // buffer is empty now
	    m_buffer_position = m_buffer_used = 0;
	}
	if (!rest) return count; // done
    }

    // take the rest directly out of the stripe(s)
    if (m_src_position + rest > (m_last + 1)) // clip to end of reader range
	rest = (m_last + 1) - m_src_position;
    if (dstoff + rest > buffer.size()) // clip to end of buffer
	rest = buffer.size() - dstoff;
    Q_ASSERT(dstoff + rest <= buffer.size());
    unsigned int len = m_track.readSamples(m_src_position,
	buffer, dstoff, rest);
    Q_ASSERT(len == rest);
    m_src_position += len;
    count += len;

    emit proceeded();
    return count;
}

//***************************************************************************
void SampleReader::skip(unsigned int count)
{
    if (m_buffer_position+count < m_buffer_used) {
	// skip within the buffer
	m_buffer_position += count;
    } else {
	// skip out of the buffer
	count -= m_buffer_used;
	m_src_position += count;
	m_buffer_position = m_buffer_used = 0;
    }
}

//***************************************************************************
void SampleReader::seek(unsigned int pos)
{
    const unsigned int current_pos = m_src_position +
	m_buffer_position - m_buffer_used;

    if (pos == current_pos) return; // nothing to do

    if (pos < current_pos) {
	// seek backwards
	const unsigned int count = current_pos - pos;
	if (count <= m_buffer_position) {
	    // go back within the buffer
	    m_buffer_position -= count;
	} else {
	    // skip out of the buffer
	    m_src_position = pos;
	    m_buffer_position = m_buffer_used = 0;
	}
    } else {
	// seek forward
	skip(pos - current_pos);
    }
}

//***************************************************************************
SampleReader &SampleReader::operator >> (sample_t &sample)
{
    // get new buffer if end of last buffer reached
    if (m_buffer_position >= m_buffer_used) fillBuffer();
    sample = (m_buffer_position < m_buffer_used) ?
              m_buffer[m_buffer_position++] : 0;
    return *this;
}

//***************************************************************************
SampleReader &SampleReader::operator >> (Kwave::SampleArray &buffer)
{
    unsigned int size = buffer.size();
    unsigned int count = read(buffer, 0, size);
    if (count != size) buffer.resize(count);
    return *this;
}

//***************************************************************************
void SampleReader::goOn()
{
    Kwave::SampleArray buffer(blockSize());
    read(buffer, 0, blockSize());
    emit output(buffer);
}

//***************************************************************************
#include "SampleReader.moc"
//***************************************************************************
//***************************************************************************
