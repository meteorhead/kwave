/***************************************************************************
       FileInfoPlugin.h  -  plugin for editing file properties
                             -------------------
    begin                : Fri Jul 19 2002
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

#ifndef _FILE_INFO_PLUGIN_H_
#define _FILE_INFO_PLUGIN_H_

#include "config.h"
#include <qobject.h>
#include "libgui/KwavePlugin.h"

class FileInfoPlugin: public KwavePlugin
{
    Q_OBJECT
public:

    /** Constructor */
    FileInfoPlugin(PluginContext &context);

    /** virtual Destructor */
    virtual ~FileInfoPlugin();

    /**
     * Shows a dialog for editing file properties.
     * @see KwavePlugin::setup
     */
    virtual QStringList *setup(QStringList &);

};

#endif /* _FILE_INFO_PLUGIN_H_ */