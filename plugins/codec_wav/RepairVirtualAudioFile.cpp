/*************************************************************************
    RepairVirtualAudioFile.cpp  - emulation of a repaired sane audio file
                             -------------------
    begin                : Sun May 12 2002
    copyright            : (C) 2002 by Thomas Eschenbacher
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

#include <qfile.h>
#include <qiodevice.h>
#include "RecoverySource.h"
#include "RepairVirtualAudioFile.h"

//***************************************************************************
RepairVirtualAudioFile::RepairVirtualAudioFile(QIODevice &device,
    QList<RecoverySource> *repair_list)
    :VirtualAudioFile(device), m_position(0),
     m_repair_list(repair_list)
{
//    QFile file("/tmp/restored.wav");
//    file.open(IO_WriteOnly);
//
//    char buffer[65536];
//    unsigned int len;
//
//    do {
//        len = sizeof(buffer);
//        len = read(buffer, len);
//        file.writeBlock(buffer, len);
//    } while (len);
//    file.close();
}

//***************************************************************************
RepairVirtualAudioFile::~RepairVirtualAudioFile()
{
    if (m_repair_list) {
        m_repair_list->setAutoDelete(true);
        delete m_repair_list;
    }
}

//***************************************************************************
unsigned int RepairVirtualAudioFile::read(char *data, unsigned int nbytes)
{
    ASSERT(m_repair_list);
    ASSERT(data);
    if (!m_repair_list) return 0;
    if (!nbytes) return 0;
    if (!data) return 0;

    bzero(data, nbytes);
    size_t read_bytes = 0;
    QListIterator<RecoverySource> it(*m_repair_list);

    for (; it.current(); ++it) {
	unsigned int len = it.current()->read(m_position, data, nbytes);
	ASSERT(len <= nbytes);
	nbytes     -= len;
	m_position += len;
	data       += len;
	read_bytes += len;
	
	if (!nbytes) break;
    }

    return read_bytes;
}

//***************************************************************************
long RepairVirtualAudioFile::length()
{
    ASSERT(m_repair_list);
    if (!m_repair_list) return 0;
    RecoverySource *last = m_repair_list->last();
    ASSERT(last);
    if (!last) return 0;

    return last->offset() + last->length();
}

//***************************************************************************
unsigned int RepairVirtualAudioFile::write(const char */*data*/,
                                           unsigned int /*nbytes*/)
{
    warning("RepairVirtualAudioFile::write() is forbidden !");
    return 0;
}

//***************************************************************************
long RepairVirtualAudioFile::seek(long offset, int is_relative)
{
    if (is_relative == SEEK_CUR)
	m_position += offset;
    else if (is_relative == SEEK_SET)
	m_position = offset;
    else
	return -1;

    if ((long)m_position >= length()) return -1;
    return 0;
}

//***************************************************************************
long RepairVirtualAudioFile::tell()
{
    return m_position;
}

//***************************************************************************
//***************************************************************************
