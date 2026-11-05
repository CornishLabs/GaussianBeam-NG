/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>
   Modified 2026-11-05 by Daniel Ruttley: ported to Qt 5 and Windows (see CHANGES.md)

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

#include "src/Utils.h"
#include "gui/GaussianBeamModel.h"
#include "gui/OpticsWidgets.h"
#include "gui/Unit.h"
#include "gui/Names.h"

#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <QDebug>
#include <QTextStream>

GaussianBeamModel::GaussianBeamModel(OpticsBench& bench, TablePropertySelector* propertySelector, QObject* parent)
	: QAbstractTableModel(parent)
	, OpticsBenchNotify(bench)
	, m_propertySelector(propertySelector)

{
	m_columns = m_propertySelector->checkedItems();
	m_bench.registerNotify(this);

	connect(m_propertySelector, SIGNAL(propertyChanged()), this, SLOT(propertyWidgetModified()));
}

GaussianBeamModel::~GaussianBeamModel()
{
}

void GaussianBeamModel::propertyWidgetModified()
{
	beginResetModel();
	m_columns = m_propertySelector->checkedItems();
	endResetModel();
}

int GaussianBeamModel::rowCount(const QModelIndex& /*parent*/) const
{
	return m_bench.nOptics();
}

int GaussianBeamModel::columnCount(const QModelIndex& /*parent*/) const
{
	return m_columns.size();
}

QVariant GaussianBeamModel::data(const QModelIndex& index, int role) const
{
	if (!index.isValid() || role != Qt::DisplayRole)
		return QVariant();

	int row = index.row();
	Property::Type column = m_columns[index.column()];

	QString string;
	QTextStream data(&string);
	data.setRealNumberNotation(QTextStream::SmartNotation);
//	data.setRealNumberPrecision(2);

	if (column == Property::OpticsType)
		data << OpticsName::fullName[m_bench.optics(row)->type()];
	else if (column == Property::OpticsPosition)
		data << m_bench.optics(row)->position()*Units::getUnit(UnitPosition).divider();
	else if ((column == Property::OpticsRelativePosition) && (row > 0))
		data << (m_bench.optics(row)->position() - m_bench.optics(row-1)->position())*Units::getUnit(UnitPosition).divider();
	else if (column == Property::OpticsProperties)
	{
		if (m_bench.optics(row)->type() == CreateBeamType)
		{
			return QString("n = ") + QString::number(dynamic_cast<const CreateBeam*>(m_bench.optics(row))->index()) +
			       QString(", " + tr("M²") + " = ") + QString::number(dynamic_cast<const CreateBeam*>(m_bench.optics(row))->M2());
		}
		else if (m_bench.optics(row)->type() == LensType)
		{
			return QString("f = ") + QString::number(dynamic_cast<const Lens*>(m_bench.optics(row))->focal()*Units::getUnit(UnitFocal).divider())
			                       + Units::getUnit(UnitFocal).string();
		}
		else if (m_bench.optics(row)->type() == CurvedMirrorType)
		{
			return QString("R = ") + QString::number(dynamic_cast<const CurvedMirror*>(m_bench.optics(row))->curvatureRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string();
		}
		else if (m_bench.optics(row)->type() == FlatInterfaceType)
		{
			return QString("n2/n1 = ") + QString::number(dynamic_cast<const FlatInterface*>(m_bench.optics(row))->indexRatio());
		}
		else if (m_bench.optics(row)->type() == CurvedInterfaceType)
		{
			const CurvedInterface* optics = dynamic_cast<const CurvedInterface*>(m_bench.optics(row));
			return QString("n2/n1 = ") + QString::number(optics->indexRatio()) +
			       QString("\nR = ") + QString::number(optics->surfaceRadius()*Units::getUnit(UnitCurvature).divider())
			                       + Units::getUnit(UnitCurvature).string();
		}
		else if (m_bench.optics(row)->type() == DielectricSlabType)
		{
			const DielectricSlab* optics = dynamic_cast<const DielectricSlab*>(m_bench.optics(row));
			return QString("n2/n1 = ") + QString::number(optics->indexRatio()) +
			       QString("\n") + tr("width") + " = " + QString::number(m_bench.optics(row)->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string();
		}
		else if (m_bench.optics(row)->type() == GenericABCDType)
		{
			const ABCD* optics = dynamic_cast<const ABCD*>(m_bench.optics(row));
			return QString("A = ") + QString::number(optics->A()) +
			       QString("\nB = ") + QString::number(optics->B()*Units::getUnit(UnitABCD).divider())
			                         + Units::getUnit(UnitABCD).string() +
			       QString("\nC = ") + QString::number(optics->C()/Units::getUnit(UnitABCD).divider())
			                         + " /" + Units::getUnit(UnitABCD).string(false) +
			       QString("\nD = ") + QString::number(optics->D()) +
			       QString("\n") + tr("width") + " = " + QString::number(m_bench.optics(row)->width()*Units::getUnit(UnitWidth).divider())
			                       + Units::getUnit(UnitWidth).string();
		}
	}
	else if (column == Property::BeamWaist)
		data << m_bench.beam(row)->waist()*Units::getUnit(UnitWaist).divider();
	else if (column == Property::BeamWaistPosition)
		data << m_bench.beam(row)->waistPosition()*Units::getUnit(UnitPosition).divider();
	else if (column == Property::BeamRelativeWaistPosition)
		// Distance from the output face of the optics to the waist of the beam leaving it
		data << (m_bench.beam(row)->waistPosition() - m_bench.optics(row)->endPosition())*Units::getUnit(UnitPosition).divider();
	else if (column == Property::BeamRayleigh)
		data << m_bench.beam(row)->rayleigh()*Units::getUnit(UnitRayleigh).divider();
	else if (column == Property::BeamDivergence)
		data << m_bench.beam(row)->divergence()*Units::getUnit(UnitDivergence).divider();
	else if (column == Property::OpticsSensitivity)
		data << fabs(m_bench.sensitivity(row))*100./sqr(Units::getUnit(UnitPosition).divider());
	else if (column == Property::OpticsName)
		data << QString::fromUtf8(m_bench.optics(row)->name().c_str());
	else if (column == Property::OpticsLock)
	{
		if (m_bench.optics(row)->absoluteLock())
			data << tr("absolute");
		else if (m_bench.optics(row)->relativeLockParent())
			data << QString::fromUtf8(m_bench.optics(row)->relativeLockParent()->name().c_str());
		else
			data << tr("none");
	}
	else
		return QVariant();

	return string;
}

QVariant GaussianBeamModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole)
		if (orientation == Qt::Horizontal)
		{
			Property::Type type = m_columns[section];
			QString header = m_propertySelector->showFullName() ? Property::fullName[type] : Property::shortName[type];
			if (Property::unit[type] != UnitLess)
				header += " (" + Units::getUnit(Property::unit[type]).string(false) + ")";
			/// @todo handle this special case in a more general way
			if (type == Property::OpticsSensitivity)
				header += " (%/" + Units::getUnit(UnitPosition).string(false) + tr("²") + ")";
			return breakString(header);
		}
		else if (orientation == Qt::Vertical)
			return section;

	return QVariant();
}

