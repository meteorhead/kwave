/***************************************************************************
    libkwave/WorkerThread.h  -  worker thread for Kwave
                             -------------------
    begin                : Sun Apr 06 2008
    copyright            : (C) 2008 by Thomas Eschenbacher
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

#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "config.h"
#include <pthread.h>

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QVariant>

#include <kdemacros.h>

namespace Kwave
{
    class Runnable;

    class KDE_EXPORT WorkerThread : public QThread
    {
	Q_OBJECT
    public:

	/** Constructor */
	explicit WorkerThread(Kwave::Runnable *runnable, QVariant params);

	/** Destructor, calls stop() if the thread is still running. */
	virtual ~WorkerThread();

	/** Starts the thread's execution. */
	virtual void start();

	/**
	 * Stops the thread execution. Please note that you <b>MUST</b> call
	 * this function at the end if you derived a class from this one.
	 * @param timeout the timeout in milliseconds, default = 10s
	 * @return zero if successful or an error code if failed
	 * @see errno.h
	 */
	virtual int stop(unsigned int timeout = 10000);

	/**
	 * A wrapper for the run() function, calls the run_wrapper(...)
	 * of m_runnable with the parameters passed in the constructor.
	 */
	virtual void run();

	/**
	 * Sets an internal flag that signals the worker thread to cancel,
	 * so that the next call to "shouldStop()" returns true.
	 */
	virtual void cancel();

	/**
	 * Returns true if the thread should stop. Should be polled
	 * by the thread's run() function to wait for a termination
	 * signal.
	 */
	bool shouldStop();

    private:

	/** pointer to the object that has a run() function */
	Kwave::Runnable *m_runnable;

	/** parameter pointer passed to the run() function */
	QVariant m_params;

	/** Mutex to control access to the thread itself */
	QMutex m_lock;

	/** Mutex for protecting SIGHUP <-> thread exit */
	QMutex m_lock_sighup;

	/** set to signal the thread that it should stop */
	bool m_should_stop;

	/**
	 * POSIX compatible thread ID of the worker thread.
	 * only needed and only valid while the thread is running.
	 * (needs a POSIX 1003.1-2001 system libc)
	 */
	pthread_t m_tid;

	/** POSIX compatible thread ID of the owner thread. */
	pthread_t m_owner_tid;

    };

}

#endif /* _WORKER_THREAD_H_ */

//***************************************************************************
//***************************************************************************
