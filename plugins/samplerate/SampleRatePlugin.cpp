/***************************************************************************
   SampleRatePlugin.cpp  -  sample rate conversion
                             -------------------
    begin                : Tue Jul 07 2009
    copyright            : (C) 2009 by Thomas Eschenbacher
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

#include <KLocalizedString> // for the i18n macro

#include <QList>
#include <QListIterator>
#include <QStringList>

#include "libkwave/Connect.h"
#include "libkwave/FileInfo.h"
#include "libkwave/MetaDataList.h"
#include "libkwave/MultiTrackReader.h"
#include "libkwave/MultiTrackWriter.h"
#include "libkwave/PluginManager.h"
#include "libkwave/SignalManager.h"
#include "libkwave/Utils.h"
#include "libkwave/Writer.h"
#include "libkwave/modules/RateConverter.h"
#include "libkwave/undo/UndoTransactionGuard.h"

#include "SampleRatePlugin.h"

KWAVE_PLUGIN(samplerate, SampleRatePlugin)

//***************************************************************************
Kwave::SampleRatePlugin::SampleRatePlugin(QObject *parent,
                                          const QVariantList &args)
    :Kwave::Plugin(parent, args), m_params(), m_new_rate(0.0),
     m_whole_signal(false)
{
}

//***************************************************************************
Kwave::SampleRatePlugin::~SampleRatePlugin()
{
}

//***************************************************************************
int Kwave::SampleRatePlugin::interpreteParameters(QStringList &params)
{
    bool ok = false;
    QString param;

    // set defaults
    m_new_rate     = 44100.0;
    m_whole_signal = false;

    // evaluate the parameter list
    if (params.count() < 1) return -EINVAL;

    param = params[0];
    m_new_rate = param.toDouble(&ok);
    Q_ASSERT(ok);
    if (!ok) return -EINVAL;

    // check whether we should change the whole signal (optional)
    if (params.count() == 2) {
	if (params[1] != _("all"))
	    return -EINVAL;
	m_whole_signal = true;
    }

    // all parameters accepted
    m_params = params;

    return 0;
}

//***************************************************************************
void Kwave::SampleRatePlugin::run(QStringList params)
{
    Kwave::SignalManager &mgr = signalManager();

    // parse parameters
    if (interpreteParameters(params) < 0)
	return;

    double old_rate = Kwave::FileInfo(signalManager().metaData()).rate();
    if ((old_rate <= 0) || qFuzzyCompare(old_rate, m_new_rate)) return;

    Kwave::UndoTransactionGuard undo_guard(*this, i18n("Change sample rate"));

    // get the current selection and the list of affected tracks
    sample_index_t first = 0;
    sample_index_t last  = 0;
    sample_index_t length;
    QList<unsigned int> tracks;
    if (m_whole_signal) {
	length = signalLength();
	last   = (length) ? (length - 1) : 0;
	tracks = mgr.allTracks();
    } else {
	length = selection(&tracks, &first, &last, true);
	if ((length == signalLength()) &&
	    (tracks.count() == Kwave::toInt(mgr.tracks())))
	{
	    // manually selected the whole signal
	    m_whole_signal = true;
	}
    }
    qDebug("SampleRatePlugin: from %9lu - %9lu (%9lu)",
	   static_cast<unsigned long int>(first),
	   static_cast<unsigned long int>(last),
	   static_cast<unsigned long int>(length));
    if (!length || tracks.isEmpty()) return;

    // calculate the new length
    double ratio = m_new_rate / old_rate;
    sample_index_t new_length = static_cast<sample_index_t>(length * ratio);
    if ((new_length == length) || !new_length) return;

    // if the new length is bigger than the current length,
    // insert some space at the end
    if (new_length > length) {
	qDebug("SampleRatePlugin: inserting %lu at %lu",
	       static_cast<unsigned long int>(new_length - length + 1),
	       static_cast<unsigned long int>(last + 1));
	mgr.insertSpace(last + 1, new_length - length + 1, tracks);
    }

    Kwave::MultiTrackReader source(Kwave::SinglePassForward,
	mgr, tracks, first, last);

    // connect the progress dialog
    connect(&source, SIGNAL(progress(qreal)),
	    this,  SLOT(updateProgress(qreal)),
	     Qt::BlockingQueuedConnection);
    emit setProgressText(
	i18n("Changing sample rate from %1 kHz to %2 kHz...",
	     (old_rate   / 1E3), (m_new_rate / 1E3))
    );

    // create the converter
    Kwave::MultiTrackSource<Kwave::RateConverter, true> converter(
	tracks.count(), this);
    converter.setAttribute(SLOT(setRatio(QVariant)), QVariant(ratio));

    // create the writer with the appropriate length
    Kwave::MultiTrackWriter sink(mgr, tracks, Kwave::Overwrite,
	first, first + new_length - 1);

    // connect the objects
    bool ok = true;
    if (ok) ok = Kwave::connect(
	source,    SIGNAL(output(Kwave::SampleArray)),
	converter, SLOT(input(Kwave::SampleArray)));
    if (ok) ok = Kwave::connect(
	converter, SIGNAL(output(Kwave::SampleArray)),
	sink,      SLOT(input(Kwave::SampleArray)));
    if (!ok) {
	return;
    }

    while (!shouldStop() && !source.eof()) {
	source.goOn();
	converter.goOn();
    }

    sink.flush();

    // find out how many samples have been written and delete the leftovers
    sample_index_t written = sink[0]->position() - first;
//     qDebug("SampleRatePlugin: old=%u, expexted=%u, written=%u",
// 	    length, new_length, written);
    if (written < length) {
	sample_index_t to_delete = length - written;
	mgr.deleteRange(written, to_delete, tracks);
    }

    // adjust meta data locations
    Kwave::MetaDataList meta = mgr.metaData().copy(first, last, tracks);
    if (!meta.isEmpty()) {
	// adjust all positions in the originally selected range
	// NOTE: if the ratio is > 1, work backwards, otherwise forward

	Kwave::MetaDataList::MutableIterator it(meta);
	(ratio > 1) ? it.toBack() : it.toFront();
	while ((ratio > 1) ? it.hasPrevious() : it.hasNext()) {
	    Kwave::MetaData &m = (ratio > 1) ?
		it.previous().value() : it.next().value();

	    QStringList properties =
	        Kwave::MetaData::positionBoundPropertyNames();
	    foreach (const QString &property, properties) {
		if (!m.hasProperty(property))
		    continue;

		sample_index_t pos = static_cast<sample_index_t>(
		    m[property].toULongLong());
		if (pos < first) continue;
		if (pos > last)  continue;

		// is in our range -> calculate new position
		pos -= first;
		pos *= ratio;
		pos += first;

		m[property] = pos;
	    }
	}

	mgr.metaData().deleteRange(first, last, tracks);
	mgr.metaData().merge(meta);
    }

    // update the selection if it was not empty
    length = selection(0, &first, &last, false);
    if (length) {
	if (m_whole_signal) {
	    // if whole signal selected -> adjust start and end
	    first *= ratio;
	    last  *= ratio;
	    length = last - first + 1;
	} else {
	    // only a portion selected -> adjust only length
	    length *= ratio;
	}

	mgr.selectRange(first, length);
    }

    // set the sample rate if we modified the whole signal
    if (m_whole_signal) {
	Kwave::FileInfo info(signalManager().metaData());
	info.setRate(m_new_rate);
	mgr.setFileInfo(info, false);
    }

}

//***************************************************************************
#include "SampleRatePlugin.moc"
//***************************************************************************
//***************************************************************************
