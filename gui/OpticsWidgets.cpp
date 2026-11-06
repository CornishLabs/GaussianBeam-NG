/* This file is part of the GaussianBeam project
   Copyright (C) 2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>
   Modified 2026 by Daniel Ruttley: ported to Qt 5 and Windows; added wavefront drawing (see CHANGES.md)

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

#include "gui/Names.h"
#include "gui/OpticsWidgets.h"
#include "gui/OpticsView.h"
#include "gui/Unit.h"

#include <QWheelEvent>
#include <QListWidget>
#include <QCheckBox>
#include <QSettings>
#include <QDebug>

#include <cmath>
#include <cstdlib>

/////////////////////////////////////////////////
// RullerSlider

RullerSlider::RullerSlider(OpticsView* view, bool zoomScroll)
	: QScrollBar(view)
{
	m_view = view;
	m_zoomScroll = zoomScroll;
}

double RullerSlider::rullerScale() const
{
	int length = maximum() + pageStep() - minimum();
	double scale = double(length);
	if (orientation() == Qt::Horizontal)
		scale /= m_view->scene()->width();
	else
		scale /= m_view->scene()->height();

	return scale;
}

void RullerSlider::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);
}

void RullerSlider::wheelEvent(QWheelEvent* event)
{
	if (m_zoomScroll)
	{
		int numDegrees = event->delta()/8;
		int numSteps = numDegrees/15;
		double zoomFactor = pow(1.2, numSteps);
		if (orientation() == Qt::Horizontal)
			m_view->setHorizontalRange(m_view->horizontalRange()*zoomFactor);
		else
			m_view->setVerticalRange(m_view->verticalRange()*zoomFactor);
	}
	else
		QScrollBar::wheelEvent(event);
}

void RullerSlider::drawGraduation(QPainter& painter, double position, double fractionalLength)
{
	if (orientation() == Qt::Horizontal)
		painter.drawLine(QPointF(position, 0.), QPointF(position, height()*fractionalLength));
	else
		painter.drawLine(QPointF(0., position), QPointF(width()*fractionalLength, position));
}

void RullerSlider::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QColor backgroundColor(245, 245, 200);
	QBrush backgroundBrush(backgroundColor);
	QPen backgroundPen(backgroundColor);
	QPen mainTickPen(Qt::black);
	QPen secondTickPen(Qt::lightGray);

	double scale = rullerScale();
	double spacing = (orientation() == Qt::Horizontal ? 0.01 : 0.00005);
	int lastStep = int(double(value() + pageStep())/(scale*spacing)) + 1;
	int firstStep = int(double(value())/(scale*spacing)) - 1;

	painter.setBrush(backgroundBrush);
	painter.setPen(backgroundPen);
	painter.drawRect(rect());
	painter.setPen(secondTickPen);
	for (double x = double(lastStep)*spacing; x >= double(firstStep)*spacing; x -= spacing)
	{
		double pos = x*scale - double(value());
		drawGraduation(painter, pos, 0.4);
		if (abs((int(round(x/spacing)) % 10)) == 5)
			drawGraduation(painter, pos, 0.65);
		else if ((int(round(x/spacing)) % 10) == 0)
		{
			painter.setPen(mainTickPen);
			drawGraduation(painter, pos, 0.8);
			QString text;
			text.setNum(round(x*1000.));
			QRectF textRect(0., 0., 0., 0.);
			textRect.moveCenter(QPointF(pos + 2., height()/2.));
			textRect = painter.boundingRect(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
			painter.drawText(textRect, Qt::AlignCenter, text);
			painter.setPen(secondTickPen);
		}
	}
}

/////////////////////////////////////////////////
// OpticsViewProperties

OpticsViewProperties::OpticsViewProperties(OpticsView* view)
	: QWidget(view)
{
	m_view = view;
	m_update = false;
	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose);
	if (OpticsScene* scene = dynamic_cast<OpticsScene*>(m_view->scene()))
		checkBox_Wavefronts->setChecked(scene->wavefrontsVisible());
	m_update = true;
}

void OpticsViewProperties::on_doubleSpinBox_Width_valueChanged(double value)
{
	if (m_update)
		m_view->setHorizontalRange(value*Units::getUnit(UnitPosition).multiplier());
}

void OpticsViewProperties::on_doubleSpinBox_Height_valueChanged(double value)
{
	if (m_update)
		m_view->setVerticalRange(value*Units::getUnit(UnitWaist).multiplier());
}

void OpticsViewProperties::on_doubleSpinBox_Origin_valueChanged(double value)
{
	if (m_update)
		m_view->setOrigin(value*Units::getUnit(UnitPosition).multiplier());
}

void OpticsViewProperties::on_checkBox_Wavefronts_toggled(bool checked)
{
	if (!m_update)
		return;
	if (OpticsScene* scene = dynamic_cast<OpticsScene*>(m_view->scene()))
		scene->showWavefronts(checked);
}

void OpticsViewProperties::setViewWidth(double width)
{
	m_update = false;
	doubleSpinBox_Width->setValue(width*Units::getUnit(UnitPosition).divider());
	m_update = true;
}

void OpticsViewProperties::setViewHeight(double height)
{
	m_update = false;
	doubleSpinBox_Height->setValue(height*Units::getUnit(UnitWaist).divider());
	m_update = true;
}

void OpticsViewProperties::setViewOrigin(double origin)
{
	m_update = false;
	doubleSpinBox_Origin->setValue(origin*Units::getUnit(UnitPosition).divider());
	m_update = true;
}

/////////////////////////////////////////////////
// CornerWidget

CornerWidget::CornerWidget(QColor backgroundColor, const char* pixmapName, QWidget* widget, QWidget* parent)
	: QWidget(parent)
	, m_backgroundColor(backgroundColor)
	, m_widget(widget)
{
	m_pixmap = QPixmap(pixmapName);
	setFixedSize(m_pixmap.rect().size());
}

void CornerWidget::paintEvent(QPaintEvent* event)
{
	Q_UNUSED(event);

	QPainter painter(this);
	QBrush backgroundBrush(m_backgroundColor);
	QPen backgroundPen(m_backgroundColor);
	painter.setBrush(backgroundBrush);
	painter.setPen(backgroundPen);
	painter.drawRect(rect());
	painter.drawPixmap(m_pixmap.rect(), m_pixmap, m_pixmap.rect());
}

void CornerWidget::mousePressEvent(QMouseEvent* event)
{
	Q_UNUSED(event);

	m_widget->setVisible(!m_widget->isVisible());
}

/////////////////////////////////////////////////
// Property selector

PropertySelector::PropertySelector(QWidget* parent)
	: QWidget(parent)
{
	m_propertyListWidget = new QListWidget;
	m_propertyListWidget->setDragDropMode(QAbstractItemView::InternalMove);

	m_symbolCheck = new QCheckBox("Display full name");

	QVBoxLayout* layout = new QVBoxLayout;
	layout->addWidget(new QLabel("Check and sort properties:"));
	layout->addWidget(m_propertyListWidget);
	layout->addWidget(m_symbolCheck);
	setLayout(layout);
	connect(m_symbolCheck, SIGNAL(stateChanged(int)), this, SLOT(checkBoxModified(int)));
	connect(m_propertyListWidget, SIGNAL(currentRowChanged(int)), this, SLOT(checkBoxModified(int)));
	connect(m_propertyListWidget, SIGNAL(itemChanged(QListWidgetItem*)), this, SLOT(itemModified(QListWidgetItem*)));

	setWindowIcon(QIcon(":/images/preferences-system.png"));
}

PropertySelector::~PropertySelector()
{
	writeSettings();
}

void PropertySelector::checkBoxModified(int)
{
	emit(propertyChanged());
}

void PropertySelector::itemModified(QListWidgetItem* item)
{
	emit(propertyChanged());
}

bool PropertySelector::showFullName() const
{
	return m_symbolCheck->checkState() == Qt::Checked;
}

QList<Property::Type> PropertySelector::checkedItems() const
{
	QList<Property::Type> items;

	for (int i = 0; i < m_propertyListWidget->count(); i++)
		if (m_propertyListWidget->item(i)->checkState() == Qt::Checked)
			items << Property::Type(m_propertyListWidget->item(i)->data(Qt::UserRole).toInt());

	return items;
}

void PropertySelector::readSettings(QList<Property::Type> propertyList, QList<bool> checkList)
{
	QSettings settings;
	QList<QVariant> propertySettings = settings.value(m_settingsKey + "/properties").toList();
	QList<QVariant> checkSettings = settings.value(m_settingsKey + "/checks").toList();

	// Read settings
	if ((propertySettings.size() == propertyList.size()) && (checkSettings.size() == checkList.size()))
	{
		propertyList.clear();
		checkList.clear();
		for (int i = 0; i < propertySettings.size(); i++)
		{
			propertyList << Property::Type(propertySettings[i].toInt());
			checkList << checkSettings[i].toBool();
		}
	}

	m_propertyListWidget->clear();
	for (int i = 0; i < propertyList.size(); i++)
	{
		QListWidgetItem* widgetItem = new QListWidgetItem;
		QString text = Property::fullName[propertyList[i]];
		if (Property::fullName[propertyList[i]] != Property::shortName[propertyList[i]])
			text += " (" + Property::shortName[propertyList[i]] + ")";
		widgetItem->setText(text);
		widgetItem->setCheckState(checkList[i] ? Qt::Checked : Qt::Unchecked);
		widgetItem->setData(Qt::UserRole, propertyList[i]);
		m_propertyListWidget->addItem(widgetItem);
	}

	m_symbolCheck->setCheckState(Qt::CheckState(settings.value(m_settingsKey + "/fullName", Qt::Checked).toInt()));
}

void PropertySelector::writeSettings() const
{
	QList<QVariant> propertySettings;
	QList<QVariant> checkSettings;

	for (int i = 0; i < m_propertyListWidget->count(); i++)
	{
		propertySettings << m_propertyListWidget->item(i)->data(Qt::UserRole);
		checkSettings << (m_propertyListWidget->item(i)->checkState() == Qt::Checked);
	}

	QSettings settings;
	settings.setValue(m_settingsKey + "/properties", propertySettings);
	settings.setValue(m_settingsKey + "/checks", checkSettings);
	settings.setValue(m_settingsKey + "/fullName", m_symbolCheck->checkState());
}

/////////////////////////////////////////////////
// StatusPropertySelector

StatusPropertySelector::StatusPropertySelector(QWidget* parent)
	: PropertySelector(parent)
{
	setWindowTitle(tr("Configure status bar"));
	m_settingsKey = "StatusPropertySelector";

	QList<Property::Type> defaultPropertyList;
	QList<bool> defaultCheckList;

	defaultPropertyList << Property::BeamPosition << Property::BeamRadius << Property::BeamDiameter
	                    << Property::BeamCurvature << Property::BeamGouyPhase
	                    << Property::BeamDistanceToWaist << Property::BeamParameter << Property::Index;
	defaultCheckList << true << true << false << true << false << false << false << false;

	readSettings(defaultPropertyList, defaultCheckList);
}

/////////////////////////////////////////////////
// TablePropertySelector

TablePropertySelector::TablePropertySelector(QWidget* parent)
	: PropertySelector(parent)
{
	setWindowTitle(tr("Configure table columns"));
	m_settingsKey = "TablePropertySelector";

	QList<Property::Type> defaultPropertyList;
	QList<bool> defaultCheckList;

	defaultPropertyList << Property::OpticsType << Property::OpticsPosition
	                    << Property::OpticsRelativePosition << Property::OpticsProperties
	                    << Property::BeamWaist << Property::BeamWaistPosition << Property::BeamRelativeWaistPosition
	                    << Property::BeamRayleigh << Property::BeamDivergence << Property::OpticsSensitivity
	                    << Property::OpticsName << Property::OpticsLock;
	defaultCheckList << true << true << true << true << true << true << true << true << true << true << true << true;

	readSettings(defaultPropertyList, defaultCheckList);
}

/////////////////////////////////////////////////
// StatusWidget

StatusWidget::StatusWidget(QStatusBar* statusBar)
	: QWidget(statusBar)
	, m_statusBar(statusBar)
{
	m_configWidget = new StatusPropertySelector(this);
	m_configWidget->setWindowFlags(Qt::Window);
	CornerWidget* corner = new CornerWidget(Qt::transparent, ":/images/preferences-system.png",
	                       m_configWidget, this);
	m_label = new QLabel;
	QHBoxLayout* layout = new QHBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(corner);
	layout->addWidget(m_label);
	setLayout(layout);
}

void StatusWidget::showBeamInfo(const Beam* beam, double z)
{
	m_statusBar->clearMessage();

	bool fullName = m_configWidget->showFullName();
	QList<Property::Type> items = m_configWidget->checkedItems();
	QString text;

	foreach (Property::Type type, items)
	{
		text += fullName ? (Property::fullName[type] + tr(": ")) : (Property::shortName[type] + tr(" = "));
		double value = 0.;
		if (type == Property::BeamPosition)
			value = z;
		else if (type == Property::BeamRadius)
			value = beam->radius(z);
		else if (type == Property::BeamDiameter)
			value = 2.*beam->radius(z);
		else if (type == Property::BeamCurvature)
			value = beam->curvature(z);
		else if (type == Property::BeamGouyPhase)
			value = beam->gouyPhase(z);
		else if ((type == Property::BeamDistanceToWaist) || (type == Property::BeamParameter))
			value = z - beam->waistPosition();
		else if (type == Property::Index)
			value = beam->index();

		value *=  Units::getUnit(Property::unit[type]).divider();
		text += QString::number(value, 'f', 2) + Units::getUnit(Property::unit[type]).string();

		if (type == Property::BeamParameter)
		{
			value = beam->rayleigh()*Units::getUnit(UnitRayleigh).divider();
			text += " + i" + QString::number(value, 'f', 2) + Units::getUnit(UnitRayleigh).string();
		}

		text += "    ";

	}
	m_label->setText(text);
}
