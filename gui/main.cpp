/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>
   Modified 2026 by Daniel Ruttley: ported to Qt 5 and Windows (see CHANGES.md)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "gui/GaussianBeamWidget.h"
#include "gui/GaussianBeamWindow.h"
#include "gui/Names.h"

#include <QApplication>
#include <QTextCodec>
#include <QTranslator>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	// APP_NAME and APP_VERSION are defined in GaussianBeam.pro.
	// The names also set where settings are stored
	// (registry: HKEY_CURRENT_USER\Software\GaussianBeam-NG).
	QCoreApplication::setOrganizationName(APP_NAME);
	QCoreApplication::setApplicationName(APP_NAME);
	QCoreApplication::setApplicationVersion(APP_VERSION);

#if QT_VERSION < 0x050000
	QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF8"));
#endif
	QTranslator translator;
	QString locale = QLocale::system().name();
	// Look in the working directory first, then next to the executable
	if (!translator.load(QString("GaussianBeam_") + locale))
		translator.load(QString("GaussianBeam_") + locale, QCoreApplication::applicationDirPath());
	app.installTranslator(&translator);

	initNames(&app);

	///@todo parse other arguments ?
	QString file;
	if (argc > 1)
		file = argv[1];

	GaussianBeamWindow window(file);
	window.show();
	return app.exec();
}
