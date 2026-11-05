/* This file is part of the GaussianBeam project
   Copyright (C) 2007-2008 Jérôme Lodewyck <jerome dot lodewyck at normalesup.org>

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

#include "Unit.h"

#include <cmath>

const double Unit::infinity = 1000000.;

Unit::Unit(int power, QString unitString)
{
	m_power = power;
	m_unitString = unitString;
}

QChar Unit::prefix() const
{
	if (m_power == 12)
		return 'T';
	else if (m_power == 9)
		return 'G';
	else if (m_power == 6)
		return 'M';
	else if (m_power == 3)
		return 'k';
	else if (m_power == 2)
		return 'h';
	else if (m_power == 0)
		return QChar();
	else if (m_power == -1)
		return 'd';
	else if (m_power == -2)
		return 'c';
	else if (m_power == -3)
		return 'm';
	else if (m_power == -6)
		return  QChar(0x3BC);
	else if (m_power == -9)
		return 'n';
	else if (m_power == -12)
		return 'p';
	else if (m_power == -15)
		return 'f';

	return '?';
}

QString Unit::string(bool space) const
{
	QChar pre = prefix();

	return (space ? QString(" ") : QString("")) + (pre.isNull() ? QString() : pre) + m_unitString;
}

double Unit::multiplier() const
{
	return pow(10., m_power);
}

double Unit::divider() const
{
	return pow(10., -m_power);
}

Units::Units()
{}

const Unit Units::getUnit(UnitType unit)
{
	if (unit == UnitPosition)
		return Unit(-3, "m");
	else if (unit == UnitFocal)
		return Unit(-3, "m");
	else if (unit == UnitWaist)
		return Unit(-6, "m");
	else if (unit == UnitRayleigh)
		return Unit(-3, "m");
	else if (unit == UnitWavelength)
		return Unit(-9, "m");
	else if (unit == UnitDivergence)
		return Unit(-3, "rad");
	else if (unit == UnitCurvature)
		return Unit(-3, "m");
	else if (unit == UnitHRange)
		return Unit(-3, "m");
	else if (unit == UnitVRange)
		return Unit(-6, "m");
	else if (unit == UnitABCD)
		return Unit(-3, "m");
	else if (unit == UnitWidth)
		return Unit(-3, "m");
	else if (unit == UnitPhase)
		return Unit(0, "rad");
	else if (unit == UnitLess)
		return Unit(0, QString());

	return Unit(0, "");
}
