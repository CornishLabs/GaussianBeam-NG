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

#include "Optics.h"
#include "Utils.h"

#include <iostream>
#include <cmath>

using namespace std;

int Optics::m_lastId = 0;

Optics::Optics(OpticsType type, bool ABCD, double position, string name)
	: m_id(++m_lastId)
	, m_type(type)
	, m_ABCD(ABCD)
	, m_position(position)
	, m_width(0.)
	, m_name(name)
	, m_absoluteLock(false)
	, m_relativeLockParent(0)
{
//	cerr << "Creating optics with id " << m_id << endl;
}

Optics::~Optics()
{
	// Detach all children
	for (list<Optics*>::iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		(*it)->m_relativeLockParent = 0;

	relativeUnlock();
}

void Optics::setAbsoluteLock(bool absoluteLock)
{
	if (absoluteLock)
		relativeUnlock();

	m_absoluteLock = absoluteLock;
}

bool Optics::relativeLockedTo(const Optics* const optics) const
{
	return relativeLockRootConst()->isRelativeLockDescendant(optics);
}

bool Optics::relativeLockTo(Optics* optics)
{
	if (relativeLockedTo(optics))
		return false;

	if (m_relativeLockParent)
		relativeUnlock();

	m_relativeLockParent = optics;
	optics->m_relativeLockChildren.push_back(this);

	m_absoluteLock = false;
	return true;
}

bool Optics::relativeUnlock()
{
	if (!m_relativeLockParent)
		return false;

	m_relativeLockParent->m_relativeLockChildren.remove(this);
	m_relativeLockParent = 0;

	return true;
}

Optics* Optics::relativeLockRoot()
{
	if (!m_relativeLockParent)
		return this;
	else
		return m_relativeLockParent->relativeLockRoot();
}

const Optics* Optics::relativeLockRootConst() const
{
	if (!m_relativeLockParent)
		return this;
	else
		return m_relativeLockParent->relativeLockRootConst();
}

bool Optics::isRelativeLockDescendant(const Optics* const optics) const
{
	if (optics == this)
		return true;

	for (list<Optics*>::const_iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		if ((*it)->isRelativeLockDescendant(optics))
			return true;

	return false;
}

void Optics::moveDescendant(double distance)
{
	setPosition(position() + distance);

	for (list<Optics*>::iterator it = m_relativeLockChildren.begin(); it != m_relativeLockChildren.end(); it++)
		(*it)->moveDescendant(distance);
}

void Optics::setPositionCheckLock(double pos, bool respectAbsoluteLock)
{
	if (relativeLockTreeAbsoluteLock() && respectAbsoluteLock)
		return;

	relativeLockRoot()->moveDescendant(pos - position());
}

void Optics::eraseLockingTree()
{
	m_relativeLockChildren.clear();
	m_relativeLockParent = 0;
}

/////////////////////////////////////////////////
// CreateBeam class

CreateBeam::CreateBeam(double waist, double waistPosition, double index, string name)
	: Optics(CreateBeamType, false/*Not ABCD*/, waistPosition, name)
	, m_waist(waist)
	, m_index(index)
{
	m_M2 = 1.;
}

void CreateBeam::setWaist(double waist)
{
	if (waist != 0.)
		m_waist = waist;
}

void CreateBeam::setIndex(double index)
{
	if (index > 0.)
		m_index = index;
}

void CreateBeam::setM2(double M2)
{
	if (M2 >= 1.)
		m_M2 = M2;
}

Beam CreateBeam::image(const Beam& inputBeam) const
{
	return Beam(m_waist, position(), inputBeam.wavelength(), m_index, m_M2);
}

Beam CreateBeam::antecedent(const Beam& outputBeam) const
{
	return Beam(m_waist, position(), outputBeam.wavelength(), m_index, m_M2);
}

void CreateBeam::setBeam(const Beam& beam)
{
	setPosition(beam.waistPosition());
	setWaist(beam.waist());
	setIndex(beam.index());
	setM2(beam.M2());
}


/////////////////////////////////////////////////
// Lens class

void Lens::setFocal(double focal)
{
	if (focal != 0.)
		m_focal = focal;
}

/////////////////////////////////////////////////
// CurvedMirror class

void CurvedMirror::setCurvatureRadius(double curvatureRadius)
{
	if (curvatureRadius != 0.)
		m_curvatureRadius = curvatureRadius;
}

/////////////////////////////////////////////////
// Dielectric class

void Dielectric::setIndexRatio(double indexRatio)
{
	if (indexRatio > 0.)
		m_indexRatio = indexRatio;
}

/////////////////////////////////////////////////
// CurvedInterface class

void CurvedInterface::setSurfaceRadius(double surfaceRadius)
{
	if (surfaceRadius != 0.)
		m_surfaceRadius = surfaceRadius;
}

/////////////////////////////////////////////////
// ABCD class

ABCD::ABCD(OpticsType type, double position, std::string name)
	: Optics(type, true/*Is ABCD*/, position, name)
{}

Beam ABCD::image(const Beam& inputBeam) const
{
	const complex<double> qIn = inputBeam.q(position());
	const complex<double> qOut = (A()*qIn + B()) / (C()*qIn + D());
	return Beam(qOut, position() + width(), inputBeam.wavelength(), inputBeam.index()*indexJump(), inputBeam.M2());
}

Beam ABCD::antecedent(const Beam& outputBeam) const
{
	const complex<double> qOut = outputBeam.q(position() + width());
	const complex<double> qIn = (B() - D()*qOut) / (C()*qOut - A());
	return Beam(qIn, position(), outputBeam.wavelength(), outputBeam.index()/indexJump(), outputBeam.M2());
}

bool ABCD::stabilityCriterion1() const
{
	return fabs((A() + B())/2.) < 1.;
}

bool ABCD::stabilityCriterion2() const
{
	return sqr(D() - A()) + 4.*C()*B() < 0.;
}

Beam ABCD::eigenMode(double wavelength) const
{
	/// @todo what is the index ?
	return Beam(complex<double>(-(D() - A())/(2.*C()), -sqrt(-(sqr(D() - A()) + 4.*C()*B()))/(2.*C())), position(), wavelength, 1.0, 1.0);
}

/////////////////////////////////////////////////
// GenericABCD class

GenericABCD& GenericABCD::operator*=(const ABCD& abcd)
{
	setA(A()*abcd.A() + B()*abcd.C());
	setB(A()*abcd.B() + B()*abcd.D());
	setC(C()*abcd.A() + D()*abcd.C());
	setD(C()*abcd.B() + D()*abcd.D());
	/// @todo check if the two objects are adjacent ?
	setWidth(width() + abcd.width());

	return *this;
}

GenericABCD operator*(const ABCD& abcd1, const ABCD& abcd2)
{
	GenericABCD r(abcd1);
	r *= abcd2;
	return r;
}
