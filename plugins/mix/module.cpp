#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <qpushbutton.h>
#include <qkeycode.h>
#include "module.h"
#include <kapp.h>

const char *version = "1.0";
const char *author = "Martin Wilz";
const char *name = "mix";

//**********************************************************
Dialog *getDialog (DialogOperation *operation) 
{
    return new ChannelMixDialog(operation->isModal(), 
                                operation->getChannels());
}

//**********************************************************
ChannelMixDialog::ChannelMixDialog(bool modal, int channels)
    :Dialog(modal) 
{
    dbmode = true;
    tflag = false;

    ok = new QPushButton (OK, this);
    cancel = new QPushButton (CANCEL, this);

    int bsize = ok->sizeHint().height();

    this->channels = channels;
    channelname = new QLabel * [channels];
    value = new double[channels];
    valuebox = new FloatLine * [channels];
    slider = new Slider * [channels];

    tochannellabel = new QLabel (i18n("mix to channel :"), this);
    tochannel = new QComboBox (this);
    usedb = new QCheckBox (i18n ("use dB scale"), this);
    usedb->setChecked (true);
    tochannel->insertItem (i18n("new"));

    if (slider && channelname && value && valuebox)
	for (int i = 0; i < channels; i++) {
	    value[i] = 0;
	    char buf[128];
	    snprintf(buf, sizeof(buf), i18n("Channel %d :"), i + 1);
	    channelname[i] = new QLabel (buf, this);
	    slider[i] = new Slider(0, 25 * 60, 1, 0, 
	                           Slider::Horizontal, this);
	    valuebox[i] = new FloatLine (this);
	    connect(valuebox[i], SIGNAL(textChanged(const char *)), 
	           this, SLOT(setValue(const char *)));

	    connect(usedb, SIGNAL(toggled(bool)), 
	            this, SLOT(setdBMode(bool)));
	    connect(slider[i], SIGNAL(valueChanged(int)), 
	            this, SLOT(setValue(int)));
	    snprintf(buf, sizeof(buf), "%d", i + 1);
	    tochannel->insertItem (buf);
	}

    setValue (0);

    setMinimumSize (bsize*8, (bsize + 8)*(4 + channels));
    resize (bsize*16, (bsize + 8)*(4 + channels));

    ok->setAccel (Key_Return);
    cancel->setAccel(Key_Escape);
    ok->setFocus ();
    connect (ok , SIGNAL(clicked()), SLOT (accept()));
    connect (cancel, SIGNAL(clicked()), SLOT (reject()));
}

//**********************************************************
void ChannelMixDialog::resizeEvent(QResizeEvent *) 
{
    int bsize = ok->sizeHint().height();
    int boxsize = bsize * 7 / 2;
    int y = 8;

    if (slider && channelname && valuebox) {
	int labelsize = channelname[channels - 1]->sizeHint().width();

	for (int i = 0; i < channels; i++) {
	    channelname[i]->setGeometry (8, y, labelsize, bsize);
	    slider[i]->setGeometry(labelsize + 16, y, 
	                           width() - boxsize - labelsize - 32, bsize);
	    valuebox[i]->setGeometry(width() - boxsize - 8, y, boxsize, bsize);
	    y += bsize + 8;
	}
	tochannellabel->setGeometry(8, y, width()*8 / 10-16, bsize);
	tochannel->setGeometry(8 + width()*8 / 10, y, 
	                       width()*2 / 10-16, bsize);
	y += bsize + 8;
	usedb->setGeometry(8, y, width() - 16, bsize);
    }
    ok->setGeometry(width() / 10, height() - bsize*3 / 2, 
                    width()*3 / 10, bsize);
    cancel->setGeometry(width()*6 / 10, height() - bsize*3 / 2, 
                        width()*3 / 10, bsize);
}

//**********************************************************
void ChannelMixDialog::setValue(int val) 
{
    if (!tflag)
	for (int i = 0; i < channels; i++)
	    if (slider[i]->value() == val) {
		char buf[64];

		if (dbmode) {
		    if (val == 0) strncpy (buf, "-Inf dB", sizeof(buf));
		    else
			snprintf(buf, sizeof(buf), "%.1f dB", 
			    ((double)val - (24*60)) / 10);
		} else snprintf(buf, sizeof(buf), "%.1f %%", 
		                ((double)val) / 10);

		valuebox[i]->setText (buf);
	    }
}

//**********************************************************
void ChannelMixDialog::setValue(const char* text) 
{
    tflag = true;
    for (int i = 0; i < channels; i++)
	if (strcmp(valuebox[i]->text(), text) == 0) {
	    double val = valuebox[i]->value();

	    if (dbmode) slider[i]->setValue ((val + 24*6)*10);
	    else slider[i]->setValue ((int)(val*10));
	}
    tflag = false;
}

//**********************************************************
void ChannelMixDialog::setdBMode(bool val) 
{
    dbmode = val;

    if (dbmode)
	for (int i = 0; i < channels; i++) {
	    double per = (slider[i]->value() / 10);
	    double db = ( -6 * (1 / log10(2)) * log10(per));
	    //convert values...
	    slider[i]->setRange(0, 25*60);
	    slider[i]->setValue((db + 24*6)*10);
	}
    else
	for (int i = 0; i < channels; i++) {
	    double db = -(slider[i]->value() - (24 * 60)) / 10;
	    int x = (int)((1 / pow(2, -db / 6)) * 1000);
	    slider[i]->setRange(0, 2000);
	    slider[i]->setValue(x);
	}
}

//**********************************************************
const char* ChannelMixDialog::getCommand() 
{
    deleteString (comstr);
    //  for (int i=0;i<channels;i++)
    //    {
    //  double db=-(slider[i]->value()-(24*60))/10;
    //  value[i]=((1/pow(2,-db/6)));
    //}
    return comstr;
}

//**********************************************************
ChannelMixDialog::~ChannelMixDialog() 
{
    if (slider) delete slider;
    if (channelname) delete channelname;
    if (value) delete value;
    if (valuebox) delete valuebox;
    deleteString (comstr);
}
