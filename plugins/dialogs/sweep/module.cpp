#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <qcombobox.h>
#include <qpushbutton.h>
#include <qkeycode.h>
#include <qlabel.h>

#include <kintegerline.h>
#include <kfiledialog.h>

#include "libkwave/DialogOperation.h"
#include "libkwave/Curve.h"
#include "libkwave/PointSet.h"

#include "libgui/CurveWidget.h"
#include "libgui/TimeLine.h"

#include "module.h"


const char *version = "1.0";
const char *author = "Martin Wilz";
const char *name = "sweep";

//**********************************************************
Dialog *getDialog (DialogOperation *operation) 
{
    return new SweepDialog(operation->getRate(), operation->isModal());
}

//**********************************************************
static const char *notetext[] = {
    "C 0", "C# 0", "D 0", "D# 0", "E 0", "F 0", "F# 0", "G 0", "G# 0", "A 0", "A# 0", "B 0",
    "C 1", "C# 1", "D 1", "D# 1", "E 1", "F 1", "F# 1", "G 1", "G# 1", "A 1", "A# 1", "B 1",
    "C 2", "C# 2", "D 2", "D# 2", "E 2", "F 2", "F# 2", "G 2", "G# 2", "A 2", "A# 2", "B 2",
    "C 3", "C# 3", "D 3", "D# 3", "E 3", "F 3", "F# 3", "G 3", "G# 3", "A 3", "A# 3", "B 3",
    "C 4", "C# 4", "D 4", "D# 4", "E 4", "F 4", "F# 4", "G 4", "G# 4", "A 4", "A# 4", "B 4",
    "C 5", "C# 5", "D 5", "D# 5", "E 5", "F 5", "F# 5", "G 5", "G# 5", "A 5", "A# 5", "B 5",
    "C 6", "C# 6", "D 6", "D# 6", "E 6", "F 6", "F# 6", "G 6", "G# 6", "A 6", "A# 6", "B 6",
    "C 7", "C# 7", "D 7", "D# 7", "E 7", "F 7", "F# 7", "G 7", "G# 7", "A 7", "A# 7", "B 7",
    "C 8", "C# 8", "D 8", "D# 8", "E 8", "F 8", "F# 8", "G 8", "G# 8", "A 8", "A# 8", "B 8",
    0
};

float notefreq[] =    //frequency in Hz for the above named notes...
    {
	16.4, 17.3, 18.4, 19.4, 20.6, 21.8, 23.1, 24.5, 26.0, 27.5, 29.1, 30.9,
	32.7, 34.6, 36.7, 38.9, 41.2, 43.7, 46.2, 49.0, 51.9, 55.0, 58.3, 61.7,
	65.4, 69.3, 73.4, 77.8, 82.4, 87.3, 92.5, 98.0, 103.8, 110, 116.5, 123.5,
	130.8, 138.6, 146.8, 155.6, 164.8, 174.6, 185, 196, 207.7, 220, 233.1, 246.9,
	261.4, 277.2, 293.7, 311.1, 329.6, 349.2, 370, 392, 415.3, 440, 466.2, 493.9,
	523.3, 554.4, 587.3, 622.3, 659.3, 698.5, 740, 784, 830.6, 880, 932.3, 987.8,
	1046.5, 1108.7, 1174.7, 1244.5, 1318.5, 1369.9, 1480, 1568, 1661.2, 1760, 1864.7, 1975.5,
	2093, 2217.5, 2349.3, 2489, 2637, 2793.8, 2960, 3136, 3322.4, 3520, 3729.3, 3951,
	4186, 4435, 4699.6, 4978, 5274, 5587.5, 5920, 6272, 6644.8, 7040, 7458.6, 7902
    };

//**********************************************************
SweepDialog::SweepDialog (int rate, bool modal)
    :Dialog(modal) 
{
    command = 0;
    setCaption ("Select Sweep Function:");
    this->rate = rate;

    curve = new CurveWidget (this);

    lowfreq = new KIntegerLine (this);
    highfreq = new KIntegerLine (this);

    note1 = new QComboBox (true, this);
    note1->insertStrList (notetext, -1);
    note2 = new QComboBox (true, this);
    note2->insertStrList (notetext, -1);
    note1->setCurrentItem (12);
    note2->setCurrentItem (64);
    showLowFreq (12);
    showHighFreq (64);

    notelabel1 = new QLabel (i18n("From"), this);
    notelabel2 = new QLabel (i18n("to"), this);

    showTime (100);

    load = new QPushButton (i18n("Import"), this);
    ok = new QPushButton (i18n("Ok"), this);
    cancel = new QPushButton (i18n("Cancel"), this);
    int bsize = load->sizeHint().height();

    setMinimumSize (320, bsize*9);
    resize (320, bsize*9);

    connect (load, SIGNAL(clicked()), SLOT (import()));
    connect (note1, SIGNAL(activated(int)), SLOT (showLowFreq(int)));
    connect (note2, SIGNAL(activated(int)), SLOT (showHighFreq(int)));
}

