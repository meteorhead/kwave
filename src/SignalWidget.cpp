//methods for SignalWidget the view for MSignal objects.
//methods concerning markers may be found in markers.cpp

#include "config.h"
#include <math.h>
#include <stdlib.h>

#include <qobject.h>
#include <qtimer.h>
#include <qfiledlg.h>

#include <kmsgbox.h>
#include <kapp.h>

#include <libkwave/Label.h>
#include <libkwave/LabelList.h>
#include <libkwave/Parser.h>
#include <libkwave/Global.h>
#include <libkwave/DynamicLoader.h>
#include <libkwave/Signal.h>
#include <libkwave/DialogOperation.h>
#include <libkwave/Color.h>

#include "libgui/Dialog.h"
#include "libgui/MenuManager.h"

#include "SignalWidget.h"
#include "SignalManager.h"
#include "MouseMark.h"
#include "ProgressDialog.h"
#include "sampleop.h"

#ifdef DEBUG
#include <time.h>
#include <sys/time.h>
#endif

#define LAYER_SIGNAL    0
#define LAYER_SELECTION 1
#define LAYER_MARKERS   2

/**
 * useful macro for command parsing
 */
#define CASE_COMMAND(x) } else if (matchCommand(command, x)) {

/**
 * math.h didn't define PI :-(
 */
#define PI 3.14159265358979323846264338327

/**
 * This factor determines how many times the order is over than
 * the minimum required order. Higher values give less problems with
 * aliasing but some amplitude errors. Lower values make less
 * amplitude errors but more aliasing problems.
 * This should be a good compromise...
 */
#define INTERPOLATION_PRECISION 4

/**
 * Limits the zoom to a minimum number of samples visible in one
 * screen.
 */
#define MINIMUM_SAMPLES_PER_SCREEN 5

extern Global globals;

//****************************************************************************
ProgressDialog *createProgressDialog(TimeOperation *operation,
                                     const char *caption)
{
//    ProgressDialog *dialog = new ProgressDialog (operation, caption);
//    if (dialog) {
//	dialog->show();
//	return dialog;
//    }

    return 0;
}

//****************************************************************************
SignalWidget::SignalWidget(QWidget *parent, MenuManager &menu_manager)
    :QWidget(parent),
    menu(menu_manager)
{
    debug("SignalWidget::SignalWidget()");
    down = false;
    height = 0;
    interpolation_alpha = 0;
    interpolation_order = 0;
    labels = 0;
    lastplaypointer = -1;
    lasty = -1;
    markertype = 0;
    offset = 0;
    pixmap = 0;
    playing = false;
    playpointer = -1;
    redraw = false;
    select = 0;
    signalmanage = 0;
    timer = 0;
    width = 0;
    zoom = 1.0;
    zoomy = 1;

    for (int i=0; i < 3; i++) {
	layer[i] = 0;
	update_layer[i] = true;
	layer_rop[i] = CopyROP;
    }

    select = new MouseMark(this);
    ASSERT(select);
    if (!select) return;
    connect(select, SIGNAL(refresh()),
            this, SLOT(refreshSelection()));

    labels = new LabelList();
    ASSERT(labels);
    if (!labels) return;
    labels->setAutoDelete (true);

    menu.clearNumberedMenu("ID_LABELS_TYPE");
    for (LabelType *tmp = globals.markertypes.first(); tmp;
         tmp = globals.markertypes.next())
    {
	menu.addNumberedMenuEntry("ID_LABELS_TYPE", (char *)tmp->name);
    }

    markertype = globals.markertypes.first();

    setBackgroundColor(black);
    setMouseTracking(true);

    zoomAll();

    debug("SignalWidget::SignalWidget(): done.");
}

//****************************************************************************
void SignalWidget::refreshSelection()
{
    refreshLayer(LAYER_SELECTION);
}

//****************************************************************************
bool SignalWidget::isOK()
{
    ASSERT(labels);
    ASSERT(select);
    return ( labels && select );
}

//****************************************************************************
SignalWidget::~SignalWidget()
{
    debug("SignalWidget::~SignalWidget()");
    ASSERT(KApplication::getKApplication());

    if (timer) delete timer;
    timer = 0;

    if (signalmanage) delete signalmanage;
    signalmanage = 0;

    if (pixmap == 0) delete pixmap;
    pixmap = 0;

    if (labels) delete labels;
    labels = 0;

    if (select) delete select;
    select = 0;

    if (interpolation_alpha) delete[] interpolation_alpha;
    interpolation_alpha = 0;

    for (int i=0; i < 3; i++) {
	if (layer[i]) layer[i] = 0;
	layer[i] = 0;
    }

    debug("SignalWidget::~SignalWidget(): done");
}

//****************************************************************************
void SignalWidget::saveSignal(const char *filename, int bits,
			      int type, bool selection)
{
    if (!signalmanage) return ;
    if (type == ASCII) {
	signalmanage->exportAscii(filename);
    } else {
	signalmanage->save(filename, bits, selection);
    }
}

//****************************************************************************
unsigned char *SignalWidget::getOverview (int size)
{
//    int step, max = 0, min = 0;
//
//    unsigned char *overview = new unsigned char [size];
//
//    for (int i = 0; i < size; overview[i++] = 0);    //clear
//
//    if (overview && signalmanage) {
//	double z = ((double)(signalmanage->getLength())) / size;
//	int channels = signalmanage->getChannelCount();
//
//	for (int c = 0; c < channels; c++) {
//	    for (int i = 0; i < size; i++) {
//		step = ((int) (((double)i) * z));
//		signalmanage->getMaxMin (c, max, min, step, (int) z + 2);
//		if ( -min > max) max = -min;
//		overview[i] += max / 65536;
//	    }
//	}
//    }
//    return overview;

    return 0;
}

//****************************************************************************
void SignalWidget::toggleChannel(int channel)
{
    ASSERT(signalmanage);
    if (signalmanage) signalmanage->toggleChannel(channel);
}

//****************************************************************************
bool SignalWidget::executeNavigationCommand(const char *command)
{
    if (!signalmanage) return false;
    if (!command) return false;

    if (false) {
    CASE_COMMAND("zoomin")
	zoomIn();
    CASE_COMMAND("zoomout")
	zoomOut();
    CASE_COMMAND("zoomrange")
	zoomRange();
    CASE_COMMAND("scrollright")
	setOffset(offset + pixels2samples(width / 10));
	refreshAllLayers();
    CASE_COMMAND("scrollleft")
	setOffset(offset - pixels2samples(width / 10));
	refreshAllLayers();
    CASE_COMMAND("viewnext")
	setOffset(offset + pixels2samples(width));
	refreshAllLayers();
    CASE_COMMAND("viewprev")
	setOffset(offset - pixels2samples(width));
	refreshAllLayers();
    CASE_COMMAND("selectall")
	setRange(0, signalmanage->getLength() - 1);
    CASE_COMMAND("selectnext")
	int r = signalmanage->getRMarker();
	int l = signalmanage->getLMarker();
	setRange(r + 1, r + 1 + (r - l));
    CASE_COMMAND("selectprev")
	int r = signalmanage->getRMarker();
	int l = signalmanage->getLMarker();
	setRange(l - (r - l) - 1, l - 1);
    CASE_COMMAND("selecttoleft")
	int l = 0;
	int r = signalmanage->getRMarker();
	setRange(l, r);
    CASE_COMMAND("selecttoright")
	int l = signalmanage->getLMarker();
	int r = signalmanage->getLength() - 1;
	setRange(l, r);
    CASE_COMMAND("selectvisible")
	setRange(offset, offset + pixels2samples(width) - 1);
    CASE_COMMAND("selectnone")
	setRange(offset, offset);
    CASE_COMMAND("selectrange")
	selectRange();
    } else return false;

    return true;
}

