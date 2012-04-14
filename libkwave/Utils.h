/***************************************************************************
                Utils.h  -  some commonly used utility functions
                             -------------------
    begin                : Sun Mar 27 2011
    copyright            : (C) 2011 by Thomas Eschenbacher
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

#ifndef _KWAVE_UTILS_H_
#define _KWAVE_UTILS_H_

#include "config.h"

#include <QString>

#include "kdemacros.h"

namespace Kwave {

    /**
     * Gives the control to the next thread. This can be called from
     * within the run() function.
     */
    void KDE_EXPORT yield();

    /**
     * Converts a zoom factor into a string. The number of decimals
     * is automatically adjusted in order to give a nice formated
     * percent value. If the zoom factor gets too high for a reasonable
     * display in percent, the factor is displayed as a numeric
     * multiplier.
     * examples: "0.1 %", "12.3 %", "468 %", "11x"
     * @param percent the zoom factor to be formated, a value of "100.0"
     *             means "100%", "0.1" means "0.1%" and so on.
     */
    QString KDE_EXPORT zoom2string(double percent);

    /**
     * Converts a time in milliseconds into a string. Times below one
     * millisecond are formated with an automatically adjusted number
     * of decimals. Times below one second are formated like "9.9 ms".
     * Times above one second and below one minute are rounded up
     * to full seconds and shown as "12.3 s". From one full minute
     * upwards time is shown as "12:34" (like most CD players do).
     * @param ms time in milliseconds
     * @param precision number of digits after the comma, for limiting
     *                  the length. optional, default = 6 digits,
     *                  must be >= 3 !
     * @return time formatted as user-readable string
     */
    QString KDE_EXPORT ms2string(double ms, int precision = 6);

    /**
     * Converts a time in milliseconds into a string with hours,
     * minutes, seconds and milliseconds.
     * @param ms time in milliseconds
     * @return time formatted as HH:MM:SS:mmmm
     */
    QString KDE_EXPORT ms2hms(double ms);
    
    /**
     * Converts the given number into a string with the current locale's
     * separator between the thousands.
     * @param number the unsigned number to be converted
     * @return QString with the number
     */
    QString KDE_EXPORT dottedNumber(unsigned int number);

}

#endif /* _KWAVE_UTILS_H_ */