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

#include "gui/Theme.h"

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QPixmap>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QSvgRenderer>

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwmapi.h>
#endif

Theme* Theme::instance()
{
	static Theme* theme = new Theme;
	return theme;
}

Theme::Theme()
	: QObject(qApp)
	, m_mode(Light)
{
}

bool Theme::systemPrefersDark()
{
#ifdef Q_OS_WIN
	QSettings personalize("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
	                      QSettings::NativeFormat);
	return personalize.value("AppsUseLightTheme", 1).toInt() == 0;
#else
	return false;
#endif
}

void Theme::apply()
{
	// The saved choice, or the system setting on first run
	QSettings settings;
	m_mode = settings.value("Theme/dark", systemPrefersDark()).toBool() ? Dark : Light;

	// The native Windows style ignores dark palettes, so use Fusion in both modes
	// (Fusion is part of Qt 5 and later)
	if (QStyle* fusion = QStyleFactory::create("Fusion"))
		QApplication::setStyle(fusion);
	// No style sheet here: with one set, Qt 5 does not reliably pass later
	// palette changes (switching mode) on to the widgets
	QApplication::setPalette(palette());

	// Catch windows as they are shown to colour their title bar
	qApp->installEventFilter(this);
}

void Theme::setMode(Mode mode)
{
	if (mode == m_mode)
		return;

	m_mode = mode;
	QSettings().setValue("Theme/dark", isDark());
	m_iconCache.clear();
	QApplication::setPalette(palette());

	foreach (QWidget* widget, QApplication::topLevelWidgets())
		if (widget->internalWinId())
			updateTitleBar(widget);

	emit changed();
}

void Theme::toggle()
{
	setMode(isDark() ? Light : Dark);
}

QPalette Theme::palette() const
{
	QPalette p;

	if (isDark())
	{
		p.setColor(QPalette::Window,          QColor("#2b2d30"));
		p.setColor(QPalette::WindowText,      QColor("#dfe1e5"));
		p.setColor(QPalette::Base,            QColor("#1e1f22"));
		p.setColor(QPalette::AlternateBase,   QColor("#26282b"));
		p.setColor(QPalette::ToolTipBase,     QColor("#393b40"));
		p.setColor(QPalette::ToolTipText,     QColor("#dfe1e5"));
		p.setColor(QPalette::Text,            QColor("#dfe1e5"));
		p.setColor(QPalette::Button,          QColor("#393b40"));
		p.setColor(QPalette::ButtonText,      QColor("#dfe1e5"));
		p.setColor(QPalette::BrightText,      QColor("#ff6b68"));
		p.setColor(QPalette::Light,           QColor("#4e5157"));
		p.setColor(QPalette::Midlight,        QColor("#43454a"));
		p.setColor(QPalette::Mid,             QColor("#323438"));
		p.setColor(QPalette::Dark,            QColor("#1b1c1f"));
		p.setColor(QPalette::Shadow,          QColor("#111214"));
		p.setColor(QPalette::Highlight,       QColor("#3574f0"));
		p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
		p.setColor(QPalette::Link,            QColor("#589df6"));
		p.setColor(QPalette::LinkVisited,     QColor("#a571e6"));
#if QT_VERSION >= 0x050C00
		p.setColor(QPalette::PlaceholderText, QColor("#7e8187"));
#endif
		const QColor disabled("#6f737a");
		p.setColor(QPalette::Disabled, QPalette::WindowText,      disabled);
		p.setColor(QPalette::Disabled, QPalette::Text,            disabled);
		p.setColor(QPalette::Disabled, QPalette::ButtonText,      disabled);
		p.setColor(QPalette::Disabled, QPalette::Highlight,       QColor("#43454a"));
		p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
		p.setColor(QPalette::Disabled, QPalette::Base,            QColor("#2b2d30"));
		p.setColor(QPalette::Disabled, QPalette::Button,          QColor("#323438"));
	}
	else
	{
		p.setColor(QPalette::Window,          QColor("#f2f3f5"));
		p.setColor(QPalette::WindowText,      QColor("#1f2328"));
		p.setColor(QPalette::Base,            QColor("#ffffff"));
		p.setColor(QPalette::AlternateBase,   QColor("#f5f6f8"));
		p.setColor(QPalette::ToolTipBase,     QColor("#ffffff"));
		p.setColor(QPalette::ToolTipText,     QColor("#1f2328"));
		p.setColor(QPalette::Text,            QColor("#1f2328"));
		p.setColor(QPalette::Button,          QColor("#f7f8fa"));
		p.setColor(QPalette::ButtonText,      QColor("#1f2328"));
		p.setColor(QPalette::BrightText,      QColor("#d1242f"));
		p.setColor(QPalette::Light,           QColor("#ffffff"));
		p.setColor(QPalette::Midlight,        QColor("#e8eaed"));
		p.setColor(QPalette::Mid,             QColor("#c4c8ce"));
		p.setColor(QPalette::Dark,            QColor("#a3a8b0"));
		p.setColor(QPalette::Shadow,          QColor("#6b7078"));
		p.setColor(QPalette::Highlight,       QColor("#3574f0"));
		p.setColor(QPalette::HighlightedText, QColor("#ffffff"));
		p.setColor(QPalette::Link,            QColor("#2f6fde"));
		p.setColor(QPalette::LinkVisited,     QColor("#8250df"));
#if QT_VERSION >= 0x050C00
		p.setColor(QPalette::PlaceholderText, QColor("#8a8f97"));
#endif
		const QColor disabled("#9aa0a6");
		p.setColor(QPalette::Disabled, QPalette::WindowText,      disabled);
		p.setColor(QPalette::Disabled, QPalette::Text,            disabled);
		p.setColor(QPalette::Disabled, QPalette::ButtonText,      disabled);
		p.setColor(QPalette::Disabled, QPalette::Highlight,       QColor("#d5d8dd"));
		p.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);
		p.setColor(QPalette::Disabled, QPalette::Base,            QColor("#f2f3f5"));
	}

	return p;
}