//**********************************************************
const char* SweepDialog::getCommand() 
{
    deleteString (command);
    command = catString ("sweep(",
			 lowfreq->text(),
			 "",
			 highfreq->text(),
			 "",
			 curve->getCommand(),
			 ")");
    return command;
}

//**********************************************************
void SweepDialog::convert(Curve *times)
//converts file format list to standard list with points with  range [0-1] for
//frequency (y) and time (x)
{
    double time = times->last()->x;
    double max = 0;
    double min = 20000;

    //get maximum and minim...
    for (Point *tmp = times->first(); tmp; tmp = times->next(tmp)) {
	if (tmp->y > max) max = ceil(tmp->y);
	if (tmp->y < min) min = floor(tmp->y);
    }

    //rescale points for use with curvewidget...
    for (Point *tmp = times->first(); tmp; tmp = times->next(tmp)) {
	tmp->x /= time;
	tmp->y = (tmp->y - min) / (max - min);
    }

    curve->setCurve (times->getCommand());

    lowfreq->setValue (min);
    highfreq->setValue (max);
}

//**********************************************************
void SweepDialog::import()
//reads file as generated by label->savefrequency into list
{
    QString name = KFileDialog::getOpenFileName (0, "*.dat", this);
    if (!name.isNull()) {
	Curve *times = new Curve();
	QFile in(name.data());
	if (times && in.exists()) {
	    if (in.open(IO_ReadWrite)) {
		char buf[80];
		float time;
		float freq;

		while (in.readLine(&buf[0], 80) > 0) {
		    if ((buf[0] != '#') && (buf[0] != '/'))
			//check for commented lines
		    {
			char *p;
			sscanf (buf, "%e", &time);
			p = strchr (buf, '\t');
			if (p == 0) p = strchr (buf, ' ');
			sscanf (p, "%e", &freq);

			times->append ((double)time, (double)freq);
		    }
		}
		convert (times);
		delete times;
	    } else printf (i18n("could not open file !\n"));
	} else printf (i18n("file %s does not exist!\n"), name.data());
    }
}

//**********************************************************
void SweepDialog::showLowFreq(int val) 
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%4.2f Hz", notefreq[val]);
    lowfreq->setText (buf);
}

//**********************************************************
void SweepDialog::showHighFreq(int val) 
{
    char buf[64];
    snprintf(buf, sizeof(buf), "%4.2f Hz", notefreq[val]);
    highfreq->setText (buf);
}

//**********************************************************
double SweepDialog::getLowFreq() 
{
    QString tmp(lowfreq->text ());
    return (tmp.toDouble());
}

//**********************************************************
double SweepDialog::getHighFreq() 
{
    QString tmp(highfreq->text ());
    return (tmp.toDouble());
}

//**********************************************************
void SweepDialog::showTime(int) 
{
}

//**********************************************************
void SweepDialog::resizeEvent(QResizeEvent *) 
{
    int bsize = load->sizeHint().height();
    int offset = height() - bsize * 3 - bsize / 2 - 8;
    curve->setGeometry(8, 8, width() - 16, offset - 8);
    offset += bsize / 4;
    notelabel1->setGeometry(8, offset + bsize / 2, width()*3 / 20, bsize);
    notelabel2->setGeometry(width()*11 / 20, offset + bsize / 2, 
	                    width()*3 / 20, bsize);
    note1->setGeometry(width()*4 / 20, offset, width()*5 / 20, bsize);
    note2->setGeometry(width()*14 / 20, offset, width()*5 / 20, bsize);
    lowfreq->setGeometry(width()*4 / 20, offset + bsize, 
                         width()*5 / 20, bsize);
    highfreq->setGeometry(width()*14 / 20, offset + bsize, 
                          width()*5 / 20, bsize);
    offset += bsize * 2 + bsize / 4;
    ok->setGeometry(width()*1 / 20, offset, 
                    width()*5 / 20, bsize);
    load->setGeometry(width()*15 / 40, offset, width()*5 / 20, bsize);
    cancel->setGeometry (width()*14 / 20, offset, width()*5 / 20, bsize);
}

//**********************************************************
SweepDialog::~SweepDialog() 
{
    deleteString (command);
}
