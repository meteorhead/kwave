/***************************************************************************
  StringEnterPlugin.cpp  -  plugin for entering a text command
                             -------------------
    begin                : Sat Mar 14 2015
    copyright            : (C) 2015 by Thomas Eschenbacher
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

#include "config.h"
#include "errno.h"

#include <QPointer>
#include <QStringList>

#include <KLocalizedString>

#include "libkwave/String.h"
#include "libkwave/Utils.h"

#include "StringEnterDialog.h"
#include "StringEnterPlugin.h"

KWAVE_PLUGIN(stringenter, StringEnterPlugin)

//***************************************************************************
Kwave::StringEnterPlugin::StringEnterPlugin(QObject *parent,
                                            const QVariantList &args)
    :Kwave::Plugin(parent, args)
{
}

//***************************************************************************
Kwave::StringEnterPlugin::~StringEnterPlugin()
{
}

//***************************************************************************
void Kwave::StringEnterPlugin::load(QStringList &params)
{
    Q_UNUSED(params);
    QString entry = _("menu(plugin:setup(stringenter),%1/%2,F12)");
    emitCommand(entry.arg(
	_(I18N_NOOP("Settings"))).arg(_(I18N_NOOP("Enter Command")))
    );
}

//***************************************************************************
QStringList *Kwave::StringEnterPlugin::setup(QStringList &previous_params)
{
    QString preset;
    if (previous_params.count() == 1)
	preset = previous_params[0];

    // create the setup dialog
    QPointer<Kwave::StringEnterDialog> dialog =
	new Kwave::StringEnterDialog(parentWidget(), preset);
    Q_ASSERT(dialog);
    if (!dialog) return 0;

    QStringList *list = new QStringList();
    Q_ASSERT(list);
    if (list && dialog->exec()) {
	// user has pressed "OK"
	QString command = dialog->command();
	emitCommand(command);
    } else {
	// user pressed "Cancel"
	if (list) delete list;
	list = 0;
    }

    if (dialog) delete dialog;
    return list;
}

//***************************************************************************
#include "StringEnterPlugin.moc"
//***************************************************************************
//***************************************************************************
