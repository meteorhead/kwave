#include "fftview.h"
#include <qpainter.h>
#include <math.h>
#include <limits.h>
#include "faderwidget.h"

//****************************************************************************
FaderWidget::FaderWidget (QWidget *parent,int dir) : QWidget
(parent)
{
  this->dir=dir;
  height=-1;
  curve=0;
  this->setBackgroundColor (QColor(black));    
}
//****************************************************************************
FaderWidget::~FaderWidget (QWidget *parent,const char *name)
{
}
//****************************************************************************
void FaderWidget::setCurve (int c)
{
  curve=c*c*c;
  repaint ();
}
//****************************************************************************
int FaderWidget::getCurve ()
{
  return curve;
}
//****************************************************************************
void FaderWidget::paintEvent  (QPaintEvent *)
{
  height=rect().height();
  width=rect().width();

  p.begin (this);
  p.setPen (white);

  int lx=(dir==1)?0:width;
  int ly=height;

  if (curve==0)
    {
      for (int i=1;i<width;i++)
	{
	  if (dir==1)
	    {
	      p.drawLine (lx,ly,i,(width-i)*height/width);
	      lx=i;
	    }
	  else
	    {
	      p.drawLine (lx,ly,width-i,(width-i)*height/width);
	      lx=width-i;
	    }

	  ly=(width-i)*height/width;
	}
    }
  else
    if (curve<0)
      for (int i=1;i<width;i++)
	{
	  int y=height-(int)(log10(1+(-curve*((double)i)/width))*height/log10(1-curve));
	  if (dir==1)
	    {
	      p.drawLine (lx,ly,i,y);
	      lx=i;
	    }
	  else
	    {
	      p.drawLine (lx,ly,width-i,y);
	      lx=width-i;
	    }

	  ly=y;
	}
    else
      for (int i=1;i<width;i++)
	{
	  int y=(int)(log10(1+(curve*((double)(width-i))/width))*height/log10(1+curve));

	  if (dir==1)
	    {
	      p.drawLine (lx,ly,i,y);
	      lx=i;
	    }
	  else
	    {
	      p.drawLine (lx,ly,width-i,y);
	  
	      lx=width-i;
	    }

	  ly=y;
	}

  p.end();
}

