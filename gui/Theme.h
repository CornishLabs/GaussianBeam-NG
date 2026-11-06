/* This file is part of GaussianBeam-NG, a continuation of the GaussianBeam project
   Copyright (C) 2026 Daniel Ruttley

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

#ifndef THEME_H
#define THEME_H

#include <QObject>
#include <QColor>
#include <QIcon>
#include <QHash>

class QWidget;

/**
* Light and dark colour themes.
*
* Sets the application style and palette, provides the colours used by the
* custom-painted widgets (optics view, rulers), and builds the icons, which are
* monochrome SVGs recoloured to match the palette.
*
* Emits changed() when the mode changes. Anything that sets icons or colours
* itself should connect to it.
*/
class Theme : public QObject
{
	Q_OBJECT

public:
	enum Mode { Light, Dark };

	static Theme* instance();

	/// Set up the style and palette. Call once, after QCoreApplication's names are set.
	void apply();

	Mode mode() const { return m_mode; }
	bool isDark() const { return m_mode == Dark; }
	void setMode(Mode mode);

public slots:
	void toggle();

signals:
	void changed();

// Colours and icons for the current mode
public:
	static QIcon icon(const QString& name);
	static QColor viewBackground();
	/// Optics outlines, mirror hatching and waist labels
	static QColor viewForeground();
	static QColor rulerBackground();
	static QColor rulerMajorTick();
	static QColor rulerMinorTick();
	static QColor opticsFill();
	static QColor abcdFill();

protected:
	virtual bool eventFilter(QObject* object, QEvent* event);

private:
	Theme();
	static bool systemPrefersDark();
	QPalette palette() const;
	void updateTitleBar(QWidget* window) const;

private:
	Mode m_mode;
	QHash<QString, QIcon> m_iconCache;
};

#endif
