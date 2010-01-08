/***************************************************************************
         MainWidget.cpp  -  main widget of the Kwave TopWidget
			     -------------------
    begin                : 1999
    copyright            : (C) 1999 by Martin Wilz
    email                : Martin Wilz <mwilz@ernie.mi.uni-koeln.de>
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
#include <math.h>
#include <stdlib.h>

#include <QBitmap>
#include <QComboBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMainWindow>
#include <QResizeEvent>
#include <QScrollBar>
#include <QWheelEvent>

#include <klocale.h>

#include "libkwave/ApplicationContext.h"
#include "libkwave/KwavePlugin.h" // for some helper functions
#include "libkwave/Parser.h"
#include "libkwave/SignalManager.h"

#include "libgui/OverViewWidget.h"
#include "libgui/SignalWidget.h"

#include "MainWidget.h"

/**
 * useful macro for command parsing
 */
#define CASE_COMMAND(x) } else if (parser.command() == x) {

/**
 * Limits the zoom to a minimum number of samples visible in one
 * screen.
 */
#define MINIMUM_SAMPLES_PER_SCREEN 5

/**
 * Default widht of the display in seconds when in streaming mode,
 * where no initial length information is available
 */
#define DEFAULT_DISPLAY_TIME 60.0

//***************************************************************************
MainWidget::MainWidget(QWidget *parent, Kwave::ApplicationContext &context)
    :QWidget(parent), m_context(context), m_overview(0), m_view_port(this),
     m_signal_widget(&m_view_port, context), m_vertical_scrollbar(0),
     m_horizontal_scrollbar(0), m_offset(0), m_width(0), m_zoom(1.0)
{
    QPalette palette;
//    qDebug("MainWidget::MainWidget()");

    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return;

    // topLayout:
    // - hbox
    // - overview
    // - horizontal scroll bar
    QVBoxLayout *topLayout = new QVBoxLayout(this);
    Q_ASSERT(topLayout);
    if (!topLayout) return;

    // -- signal widget --

    // hbox: left = viewport, right = vertical scroll bar
    QHBoxLayout *hbox = new QHBoxLayout();
    Q_ASSERT(hbox);
    if (!hbox) return;

    // viewport for the SignalWidget, does the clipping
    m_view_port.setFrameStyle(0);
    hbox->addWidget(&m_view_port);

    // -- vertical scrollbar for the view port --

    m_vertical_scrollbar = new QScrollBar();
    Q_ASSERT(m_vertical_scrollbar);
    if (!m_vertical_scrollbar) return;
    m_vertical_scrollbar->setOrientation(Qt::Vertical);
    m_vertical_scrollbar->setFixedWidth(
	m_vertical_scrollbar->sizeHint().width());
    hbox->addWidget(m_vertical_scrollbar);
    topLayout->addLayout(hbox);
    connect(m_vertical_scrollbar, SIGNAL(valueChanged(int)),
            this, SLOT(verticalScrollBarMoved(int)));

    // -- overview widget --

    m_overview = new OverViewWidget(*signal_manager, this);
    Q_ASSERT(m_overview);
    if (!m_overview) return;
    m_overview->setMinimumHeight(m_overview->sizeHint().height());
    m_overview->setSizePolicy(
	QSizePolicy::MinimumExpanding, QSizePolicy::Fixed
    );
    topLayout->addWidget(m_overview);
    connect(m_overview, SIGNAL(valueChanged(unsigned int)),
	    this, SLOT(setOffset(unsigned int)));
    connect(m_overview, SIGNAL(sigCommand(const QString &)),
            this, SLOT(forwardCommand(const QString &)));
    m_overview->labelsChanged(signal_manager->labels());

    // -- horizontal scrollbar --

    m_horizontal_scrollbar = new QScrollBar(this);
    Q_ASSERT(m_horizontal_scrollbar);
    if (!m_horizontal_scrollbar) return;
    m_horizontal_scrollbar->setOrientation(Qt::Horizontal);
    topLayout->addWidget(m_horizontal_scrollbar);
    connect(m_horizontal_scrollbar, SIGNAL(valueChanged(int)),
	    this, SLOT(horizontalScrollBarMoved(int)));

//     connect(&m_signal_widget, SIGNAL(selectedTimeInfo(unsigned int,
// 	unsigned int, double)),
// 	m_overview, SLOT(setSelection(unsigned int, unsigned int, double)));

//     connect(&(playbackController()), SIGNAL(sigPlaybackPos(unsigned int)),
// 	m_overview, SLOT(playbackPositionChanged(unsigned int)));
//     connect(&(playbackController()), SIGNAL(sigPlaybackStopped()),
// 	m_overview, SLOT(playbackStopped()));

    this->setLayout(topLayout);

//     // -- connect all signals from/to the signal widget --
//
//     connect(&m_signal_widget, SIGNAL(viewInfo(unsigned int,
// 	    unsigned int, unsigned int)),
// 	    this, SLOT(updateViewInfo(unsigned int, unsigned int,
// 	    unsigned int)));
//     connect(&m_signal_widget, SIGNAL(sigCommand(const QString &)),
// 	    this, SLOT(forwardCommand(const QString &)));
//     connect(&m_signal_widget, SIGNAL(selectedTimeInfo(unsigned int,
//             unsigned int, double)),
// 	    this, SIGNAL(selectedTimeInfo(unsigned int,
//             unsigned int, double)));
//     connect(&m_signal_widget, SIGNAL(sigMouseChanged(int)),
// 	    this, SIGNAL(sigMouseChanged(int)));
//     connect(signal_manager, SIGNAL(sigTrackSelectionChanged()),
// 	    this, SLOT(resizeViewPort()));

    // -- connect all signals from/to the signal manager --

    connect(signal_manager, SIGNAL(sigTrackInserted(unsigned int, Track *)),
	    this, SLOT(slotTrackInserted(unsigned int, Track *)));
    connect(signal_manager, SIGNAL(sigTrackDeleted(unsigned int)),
	    this, SLOT(slotTrackDeleted(unsigned int)));

    resizeViewPort();

//    qDebug("MainWidget::MainWidget(): done.");
}