bool GaussianBeamModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
	if (!index.isValid() || (role != Qt::EditRole) || value.isNull() || !value.isValid())
		return false;

	int row = index.row();
	Property::Type column = m_columns[index.column()];

	if (column == Property::OpticsPosition)
		m_bench.setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier(), row != 0);
	else if (column == Property::OpticsRelativePosition)
	{
		if (row == 0)
			return false;

		const Optics* optics = m_bench.optics(row);
		const Optics* preceedingOptics = m_bench.optics(row-1);
		if (optics->relativeLockedTo(preceedingOptics))
			return false;
		m_bench.setOpticsPosition(row, value.toDouble()*Units::getUnit(UnitPosition).multiplier() + preceedingOptics->position());
	}
	else if (column == Property::OpticsProperties)
	{
		Optics* optics = m_bench.opticsForPropertyChange(row);

		if (optics->type() == CreateBeamType)
		{
			CreateBeam* createBeam = dynamic_cast<CreateBeam*>(optics);
			createBeam->setIndex(value.toList()[0].toDouble());
			createBeam->setM2(value.toList()[1].toDouble());
		}
		else if (optics->type() == LensType)
			dynamic_cast<Lens*>(optics)->setFocal(value.toList()[0].toDouble()*Units::getUnit(UnitFocal).multiplier());
		else if (optics->type() == CurvedMirrorType)
			dynamic_cast<CurvedMirror*>(optics)->setCurvatureRadius(value.toList()[0].toDouble()*Units::getUnit(UnitCurvature).multiplier());
		else if (optics->type() == FlatInterfaceType)
			dynamic_cast<FlatInterface*>(optics)->setIndexRatio(value.toList()[0].toDouble());
		else if (optics->type() == CurvedInterfaceType)
		{
			CurvedInterface* curvedInterfaceOptics = dynamic_cast<CurvedInterface*>(optics);
			curvedInterfaceOptics->setIndexRatio(value.toList()[0].toDouble());
			curvedInterfaceOptics->setSurfaceRadius(value.toList()[1].toDouble()*Units::getUnit(UnitCurvature).multiplier());
		}
		else if (optics->type() == DielectricSlabType)
		{
			DielectricSlab* dielectricSlabOptics = dynamic_cast<DielectricSlab*>(optics);
			dielectricSlabOptics->setIndexRatio(value.toList()[0].toDouble());
			dielectricSlabOptics->setWidth(value.toList()[1].toDouble()*Units::getUnit(UnitWidth).multiplier());
		}
		else if (m_bench.optics(row)->type() == GenericABCDType)
		{
			/// @todo check that the ABCD matrix is valid, e.g. by introducing bool GenericABCD::isValid()
			GenericABCD* ABCDOptics = dynamic_cast<GenericABCD*>(optics);
			ABCDOptics->setA(value.toList()[0].toDouble());
			ABCDOptics->setB(value.toList()[1].toDouble()*Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setC(value.toList()[2].toDouble()/Units::getUnit(UnitABCD).multiplier());
			ABCDOptics->setD(value.toList()[3].toDouble());
			ABCDOptics->setWidth(value.toList()[4].toDouble()*Units::getUnit(UnitWidth).multiplier());
		}
		m_bench.opticsPropertyChanged(row);
	}
	else if (column == Property::BeamWaist)
	{
		Beam beam = *m_bench.beam(row);
		beam.setWaist(value.toDouble()*Units::getUnit(UnitWaist).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == Property::BeamWaistPosition)
	{
		Beam beam = *m_bench.beam(row);
		beam.setWaistPosition(value.toDouble()*Units::getUnit(UnitPosition).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == Property::BeamRelativeWaistPosition)
	{
		Beam beam = *m_bench.beam(row);
		beam.setWaistPosition(value.toDouble()*Units::getUnit(UnitPosition).multiplier() + m_bench.optics(row)->endPosition());
		m_bench.setBeam(beam, row);
	}
	else if (column == Property::BeamRayleigh)
	{
		Beam beam = *m_bench.beam(row);
		beam.setRayleigh(value.toDouble()*Units::getUnit(UnitRayleigh).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == Property::BeamDivergence)
	{
		Beam beam = *m_bench.beam(row);
		beam.setDivergence(value.toDouble()*Units::getUnit(UnitDivergence).multiplier());
		m_bench.setBeam(beam, row);
	}
	else if (column == Property::OpticsName)
		m_bench.setOpticsName(row, value.toString().toUtf8().data());
 	else if (column == Property::OpticsLock)
	{
		/// @todo make specific functions in OpticsBench to change this. Move this logic to OpticsBench
		int lockId = value.toInt();
		if (value == -2)
		{
			Optics* optics = m_bench.opticsForPropertyChange(row);
			optics->setAbsoluteLock(false);
			optics->relativeUnlock();
			m_bench.opticsPropertyChanged(row);
		}
		else if (value == -1)
		{
			Optics* optics = m_bench.opticsForPropertyChange(row);
			optics->setAbsoluteLock(true);
			m_bench.opticsPropertyChanged(row);
		}
		else
			m_bench.lockTo(row, lockId);
	}

	return true;
}

Qt::ItemFlags GaussianBeamModel::flags(const QModelIndex& index) const
{
	int row = index.row();
	Property::Type column = m_columns[index.column()];

	Qt::ItemFlags flags = Qt::ItemIsSelectable | Qt::ItemIsEnabled;

	if ((column == Property::OpticsName) ||
		(column == Property::OpticsLock) ||
		(column == Property::BeamWaist) ||
		(column == Property::BeamWaistPosition) ||
		(column == Property::BeamRelativeWaistPosition) ||
		(column == Property::BeamRayleigh) ||
		(column == Property::BeamDivergence) ||
		(column == Property::OpticsProperties)
		 && (m_bench.optics(row)->type() != FlatMirrorType))
			flags |= Qt::ItemIsEditable;

	if (column == Property::OpticsPosition)
	{
		if (row == 0)
			flags |= Qt::ItemIsEditable;

		const Optics* optics = m_bench.optics(row);
		if (!optics->relativeLockTreeAbsoluteLock())
			flags |= Qt::ItemIsEditable;
	}

	if ((column == Property::OpticsRelativePosition) && (row != 0))
	{
		const Optics* optics = m_bench.optics(row);
		const Optics* preceedingOptics = m_bench.optics(row-1);
		if (!optics->relativeLockedTo(preceedingOptics) &&
		    !optics->relativeLockTreeAbsoluteLock())
			flags |= Qt::ItemIsEditable;
	}

	return flags;
}

bool GaussianBeamModel::insertRows(int row, int count, const QModelIndex& parent)
{
	beginInsertRows(parent, row, row + count -1);
	endInsertRows();
	return true;
}

bool GaussianBeamModel::removeRows(int row, int count, const QModelIndex& parent)
{
	beginRemoveRows(parent, row, row + count -1);
	endRemoveRows();
	return true;
}

void GaussianBeamModel::OpticsBenchDataChanged(int startOptics, int endOptics)
{
	emit dataChanged(index(startOptics, 0), index(endOptics, columnCount()-1));
}

void GaussianBeamModel::OpticsBenchOpticsAdded(int index)
{
	insertRows(index, 1);
}

void GaussianBeamModel::OpticsBenchOpticsRemoved(int index, int count)
{
	removeRows(index, count);
}
