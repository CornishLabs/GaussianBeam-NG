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

#include "Cavity.h"
#include "OpticsBench.h"

#include <iostream>

using namespace std;

Cavity::Cavity()
{
	m_ringCavity = true;
}

void Cavity::computeBeam()
{
	if (m_opticsList.empty())
		return;

	// Compute cavity
	m_matrix = *m_opticsList.front();
	cerr << "Initial Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
	list<const ABCD*>::iterator lastIt = m_opticsList.begin();
	for (list<const ABCD*>::iterator it = lastIt; ++it != m_opticsList.end(); lastIt = it)
	{
		static FreeSpace freeSpace(0., 0.);
		freeSpace.setWidth((*it)->position() - (*(lastIt))->endPosition());
		freeSpace.setPosition((*it)->endPosition());
		cerr << "freespace B = " << freeSpace.B() << endl;
		m_matrix = m_matrix * freeSpace;
		cerr << "freespace Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
		m_matrix *= *(*it);
		cerr << "added Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
	}
	/// @todo matrix.setWidth()

//		cerr << "Backwards" << endl;
/*
		if (m_ringCavity)
		{
			for (int i = m_lastCavityIndex - 1; i > m_firstCavityIndex; i--)
			{
				FreeSpace freeSpace(m_bench.optics(i)->endPosition() - m_bench.optics(i-1)->endPosition(), m_bench.optics(i-1)->endPosition());
				m_matrix *= freeSpace;
				cerr << "freespace Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
				if (m_bench.optics(i)->isABCD())
					m_matrix *= *dynamic_cast<const ABCD*>(m_bench.optics(i));
				cerr << "added Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
			}
			FreeSpace freeSpace(optics(i+1)->position() - m_bench.optics(i)->endPosition(), m_bench.optics(i)->endPosition());
			m_matrix *= freeSpace;
			cerr << "freespace Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;
		}
		else
		{
			/// @todo add a user defined free space between the last and the first optics for linear cavities.
		}
*/
/*	static FreeSpace freeSpace(0., 0.);
	freeSpace.setWidth(m_bench.optics(m_lastCavityIndex)->position() - m_bench.optics(m_firstCavityIndex)->endPosition());
	freeSpace.setPosition(m_bench.optics(m_firstCavityIndex)->endPosition());
	m_matrix *= freeSpace;
*/
//	cerr << "Final Cavity ABCD = " << m_matrix.A() << " " << m_matrix.B() << " " << m_matrix.C() << " " << m_matrix.D() << endl;

}

bool Cavity::isStable() const
{
	if (m_opticsList.empty())
		return false;

	if (m_matrix.stabilityCriterion1())
	{
		if (m_matrix.stabilityCriterion2())
			return true;
		else
			cerr << "Cavity stable for 1 and not 2 !!!!";
	}
	else if (m_matrix.stabilityCriterion2())
		cerr << "Cavity stable for 2 and not 1 !!!!";

	return false;
}

const Beam Cavity::eigenBeam(double wavelength, int index) const
{
	/// @todo chache compute beam ?
//	computeBeam();

	/// @todo reimplement checks
/*	if (!isStable() ||
	   (index < m_firstCavityIndex) ||
	   (m_ringCavity && (index >= m_lastCavityIndex)) ||
	   (!m_ringCavity && (index > m_lastCavityIndex)))
		return Beam();
*/
	Beam beam = m_matrix.eigenMode(wavelength);

//	for (int i = m_firstCavityIndex; i <= index; i++)
//		beam = m_bench.optics(i)->image(beam);

	return beam;
}