//***************************************************************************
bool MainWidget::isOK()
{
    return (m_vertical_scrollbar && m_horizontal_scrollbar && m_overview);
}

//***************************************************************************
MainWidget::~MainWidget()
{
}

//***************************************************************************
void MainWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    resizeViewPort();
}

//***************************************************************************
void MainWidget::wheelEvent(QWheelEvent *event)
{
    if (!event || !m_overview) return;

    // process only wheel events on the signal and overview frame,
    // not on the channel controls or scrollbars
    if (!m_view_port.geometry().contains(event->pos()) &&
	!m_overview->geometry().contains(event->pos()) )
    {
	event->ignore();
	return;
    }

    switch (event->modifiers()) {
	case Qt::NoModifier: {
	    // no modifier + <WheelUp/Down> => scroll left/right
	    if (event->delta() > 0)
		executeCommand("scrollleft()");
	    else if (event->delta() < 0)
		executeCommand("scrollright()");
	    event->accept();
	    break;
	}
	case Qt::ShiftModifier:
	    // <Shift> + <WheelUp/Down> => page up/down
	    if (event->delta() > 0)
		executeCommand("viewprev()");
	    else if (event->delta() < 0)
		executeCommand("viewnext()");
	    event->accept();
	    break;
	case Qt::ControlModifier:
	    // <Ctrl> + <WheelUp/Down> => zoom in/out
	    if (event->delta() > 0)
		executeCommand("zoomin()");
	    else if (event->delta() < 0)
		executeCommand("zoomout()");
	    event->accept();
	    break;
	default:
	    event->ignore();
    }
}

//***************************************************************************
void MainWidget::verticalScrollBarMoved(int newval)
{
    m_signal_widget.move(0, -newval); // move the signal views
}

