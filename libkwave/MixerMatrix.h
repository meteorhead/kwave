/***************************************************************************
          MixerMatrix.h  -  channel mixer matrix
			     -------------------
    begin                : 2012-05-11
    copyright            : (C) 2012 by Thomas Eschenbacher
    email                : Thomas Eschenbacher <Thomas.Eschenbacher@gmx.de>

 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _MIXER_MATRIX_H_
#define _MIXER_MATRIX_H_

#include "config.h"

#include <kdemacros.h>

#include "libkwave/Matrix.h"

namespace Kwave
{

    class KDE_EXPORT MixerMatrix: public Kwave::Matrix<double>
    {
    public:
	/**
	 * Constructor
	 * @param inputs number of inputs
	 * @param outputs number of outputs
	 */
	MixerMatrix(unsigned int inputs, unsigned int outputs);

	/** Destructor */
	virtual ~MixerMatrix();
    };

}

#endif /* _MIXER_MATRIX_H_ */

//***************************************************************************
//***************************************************************************