/////////////////////////////////////////////////
// Colours for custom painting

QColor Theme::viewBackground()  { return instance()->isDark() ? QColor("#1e1f22") : QColor(Qt::white); }
QColor Theme::viewForeground()  { return instance()->isDark() ? QColor("#c9ccd1") : QColor(Qt::black); }
QColor Theme::rulerBackground() { return instance()->isDark() ? QColor("#2b2d30") : QColor("#eceef1"); }
QColor Theme::rulerMajorTick()  { return instance()->isDark() ? QColor("#c9ccd1") : QColor("#2c3036"); }
QColor Theme::rulerMinorTick()  { return instance()->isDark() ? QColor("#55585e") : QColor("#b4b8bf"); }
QColor Theme::opticsFill()      { return instance()->isDark() ? QColor(120, 185, 240, 130) : QColor(153, 209, 247, 150); }
QColor Theme::abcdFill()        { return instance()->isDark() ? QColor(150, 150, 150, 120) : QColor(193, 193, 193, 150); }

/////////////////////////////////////////////////
// Icons

/// Render the SVG icon @p name (from :/icons/), drawn with "currentColor", in the palette's colours.
QIcon Theme::icon(const QString& name)
{
	Theme* theme = instance();
	if (theme->m_iconCache.contains(name))
		return theme->m_iconCache.value(name);

	QFile file(":/icons/" + name + ".svg");
	if (!file.open(QIODevice::ReadOnly))
		return QIcon();
	const QByteArray svg = file.readAll();

	const QPalette palette = QApplication::palette();
	struct State { QIcon::Mode mode; QColor color; };
	const State states[] = {
		{ QIcon::Normal,   palette.color(QPalette::Active,   QPalette::ButtonText) },
		{ QIcon::Disabled, palette.color(QPalette::Disabled, QPalette::ButtonText) },
		{ QIcon::Selected, palette.color(QPalette::Active,   QPalette::HighlightedText) } };
	const int sizes[] = { 16, 20, 24, 32, 48, 64 };

	QIcon result;
	for (unsigned int s = 0; s < sizeof(states)/sizeof(State); s++)
	{
		QByteArray coloured = svg;
		coloured.replace("currentColor", states[s].color.name().toLatin1());
		QSvgRenderer renderer(coloured);
		for (unsigned int i = 0; i < sizeof(sizes)/sizeof(int); i++)
		{
			QPixmap pixmap(sizes[i], sizes[i]);
			pixmap.fill(Qt::transparent);
			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing);
			renderer.render(&painter);
			painter.end();
			result.addPixmap(pixmap, states[s].mode);
		}
	}

	theme->m_iconCache.insert(name, result);
	return result;
}

/////////////////////////////////////////////////
// Window title bar (Windows 10 20H1 and later)

bool Theme::eventFilter(QObject* object, QEvent* event)
{
	if (event->type() == QEvent::Show)
		if (QWidget* widget = qobject_cast<QWidget*>(object))
			if (widget->isWindow())
				updateTitleBar(widget);

	return QObject::eventFilter(object, event);
}

void Theme::updateTitleBar(QWidget* window) const
{
#ifdef Q_OS_WIN
	const Qt::WindowType type = window->windowType();
	if ((type != Qt::Window) && (type != Qt::Dialog))
		return;

	HWND hwnd = reinterpret_cast<HWND>(window->winId());
	BOOL dark = isDark() ? TRUE : FALSE;
	// DWMWA_USE_IMMERSIVE_DARK_MODE is 20, or 19 before Windows 10 20H1
	if (FAILED(DwmSetWindowAttribute(hwnd, 20, &dark, sizeof(dark))))
		DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
	// Ask Windows to redraw the title bar of windows that are already shown
	SetWindowPos(hwnd, 0, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#else
	Q_UNUSED(window);
#endif
}