//***************************************************************************
void MainWidget::slotTrackInserted(unsigned int index, Track *track)
{
//     qDebug("MainWidget::slotTrackInserted(%u, %p)",
// 	   index, static_cast<void *>(track));
    Q_UNUSED(index);
    Q_UNUSED(track);

    // when the first track has been inserted, set some reasonable zoom
    SignalManager *signal_manager = m_context.signalManager();
    bool first_track = (signal_manager && (signal_manager->tracks() == 1));
    if (first_track) zoomAll();

    resizeViewPort();
    updateViewRange();
}

//***************************************************************************
void MainWidget::slotTrackDeleted(unsigned int index)
{
//     qDebug("MainWidget::slotTrackDeleted(%u)", index);
    Q_UNUSED(index);

    resizeViewPort();
    updateViewRange();
}

//***************************************************************************
double MainWidget::zoom() const
{
    return m_zoom;
}

//***************************************************************************
void MainWidget::forwardCommand(const QString &command)
{
    emit sigCommand(command);
}

//***************************************************************************
bool MainWidget::executeCommand(const QString &command)
{
    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return 0;

    const unsigned int visible_samples = displayWidth();
    if (!command.length()) return false;
    Parser parser(command);

    if (false) {

    // -- zoom --
    CASE_COMMAND("zoomin")
	zoomIn();
    CASE_COMMAND("zoomout")
	zoomOut();
    CASE_COMMAND("zoomselection")
	zoomSelection();
    CASE_COMMAND("zoomall")
	zoomAll();
    CASE_COMMAND("zoomnormal")
	zoomNormal();

    // -- navigation --
    CASE_COMMAND("goto")
	unsigned int offset = parser.toUInt();
	setOffset((offset > (visible_samples / 2)) ?
	          (offset - (visible_samples / 2)) : 0);
	signal_manager->selectRange(offset, 0);
    CASE_COMMAND("scrollright")
	setOffset(m_offset + visible_samples / 10);
    CASE_COMMAND("scrollleft")
	setOffset(((visible_samples / 10) < m_offset) ?
	          (m_offset - visible_samples / 10) : 0);
    CASE_COMMAND("viewstart")
	setOffset(0);
	signal_manager->selectRange(0, 0);
    CASE_COMMAND("viewend")
	unsigned int len = signal_manager->length();
	if (len >= visible_samples) setOffset(len - visible_samples);
    CASE_COMMAND("viewnext")
	setOffset(m_offset + visible_samples);
    CASE_COMMAND("viewprev")
	setOffset((visible_samples < m_offset) ?
	          (m_offset - visible_samples) : 0);

    // -- selection --
    CASE_COMMAND("selectall")
	signal_manager->selectRange(0, signal_manager->length());
    CASE_COMMAND("selectnext")
	if (signal_manager->selection().length())
	    signal_manager->selectRange(signal_manager->selection().last()+1,
	                signal_manager->selection().length());
	else
	    signal_manager->selectRange(signal_manager->length() - 1, 0);
    CASE_COMMAND("selectprev")
	unsigned int ofs = signal_manager->selection().first();
	unsigned int len = signal_manager->selection().length();
	if (!len) len = 1;
	if (len > ofs) len = ofs;
	signal_manager->selectRange(ofs-len, len);
    CASE_COMMAND("selecttoleft")
	signal_manager->selectRange(0, signal_manager->selection().last()+1);
    CASE_COMMAND("selecttoright")
	signal_manager->selectRange(signal_manager->selection().first(),
	    signal_manager->length() - signal_manager->selection().first()
	);
    CASE_COMMAND("selectvisible")
	signal_manager->selectRange(m_offset, pixels2samples(m_width) - 1);
    CASE_COMMAND("selectnone")
	signal_manager->selectRange(m_offset, 0);
    } else return false;

    return true;
}

