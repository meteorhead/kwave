/***************************************************************************
        TrackWriter.cpp  -  stream for inserting samples into a track
			     -------------------
    begin                : Feb 11 2001
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

#include <QApplication>

#include "libkwave/memcpy.h"
#include "libkwave/InsertMode.h"
#include "libkwave/Track.h"
#include "libkwave/TrackWriter.h"

/** minimum time between emitting the "progress()" signal [ms] */
#define MIN_PROGRESS_INTERVAL 500

//***************************************************************************
Kwave::TrackWriter::TrackWriter(Track &track, InsertMode mode,
    unsigned int left, unsigned int right)
    :Kwave::Writer(mode, left, right),
     m_track(track), m_progress_time()
{
    m_track.use();
    m_progress_time.start();
}

//***************************************************************************
Kwave::TrackWriter::~TrackWriter()
{
    flush();
    m_track.release();
}

//***************************************************************************
bool Kwave::TrackWriter::write(const Kwave::SampleArray &buffer,
                               unsigned int &count)
{
    if ((m_mode == Overwrite) && (m_position + count > m_last + 1)) {
	// need clipping
	count = m_last + 1 - m_position;
// 	qDebug("TrackWriter::write() clipped to count=%u", count);
    }

    if (count == 0) return true; // nothing to flush

    Q_ASSERT(count <= buffer.size());
    if (!m_track.writeSamples(m_mode, m_position, buffer, 0, count))
	return false; /* out of memory */

    m_position += count;

    // fix m_last, this might be needed in Append and Insert mode
    if (m_position - 1 > m_last) m_last = m_position - 1;
    count = 0;

    // inform others that we proceeded
    if (m_progress_time.elapsed() > MIN_PROGRESS_INTERVAL) {
	m_progress_time.restart();
	emit proceeded();
	QApplication::sendPostedEvents();
    }

    return true;
}

//***************************************************************************
#include "TrackWriter.moc"
//***************************************************************************
//***************************************************************************
