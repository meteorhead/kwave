#include <stdio.h>
#include <stdlib.h>
#include <qpushbutton.h>
#include <qkeycode.h>
#include <qcombobox.h>
#include <qtooltip.h>
#include "module.h"
#include <libkwave/String.h>
#include <libkwave/WindowFunction.h>
#include <kapp.h>

const char *version = "1.0";
const char *author = "Martin Wilz";
const char *name = "averagefft";

//**********************************************************
Dialog *getDialog(DialogOperation *operation) 
{
    return new AverageFFTDialog(operation->getRate(), operation->isModal());
}

//**********************************************************
AverageFFTDialog::AverageFFTDialog(int rate, bool modal)
    :Dialog(modal)
{
    WindowFunction w(0);
    this->rate = rate;
    resize (320, 200);
    setCaption (i18n("Choose window type and length :"));

    pointlabel = new QLabel (i18n("Length of window :"), this);
    windowlength = new TimeLine (this);
    windowlength->setMs (100);

    windowtypebox = new QComboBox (true, this);
    windowtypebox->insertStrList (w.getTypes(), w.getCount());
    QToolTip::add(windowtypebox, i18n("Choose windowing function here."));

    windowtypelabel = new QLabel (i18n("Window Function :"), this);

    ok = new QPushButton (OK, this);
    cancel = new QPushButton (CANCEL, this);

    int bsize = ok->sizeHint().height();

    setMinimumSize (320, bsize*8);
    resize (320, bsize*3);

    ok->setAccel (Key_Return);
    cancel->setAccel(Key_Escape);
    ok->setFocus ();
    connect (ok , SIGNAL(clicked()), SLOT (accept()));
    connect (cancel , SIGNAL(clicked()), SLOT (reject()));
}

//**********************************************************
const char *AverageFFTDialog::getCommand() 
{
    char buf[512];
    deleteString (comstr);
    snprintf(buf, sizeof(buf), "%f", windowlength->getMs());

    comstr = catString ("averagefft (",
			buf,
			",",
			windowtypebox->currentText(),
			")");
    return comstr;
}

//**********************************************************
void AverageFFTDialog::resizeEvent(QResizeEvent *) 
{
    int bsize = ok->sizeHint().height();

    pointlabel->setGeometry (8, bsize / 2, width() / 2 - 8, bsize);
    windowlength->setGeometry(width() / 2, bsize / 2, width()*3 / 10, bsize);

    windowtypelabel->setGeometry(8, bsize*3 / 2 + 8, width() / 2 - 8, bsize);
    windowtypebox->setGeometry(width() / 2, bsize*3 / 2 + 8, 
                               width() / 2 - 8, bsize);

    ok->setGeometry(width() / 10, height() - bsize*3 / 2, 
                    width()*3 / 10, bsize);
    cancel->setGeometry(width()*6 / 10, height() - bsize*3 / 2, 
                        width()*3 / 10, bsize);
}

//**********************************************************
AverageFFTDialog::~AverageFFTDialog()
{
    deleteString (comstr);
}

//**********************************************************
