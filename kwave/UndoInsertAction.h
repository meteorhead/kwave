/***************************************************************************
     UndoInsertAction.h  -  UndoAction for insertion of a range of samples
			     -------------------
    begin                : Jun 14 2001
    copyright            : (C) 2001 by Thomas Eschenbacher
    email                : Thomas Eschenbacher <thomas.eschenbacher@gmx.de>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _UNDO_INSERT_ACTION_H_
#define _UNDO_INSERT_ACTION_H_

#include <qstring.h>
#include "UndoAction.h"

class SignalManager;

class UndoInsertAction: public UndoAction
{
public:

    /**
     * Constructor.
     * @param track index of the track
     * @param offset index of the first inserted sample
     * @param length number of inserted samples
     */
    UndoInsertAction(unsigned int track, unsigned int offset,
                     unsigned int length);

    /** @see UndoAction::description() */
    virtual QString description();

    /** @see UndoAction::undoSize() */
    virtual unsigned int undoSize();

    /** @see UndoAction::redoSize() */
    virtual int redoSize();

    /**
     * @see UndoAction::store()
     */
    virtual void store(SignalManager &manager);

    /**
     * Removes samples from the track.
     * @see UndoAction::undo()
     */
    virtual UndoAction *undo(SignalManager &manager, bool with_redo);

protected:

    /** index of the modified track */
    unsigned int m_track;

    /** first sample */
    unsigned int m_offset;

    /** number of samples */
    unsigned int m_length;

};

#endif /* _UNDO_INSERT_ACTION_H_ */