//***************************************************************************
void MainWidget::resizeViewPort()
{
    bool vertical_scrollbar_visible = m_vertical_scrollbar->isVisible();
    const int min_height = m_signal_widget.minimumHeight();
    int h = m_view_port.height();
    int w = m_view_port.width();
    const int b = m_vertical_scrollbar->sizeHint().width();

    // if the signal widget's minimum height is smaller than the viewport
    if (min_height <= h) {
	// change the signal widget's vertical mode to "MinimumExpanding"
	m_signal_widget.setSizePolicy(
	    QSizePolicy::Expanding,
	    QSizePolicy::MinimumExpanding
	);

	// hide the vertical scrollbar
	if (vertical_scrollbar_visible) {
	    m_vertical_scrollbar->setShown(false);
	    vertical_scrollbar_visible = false;
	    w += b;
	    // qDebug("MainWidget::resizeViewPort(): hiding scrollbar");
	}
    } else {
	// -> otherwise set the widget height to "Fixed"
	m_signal_widget.setSizePolicy(
	    QSizePolicy::Expanding,
	    QSizePolicy::Preferred
	);

	// -- show the scrollbar --
	if (!vertical_scrollbar_visible) {
	    m_vertical_scrollbar->setFixedWidth(b);
	    m_vertical_scrollbar->setValue(0);
	    m_vertical_scrollbar->setShown(true);
	    vertical_scrollbar_visible = true;
	    w -= b;
	    // qDebug("MainWidget::resizeViewPort(): showing scrollbar");
	}

	// adjust the limits of the vertical scrollbar
	int min = m_vertical_scrollbar->minimum();
	int max = m_vertical_scrollbar->maximum();
	double val = (m_vertical_scrollbar->value() -
	    static_cast<double>(min)) / static_cast<double>(max - min);

	h = m_signal_widget.height();
	min = 0;
	max = h - m_view_port.height();
	m_vertical_scrollbar->setRange(min, max);
	m_vertical_scrollbar->setValue(static_cast<int>(floor(val *
	    static_cast<double>(max))));
	m_vertical_scrollbar->setSingleStep(1);
	m_vertical_scrollbar->setPageStep(m_view_port.height());
    }

    // resize the signal widget and the frame with the channel controls
    if ((m_signal_widget.width() != w) || (m_signal_widget.height() != h)) {
	m_width = w; // activate new with internally, for zoom range checks
	m_signal_widget.resize(w, h);
	fixZoomAndOffset();
    }

    // remember the last width of the signal widget, for zoom calculation
    m_width = m_signal_widget.width();
}

//***************************************************************************
void MainWidget::updateViewInfo(unsigned int, unsigned int, unsigned int)
{
    refreshHorizontalScrollBar();
}

//***************************************************************************
void MainWidget::refreshHorizontalScrollBar()
{
    if (!m_horizontal_scrollbar || !m_context.signalManager()) return;

    m_horizontal_scrollbar->blockSignals(true);

    // adjust the limits of the horizontal scrollbar
    if (m_context.signalManager()->length() > 1) {
	// get the view information in samples
	unsigned int length  = m_context.signalManager()->length();
	unsigned int offset  = m_offset;
	unsigned int visible = displaySamples();
	if (visible > length) visible = length;
	unsigned int range   = length - visible;
	if (range) range--;
	// in samples:
	// min  => 0
	// max  => length - visible - 1
	// page => visible
// 	qDebug("range = 0...%u, visible=%u, offset=%u, offset+visible=%d range+visible=%d",
// 	    range, visible, offset, offset+visible, range+visible);

	// calculate ranges in samples
	int width = displayWidth();
	int page  = static_cast<int>(floor(static_cast<double>(width) *
	    static_cast<double>(visible) / static_cast<double>(length)));
	if (page < 1) page = 1;
	if (page > width) page = width;
	int min   = 0;
	int max   = width - page;
	int pos   = (range) ? static_cast<int>(floor(
	    static_cast<double>(offset) * static_cast<double>(max) /
	    static_cast<double>(range))) : 0;
	int single = (page / 10);
	if (!single) single = 1;
// 	qDebug("width=%d,max=%d, page=%d, single=%d, pos=%d",
// 	       width, max, page, single, pos);

	m_horizontal_scrollbar->setRange(min, max);
	m_horizontal_scrollbar->setValue(pos);
	m_horizontal_scrollbar->setSingleStep(single);
	m_horizontal_scrollbar->setPageStep(page);
    } else {
	m_horizontal_scrollbar->setRange(0,0);
    }

    m_horizontal_scrollbar->blockSignals(false);
}

