/***************************************************************************
             MimeData.h  -  mime data container for Kwave's audio data
			     -------------------
    begin                : Oct 04 2008
    copyright            : (C) 2008 by Thomas Eschenbacher
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

#ifndef _MIME_DATA_H_
#define _MIME_DATA_H_

#include "config.h"

#include <QtCore/QByteArray>
#include <QtCore/QMimeData>
#include <QtCore/QObject>

#include <kdemacros.h>

#include "libkwave/MemoryManager.h"
#include "libkwave/Sample.h"

class QWidget;

namespace Kwave
{

    class MetaDataList;
    class MultiTrackReader;
    class SignalManager;

    class KDE_EXPORT MimeData: public QMimeData
    {
	Q_OBJECT
	public:
	    /** Constructor */
	    MimeData();

	    /** Destructor */
	    virtual ~MimeData();

	    /**
	     * Encodes wave data received from a MultiTrackReader into a byte
	     * array that is compatible with the format of a wav file.
	     * @param widget the widget used as parent for displaying
	     *               error messages
	     * @param src source of the samples
	     * @param meta_data information about the signal, sample rate,
	     *                  resolution and other meta data
	     * @return true if successful
	     */
	    virtual bool encode(QWidget *widget,
	                        Kwave::MultiTrackReader &src,
	                        const Kwave::MetaDataList &meta_data);

	    /**
	     * Decodes the encoded byte data of the given mime source and
	     * initializes a MultiTrackReader.
	     * @param widget the widget used for displaying error messages
	     * @param e source with encoded mime data
	     * @param sig signal that receives the mime data
	     * @param pos position within the signal where to insert the data
	     * @return number of decoded samples if successful, zero if failed
	     */
	    static sample_index_t decode(QWidget *widget, const QMimeData *e,
	                                 Kwave::SignalManager &sig,
	                                 sample_index_t pos);

	    /**
	     * Clears the content, makes the storage an empty byte array
	     */
	    virtual void clear();

	private:
	    /**
	     * interal class for buffering huge amounts of mime data with
	     * support for swap files (using Kwave::MemoryManager).
	     * Used as a "write only" stream, after writing the data can be
	     * memory mapped and accessed through a QByteArray.
	     */
	    class Buffer: public QIODevice
	    {
	    public:
		/** Constructor */
		Buffer();

		/** Destructor, closes the buffer */
		virtual ~Buffer();

		/** returns the number of bytes written */
		virtual qint64 size() const { return m_size; }

		/**
		 * Try to map the memory to a QByteArray
		 */
		bool mapToByteArray();

		/**
		 * Returns the mapped data as a QByteArray
		 */
		inline const QByteArray &byteArray() const { return m_data; }

		/**
		 * Closes the buffer and frees the memory
		 * (calling multiple times is allowed)
		 */
		virtual void close();

	    protected:
		/**
		 * read a block of data from the buffer
		 *
		 * @param data buffer that receives the data
		 * @param maxlen maximum number of bytes to read
		 * @return number of bytes read or -1 if failed
		 */
		virtual qint64 readData(char *data, qint64 maxlen);

		/**
		 * write a block of data, internally increments the buffer
		 * size if necessary
		 *
		 * @param data pointer to a buffer with data to write
		 * @param len number of bytes to write
		 * @return number of bytes written or -1 if failed
		 */
		virtual qint64 writeData(const char *data, qint64 len);

	    private:
		/** handle of the block of memory (see MemoryManager) */
		Kwave::Handle m_block;

		/** number of total bytes written */
		qint64 m_size;

		/** simple array for storage of the wave data */
		QByteArray m_data;
	    };

	private:

	    /** buffer for the mime data (with swap file support) */
	    Kwave::MimeData::Buffer m_buffer;

    };
}

#endif /* _MIME_DATA_H_ */

//***************************************************************************
//***************************************************************************
