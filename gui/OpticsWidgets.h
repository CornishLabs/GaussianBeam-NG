/* This file is part of the GaussianBeam project
   Copyright (C) 2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

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

#ifndef OPTICSWIDGETS_H
#define OPTICSWIDGETS_H

#include "ui_OpticsViewProperties.h"

#include "src/GaussianBeam.h"

#include <QScrollBar>
#include <QModelIndex>

class OpticsView;
class QCheckBox;
class QListWidget;
class QListWidgetItem;
class QStatusBar;

/**
* Displays graduated rullers instead of scroll bars
*/
class RullerSlider : public QScrollBar
{
	Q_OBJECT

public:
	RullerSlider(OpticsView* view, bool zoomScroll = false);

public:
	double rullerScale() const;

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);
	virtual void wheelEvent(QWheelEvent* event);

private:
	void drawGraduation(QPainter& painter, double position, double fractionalLength);

private:
	OpticsView* m_view;
	bool m_zoomScroll;
};

/**
* Widget used to tune the range and offset of a view
*/
class OpticsViewProperties : public QWidget, private Ui::OpticsViewProperties
{
	Q_OBJECT

public:
	OpticsViewProperties(OpticsView* view);

public:
	void setViewWidth(double width);
	void setViewHeight(double height);
	void setViewOrigin(double origin);

// UI slots
protected slots:
	void on_doubleSpinBox_Width_valueChanged(double value);
	void on_doubleSpinBox_Height_valueChanged(double value);
	void on_doubleSpinBox_Origin_valueChanged(double value);

private:
	OpticsView* m_view;
	bool m_update;
};

/**
* Widget that displays a small icon used to trigger a widget
*/
class CornerWidget : public QWidget
{
public:
	CornerWidget(QColor backgroundColor, const char*  pixmapName, QWidget* widget, QWidget* parent = 0);

protected:
	virtual void paintEvent(QPaintEvent* event);
	virtual void mousePressEvent(QMouseEvent* event);

private:
	QColor m_backgroundColor;
	QPixmap m_pixmap;
	QWidget* m_widget;
};

/**
* Widget with a selectable and sortable list of properties
*/
class PropertySelector : public QWidget
{
	Q_OBJECT

public:
	PropertySelector(QWidget* parent = 0);
	~PropertySelector();

public:
	bool showFullName() const;
	QList<Property::Type> checkedItems() const;

signals:
	void propertyChanged();

protected:
	void readSettings(QList<Property::Type> propertyList, QList<bool> checkList);

private:
	void writeSettings() const;

protected:
	QString m_settingsKey;

private:
	QListWidget* m_propertyListWidget;
	QCheckBox* m_symbolCheck;

private slots:
	void checkBoxModified(int state);
	void itemModified(QListWidgetItem* item);
};

/**
* Property selector for status bar properties
*/
class StatusPropertySelector : public PropertySelector
{
	Q_OBJECT

public:
	StatusPropertySelector(QWidget* parent = 0);
};

/**
* Property selector for table columns
*/
class TablePropertySelector : public PropertySelector
{
	Q_OBJECT

public:
	TablePropertySelector(QWidget* parent = 0);
};

/**
* Optics view status bar widget
*/
class StatusWidget : public QWidget
{
	Q_OBJECT

public:
	StatusWidget(QStatusBar* statusBar);

public:
	void showBeamInfo(const Beam* beam, double z);

private:
	QStatusBar* m_statusBar;
	QLabel* m_label;
	StatusPropertySelector* m_configWidget;
};

#endif