//***************************************************************************
void MainWidget::horizontalScrollBarMoved(int newval)
{
    if (!m_horizontal_scrollbar) return;

    unsigned int max = m_horizontal_scrollbar->maximum();
    if (max < 1) {
	setOffset(0);
	return;
    }

    // convert the current position into samples
    unsigned int length = m_context.signalManager()->length();
    unsigned int visible = displaySamples();
    if (visible > length) visible = length;
    unsigned int range   = length - visible;
    if (range) range--;

    unsigned int pos = static_cast<int>(floor(static_cast<double>(range) *
	static_cast<double>(newval) / static_cast<double>(max)));
//     qDebug("horizontalScrollBarMoved(%d) -> %u", newval, pos);
    setOffset(pos);
}

//***************************************************************************
void MainWidget::updateViewRange()
{
    SignalManager *signal_manager = m_context.signalManager();
    unsigned int total = (signal_manager) ? signal_manager->length() : 0;

    // forward the zoom and offset to the signal widget and overview
    m_signal_widget.setZoomAndOffset(m_zoom, m_offset);
    if (m_overview) m_overview->setRange(m_offset, displaySamples(), total);
}

//***************************************************************************
unsigned int MainWidget::ms2samples(double ms)
{
    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return 0;

    return static_cast<unsigned int>(
	rint(ms * signal_manager->rate() / 1E3));
}

//***************************************************************************
double MainWidget::samples2ms(unsigned int samples)
{
    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return 0.0;

    double rate = signal_manager->rate();
    if (rate == 0.0) return 0.0;
    return static_cast<double>(samples) * 1E3 / rate;
}

//***************************************************************************
unsigned int MainWidget::pixels2samples(int pixels) const
{
    if ((pixels <= 0) || (m_zoom <= 0.0)) return 0;
    return static_cast<unsigned int>(rint(
	static_cast<double>(pixels) * m_zoom));
}

//***************************************************************************
int MainWidget::samples2pixels(int samples) const
{
    if (m_zoom == 0.0) return 0;
    return static_cast<int>(rint(static_cast<double>(samples) / m_zoom));
}


//***************************************************************************
int MainWidget::displaySamples() const
{
    return pixels2samples(m_width);
}

//***************************************************************************
double MainWidget::fullZoom()
{
    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return 0.0;
    if (signal_manager->isEmpty()) return 0.0;    // no zoom if no signal

    unsigned int length = signal_manager->length();
    if (!length) {
        // no length: streaming mode -> start with a default
        // zoom, use one minute (just guessed)
        length = static_cast<unsigned int>(ceil(DEFAULT_DISPLAY_TIME *
	    signal_manager->rate()));
    }

    // example: width = 100 [pixels] and length = 3 [samples]
    //          -> samples should be at positions 0, 49.5 and 99
    //          -> 49.5 [pixels / sample]
    //          -> zoom = 1 / 49.5 [samples / pixel]
    // => full zoom [samples/pixel] = (length - 1) / (width - 1)
    return static_cast<double>(length - 1) /
	   static_cast<double>(m_width - 1);
}