//****************************************************************************
bool SignalWidget::executeLabelCommand (const char *command)
{
    if (false) {
//    CASE_COMMAND("chooselabel")
//	Parser parser(command);
//	markertype = globals.markertypes.at(parser.toInt());
//    CASE_COMMAND("amptolabel")
//	markSignal(command);
//    CASE_COMMAND("pitch")
//	markPeriods(command);
////    CASE_COMMAND("labeltopitch")
////      convertMarkstoPitch(command);
//    CASE_COMMAND("deletelabel")
//	deleteLabel();
//    CASE_COMMAND("insertlabel")
//	appendLabel();
//    CASE_COMMAND("loadlabel")
//	loadLabel();
//    CASE_COMMAND("savelabel")
//	saveLabel(command);
//    CASE_COMMAND("label")
//	addLabel(command);
//    CASE_COMMAND("newlabeltype")
//	addLabelType(command);
//    CASE_COMMAND("expandtolabel")
//	jumptoLabel();
//    CASE_COMMAND("mark")
//	markSignal(command);
//    CASE_COMMAND("markperiod")
//	markPeriods(command);
//    CASE_COMMAND("saveperiods")
//	savePeriods();
    } else return false;

    return true;
}

//****************************************************************************
bool SignalWidget::executeCommand(const char *command)
{
    ASSERT(command);
    if (!command) return false;

    debug("SignalWidget::executeCommand(%s)", command);    // ###

    if (false) {
    CASE_COMMAND("dialog")
	Parser parser(command);
	const char *name = parser.getFirstParam();
	debug("SignalWidget::executeCommand(): loading dialog %s", name);
	showDialog(name);
    CASE_COMMAND("refresh")
	refreshAllLayers();
    CASE_COMMAND("newsignal")
	createSignal(command);
    CASE_COMMAND("cut")
	ASSERT(signalmanage);
	if (signalmanage) {
	    //set range after cutting
	    bool x = signalmanage->executeCommand(command);
	    setRange(signalmanage->getLMarker(), signalmanage->getLMarker());
	    return x;
	}
//    } else if (executeLabelCommand(command)) {
//	return true;
    } else if (executeNavigationCommand(command)) {
	return true;
    } else if (signalmanage) {
	return signalmanage->executeCommand(command);
    } else return false;

    return true;
}

//**********************************************************
void SignalWidget::showDialog(const char *name)
{
    debug("SignalWidget::showDialog(%s)", name);
    int length = 0;
    int rate = 44100;
    int channels = 0;
    if (signalmanage) length = signalmanage->getLength ();
    if (signalmanage) rate = signalmanage->getRate ();
    if (signalmanage) channels = signalmanage->getChannelCount ();

    DialogOperation *operation =
	new DialogOperation(&globals, length, rate, channels, true);
    ASSERT(operation);

    if (operation) {
	Dialog *dialog = DynamicLoader::getDialog(name, operation);
	if (dialog) {
	    connect(dialog, SIGNAL(command(const char*)),
		    this, SLOT(forwardCommand(const char *)));
	    // ### dialog->show();
	    dialog->exec();
	    delete dialog;
	} else debug ("error: could not get dialog !\n");

	delete operation;
    }
}

//****************************************************************************
void SignalWidget::showMessage(const char *caption, const char *text,
			       int flags)
{
    debug("SignalWidget::showMessage(...)");
    ASSERT(caption);
    ASSERT(text);
    if (!caption || !text) return;
    KMsgBox::message(this, caption, text, flags);
}

//****************************************************************************
void SignalWidget::playback_setOp(int op)
{
    if (signalmanage) {
	signalmanage->setOp(op);

	switch (op) {
	    case PLAY:
	    case LOOP:
		if (timer == 0) {
		    timer = new QTimer(this);
		    ASSERT(timer);
		    if (timer) connect(timer, SIGNAL(timeout()),
		                       this, SLOT(playback_time()));
		}
		playing = true;
		if (timer) timer->start(200);
		break;
	    case PSTOP:
		ASSERT(timer);
		if (timer) timer->stop();
		playing = false;
		playpointer = -1;
		if (lastplaypointer >= 0) repaint(false);
		break;
	    case PHALT:    //halt by Gerhard Zintel
		{
		    ASSERT(timer);
		    if (timer) timer->stop();
		    playing = false;
		    int lmarker = signalmanage->getPlayPosition();
		    int rmarker = signalmanage->getRMarker();

		    if (rmarker < lmarker) rmarker = lmarker;
		    setRange(lmarker, rmarker);
		    playpointer = -1;
		    break;
		}
	}
    }
}

//****************************************************************************
void SignalWidget::selectRange()
{
    ASSERT(signalmanage);
    if (!signalmanage) return;
    if (!getChannelCount()) return ;

    int rate = signalmanage->getRate();

    Dialog *dialog = DynamicLoader::getDialog
		     ("time", new DialogOperation (rate, true));
    ASSERT(dialog);
    if (!dialog) return ;

    if ( dialog->exec() != 0 ) {
	int l = signalmanage->getLMarker();

	Parser parser(dialog->getCommand());
	int len = ms2samples( parser.toDouble() );
	int siglen = signalmanage->getLength();

	if ((l + len - 1) >= siglen)
	    setRange(l, siglen - 1);    //overflow check
	else
	    setRange(l, l + len - 1);
    }
}

//****************************************************************************
void SignalWidget::connectSignal()
{
    ASSERT(signalmanage);
    ASSERT(select);
    if (!signalmanage) return;
    if (!select) return;

//    connect(signalmanage,SIGNAL(signalinserted(int,int)),
//            this,SLOT(signalinserted(int,int)));
//    connect(signalmanage,SIGNAL(signaldeleted(int,int)),
//            this, SLOT(signaldeleted(int,int)));

    connect(select, SIGNAL(selection(int, int)),
	    this, SLOT(estimateRange(int, int)));

    connect(signalmanage, SIGNAL(sigCommand(const char*)),
	    this, SLOT(forwardCommand(const char *)));
    connect(signalmanage, SIGNAL(sigChannelAdded(unsigned int)),
	    this, SLOT(forwardChannelAdded(unsigned int)));
    connect(signalmanage, SIGNAL(sigChannelDeleted(unsigned int)),
	    this, SLOT(forwardChannelDeleted(unsigned int)));
    connect(signalmanage, SIGNAL(signalChanged(int, int)),
	    this, SLOT(signalChanged(int, int)));

}

//****************************************************************************
void SignalWidget::forwardCommand(const char *command)
{
    debug("SignalWidget::forwardCommand(%s)", command);    // ###
    emit sigCommand(command);
}

//****************************************************************************
void SignalWidget::forwardChannelAdded(unsigned int channel)
{
    debug("SignalWidget::forwardChannelAdded(%u)", channel);
    emit sigChannelAdded(channel);
}

//****************************************************************************
void SignalWidget::forwardChannelDeleted(unsigned int channel)
{
    debug("SignalWidget::forwardChannelDeleted(%u)", channel);
    emit sigChannelDeleted(channel);
}

//****************************************************************************
int SignalWidget::getChannelCount()
{
    return (signalmanage) ? signalmanage->getChannelCount() : 0;
}

//****************************************************************************
int SignalWidget::getBitsPerSample()
{
    ASSERT(signalmanage);
    return (signalmanage) ? signalmanage->getBitsPerSample() : 0;
}

//****************************************************************************
void SignalWidget::createSignal(const char *str)
{
//    Parser parser (str);
//
//    int rate = parser.toInt();
//    double ms = parser.toDouble();
//
//    int numsamples = (int)(ms * rate / 1000);
//
//    if (signalmanage) delete signalmanage;
//    labels->clear ();
//
//    signalmanage = new SignalManager(numsamples, rate, 1);
//    if (signalmanage) {
//	connectSignal();
//	emit sigChannelAdded(0);;
//	zoomAll();
//    }
}

//****************************************************************************
void SignalWidget::estimateRange(int l, int r)
{
//    debug("SignalWidget::estimateRange(%d, %d)", l, r);
    emit selectedTimeInfo(samples2ms(r - l + 1));
}

