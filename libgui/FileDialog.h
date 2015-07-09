/*************************************************************************
      FileDialog.h  -  enhanced KFileDialog
                             -------------------
    begin                : Thu May 30 2002
    copyright            : (C) 2002 by Thomas Eschenbacher
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

#ifndef FILE_DIALOG_H
#define FILE_DIALOG_H

#include "config.h"

#include <QtGlobal>
#include <QFileDialog>
#include <QObject>
#include <QString>
#include <QUrl>

class QWidget;

namespace Kwave
{
    /**
     * An improved version of KFileDialog that does not forget the previous
     * directory and pre-selects the previous file extension.
     */
    class Q_DECL_EXPORT FileDialog: public QFileDialog
    {
	Q_OBJECT
    public:
	typedef enum {
	    Saving = 0,
	    Opening
	} OperationMode;

	/**
	 * Constructor.
	 * @see QFileDialog
	 */
	FileDialog(const QString& startDir,
	           OperationMode mode,
	           const QString& filter,
	           QWidget *parent, bool modal,
	           const QUrl last_url = QUrl(),
	           const QString last_ext = QString());

	/** Destructor */
	virtual ~FileDialog()
	{
	}

	/**
	 * Returns the previously used extension, including "*."
	 */
	QString selectedExtension();

	/**
	 * Returns the first selected URL (if any)
	 */
	QUrl selectedUrl() const;

    protected:

	/** load previous settings */
	void loadConfig(const QString &section);

    protected slots:

	/** save current settings */
	void saveConfig();

    private:

	/** name of the group in the config file */
	QString m_config_group;

	/** URL of the previously opened file or directory */
	QUrl m_last_url;

	/** extension of the last selected single URL or file */
	QString m_last_ext;

    };
}

#endif /* FILE_DIALOG_H */

//***************************************************************************
//***************************************************************************
