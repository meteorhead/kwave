/***************************************************************************
       VolumePlugin.cpp  -  Plugin for adjusting a signal's volume
                             -------------------
    begin                : Sun Oct 27 2002
    copyright            : (C) 2001 by Thomas Eschenbacher
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
#include <errno.h>

#include <qstringlist.h>
#include <klocale.h>

#include "libkwave/Parser.h"

#include "libkwave/ArtsMultiTrackSink.h"
#include "libkwave/ArtsMultiTrackSource.h"
#include "libkwave/ArtsKwaveMultiTrackFilter.h"
#include "libkwave/ArtsNativeMultiTrackFilter.h"

#include "kwave/PluginManager.h"
#include "kwave/UndoTransactionGuard.h"

#include "VolumePlugin.h"
#include "VolumeDialog.h"

KWAVE_PLUGIN(VolumePlugin,"volume","Thomas Eschenbacher");

//***************************************************************************
VolumePlugin::VolumePlugin(PluginContext &context)
    :KwavePlugin(context), m_params(), m_factor(1.0), m_mode(0),
     m_stop(false)
{
}

//***************************************************************************
VolumePlugin::~VolumePlugin()
{
}

//***************************************************************************
int VolumePlugin::interpreteParameters(QStringList &params)
{
    bool ok;
    QString param;

    // evaluate the parameter list
    if (params.count() != 2) return -EINVAL;

    param = params[0];
    m_factor = param.toDouble(&ok);
    ASSERT(ok);
    if (!ok) return -EINVAL;

    param = params[1];
    m_mode = param.toUInt(&ok);
    ASSERT(ok);
    if (!ok || (m_mode > 2)) return -EINVAL;

    return 0;
}

//***************************************************************************
QStringList *VolumePlugin::setup(QStringList &previous_params)
{
    // try to interprete the previous parameters
    interpreteParameters(previous_params);

    // create the setup dialog
    VolumeDialog *dialog = new VolumeDialog(parentWidget());
    ASSERT(dialog);
    if (!dialog) return 0;

    if (!m_params.isEmpty()) dialog->setParams(m_params);

    QStringList *list = new QStringList();
    ASSERT(list);
    if (list && dialog->exec()) {
	// user has pressed "OK"
	QString cmd = dialog->getCommand();
	Parser p(cmd);
	while (!p.isDone()) *list << p.nextParam();
	emitCommand(cmd);
    } else {
	// user pressed "Cancel"
	if (list) delete list;
	list = 0;
    }

    if (dialog) delete dialog;
    return list;
};

//***************************************************************************
void VolumePlugin::run(QStringList params)
{
    unsigned int first, last;

    Arts::Dispatcher *dispatcher = manager().artsDispatcher();
    dispatcher->lock();
    ASSERT(dispatcher);
    if (!dispatcher) close();

    UndoTransactionGuard undo_guard(*this, i18n("volume"));
    m_stop = false;

    interpreteParameters(params);

    MultiTrackReader source;
    MultiTrackWriter sink;

    /*unsigned int input_length =*/ selection(&first, &last, true);
    manager().openMultiTrackReader(source, selectedTracks(), first, last);
    manager().openMultiTrackWriter(sink, selectedTracks(), Overwrite,
	first, last);

    // create all objects
    ArtsMultiTrackSource arts_source(source);

    unsigned int tracks = selectedTracks().count();
    ArtsNativeMultiTrackFilter mul(tracks, "Arts::Synth_MUL");
    ArtsMultiTrackSink   arts_sink(sink);

    // connect them
    mul.setValue("invalue1", m_factor);
    mul.connectInput(arts_source,   "source",   "invalue2");
    mul.connectOutput(arts_sink,    "sink",     "outvalue");

    // start all
    arts_source.start();
    mul.start();
    arts_sink.start();

    // transport the samples
    debug("VolumePlugin: filter started...");
    while (!m_stop && !(arts_source.done())) {
	arts_sink.goOn();
    }
    debug("VolumePlugin: filter done.");

    // shutdown
    mul.stop();
    arts_sink.stop();
    arts_source.stop();

    dispatcher->unlock();

    close();
}

//***************************************************************************
int VolumePlugin::stop()
{
    m_stop = true;
    return KwavePlugin::stop();
}

//***************************************************************************
//***************************************************************************