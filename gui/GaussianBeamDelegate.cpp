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

#include "gui/GaussianBeamDelegate.h"
#include "gui/GaussianBeamModel.h"
#include "gui/Unit.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif

PropertyEditor::PropertyEditor(QList<EditorProperty>& properties, QWidget* parent)
	: QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(0);
	layout->setSpacing(0);

	foreach (EditorProperty property, properties)
	{
		QDoubleSpinBox* spinBox = new QDoubleSpinBox(this);
		spinBox->setAccelerated(true);
		spinBox->setMinimum(property.minimum);
		spinBox->setMaximum(property.maximum);
		spinBox->setPrefix(property.prefix);
		spinBox->setSuffix(property.suffix);
		layout->addWidget(spinBox);
		m_doubleSpinBoxes << spinBox;
	}

	setLayout(layout);
}

QList<QVariant> PropertyEditor::values() const
{
	QList<QVariant> properties;

	foreach (QDoubleSpinBox* spinBox, m_doubleSpinBoxes)
		properties << spinBox->value();

	return properties;
}

GaussianBeamDelegate::GaussianBeamDelegate(QObject* parent, GaussianBeamModel* model, OpticsBench& bench)
	: QItemDelegate(parent)
	, m_model(model)
	, m_bench(bench)
{
}

QWidget *GaussianBeamDelegate::createEditor(QWidget* parent,
	const QStyleOptionViewItem& /*option*/, const QModelIndex& index) const
{
	/// @todo where are all these "new" things deleted ? Check Qt example

	int row = index.row();
	Property::Type column = m_model->columnContent(index.column());
	const Optics* optics = m_bench.optics(row);

	switch (column)
	{
	case Property::BeamWaist:
	case Property::BeamRayleigh:
	case Property::BeamDivergence:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(0.);
		editor->setMaximum(Unit::infinity);
		return editor;
	}
	case Property::OpticsPosition:
	case Property::OpticsRelativePosition:
	case Property::BeamWaistPosition:
	case Property::BeamRelativeWaistPosition:
	{
		QDoubleSpinBox* editor = new QDoubleSpinBox(parent);
		editor->setAccelerated(true);
		editor->setMinimum(-Unit::infinity);
		editor->setMaximum(Unit::infinity);
		return editor;
	}
	case Property::OpticsProperties:
	{
		QList<EditorProperty> properties;
		if (optics->type() == CreateBeamType)
			properties << EditorProperty(0., Unit::infinity, "n = ")
			           << EditorProperty(1., Unit::infinity, tr("M²") + " = ");
		else if (optics->type() == LensType)
			properties << EditorProperty(-Unit::infinity, Unit::infinity, "f = ", Units::getUnit(UnitFocal).string());
		else if (optics->type() == CurvedMirrorType)
			properties << EditorProperty(-Unit::infinity, Unit::infinity, "R = ", Units::getUnit(UnitCurvature).string());
		else if (optics->type() == FlatInterfaceType)
			properties << EditorProperty(0., Unit::infinity, "n2/n1 = ");
		else if (optics->type() == CurvedInterfaceType)
			properties << EditorProperty(0., Unit::infinity, "n2/n1 = ")
			           << EditorProperty(-Unit::infinity, Unit::infinity, "R = ", Units::getUnit(UnitCurvature).string());
		else if (optics->type() == DielectricSlabType)
			properties << EditorProperty(0., Unit::infinity, "n2/n1 = ")
			           << EditorProperty(0., Unit::infinity, "width = ", Units::getUnit(UnitWidth).string());
		else if (optics->type() == GenericABCDType)
			properties << EditorProperty(-Unit::infinity, Unit::infinity, "A = ")
			           << EditorProperty(-Unit::infinity, Unit::infinity, "B = ", Units::getUnit(UnitABCD).string())
			           << EditorProperty(-Unit::infinity, Unit::infinity, "C = ", " /" + Units::getUnit(UnitABCD).string(false))
			           << EditorProperty(-Unit::infinity, Unit::infinity, "D = ")
			           << EditorProperty(0., Unit::infinity, "width = ", Units::getUnit(UnitWidth).string());

		return new PropertyEditor(properties, parent);
	}
	case Property::OpticsName:
	{
		QLineEdit* editor = new QLineEdit(parent);
		return editor;
	}
	case Property::OpticsLock:
	{
		QComboBox* editor = new QComboBox(parent);
		editor->addItem(tr("none"), -2);
		editor->addItem(tr("absolute"), -1);
		for (int i = 0; i < m_model->rowCount(); i++)
			if ((!m_bench.optics(i)->relativeLockedTo(optics)) || (m_bench.optics(i) == optics->relativeLockParent()))
				editor->addItem(QString::fromUtf8(m_bench.optics(i)->name().c_str()), m_bench.optics(i)->id());
		return editor;
	}
	default:
		return 0;
	}

	return 0;
}

void GaussianBeamDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
	/// @todo why static cast ?

	if (!index.isValid() || (editor == 0))
		return;

	int row = index.row();
	Property::Type column = m_model->columnContent(index.column());
	const Optics* optics = m_bench.optics(row);

	switch (column)
	{
	case Property::OpticsPosition:
	case Property::OpticsRelativePosition:
	case Property::BeamWaist:
	case Property::BeamWaistPosition:
	case Property::BeamRelativeWaistPosition:
	case Property::BeamRayleigh:
	case Property::BeamDivergence:
	{
		double value = m_model->data(index, Qt::DisplayRole).toDouble();
		QDoubleSpinBox* spinBox = static_cast<QDoubleSpinBox*>(editor);
		spinBox->setValue(value);
		break;
	}
	case Property::OpticsProperties:
	{
		PropertyEditor* propertyEditor = static_cast<PropertyEditor*>(editor);
		if (optics->type() == CreateBeamType)
		{
			const CreateBeam* createBeam = dynamic_cast<const CreateBeam*>(optics);
			propertyEditor->setValue(0, createBeam->index());
			propertyEditor->setValue(1, createBeam->M2());
		}
		else if (optics->type() == LensType)
			propertyEditor->setValue(0, dynamic_cast<const Lens*>(optics)->focal()*Units::getUnit(UnitFocal).divider());
		else if (optics->type() == CurvedMirrorType)
			propertyEditor->setValue(0, dynamic_cast<const CurvedMirror*>(optics)->curvatureRadius()*Units::getUnit(UnitCurvature).divider());
		else if (optics->type() == FlatInterfaceType)
			propertyEditor->setValue(0, dynamic_cast<const FlatInterface*>(optics)->indexRatio());
		else if (optics->type() == CurvedInterfaceType)
		{
			const CurvedInterface* curvedInterface = dynamic_cast<const CurvedInterface*>(optics);
			propertyEditor->setValue(0, curvedInterface->indexRatio());
			propertyEditor->setValue(1, curvedInterface->surfaceRadius()*Units::getUnit(UnitCurvature).divider());
		}
		else if (optics->type() == GenericABCDType)
		{
			const GenericABCD* ABCDOptics = dynamic_cast<const GenericABCD*>(optics);
			propertyEditor->setValue(0, ABCDOptics->A());
			propertyEditor->setValue(1, ABCDOptics->B()*Units::getUnit(UnitABCD).divider());
			propertyEditor->setValue(2, ABCDOptics->C()/Units::getUnit(UnitABCD).divider());
			propertyEditor->setValue(3, ABCDOptics->D());
			propertyEditor->setValue(4, ABCDOptics->width()*Units::getUnit(UnitWidth).divider());
			break;
		}
		else if (optics->type() == DielectricSlabType)
		{
			const DielectricSlab* dielectricSlab = dynamic_cast<const DielectricSlab*>(optics);
			propertyEditor->setValue(0, dielectricSlab->indexRatio());
			propertyEditor->setValue(1, dielectricSlab->width()*Units::getUnit(UnitWidth).divider());
		}

		break;
	}
	case Property::OpticsName:
	{
		QString name = m_model->data(index, Qt::DisplayRole).toString();
		QLineEdit* lineEdit = static_cast<QLineEdit*>(editor);
		lineEdit->setText(name);
		break;
	}
	case Property::OpticsLock:
	{
		QString value = m_model->data(index, Qt::DisplayRole).toString();
		int lockId = -2;
		if (optics->absoluteLock())
			lockId = -1;
		else if (m_bench.optics(row)->relativeLockParent())
			lockId = m_bench.optics(row)->relativeLockParent()->id();
		QComboBox* comboBox = static_cast<QComboBox*>(editor);
		comboBox->setCurrentIndex(comboBox->findData(lockId));
		break;
	}
	default:
		return QItemDelegate::setEditorData(editor, index);
	}
}

void GaussianBeamDelegate::setModelData(QWidget* editor, QAbstractItemModel* model,
	const QModelIndex& index) const
{
	if (!index.isValid() || (editor == 0))
		return;

	Property::Type column = m_model->columnContent(index.column());

	switch (column)
	{
	case Property::OpticsLock:
	{
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		model->setData(index, comboBox->itemData(comboBox->currentIndex()));
		break;
	}
	case Property::OpticsProperties:
	{
		model->setData(index, static_cast<PropertyEditor*>(editor)->values());
		break;
	}
	default:
		QItemDelegate::setModelData(editor, model, index);
	}
}

void GaussianBeamDelegate::updateEditorGeometry(QWidget* editor,
	const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	if (!index.isValid() || (editor == 0))
		return;

	QItemDelegate::updateEditorGeometry(editor, option, index);
}
