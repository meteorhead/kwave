/***************************************************************************
   PitchShiftPlugin.cpp  -  plugin for modifying the "pitch_shift"
                             -------------------
    begin                : Sun Mar 23 2003
    copyright            : (C) 2003 by Thomas Eschenbacher
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

#include "PitchShiftPlugin.h"
#include "PitchShiftDialog.h"

#include "synth_pitch_shift_bugfixed_impl.h" // bugfix for the aRts plugin

KWAVE_PLUGIN(PitchShiftPlugin,"pitch_shift","Thomas Eschenbacher");

//***************************************************************************
PitchShiftPlugin::PitchShiftPlugin(PluginContext &context)
    :KwavePlugin(context), m_params(), m_speed(1.0),
     m_frequency(5.0), m_percentage_mode(false),
     m_stop(false), m_listen(false)
{
}

//***************************************************************************
PitchShiftPlugin::~PitchShiftPlugin()
{
}

//***************************************************************************
int PitchShiftPlugin::interpreteParameters(QStringList &params)
{
    bool ok;
    QString param;

    // evaluate the parameter list
    if (params.count() != 3) return -EINVAL;

    param = params[0];
    m_speed = param.toDouble(&ok);
    Q_ASSERT(ok);
    if (!ok) return -EINVAL;

    param = params[1];
    m_frequency = param.toDouble(&ok);
    Q_ASSERT(ok);
    if (!ok) return -EINVAL;

    param = params[2];
    m_percentage_mode = (param.toUInt(&ok));
    Q_ASSERT(ok);
    if (!ok) return -EINVAL;

    // all parameters accepted
    m_params = params;
    
    return 0;
}

//***************************************************************************
QStringList *PitchShiftPlugin::setup(QStringList &previous_params)
{
    // try to interprete the previous parameters
    interpreteParameters(previous_params);

    // create the setup dialog
    PitchShiftDialog *dialog = new PitchShiftDialog(parentWidget());
    Q_ASSERT(dialog);
    if (!dialog) return 0;

    if (!m_params.isEmpty()) dialog->setParams(m_params);

    // connect the signals for pre-listen mode
    connect(dialog, SIGNAL(changed(double,double)),
            this, SLOT(setValues(double,double)));
    connect(dialog, SIGNAL(startPreListen()),
            this, SLOT(startPreListen()));
    connect(dialog, SIGNAL(stopPreListen()),
            this, SLOT(stopPreListen()));
    connect(this, SIGNAL(sigDone()),
            dialog, SLOT(listenStopped()));

    QStringList *list = new QStringList();
    Q_ASSERT(list);
    if (list && dialog->exec()) {
	// user has pressed "OK"
	*list = dialog->params();
    } else {
	// user pressed "Cancel"
	if (list) delete list;
	list = 0;
    }

    if (dialog) delete dialog;
    return list;
};

//***************************************************************************
void PitchShiftPlugin::run(QStringList params)
{
    unsigned int first, last;

    debug("PitchShiftPlugin::run(), m_listen = %d", m_listen);
    
    Arts::Dispatcher *dispatcher = manager().artsDispatcher();
    Q_ASSERT(dispatcher);
    if (!dispatcher) close();
    dispatcher->lock();

    UndoTransactionGuard *undo_guard = 0;
    m_stop = false;

    interpreteParameters(params);

    MultiTrackReader source;
    MultiTrackWriter sink;

    selection(&first, &last, true);
    manager().openMultiTrackReader(source, selectedTracks(), first, last);

    // create all objects
    ArtsMultiTrackSource arts_source(source);
    ArtsMultiSink *arts_sink = 0;

    unsigned int tracks = selectedTracks().count();

//  as long as the original aRts module is buggy:
//  ArtsNativeMultiTrackFilter pitch(tracks, "Arts::Synth_PITCH_SHIFT");
//  we use our own copy instead:
    ArtsKwaveMultiTrackFilter<Synth_PITCH_SHIFT_bugfixed,
                              Synth_PITCH_SHIFT_bugfixed_impl>
                              pitch(tracks);

    if (m_listen) {
	// pre-listen mode
	arts_sink = manager().openMultiTrackPlayback(selectedTracks().count());
    } else {
	// normal mode, with undo
	undo_guard = new UndoTransactionGuard(*this, i18n("pitch shift"));
	Q_ASSERT(undo_guard);
	if (!undo_guard) {
	    close();
	    return;
	}
	manager().openMultiTrackWriter(sink, selectedTracks(), Overwrite,
	    first, last);
	arts_sink = new ArtsMultiTrackSink(sink);
    }
    
    Q_ASSERT(arts_sink);
    if (!arts_sink || arts_sink->done()) {
	if (arts_sink)  delete arts_sink;
	if (undo_guard) delete undo_guard;
	if (!m_listen) close();
	return;
    }
    
    pitch.setAttribute("frequency", m_frequency);
    pitch.setAttribute("speed",     m_speed);
    
    // connect them
    pitch.connectInput(arts_source,  "source",  "invalue");
    pitch.connectOutput(*arts_sink,  "sink",    "outvalue");

    // start all
    arts_source.start();
    pitch.start();
    arts_sink->start();

    // remember last settings
    double last_speed = m_speed;
    double last_freq  = m_frequency;
    
    // transport the samples
    while (!m_stop && (!arts_source.done() || m_listen)) {
	arts_sink->goOn();
	
	// watch out for changed parameters when in
	// pre-listen mode
	if (m_listen && ((m_speed != last_speed) ||
	    (m_frequency != last_freq)))
	{
	    if (m_frequency != last_freq)
		pitch.setAttribute("frequency", m_frequency);
	
	    if (m_speed != last_speed)
		pitch.setAttribute("speed", m_speed);
	
	    last_freq  = m_frequency;
	    last_speed = m_speed;
        }

	if (m_listen && arts_source.done()) {
	    // start the next loop
	    source.reset();
	    arts_source.start();
	    continue;
	}

    }

    // shutdown
    pitch.stop();
    arts_sink->stop();
    arts_source.stop();

    dispatcher->unlock();

    delete arts_sink;
    
    if (undo_guard) delete undo_guard;
    close();
}

//***************************************************************************
int PitchShiftPlugin::stop()
{
    m_stop = true;
    return KwavePlugin::stop();
}

//***************************************************************************
void PitchShiftPlugin::setValues(double speed, double frequency)
{
    debug("PitchShiftPlugin::setValues(%g, %g)", speed,frequency);
    m_speed     = speed;
    m_frequency = frequency;
}

//***************************************************************************
void PitchShiftPlugin::startPreListen()
{
    m_listen = true;
    static QStringList empty_list;
    use();
    execute(empty_list);
}

//***************************************************************************
void PitchShiftPlugin::stopPreListen()
{
    stop();
    m_listen = false;
}

//***************************************************************************
//***************************************************************************
