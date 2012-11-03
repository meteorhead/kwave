/*************************************************************************
    KwaveSampleArray.cpp  -  array with Kwave's internal sample_t
                             -------------------
    begin                : Wed Jan 02 2008
    copyright            : (C) 2008 by Thomas Eschenbacher
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

#include "config.h"

#include <stdlib.h>

#include "memcpy.h"
#include "KwaveSampleArray.h"

//***************************************************************************
Kwave::SampleArray::SampleArray()
    :m_raw_size(0), m_raw_data(0)
{
    m_storage = new SampleStorage;
}

//***************************************************************************
Kwave::SampleArray::SampleArray(unsigned int size)
    :m_raw_size(0), m_raw_data(0)
{
    m_storage = new SampleStorage;
    resize(size);
}

//***************************************************************************
Kwave::SampleArray::~SampleArray()
{
    Q_ASSERT(m_raw_size == 0);
    Q_ASSERT(m_raw_data == 0);
}

//***************************************************************************
void Kwave::SampleArray::setRawData(sample_t *data, unsigned int size)
{
    m_raw_data = data;
    m_raw_size = size;
}

//***************************************************************************
void Kwave::SampleArray::resetRawData()
{
    m_raw_data = 0;
    m_raw_size = 0;
}

//***************************************************************************
void Kwave::SampleArray::fill(sample_t value)
{
    sample_t *p = data();
    Q_ASSERT(p);
    if (!p) return;
    unsigned int count = size();
    Q_ASSERT(count);
    while (count--) {
	*p = value;
	p++;
    }
}

//***************************************************************************
sample_t & Kwave::SampleArray::operator [] (unsigned int index)
{
    static sample_t dummy;
    sample_t *p = data();
    if (KDE_ISLIKELY(p))
	return (*(p + index));
    else
	return dummy;
}

//***************************************************************************
const sample_t & Kwave::SampleArray::operator [] (unsigned int index) const
{
    static const sample_t dummy = 0;
    const sample_t *p = data();
    if (KDE_ISLIKELY(p))
	return (*(p + index));
    else
	return dummy;
}

//***************************************************************************
void Kwave::SampleArray::resize(unsigned int size)
{
    if (size == this->size()) return;
    if (!m_storage) return;

    Q_ASSERT(m_raw_data == 0);
    m_storage->resize(size);
}

//***************************************************************************
unsigned int Kwave::SampleArray::size() const
{
    return (m_raw_data) ? m_raw_size : m_storage->m_size;
}

//***************************************************************************
Kwave::SampleArray::SampleStorage::SampleStorage()
    :QSharedData()
{
    m_size     = 0;
    m_data     = 0;
}

//***************************************************************************
Kwave::SampleArray::SampleStorage::SampleStorage(const SampleStorage &other)
    :QSharedData(other)
{
    m_data     = 0;
    m_size     = 0;

    const unsigned int other_size = other.m_size;
    const sample_t    *other_data = other.m_data;

    if (other_size) {
	m_data = static_cast<sample_t *>(
	    ::malloc(other_size * sizeof(sample_t))
	);
	if (m_data) {
	    m_size = other_size;
	    MEMCPY(m_data, other_data, m_size * sizeof(sample_t));
	}
    }
}

//***************************************************************************
Kwave::SampleArray::SampleStorage::~SampleStorage()
{
    if (m_data) ::free(m_data);
}

//***************************************************************************
void Kwave::SampleArray::SampleStorage::resize(unsigned int size)
{
    if (size) {
	// resize using realloc, keep existing data
	sample_t *new_data = static_cast<sample_t *>(
	    ::realloc(m_data, size * sizeof(sample_t)));
	if (new_data) {
	    // successful
	    // NOTE: if we grew, the additional memory is *not* initialized!
	    m_data = new_data;
	    m_size = size;
	} else {
	    qWarning("OOM in Kwave::SampleArray::SampleStorage::resize(%u)",
		size);
	}
    } else {
	// resize to zero == delete/free memory
	Q_ASSERT(m_data);
	::free(m_data);
	m_data = 0;
	m_size = 0;
    }
}

//***************************************************************************
//***************************************************************************
