/*************************************************************************
     RecordController.h  -  controller/state maching for the audio recorder
                             -------------------
    begin                : Sat Oct 04 2003
    copyright            : (C) 2003 by Thomas Eschenbacher
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

#ifndef _RECORD_CONTROLLER_H_
#define _RECORD_CONTROLLER_H_

#include "config.h"
#include <qobject.h>
#include <qvaluestack.h>

#include "RecordState.h"

class RecordController: public QObject
{
    Q_OBJECT
public:

    /** Constructor */
    RecordController();

    /** Destructor */
    virtual ~RecordController();

public slots:

    /** Clear all recorded data and prepare for new recording */
    void actionReset();

    /** Stop the recording */
    void actionStop();

    /** Pause the recording */
    void actionPause();

    /** Start the recording */
    void actionStart();

    /** The device has started recording */
    void deviceRecordStarted();

    /** The device buffer contains data */
    void deviceBufferFull();

    /** The record trigger has been reached */
    void deviceTriggerReached();

    /** The device has stopped recording */
    void deviceRecordStopped(int);

    /** The recording trigger has been enabled/disabled */
    void enableTrigger(bool enable);

signals:

    /** emitted when the state of the recording changed */
    void stateChanged(RecordState state);

    /** All recorded data has to be cleared */
    void sigReset(bool &accepted);

    /** Recording should start */
    void sigStartRecord();

    /** Recording should stop */
    void sigStopRecord(int errorcode);

private:

    /** current state of the recording engine */
    RecordState m_state;

    /**
     * state of the recording engine after finishing the last action, not
     * needed for all state changes.
     */
    RecordState m_next_state;


    /** use a trigger */
    bool m_trigger_set;

    /** use prerecording */
    bool m_use_prerecording;

};

#endif /* _RECORD_CONTROLLER_H_ */
