#ifndef _SCALE_H_
#define _SCALE_H_ 1

#include <stdlib.h>
#include <qapp.h>
#include <qwidget.h>
#include <qpainter.h>

class ScaleWidget : public QWidget
{
 Q_OBJECT

 public:

 	ScaleWidget 	(QWidget *parent=0,int=0,int=100,char *unittext="%");
 	~ScaleWidget 	();
 void   paintText       (QPainter *,int, int,int,int,char *);
 void   setMaxMin       (int,int);
 void   setUnit         (char *);	
 void   setLogMode      (bool);	
 void   drawLinear      (QPainter *,int,int);	
 void   drawLog         (QPainter *,int,int);	

 signals:

 public slots:

 protected:

 void   paintEvent(QPaintEvent *); 

 private:

 int  low,high;    //range of display
 bool logmode;     //conditional: logarithmic mode or not
 char *unittext;   //string containing the name of the unit
};
//*****************************************************
class CornerPatchWidget : public QWidget
{
 Q_OBJECT

 public:

 	CornerPatchWidget 	(QWidget *parent=0,int=0);
 	~CornerPatchWidget 	();

 signals:

 public slots:

 protected:

 void   paintEvent(QPaintEvent *); 

 private:

 int  pos;         //
};


#endif  /* Scale.h */   