//****************************************************************************
void SignalWidget::setRange(int l, int r, bool set)
{
    //    debug("SignalWidget::setRange(%d,%d,%d)",l,r,set);
    if (!signalmanage) return;

    ASSERT(select);
    ASSERT(zoom);
    if (set && select && zoom) {
	select->set(((l - offset) / zoom), ((r - offset) / zoom));
    }

    signalmanage->setRange(l, r);
    estimateRange(l, r);
}

//****************************************************************************
void SignalWidget::setSignal(SignalManager *sigs)
{
    closeSignal();
    if (!sigs) return ;

    signalmanage = sigs;
    offset = 0;
    if ((signalmanage) && (signalmanage->getLength())) {
	connectSignal();
	zoomAll();
	for (unsigned int i=0; i < signalmanage->getChannelCount(); i++)
	    emit sigChannelAdded(i);
    }
}

//****************************************************************************
void SignalWidget::setSignal(const char *filename, int type)
{
    closeSignal();
    offset = 0;
    ASSERT(labels);
    if (labels) labels->clear();
    if (!filename) return;

    // load a new signal
    signalmanage = new SignalManager(filename, type);
    ASSERT(signalmanage);
    if (!signalmanage) {
	warning("SignalWidget::setSignal() failed, out of memory?");
	return ;
    }

    if (signalmanage->getLength() <= 0) {
	warning("SignalWidget::setSignal() failed:"\
		" zero-length or out of memory?");
	delete signalmanage;
	signalmanage = 0;
	return ;
    }
    connectSignal();
    setRange(0, 0);
    zoomAll();
    for (unsigned int i=0; i < signalmanage->getChannelCount(); i++)
	emit sigChannelAdded(i);
}

