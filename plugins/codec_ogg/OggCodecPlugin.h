/*************************************************************************
       OggCodecPlugin.h  -  import/export of Ogg/Vorbis data
                             -------------------
    begin                : Tue Sep 10 2002
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

#ifndef _OGG_CODEC_PLUGIN_H_
#define _OGG_CODEC_PLUGIN_H_

#include "config.h"
#include "libgui/KwavePlugin.h"

class Encoder;
class Decoder;

class OggCodecPlugin: public KwavePlugin
{
    Q_OBJECT
public:

    /** Constructor */
    OggCodecPlugin(PluginContext &c);

    /** Destructor */
    virtual ~OggCodecPlugin();

    /**
     * This plugin needs to be persistent!
     * @see KwavePlugin::isPersistent()
     */
    virtual bool isPersistent() { return true; };

    /**
     * Gets called when the plugin is first loaded.
     */
    virtual void load(QStringList &/* params */);

private:
    /** decoder used as factory */
    Decoder *m_decoder;

    /** encoder used as factory */
    Encoder *m_encoder;
};

#define LOAD_MIME_TYPES { \
    /* original from Ogg/Vorbis documentation: */ \
    addMimeType("audio/x-ogg",       i18n("Ogg, Vorbis audio"), "*.ogg; *.OGG"); \
    /* included in KDE: */ \
    addMimeType("application/x-ogg", i18n("Ogg, Vorbis audio"), "*.ogg; *.OGG"); \
}

#define DEFAULT_MIME_TYPE "audio/x-ogg"

#endif /* _OGG_CODEC_PLUGIN_H_ */