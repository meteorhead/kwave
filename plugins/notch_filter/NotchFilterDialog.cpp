/***************************************************************************
  NotchFilterDialog.cpp  -  dialog for the "notch_filter" plugin
                             -------------------
    begin                : Thu Jun 19 2003
    copyright            : (C) 2003 by Dave Flogeras
    email                : d.flogeras@unb.ca
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
#include "math.h"

#include <qobject.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qwidget.h>
#include <knuminput.h>

#include <klocale.h>
#include "libkwave/Parser.h"
#include "libgui/ScaleWidget.h"
#include "libgui/FrequencyResponseWidget.h"

#include "NotchFilterDialog.h"
#include "NotchFilter.h"

//***************************************************************************
NotchFilterDialog::NotchFilterDialog(QWidget *parent, double sample_rate)
    :NotchFilterDlg(parent, 0, true),
     KwavePluginSetupDialog(),
     m_frequency(3500),m_bw(100),
     m_sample_rate(sample_rate), m_filter(0)
{

    // set maximum frequency to sample rate / 2
    double f_max = sample_rate / 2.0;

    slider->setMaxValue((int)f_max);
    spinbox->setMaxValue((int)f_max);

    slider_2->setMaxValue((int)f_max);
    spinbox_2->setMaxValue((int)f_max);

    // initialize the frequency scale widget
    scale_freq->setMinMax(0, (int)f_max);
    scale_freq->setLogMode(false);
    scale_freq->setUnit(i18n("Hz"));

    // initialize the attenuation scale widget
    scale_db->setMinMax(-24, +6);
    scale_db->setLogMode(false);
    scale_db->setUnit(i18n("dB"));

    // initialize the frequency response widget
    freq_response->init(f_max, -24, +6);

    // set up the low pass filter dunction
    m_filter = new NotchFilter();
    freq_response->setFilter(m_filter);

    // initialize the controls and the curve display
    slider->setValue((int)m_frequency);
    spinbox->setValue((int)m_frequency);
    slider_2->setValue((int)m_bw);
    spinbox_2->setValue((int)m_bw);
    updateDisplay();

    // changes in the slider or spinbox
    connect(spinbox, SIGNAL(valueChanged(int)),
            this, SLOT(freqValueChanged(int)));
    connect(spinbox_2, SIGNAL(valueChanged(int)),
    	    this, SLOT(bwValueChanged(int)));
    // click to the "Listen" button
    connect(btListen, SIGNAL(toggled(bool)),
            this, SLOT(listenToggled(bool)));

    // expand the "Listen" button to it's maximum width
    listenToggled(true);
    if (btListen->width() > btListen->minimumWidth())
        btListen->setMinimumWidth(btListen->width());
    listenToggled(false);
    if (btListen->width() > btListen->minimumWidth())
        btListen->setMinimumWidth(btListen->width());

    // set the initial size of the dialog
    int h = (width() * 3) / 5;
    if (height() < h) resize(width(), h);
    int w = (height() * 5) / 3;
    if (width() < w) resize(w, height());
}

//***************************************************************************
NotchFilterDialog::~NotchFilterDialog()
{
    // better stop pre-listen now
    listenToggled(false);

    if (freq_response) freq_response->setFilter(0);
    if (m_filter) delete m_filter;
}

//***************************************************************************
void NotchFilterDialog::freqValueChanged(int pos)
{
    if ((int)m_frequency != pos) {
	m_frequency = pos;
	updateDisplay();

	emit freqChanged(m_frequency);
    }
}

//***************************************************************************
void NotchFilterDialog::bwValueChanged(int pos)
{
    if ((int)m_bw != pos) {
    	m_bw = pos;
	updateDisplay();

	emit bwChanged(m_bw);
    }
}

//***************************************************************************
QStringList NotchFilterDialog::params()
{
    QStringList list;
    list << QString::number(m_frequency);
    list << QString::number(m_bw);
    return list;
}

//***************************************************************************
void NotchFilterDialog::setParams(QStringList &params)
{
    // evaluate the parameter list
    bool ok;
    double frequency = params[0].toDouble(&ok);
    Q_ASSERT(ok);
    if (ok) m_frequency = frequency;

    double bw = params[1].toDouble(&ok);
    Q_ASSERT(ok);
    if (ok) m_bw = bw;

    slider->setValue((int)m_frequency);
    spinbox->setValue((int)m_frequency);

    slider_2->setValue((int)m_bw);
    spinbox_2->setValue((int)m_bw);

    updateDisplay();
}

//***************************************************************************
void NotchFilterDialog::updateDisplay()
{
    double f_max = m_sample_rate / 2.0;
    if (m_filter && (f_max != 0.0))
    {
        m_filter->setFrequency((m_frequency/f_max)*M_PI,
		(m_bw/f_max)*M_PI );
    }

}

//***************************************************************************
void NotchFilterDialog::listenToggled(bool listen)
{
    Q_ASSERT(btListen);
    if (!btListen) return;

    if (listen) {
	// start pre-listen mode
	emit startPreListen();
	btListen->setText(i18n("&Stop"));
    } else {
	// stop pre-listen mode
	emit stopPreListen();
	btListen->setText(i18n("&Listen"));
    }
}

//***************************************************************************
void NotchFilterDialog::listenStopped()
{
    Q_ASSERT(btListen);
    if (!btListen) return;

    btListen->setOn(false);
}

//***************************************************************************
//***************************************************************************