//****************************************************************************
void SignalWidget::closeSignal()
{
    if (!signalmanage) return ;
    delete signalmanage;
    signalmanage = 0;

    down = false;

    setRange(0, 0);
    zoomAll();
    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::playback_time()
{
    debug("SignalWidget::playback_time()");
    ASSERT(signalmanage);
    ASSERT(zoom != 0.0);
    if (!signalmanage) return;
    if (zoom==0.0) return;

    bool needRepaint = false;
    playpointer = samples2pixels(signalmanage->getPlayPosition() - offset);
    if ((playpointer < width) && (playpointer >= 0)) {
	if (playpointer != lastplaypointer) {
	    lastplaypointer = playpointer;
	    needRepaint = true;
	}
    }

    if (signalmanage->getPlayPosition() == 0)
	emit playingfinished();

    if (needRepaint) {
	update_layer[LAYER_MARKERS] = true;
	repaint(false);
    }
}

//****************************************************************************
void SignalWidget::setOffset(int new_offset)
{
    offset = new_offset;
}

//****************************************************************************
double SignalWidget::getFullZoom()
{
    ASSERT(signalmanage);
    if (!signalmanage) return 0.0;    // no zoom if no signal

    // example: width = 100 and length=3
    //          -> samples should be at positions 0, 49.5 and 99
    //          -> 49.5 [pixels / sample]
    //          -> zoom = 1 / 49.5 [samples / pixel]
    // => full zoom [samples/pixel] = (length-1) / (width-1)
    return (double)(signalmanage->getLength() - 1) / (double)(width - 1);
}

//****************************************************************************
void SignalWidget::setZoom(double new_zoom)
{
    zoom = new_zoom;
}

//****************************************************************************
void SignalWidget::fixZoomAndOffset()
{
    double max_zoom;
    double min_zoom;
    int length;

    if (!signalmanage) return ;
    length = signalmanage->getLength();

    ASSERT(width);
    if (!width) return;

    // ensure that offset is [0...length-1]
    if (offset < 0) offset = 0;
    if (offset > length - 1) offset = length - 1;

    // ensure that the zoom is in a proper range
    max_zoom = getFullZoom();
    min_zoom = (double)MINIMUM_SAMPLES_PER_SCREEN / (double)width;
    if (zoom < min_zoom) zoom = min_zoom;
    if (zoom > max_zoom) zoom = max_zoom;

    // try to correct the offset if there is not enough data to fill
    // the current window
    // example: width=100 [pixel], length=3 [samples],
    //          offset=1 [sample], zoom=1/49.5 [samples/pixel] (full)
    //          -> current last displayed sample = length-offset
    //             = 3 - 1 = 2
    //          -> available space = pixels2samples(width-1) + 1
    //             = (99/49.5) + 1 = 3
    //          -> decrease offset by 3 - 2 = 1
    if ( (offset > 0) && (pixels2samples(width - 1) + 1 > length - offset)) {
	// there is space after the signal -> move offset left
	offset -= pixels2samples(width - 1) + 1 - (length - offset);
	if (offset < 0) offset = 0;
    }

    // if reducing the offset was not enough, zoom in
    if (pixels2samples(width - 1) + 1 > length - offset) {
	// there is still space after the signal -> zoom in
	// (this should never happen as the zoom has been limited before)
	zoom = max_zoom;
    }

    // adjust the zoom factor in order to make a whole number
    // of samples fit into the current window
    int samples = pixels2samples(width - 1) + 1;
    zoom = (double)(samples - 1) / (double)(width - 1);

    // do some final range checking
    if (zoom < min_zoom) zoom = min_zoom;
    if (zoom > max_zoom) zoom = max_zoom;

    emit zoomInfo(zoom);
}

//****************************************************************************
void SignalWidget::zoomAll()
{
    if (!signalmanage) return;
    setZoom(getFullZoom());
    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::zoomNormal()
{
    if (!signalmanage) return ;
    setOffset(offset + pixels2samples(width) / 2);
    setZoom(1.0);
    setOffset(offset - pixels2samples(width) / 2);
    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::zoomOut()
{
    setOffset(offset + pixels2samples(width) / 2);
    setZoom(zoom*3);
    setOffset(offset - pixels2samples(width) / 2);
    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::zoomIn()
{
    setOffset(offset + pixels2samples(width) / 2);
    setZoom(zoom / 3);
    setOffset(offset - pixels2samples(width) / 2);
    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::zoomRange()
{
    if (!signalmanage) return ;

    int lmarker = signalmanage->getLMarker();
    int rmarker = signalmanage->getRMarker();

    if (lmarker != rmarker) {
	setOffset(lmarker);
	setZoom(((double)(rmarker - lmarker)) / (double)(width - 1));
	refreshAllLayers();
    }
}

//****************************************************************************
void SignalWidget::refreshAllLayers()
{
    debug("SignalWidget::refreshAllLayers()");

    for (int i=0; i < 3; i++) {
	update_layer[i] = true;
    }

    fixZoomAndOffset();

    int rate = (signalmanage) ? signalmanage->getRate() : 0;
    int length = (signalmanage) ? signalmanage->getLength() : 0;

    if (select) {
	select->setOffset(offset);
	select->setLength(length);
	select->setZoom(zoom);
    }

    if (rate) emit timeInfo(samples2ms(length));
    if (rate) emit rateInfo(rate);

    int maxofs = pixels2samples(width - 1) + 1;
    emit viewInfo(offset, maxofs, length);
    emit lengthInfo(length);
    emit zoomInfo(zoom);

    redraw = true;
    repaint(false);
};

//****************************************************************************
void SignalWidget::refreshLayer(int layer)
{
    ASSERT(layer >= 0);
    ASSERT(layer < 3);
    if ((layer < 0) || (layer >= 3)) return;

    update_layer[layer] = true;

    redraw = true;
    repaint(false);
}

//****************************************************************************
void SignalWidget::signalChanged(int left, int right)
{
    debug("SignalWidget::signalChanged(%d,%d)", left, right);    // ###
    refreshLayer(LAYER_SIGNAL);
}

//****************************************************************************
void SignalWidget::slot_setOffset(int new_offset)
{
    if (new_offset != offset) {
	setOffset(new_offset);
	refreshAllLayers();
    }
}

//****************************************************************************
bool SignalWidget::checkPosition(int x)
//returns if given x coordinates is within bonds of area,
//in which labels may be reset, else false;
{
    ASSERT(select);
    if (!select) return false;

    //2 % of width tolerance
    return select->checkPosition(x, width / 50);
}

//****************************************************************************
void SignalWidget::mousePressEvent(QMouseEvent *e)
{
    ASSERT(e);
    ASSERT(select);
    if (!e) return;
    if (!select) return;

    // abort if no signal is loaded
    if (!signalmanage) return;

    if ( (!playing) && (e->button() == LeftButton) ) {
	int x = e->pos().x();
	down = true;
	(checkPosition(x)) ? select->grep(x) : select->set(x, x);
    }

    if (e->button() == RightButton) lasty = e->pos().y();
}

//****************************************************************************
void SignalWidget::mouseReleaseEvent(QMouseEvent *e)
{
    ASSERT(e);
    ASSERT(select);
    if (!e) return;
    if (!select) return;

    // abort if no signal is loaded
    if (!signalmanage) return;

    if (down) {
	int x = e->pos().x();
	if (x >= width) x = width - 1;    //check for some bounds
	if (x < 0) x = 0;
	select->update(x);
	setRange(select->getLeft(), select->getRight(), false);
	down = false;
    }
}

//****************************************************************************
void SignalWidget::mouseMoveEvent( QMouseEvent *e )
{
    ASSERT(e);
    ASSERT(select);
    if (!e) return;
    if (!select) return;

    // abort if no signal is loaded
    if (!signalmanage) return;

//    if ( (e->state() == RightButton) && height) {
//	//zooming on y axis... not very useful, will perhaps be replaced by
//	//more useful funcitonality...
//	//also very time consuming, because the hole viewable range of signal
//	//has to be redisplayed with every mousemove...
//	double old = zoomy;
//
//	zoomy += (double (e->pos().y() - lasty)) * 2 / height;
//
//	if (zoomy < 1) zoomy = 1;
//	if (zoomy > 10) zoomy = 10;
//
//	lasty = e->pos().y();
//
//	if (zoomy != old) {
//	    redraw = true;
//	    repaint();
//	}
//    }

    if (down) {
	// in move mode, a new selection was created or an old one grabbed
	// this does the changes with every mouse move...
	int x = e->pos().x();
	if (x >= width) x = width - 1;    //check for some bounds
	if (x < 0) x = 0;
	select->update(x);
	setRange(select->getLeft(), select->getRight(), false);
    } else
	//yes, this code gives the nifty cursor change....
	if (checkPosition (e->pos().x())) setCursor (sizeHorCursor);
	else setCursor (arrowCursor);

}

//****************************************************************************
void SignalWidget::drawOverviewSignal(int channel, int middle, int height,
				      int first, int last)
{
    debug("SignalWidget::drawOverviewSignal()");

    float scale_y;
    int step, max = 0, min = 0;

    ASSERT(signalmanage);
    if (!signalmanage) return;

    // scale_y: pixels per unit
    scale_y = height * zoomy / (1 << 24);

    for (int i = 0; i < width; i++) {
	step = offset + pixels2samples(i);
	signalmanage->getMaxMin(channel, max, min, step, pixels2samples(1));
	max = (int)(max * scale_y);
	min = (int)(min * scale_y);
	p.drawLine(i, middle - max, i, middle - min);
    }
}

//****************************************************************************
void SignalWidget::calculateInterpolation()
{
    float f;
    float Fg;
    int k;
    int N;

    debug("SignalWidget::calculateInterpolation()");

    // remove all previous coefficents and signal buffer
    if (interpolation_alpha != 0) {
	delete[] interpolation_alpha;
	interpolation_alpha = 0;
    }

    ASSERT(zoom != 0.0);
    if (zoom == 0.0) return;

    // offset: index of first visible sample (left) [0...length-1]
    // zoom: number of samples / pixel

    // approximate the 3dB frequency of the low pass as
    // Fg = f_g / f_a
    // f_a: current "sample rate" of display (pixels) = 1.0
    // f_g: signal rate = (zoom/2)
    Fg = zoom / 2;

    // N: order of the filter, at least 2 * (1/zoom)
    N = (int)(INTERPOLATION_PRECISION / zoom);
    N |= 0x01;    // make N an odd number !

    // allocate a buffer for the coefficients
    interpolation_alpha = new float[N + 1];
    interpolation_order = N;

    ASSERT(interpolation_alpha);
    if (!interpolation_alpha) return;

    // calculate the raw coefficients and
    // apply a Hamming window
    //
    //                    sin( (2k-N) * Pi * Fg )                       2kPi
    // alpha_k = 2 * Fg * ----------------------- * [ 0,54 - 0,46 * cos ---- ]
    //                      (2k - N) * Pi * Fg                            N
    //
    f = 0.0;    // (store the sum of all coefficients in "f")
    for (k = 0; k <= N; k++) {
	interpolation_alpha[k] = sin((2 * k - N) * PI * Fg) / ((2 * k - N) * PI * Fg);
	interpolation_alpha[k] *= (0.54 - 0.46 * cos(2 * k * PI / N));
	f += interpolation_alpha[k];
    }
    // norm the coefficients to 1.0 / zoom
    f *= zoom;
    for (k = 0; k <= N; k++)
	interpolation_alpha[k] /= f;

}

//****************************************************************************
void SignalWidget::drawInterpolatedSignal(int channel, int middle, int height)
{
    register float y;
    register float *sig;
    float *sig_buffer;
    float scale_y;
    int i;
    register int k;
    int N;
    int length;
    int sample;
    int x;

    debug("SignalWidget::drawInterpolatedSignal");

    ASSERT(signalmanage);
    if (!signalmanage) return;
    length = signalmanage->getLength();
    if (!length) return;

    // scale_y: pixels per unit
    scale_y = height * zoomy / (1 << 24);

    // N: order of the filter, at least 2 * (1/zoom)
    N = INTERPOLATION_PRECISION * samples2pixels(1);
    N |= 0x01;    // make N an odd number !

    // re-calculate the interpolation's filter and buffers
    // if the current order has changed
    if (interpolation_order != N) {
	calculateInterpolation();
	N = interpolation_order;
    }

    ASSERT(interpolation_alpha);
    if (!interpolation_alpha) return;

    // buffer for intermediate resampled data
    sig_buffer = new float[width + N + 2];
    ASSERT(sig_buffer);
    if (!sig_buffer) return;

    // array with sample points
    QPointArray *points = new QPointArray(width);
    ASSERT(points);
    if (!points) {
	delete[] sig_buffer;
	return;
    }

    // fill the sample buffer with zeroes
    for (i = 0; i < width + N + 2; i++)
	sig_buffer[i] = 0.0;

    // resample
    sample = -2;    // start some samples left of the window
    x = samples2pixels(sample);
    sig = sig_buffer + (N / 2);
    while (x <= width + N / 2) {
	if ((x >= -N / 2) && (offset + sample < length)) {
	    sig[x] = signalmanage->getSingleSample(channel, offset + sample) *
		     scale_y;
	}
	sample++;
	x = samples2pixels(sample);
    }

    // pass the signal data through the filter
    for (i = 0; i < width; i++) {
	sig = sig_buffer + (i + N);
	y = 0.0;
	for (k = 0; k <= N; k++)
	    y += *(sig--) * interpolation_alpha[k];

	points->setPoint(i, i, middle - (int)y);
    }

    // display the filter's interpolated output
    p.setPen(darkGray);
    p.drawPolyline(*points, 0, i);

    // display the original samples
    sample = 0;
    x = samples2pixels(sample);
    sig = sig_buffer + (N / 2);
    p.setPen(white);
    i = 0;
    while (x < width) {
	if ((x >= 0) && (x < width)) {
	    // mark original samples
	    points->setPoint(i++, x, middle - (int)sig[x]);
	}
	sample++;
	x = samples2pixels(sample);
    }
    p.drawPoints(*points, 0, i);

    delete[] sig_buffer;
    delete points;
}

//****************************************************************************
void SignalWidget::drawPolyLineSignal(int channel, int middle, int height)
{
    float scale_y;
    int y;
    int i;
    int n;
    int sample;
    int x;

    debug("SignalWidget::drawPolyLineSignal");

    ASSERT(signalmanage);
    if (!signalmanage) return;

    // scale_y: pixels per unit
    scale_y = height * zoomy / (1 << 24);

    // array with sample points
    QPointArray *points = new QPointArray(width + 1);
    ASSERT(points);
    if (!points) return;

    // display the original samples
    sample = 0;
    x = samples2pixels(sample);
    i = 0;
    while (x < width) {
	// mark original samples
	y = (int)(signalmanage->getSingleSample(channel, offset + sample) *
		  scale_y);
	points->setPoint(i++, x, middle - y);

	sample++;
	x = samples2pixels(sample);
    }

    // set "n" to the number of displayed original samples
    n = i;

    // interpolate the rest of the display if necessary
    if (samples2pixels(sample - 1) < width - 1) {
	int x1;
	int x2;
	float y1;
	float y2;

	x1 = samples2pixels(sample - 1);
	x2 = samples2pixels(sample);
	y1 = (int)(signalmanage->getSingleSample(channel, offset + sample - 1) *
		   scale_y);
	y2 = (int)(signalmanage->getSingleSample(channel, offset + sample) *
		   scale_y);

	x = width - 1;
	y = (int)((float)(x - x1) * (float)(y2 - y1) / (float)(x2 - x1));

	points->setPoint(i++, x, middle - y);
    }

    // show the poly-line
    p.setPen(darkGray);
    p.drawPolyline(*points, 0, i);

    // show the original points
    p.setPen(white);
    p.drawPoints(*points, 0, n);

    delete points;
}

int lastWidth = 0;
int lastHeight = 0;

////****************************************************************************
//void SignalWidget::paintEvent(QPaintEvent *event)
//{
//    debug("SignalWidget::paintEvent()");
//
//#ifdef DEBUG
//    struct timeval t_start;
//    struct timeval t_end;
//    double t_elapsed;
//    gettimeofday(&t_start,0);
//#endif
//
//    int update[2] = { -1, -1};
//    int updateall = false;
//    // if pixmap has to be resized ... or is not yet allocated ...
//    if ( (rect().height() != height) ||
//	 (rect().width() != width) ||
//	 (pixmap == 0) )
//    {
//	height = rect().height();
//	width = rect().width();
//
//	if (pixmap) delete pixmap;
//	pixmap = new QPixmap (size());
//	pixmap->fill (this, 0, 0);
//	updateall = true;
//    }
//    if (pixmap) { //final security check for the case of low memory (->rare thing)
//	p.begin(pixmap);
//	p.setPen (QPen(NoPen));
//
//	if (updateall || redraw) {
//	    if (redraw) {
//		p.fillRect(0, 0, width, height, black);
//		redraw = false;
//	    }
//
//	    p.setPen(white);
//	    p.setRasterOp(CopyROP);
//
//	    unsigned int channels = getChannelCount();
//	    if (channels) {
//		int chanheight = height / channels;
//		int begin = chanheight / 2;
//
//		//check and correct zoom and offset
//		fixZoomAndOffset();
//
//		for (unsigned int i = 0; i < channels; i++) {
//		    // skip non-existent signals
//		    ASSERT(signalmanage);
//		    if (!signalmanage) continue;
//		    if (!signalmanage->getSignal(i)) continue;
//
//		    if (zoom < 0.1) {
//			drawInterpolatedSignal(i, begin, chanheight);
//		    } else if (zoom <= 1.0)
//			drawPolyLineSignal(i, begin, chanheight);
//		    else
//			drawOverviewSignal(i, begin, chanheight,
//			                   0, zoom*width);
//
//		    p.setPen(green);
//		    p.drawLine(0, begin, width, begin);
//		    p.setPen(white);
//		    begin += chanheight;
//		}
//
//		// show selected range ...
//		select->drawSelection(&p, width, height);
//		updateall = true;
//	    }
//	    lastplaypointer = -1;
//
//	    // --- show the labels ---
//	    p.setRasterOp(CopyROP);
//	    Label *act;
//	    int lastpos = offset + pixels2samples(width);
//	    ASSERT(labels);
//	    for (act = labels->first(); act; act = labels->next()) {
//		int pos = ms2samples(act->pos);
//		if ((pos >= offset) && (pos < lastpos)) {
//		    int x = (int)((pos - offset) / zoom);
//		    // debug("%d %d %d %d\n",x,pos,offset,lastpos);
//		    p.setPen (*(act->getType()->color));
//		    p.drawLine (x, 0, x, height);
//
//		    if (act->getName()) {
//			int w = p.fontMetrics().width (act->getName());
//			int h = 8;
//			h = p.fontMetrics().height();
//
//			p.fillRect(x - w / 2 - 1, 1, w + 2, h + 2,
//			           QBrush(gray));
//			p.setPen(white);
//			p.drawLine(x - w / 2 - 2, 1,
//			           x + w / 2 + 1, 1);
//			p.drawLine(x - w / 2 - 2, 1,
//			           x - w / 2 - 2, 1 + h);
//			p.setPen(black);
//			p.drawLine(x + w / 2 + 1, 1,
//			           x + w / 2 + 1, 1 + h);
//			p.drawText(x - w / 2, 3, w, h,
//			           AlignCenter, act->getName());
//		    }
//		}
//	    }
//	} else {
//	    //no full repaint needed...
//	    //only the playing marker or the range labels gets updated
//	    if ( (down) && (!playing) ) {
//		p.setBrush (yellow);
//		p.setPen (yellow);
//		p.setRasterOp (XorROP);
//
//		updateall = true;
//	    }
//	}
//
//	if (playpointer >= 0) {
//	    p.setRasterOp (XorROP);
//	    p.setPen (green);
//
//	    if (lastplaypointer >= 0)
//		p.drawLine(lastplaypointer, -height / 2,
//		           lastplaypointer, height);
//
//	    p.drawLine (playpointer, -height / 2, playpointer, height);
//	    update[1] = lastplaypointer;
//	    lastplaypointer = playpointer;
//	    update[0] = playpointer;
//	    playpointer = -1;
//	} else if (lastplaypointer >= 0) {
//	    p.setRasterOp(XorROP);
//	    p.setPen(green);
//	    p.drawLine(lastplaypointer, -height / 2,
//	               lastplaypointer, height);
//	    update[0] = lastplaypointer;
//	    lastplaypointer = -1;
//	}
//
//	p.flush();
//	p.end();
//
//	ASSERT(event);
//	if (updateall || !event) {
//	    bitBlt(this, 0, 0, pixmap);
//	} else if (update[0] < 0) {
//	    QRect pos = event->rect();
//	    bitBlt (this, pos.topLeft(), pixmap, pos);
//	}
//
//	if (update[0] != -1)
//	    bitBlt (this, update[0], 0, pixmap, update[0], 0, 1, height);
//	if (update[1] != -1)
//	    bitBlt (this, update[1], 0, pixmap, update[1], 0, 1, height);
//    }
//
//#ifdef DEBUG
//    gettimeofday(&t_end,0);
//    t_elapsed = ((double)t_end.tv_sec*1.0E6+(double)t_end.tv_usec -
//	((double)t_start.tv_sec*1.0E6+(double)t_start.tv_usec)) * 1E-3;
//
//    debug("SignalWidget::paintEvent() -- done, t=%0.3fms --",
//	t_elapsed); // ###
//#endif
//}

//****************************************************************************
void SignalWidget::paintEvent(QPaintEvent *event)
{
    debug("SignalWidget::paintEvent()");

#ifdef DEBUG
    struct timeval t_start;
    struct timeval t_end;
    double t_elapsed;
    gettimeofday(&t_start,0);
#endif

    unsigned int channels = getChannelCount();

    layer_rop[LAYER_SIGNAL] = CopyROP;
    layer_rop[LAYER_SELECTION] = XorROP;
    layer_rop[LAYER_MARKERS] = XorROP;

    width = QWidget::width();
    height = QWidget::height();
    debug("SignalWidget::paintEvent(): width=%d, height=%d",width,height);

    // --- detect size changes and refresh the whole display ---
    if ((width != lastWidth) || (height != lastHeight)) {
	debug("SignalWidget::paintEvent(): window size changed");
	for (int i=0; i<3; i++) {
	    if (layer[i]) delete layer[i];
	    layer[i] = 0;
	    update_layer[i] = true;
	}
	if (!pixmap) delete pixmap;
	pixmap = 0;
	
	lastWidth = width;
	lastHeight = height;
    }

    // --- repaint of the signal layer ---
    if ( update_layer[LAYER_SIGNAL] || !layer[LAYER_SIGNAL]) {
	if (!layer[LAYER_SIGNAL])
	     layer[LAYER_SIGNAL] = new QPixmap(size());
	ASSERT(layer[LAYER_SIGNAL]);
	if (!layer[LAYER_SIGNAL]) return;
	
	debug("SignalWidget::paintEvent(): - redraw of signal layer -");
	p.begin(layer[LAYER_SIGNAL]);

	p.setPen(white);
	p.setRasterOp(CopyROP);
	p.fillRect(0, 0, width, height, black);

	//check and correct zoom and offset
	fixZoomAndOffset();

	int chanheight = (channels) ? (height / channels) : 0;
	int begin = chanheight / 2;

	for (unsigned int i = 0; i < channels; i++) {
	    // skip non-existent signals
	    ASSERT(signalmanage);
	    if (!signalmanage) continue;
	    ASSERT(signalmanage->getSignal(i));
	    if (!signalmanage->getSignal(i)) continue;

	    if (zoom < 0.1) {
		drawInterpolatedSignal(i, begin, chanheight);
	    } else if (zoom <= 1.0)
		drawPolyLineSignal(i, begin, chanheight);
	    else
		drawOverviewSignal(i, begin, chanheight,
		                   0, zoom*width);

	    p.setPen(green);
	    p.drawLine(0, begin, width, begin);
	    p.setPen(white);
	    begin += chanheight;
	}

	p.flush();
	p.end();

	update_layer[LAYER_SIGNAL] = false;
	
	if (pixmap) delete pixmap;
	pixmap = 0;
    }

    // --- repaint of the markers layer ---
    if ( update_layer[LAYER_MARKERS] || !layer[LAYER_MARKERS] ) {
	if (!layer[LAYER_MARKERS])
	     layer[LAYER_MARKERS] = new QPixmap(size());
	ASSERT(layer[LAYER_MARKERS]);
	if (!layer[LAYER_MARKERS]) return;

	debug("SignalWidget::paintEvent(): - redraw of markers layer -");
	p.begin(layer[LAYER_MARKERS]);
	p.fillRect(0, 0, width, height, black);
	p.setPen (green);
	if (playpointer >= 0) {
	    p.drawLine (playpointer, -height / 2, playpointer, height);
	}
	p.flush();
	p.end();
	
	update_layer[LAYER_MARKERS] = false;
	
	if (pixmap) delete pixmap;
	pixmap = 0;
    }

    // --- repaint of the selection layer ---
    if ( update_layer[LAYER_SELECTION] || !layer[LAYER_SELECTION] ) {
	if (!layer[LAYER_SELECTION])
	    layer[LAYER_SELECTION] = new QPixmap(size());
	ASSERT(layer[LAYER_SELECTION]);
	if (!layer[LAYER_SELECTION]) return;

	debug("SignalWidget::paintEvent(): - redraw of selection layer -");
	p.begin(layer[LAYER_SELECTION]);

	p.fillRect(0, 0, width, height, black);
	p.setBrush(yellow);
	p.setPen(yellow);
	p.setRasterOp(CopyROP);
	if (select) select->drawSelection(&p, width, height);

	p.flush();
	p.end();

	if (pixmap) delete pixmap;
	pixmap = 0;
    }

    // --- re-create the buffer pixmap if it has been deleted ---
    if (!pixmap) {
	pixmap = new QPixmap(size());
	ASSERT(pixmap);
	if (!pixmap) return;
    }

    // pixmap->fill (this, 0, 0);
    for (int i=0; i < 3; i++) {
	if (layer[i]) bitBlt(
	    pixmap, 0, 0,
	    layer[i], 0, 0,
	    width, height, layer_rop[i]
	);
    }
    bitBlt(this, 0, 0, pixmap, 0, 0, width, height, CopyROP);

#ifdef DEBUG
    gettimeofday(&t_end,0);
    t_elapsed = ((double)t_end.tv_sec*1.0E6+(double)t_end.tv_usec -
	((double)t_start.tv_sec*1.0E6+(double)t_start.tv_usec)) * 1E-3;

    debug("SignalWidget::paintEvent() -- done, t=%0.3fms --",
	t_elapsed); // ###
#endif

}


//below are the methods of class SignalWidget that deal with labels
#define AUTOKORRWIN 320
//windowsize for autocorellation, propably a little bit too short for
//lower frequencies, but this will get configurable somewhere in another
//dimension or for those of you who can't zap to other dimensions, it will
//be done in future

int findNextRepeat (int *, int);
int findNextRepeatOctave (int *, int, double = 1.005);
int findFirstMark (int *, int);

float autotable [AUTOKORRWIN];
float weighttable[AUTOKORRWIN];

//****************************************************************************
int SignalWidget::ms2samples(double ms)
{
    ASSERT(signalmanage);
    if (!signalmanage) return 0;

    return (int)(ms * signalmanage->getRate() / 1000.0);
}

//****************************************************************************
double SignalWidget::samples2ms(int samples)
{
    if (!signalmanage) return 0.0;
    return (double)samples*1000.0 / (double)signalmanage->getRate();
}

//****************************************************************************
int SignalWidget::pixels2samples(int pixels)
{
    return (int)(pixels*zoom);
}

//****************************************************************************
int SignalWidget::samples2pixels(int samples)
{
    ASSERT(zoom);
    if (zoom==0) return 0;
    return (int)(samples / zoom);
}

//****************************************************************************
void selectMarkers(const char *command)
{
//    Parser parser(command);
}

//****************************************************************************
LabelType *findMarkerType (const char *txt)
{
//    int cnt = 0;
//    LabelType *act;
//
//    for (act = globals.markertypes.first(); act; act = globals.markertypes.next()) {
//	if (strcmp (act->name, txt) == 0) return act;
//	cnt++;
//    }
//    warning("could not find Labeltype %s\n", txt);
    return 0;
}
//****************************************************************************
void SignalWidget::signalinserted (int start, int len)
{
//    Label *tmp;
//    for (tmp = labels->first(); tmp; tmp = labels->next())
//	if (tmp->pos > start) tmp->pos += len;
//    setRange (start, start + len);
//    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::signaldeleted (int start, int len)
{
//    Label *tmp;
//    for (tmp = labels->first(); tmp; tmp = labels->next()) {
//	if ((tmp->pos > start) && (tmp->pos < start + len)) //if Label position is within selected boundaries
//	{
//	    labels->remove ();
//	    tmp = labels->first();
//	}
//	if (tmp->pos >= start + len) tmp->pos -= len;     //if it is greater correct position
//    }
//
//    setRange (start, start);
//    refreshAllLayers();
}

//****************************************************************************
void SignalWidget::deleteLabel ()
{
//    if (!signalmanage) return ;
//
//    Label *tmp;
//    int l = signalmanage->getLMarker();
//    int r = signalmanage->getRMarker();
//
//    for (tmp = labels->first(); tmp; tmp = labels->next()) {
//	int pos = ms2samples(tmp->pos);
//	if ( (pos >= l) && (pos < r) ) {
//	    labels->remove(tmp);
//	    tmp = labels->first();
//	}
//    }
//    refreshLayer(LAYER_MARKERS);
}

//****************************************************************************
void SignalWidget::loadLabel()
{
//    labels->clear();    //remove old Label...
//    appendLabel ();
}

//****************************************************************************
void SignalWidget::appendLabel()
{
//    QString name = QFileDialog::getOpenFileName (0, "*.label", this);
//    //  if (!name.isNull())
//    //    {
//    //      char *comstr=catString ("loadbatch (",name,")");
//    //      globals.port->putMessage (comstr);
//    //    }
//    refresh ();
}

//****************************************************************************
void SignalWidget::saveLabel (const char *typestring)
{
//    QString name = QFileDialog::getSaveFileName (0, "*.label", this);
//    if (!name.isNull()) {
//	FILE *out;
//	out = fopen (name.data(), "w");
//
//	Parser parser (typestring);
//	Label *tmp;
//	LabelType *act;
//
//	const char *actstring = parser.getFirstParam();
//
//	while (actstring) {
//	    printf ("selecting %s\n", actstring);
//	    for (act = globals.markertypes.first(); act; act = globals.markertypes.next())
//		if (strcmp(act->name, actstring) == 0) {
//		    printf ("selected\n");
//		    act->selected = true;
//		    break;
//		}
//	    actstring = parser.getNextParam();
//	}
//
//	for (act = globals.markertypes.first(); act; act = globals.markertypes.next())
//	    //write out all selected label types
//	    if (act->selected)
//		fprintf (out, "%s\n", act->getCommand());
//
//	//ended writing of types, so go on with the labels...
//
//	for (tmp = labels->first(); tmp; tmp = labels->next())  //write out labels
//	{
//	    fprintf (out, tmp->getCommand());
//	    fprintf (out, "\n");
//	}
//
//	fclose (out);
//    }
}

//****************************************************************************
void SignalWidget::addLabel (const char *params)
{
//    if (signalmanage && markertype) {
//	Parser parser(params);
//	Label *newmark;
//
//	if (parser.countParams() > 0) {
//	    newmark = new Label (params);
//	} else {
//	    double pos = ((double)signalmanage->getLMarker()) * 1000 / signalmanage->getRate();
//	    newmark = new Label (pos, markertype);
//
//	    //should it need a name ?
//	    if (markertype->named) {
//		Dialog *dialog =
//		    DynamicLoader::getDialog ("stringenter", new DialogOperation("Enter name of label :", true));    //create a modal dialog
//
//		if (dialog) {
//		    dialog->show ();
//
//		    if (dialog->result()) {
//			printf ("dialog:%s\n", dialog->getCommand());
//			newmark->setName (dialog->getCommand());
//			delete dialog;
//		    } else {
//			delete newmark;
//			newmark = 0;
//		    }
//		} else {
//		    KMsgBox::message (this, "Error", i18n("Dialog not loaded !"));
//		    delete newmark;
//		    newmark = 0;
//		}
//	    }
//	}
//
//	if (newmark) {
//	    labels->inSort (newmark);
//
//	    refreshLayer(LAYER_MARKERS);
//	}
//    }
}

//****************************************************************************
void SignalWidget::jumptoLabel ()
// another fine function contributed by Gerhard Zintel
// if lmarker == rmarker (no range selected) cursor jumps to the nearest label
// if lmarker <  rmarker (range is selected) lmarker jumps to next lower label or zero
// rmarker jumps to next higher label or end
{
//    if (signalmanage) {
//	int lmarker = signalmanage->getLMarker();
//	int rmarker = signalmanage->getRMarker();
//	bool RangeSelected = (rmarker - lmarker) > 0;
//	if (labels) {
//	    Label *tmp;
//	    int position = 0;
//	    for (tmp = labels->first(); tmp; tmp = labels->next())
//		if (RangeSelected) {
//		    if (tmp->pos < lmarker)
//			if (abs(lmarker - position) >
//			    abs(lmarker - ms2samples(tmp->pos)))
//			    position = ms2samples(tmp->pos);
//		} else if (abs(lmarker - position) >
//			   abs(lmarker - ms2samples(tmp->pos)))
//		    position = ms2samples(tmp->pos);
//
//	    lmarker = position;
//	    position = signalmanage->getLength();
//	    for (tmp = labels->first(); tmp; tmp = labels->next())
//		if (tmp->pos > rmarker)
//		    if (abs(rmarker - position) >
//			abs(rmarker - ms2samples(tmp->pos)))
//			position = ms2samples(tmp->pos);
//	    rmarker = position;
//	    if (RangeSelected) setRange(lmarker, rmarker);
//	    else setRange (lmarker, lmarker);
//	    refresh ();
//	}
//    }
}

//****************************************************************************
void SignalWidget::savePeriods ()
{
//    if (signalmanage) {
//	Dialog *dialog =
//	    DynamicLoader::getDialog ("marksave", new DialogOperation(&globals, signalmanage->getRate(), 0, 0));
//
//	if ((dialog) && (dialog->exec())) {
//	    selectMarkers (dialog->getCommand());
//
//	    LabelType *act;
//	    Label *tmp;
//	    int last = 0;
//	    int rate = signalmanage->getRate ();
//
//	    QString name = QFileDialog::getSaveFileName (0, "*.dat", this);
//	    if (!name.isNull()) {
//		QFile out(name.data());
//		char buf[160];
//		float freq = 0, time, lastfreq = 0;
//		out.open (IO_WriteOnly);
//		int first = true;
//
//		for (act = globals.markertypes.first(); act; act = globals.markertypes.next())
//		    //write only selected label type
//		    if (act->selected)
//			//traverse list of all labels
//			for (tmp = labels->first(); tmp; tmp = labels->next()) {
//			    if (tmp->getType() == act) {
//				freq = tmp->pos - last;
//				time = last * 1000 / rate;
//
//				if ((!first) && (freq != lastfreq)) {
//				    lastfreq = freq;
//				    freq = 1 / (freq / rate);
//				    snprintf (buf, sizeof(buf), "%f %f\n", 
//					time, freq);
//				    out.writeBlock (&buf[0], strlen(buf));
//				} else lastfreq = freq;
//				first = false;
//				last = ms2samples(tmp->pos);
//			    }
//			}
//
//		if (!first) //make sure last tone gets its length
//		{
//		    time = last * 1000 / rate;
//		    snprintf (buf, sizeof(buf), "%f %f\n", time, freq);
//		    out.writeBlock (&buf[0], strlen(buf));
//		}
//
//		out.close ();
//	    }
//	}
//    }
}

//****************************************************************************
void SignalWidget::saveBlocks (int bit)
{
//    if (signalmanage) {
//	Dialog *dialog =
//	    DynamicLoader::getDialog("saveblock", new DialogOperation(&globals, signalmanage->getRate(), 0, 0));
//
//	if ((dialog) && (dialog->exec())) {
//	    Parser parser (dialog->getCommand());
//
//	    const char *filename = parser.getFirstParam();
//	    QDir *savedir = new QDir (parser.getNextParam());
//
//	    LabelType *start = findMarkerType(parser.getNextParam());
//	    LabelType *stop = findMarkerType (parser.getNextParam());
//
//	    Label *tmp;
//	    Label *tmp2;
//	    int count = 0;
//	    int l = signalmanage->getLMarker();    //save old marker positions...
//	    int r = signalmanage->getRMarker();    //
//
//	    for (tmp = labels->first(); tmp; tmp = labels->next())  //traverse list of labels
//	    {
//		if (tmp->getType() == start) {
//		    for (tmp2 = tmp; tmp2; tmp2 = labels->next())  //traverse rest of list to find next stop marker
//			if (tmp2->getType() == stop) {
//			    char buf[256];
//			    snprintf (buf, sizeof(buf), "%s%04d.wav", filename, count);
//			    //lets hope noone tries to save more than 10000 blocks...
//
//			    signalmanage->setRange (tmp->pos, tmp2->pos);    //changes don't have to be visible...
//			    filename = savedir->absFilePath(buf);
//			    signalmanage->save (filename, bit, true);     //save selected range...
//			    count++;
//			    break;
//			}
//		}
//	    }
//	    signalmanage->setRange (l, r);
//	}
//    }
}

//****************************************************************************
void SignalWidget::markSignal (const char *str)
{
//    if (signalmanage) {
//	Label *newmark;
//
//	Parser parser (str);
//
//	int level = (int) (parser.toDouble() / 100 * (1 << 23));
//
//	int len = signalmanage->getLength();
//	int *sam = signalmanage->getSignal(0)->getSample();    // ### @@@ ###
//	LabelType *start = findMarkerType(parser.getNextParam());
//	LabelType *stop = findMarkerType (parser.getNextParam());
//	int time = (int) (parser.toDouble () * signalmanage->getRate() / 1000);
//
//	printf ("%d %d\n", level, time);
//	printf ("%s %s\n", start->name, stop->name);
//
//	ProgressDialog *dialog =
//	    new ProgressDialog (len, "Searching for Signal portions...");
//
//	if (dialog && start && stop) {
//	    dialog->show();
//
//	    newmark = new Label(0, start);     //generate initial Label
//
//	    labels->inSort (newmark);
//
//	    for (int i = 0; i < len; i++) {
//		if (abs(sam[i]) < level) {
//		    int j = i;
//		    while ((i < len) && (abs(sam[i]) < level)) i++;
//
//		    if (i - j > time) {
//			//insert labels...
//			newmark = new Label(i, start);
//			labels->inSort (newmark);
//
//			if (start != stop) {
//			    newmark = new Label(j, stop);
//			    labels->inSort (newmark);
//			}
//		    }
//		}
//		dialog->setProgress (i);
//	    }
//
//	    newmark = new Label(len - 1, stop);
//	    labels->inSort (newmark);
//
//	    refresh ();
//	    delete dialog;
//	}
//    }
}

//****************************************************************************
void SignalWidget::markPeriods (const char *str)
{
//    if (signalmanage) {
//	Parser parser (str);
//
//	int high = signalmanage->getRate() / parser.toInt();
//	int low = signalmanage->getRate() / parser.toInt();
//	int octave = parser.toBool ("true");
//	double adjust = parser.toDouble ();
//
//	for (int i = 0; i < AUTOKORRWIN; i++)
//	    autotable[i] = 1 - (((double)i * i * i) / (AUTOKORRWIN * AUTOKORRWIN * AUTOKORRWIN));    //generate static weighting function
//
//	if (octave) for (int i = 0; i < AUTOKORRWIN; i++) weighttable[i] = 1;    //initialise moving weight table
//
//	Label *newmark;
//	int next;
//	int len = signalmanage->getLength();
//	int *sam = signalmanage->getSignal(0)->getSample();    // ### @@@ ###
//	LabelType *start = markertype;
//	int cnt = findFirstMark (sam, len);
//
//	ProgressDialog *dialog = new ProgressDialog (len - AUTOKORRWIN, "Correlating Signal to find Periods:");
//	if (dialog) {
//	    dialog->show();
//
//	    newmark = new Label(cnt, start);
//	    labels->inSort (newmark);
//
//	    while (cnt < len - 2*AUTOKORRWIN) {
//		if (octave)
//		    next = findNextRepeatOctave (&sam[cnt], high, adjust);
//		else
//		    next = findNextRepeat (&sam[cnt], high);
//
//		if ((next < low) && (next > high)) {
//		    newmark = new Label(cnt, start);
//
//		    labels->inSort (newmark);
//		}
//		if (next < AUTOKORRWIN) cnt += next;
//		else
//		    if (cnt < len - AUTOKORRWIN) {
//			int a = findFirstMark (&sam[cnt], len - cnt);
//			if (a > 0) cnt += a;
//			else cnt += high;
//		    } else cnt = len;
//
//		dialog->setProgress (cnt);
//	    }
//
//	    delete dialog;
//
//	    refresh ();
//	}
//    }
}

//*****************************************************************************
int findNextRepeat (int *sample, int high)
//autocorellation of a windowed part of the sample
//returns length of period, if found
{
//    int i, j;
//    double gmax = 0, max, c;
//    int maxpos = AUTOKORRWIN;
//    int down, up;         //flags
//
//    max = 0;
//    for (j = 0; j < AUTOKORRWIN; j++)
//	gmax += ((double)sample[j]) * sample [j];
//
//    //correlate signal with itself for finding maximum integral
//
//    down = 0;
//    up = 0;
//    i = high;
//    max = 0;
//    while (i < AUTOKORRWIN) {
//	c = 0;
//	for (j = 0; j < AUTOKORRWIN; j++) c += ((double)sample[j]) * sample [i + j];
//	c = c * autotable[i];    //multiply window with weight for preference of high frequencies
//	if (c > max) max = c, maxpos = i;
//	i++;
//    }
//    return maxpos;
    return 0;
}

//*****************************************************************************
int findNextRepeatOctave (int *sample, int high, double adjust = 1.005)
//autocorellation of a windowed part of the sample
//same as above only with an adaptive weighting to decrease fast period changes
{
//    int i, j;
//    double gmax = 0, max, c;
//    int maxpos = AUTOKORRWIN;
//    int down, up;         //flags
//
//    max = 0;
//    for (j = 0; j < AUTOKORRWIN; j++)
//	gmax += ((double)sample[j]) * sample [j];
//
//    //correlate signal with itself for finding maximum integral
//
//    down = 0;
//    up = 0;
//    i = high;
//    max = 0;
//    while (i < AUTOKORRWIN) {
//	c = 0;
//	for (j = 0; j < AUTOKORRWIN; j++) c += ((double)sample[j]) * sample [i + j];
//	c = c * autotable[i] * weighttable[i];
//	//multiply window with weight for preference of high frequencies
//	if (c > max) max = c, maxpos = i;
//	i++;
//    }
//
//    for (int i = 0; i < AUTOKORRWIN; i++) weighttable[i] /= adjust;
//
//    weighttable[maxpos] = 1;
//    weighttable[maxpos + 1] = .9;
//    weighttable[maxpos - 1] = .9;
//    weighttable[maxpos + 2] = .8;
//    weighttable[maxpos - 2] = .8;
//
//    float buf[7];
//
//    for (int i = 0; i < 7; buf[i++] = .1)
//
//	//low pass filter
//	for (int i = high; i < AUTOKORRWIN - 3; i++) {
//	    buf[i % 7] = weighttable[i + 3];
//	    weighttable[i] = (buf[0] + buf[1] + buf[2] + buf[3] + buf[4] + buf[5] + buf[6]) / 7;
//	}
//
//    return maxpos;
   return 0;
}

//*****************************************************************************
int findFirstMark (int *sample, int len)
//finds first sample that is non-zero, or one that preceeds a zero crossing
{
//    int i = 1;
//    int last = sample[0];
//    int act = last;
//    if ((last < 100) && (last > -100)) i = 0;
//    else
//	while (i < len) {
//	    act = sample[i];
//	    if ((act < 0) && (last >= 0)) break;
//	    if ((act > 0) && (last <= 0)) break;
//	    last = act;
//	    i++;
//	}
//    return i;
   return 0;
}

//*****************************************************************************
void SignalWidget::addLabelType (LabelType *marker)
{
//    globals.markertypes.append (marker);
//    if (manage) manage->addNumberedMenuEntry ("ID_LABELS_TYPE", (char *)marker->name);
}

//*****************************************************************************
void SignalWidget::addLabelType (const char *str)
{
//    LabelType *marker = new LabelType(str);
//    if (marker) addLabelType (marker);
}

/* end of src/SignalWidget.cpp */
