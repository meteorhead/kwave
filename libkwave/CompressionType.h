/***************************************************************************
      CompressionType.h  -  Map for all known compression types
                             -------------------
    begin                : Mon Jul 29 2002
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

#ifndef _COMPRESSION_TYPE_H_
#define _COMPRESSION_TYPE_H_

#include "config.h"

#ifdef USE_BUILTIN_LIBAUDIOFILE
#include "libaudiofile/audiofile.h" // from Kwave's copy of libaudiofile
#else /* USE_BUILTIN_LIBAUDIOFILE */
#include <audiofile.h> // from system
#endif /* USE_BUILTIN_LIBAUDIOFILE */

#include "TypesMap.h"

class CompressionType: public TypesMap<int, int>
{
public:

    /** extended compression types, not from libaudiofile */
    enum {
	MPEG_LAYER_I = 600,
	MPEG_LAYER_II,
	MPEG_LAYER_III,
	OGG_VORBIS
    };

    /** Constructor */
    CompressionType();

    /** Destructor */
    virtual ~CompressionType();

    /** fills the list */
    virtual void fill();

};

#endif /* _COMPRESSION_TYPE_H_ */