//***************************************************************************
bool MainWidget::fixZoomAndOffset()
{
    double max_zoom;
    double min_zoom;
    unsigned int length;

    const int   old_width = m_width;
    const double old_zoom = m_zoom;

    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return false;

    if (!m_width) return (m_width != old_width);

    length = signal_manager->length();
    if (!length) {
	// in streaming mode we have to use a guessed length
	length = static_cast<unsigned int>(ceil(m_width * fullZoom()));
    }

    // ensure that m_offset is [0...length-1]
    if (m_offset > length- 1) m_offset = length - 1;

    // ensure that the zoom is in a proper range
    max_zoom = fullZoom();
    min_zoom = static_cast<double>(MINIMUM_SAMPLES_PER_SCREEN) /
	       static_cast<double>(m_width);
    if (m_zoom < min_zoom) m_zoom = min_zoom;
    if (m_zoom > max_zoom) m_zoom = max_zoom;

    // try to correct the offset if there is not enough data to fill
    // the current window
    // example: width=100 [pixels], length=3 [samples],
    //          offset=1 [sample], zoom=1/49.5 [samples/pixel] (full)
    //          -> current last displayed sample = length-offset
    //             = 3 - 1 = 2
    //          -> available space = pixels2samples(width-1) + 1
    //             = (99/49.5) + 1 = 3
    //          -> decrease offset by 3 - 2 = 1
    Q_ASSERT(length >= m_offset);
    if (pixels2samples(m_width - 1) + 1 > length - m_offset) {
	// there is space after the signal -> move offset right
	unsigned int shift = pixels2samples(m_width - 1) + 1 -
	                     (length - m_offset);
	if (shift >= m_offset) {
	    m_offset = 0;
	} else {
	    m_offset -= shift;
	}
    }

// not really needed, has side-effects e.g. when a .ogg file
// has been loaded
//    // if reducing the offset was not enough, zoom in
//    if (pixels2samples(m_width - 1) + 1 > length - m_offset) {
//	// there is still space after the signal -> zoom in
//	// (this should never happen as the zoom has been limited before)
//	m_zoom = max_zoom;
//    }

// this made some strange effects, so I disabled it :-[
//    // adjust the zoom factor in order to make a whole number
//    // of samples fit into the current window
//    int samples = pixels2samples(width) + 1;
//    zoom = (double)(samples) / (double)(width - 1);

    // do some final range checking
    if (m_zoom < min_zoom) m_zoom = min_zoom;
    if (m_zoom > max_zoom) m_zoom = max_zoom;

    // emit change in the zoom factor
    if (m_zoom != old_zoom) emit sigZoomChanged(m_zoom);

    return ((m_width != old_width) || (m_zoom != old_zoom));
}

//***************************************************************************
void MainWidget::setZoom(double new_zoom)
{
    double       old_zoom   = m_zoom;
    unsigned int old_offset = m_offset;

    m_zoom = new_zoom;
    fixZoomAndOffset();
    if ((m_offset != old_offset) || (m_zoom != old_zoom))
	updateViewRange();
}

//***************************************************************************
void MainWidget::setOffset(unsigned int new_offset)
{
    double       old_zoom   = m_zoom;
    unsigned int old_offset = m_offset;

    m_offset = new_offset;
    fixZoomAndOffset();
    if ((m_offset != old_offset) || (m_zoom != old_zoom))
	updateViewRange();
}

//***************************************************************************
void MainWidget::zoomSelection()
{
    SignalManager *signal_manager = m_context.signalManager();
    Q_ASSERT(signal_manager);
    if (!signal_manager) return;

    unsigned int ofs = signal_manager->selection().offset();
    unsigned int len = signal_manager->selection().length();

    if (len) {
	m_offset = ofs;
	setZoom((static_cast<double>(len)) / static_cast<double>(m_width - 1));
    }
}

//***************************************************************************
void MainWidget::zoomAll()
{
    setZoom(fullZoom());
}

//***************************************************************************
void MainWidget::zoomNormal()
{
    const unsigned int shift1 = m_width / 2;
    const unsigned int shift2 = displaySamples() / 2;
    m_offset = (m_offset + shift2 >= m_offset) ? (m_offset + shift2) : -shift2;
    m_offset = (m_offset > shift1) ? (m_offset - shift1) : 0;
    setZoom(1.0);
}

//***************************************************************************
void MainWidget::zoomIn()
{
    const unsigned int shift = displaySamples() / 3;
    m_offset = (m_offset + shift >= m_offset) ? (m_offset + shift) : -shift;
    setZoom(m_zoom / 3);
}

//***************************************************************************
void MainWidget::zoomOut()
{
    const unsigned int shift = displaySamples();
    m_offset = (m_offset > shift) ? (m_offset - shift) : 0;
    setZoom(m_zoom * 3);
}

//***************************************************************************
#include "MainWidget.moc"
//***************************************************************************
//***************************************************************************
