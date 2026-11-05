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

#include "OpticsBench.h"
#include "OpticsFunction.h"
#include "Utils.h"

#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <functional>
#include <sstream>

using namespace std;

/////////////////////////////////////////////////
// OpticsBenchNotify

OpticsBenchNotify::OpticsBenchNotify(OpticsBench& opticsBench)
	: m_bench(opticsBench)
{}

/////////////////////////////////////////////////
// OpticsBench

OpticsBench::OpticsBench()
{
	m_opticsPrefix[LensType]            = "L";
	m_opticsPrefix[FlatMirrorType]      = "M";
	m_opticsPrefix[CurvedMirrorType]    = "R";
	m_opticsPrefix[FlatInterfaceType]   = "I";
	m_opticsPrefix[CurvedInterfaceType] = "C";
	m_opticsPrefix[GenericABCDType]     = "G";
	m_opticsPrefix[DielectricSlabType]  = "D";

	m_optics.clear();
	addOptics(new CreateBeam(180e-6, 10e-3, 1., "w0"), 0);
	m_optics[0]->setAbsoluteLock(true);

	m_wavelength = 461e-9;
	m_leftBoundary = -0.1;
	m_rightBoundary = 0.7;

	m_fits.push_back(Fit(3));
}

OpticsBench::~OpticsBench()
{
	// Delete optics
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		delete (*it);

	// Delelte beams
	for (vector<Beam*>::iterator it = m_beams.begin(); it != m_beams.end(); it++)
		delete (*it);
}

int OpticsBench::nFit() const
{
	return m_fits.size();
}

Fit& OpticsBench::addFit(unsigned int index, int nData)
{
	m_fits.insert(m_fits.begin() + index, Fit(nData));

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchFitAdded(index);

	return m_fits[index];
}

Fit& OpticsBench::fit(unsigned int index)
{
	static Fit defaultFit;

	if (index < m_fits.size())
		return m_fits[index];

	return defaultFit;
}

void OpticsBench::removeFit(unsigned int index)
{
	removeFits(index, 1);
}

void OpticsBench::removeFits(unsigned int startIndex, int n)
{
	m_fits.erase(m_fits.begin() + startIndex, m_fits.begin() + startIndex + n);

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchFitsRemoved(startIndex, n);
}

int OpticsBench::opticsIndex(const Optics* optics) const
{
	/// @todo implement a lookup table
	for (vector<Optics*>::const_iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if (*it == optics)
			return it - m_optics.begin();

	cerr << "Error : looking for an optics that is no more in the optics list" << endl;
	return -1;
}

void OpticsBench::notifyFitChange(unsigned int index)
{
	cerr << "OpticsBench::notifyFitChange" << endl;
	for (std::list<OpticsBenchNotify*>::const_iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchFitDataChanged(index);
}

void OpticsBench::registerNotify(OpticsBenchNotify* notify)
{
	m_notifyList.push_back(notify);

	notify->OpticsBenchWavelengthChanged();
	notify->OpticsBenchBoundariesChanged();

	for (int i = 0; i < nOptics(); i++)
		notify->OpticsBenchOpticsAdded(i);

	for (int i = 0; i < nFit(); i++)
		notify->OpticsBenchFitAdded(i);

	///@todo call all the other notifications ?
}

void OpticsBench::setWavelength(double wavelength)
{
	m_wavelength = wavelength;
	m_targetBeam.setWavelength(m_wavelength);
	setTargetBeam(m_targetBeam);
	computeBeams();

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchWavelengthChanged();
}

void OpticsBench::setLeftBoundary(double leftBoundary)
{
	if (leftBoundary < m_rightBoundary)
		m_leftBoundary = leftBoundary;

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchBoundariesChanged();
}

void OpticsBench::setRightBoundary(double rightBoundary)
{
	if (rightBoundary > m_leftBoundary)
		m_rightBoundary = rightBoundary;

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchBoundariesChanged();
}

void OpticsBench::addOptics(Optics* optics, int index)
{
	m_optics.insert(m_optics.begin() + index,  optics);
	m_beams.insert(m_beams.begin() + index, new Beam());

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchOpticsAdded(index);

	computeBeams(index);
}

void OpticsBench::addOptics(OpticsType opticsType, int index)
{
	Optics* optics;
	stringstream stream;
	stream << m_opticsPrefix[opticsType] << ++m_lastOpticsName[opticsType] << ends;
	string name;
	stream >> name;

	if (opticsType == LensType)
		optics = new Lens(0.1, 0.0, name);
	else if (opticsType == FlatMirrorType)
		optics = new FlatMirror(0.0, name);
	else if (opticsType == CurvedMirrorType)
		optics = new CurvedMirror(0.05, 0.0, name);
	else if (opticsType == FlatInterfaceType)
		optics = new FlatInterface(1.5, 0.0, name);
	else if (opticsType == CurvedInterfaceType)
		optics = new CurvedInterface(0.1, 1.5, 0.0, name);
	else if (opticsType == DielectricSlabType)
		optics = new DielectricSlab(1.5, 0.1, 0.0, name);
	else if (opticsType == GenericABCDType)
		optics = new GenericABCD(1.0, 0.2, 0.0, 1.0, 0.1, 0.0, name);
	else
		return;

	if (index > 0)
		optics->setPosition(OpticsBench::optics(index-1)->position() + 0.05);

	addOptics(optics, index);
}

void OpticsBench::removeOptics(int index, int count)
{
	for (int i = index; i < index + count; i++)
	{
		delete m_optics[index];
		m_optics.erase(m_optics.begin() + index);
		delete m_beams[index];
		m_beams.erase(m_beams.begin() + index);
	}

	for (std::list<OpticsBenchNotify*>::iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchOpticsRemoved(index, count);

	computeBeams(index);
}

int OpticsBench::setOpticsPosition(int index, double position, bool respectAbsoluteLock)
{
	Optics* movedOptics = m_optics[index];
	m_optics[index]->setPositionCheckLock(position, respectAbsoluteLock);
	sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
	computeBeams();

	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it) == movedOptics)
			return it - m_optics.begin();

	return index;
}

void OpticsBench::lockTo(int index, string opticsName)
{
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->name() == opticsName)
		{
			m_optics[index]->relativeLockTo(*it);
			break;
		}
	emitChange(0, nOptics()-1);
}

void OpticsBench::lockTo(int index, int id)
{
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->id() == id)
		{
			m_optics[index]->relativeLockTo(*it);
			break;
		}
	emitChange(0, nOptics()-1);
}

void OpticsBench::printTree()
{
	cerr << "Locking tree" << endl;

	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
	{
		cerr << " Optics " << (*it)->name() << " " <<  (*it)->id() << endl;
		if ((*it)->absoluteLock())
			cerr << "  absolutely locked" << endl;
		if (const Optics* parent = (*it)->relativeLockParent())
			cerr << "  parent = " << parent->name() << " " << parent->id() << endl;
		for (list<Optics*>::const_iterator cit = (*it)->relativeLockChildren().begin(); cit != (*it)->relativeLockChildren().end(); cit++)
			cerr << "  child = " << (*cit)->name() << " " << (*cit)->id() << endl;
	}
}

void OpticsBench::setOpticsName(int index, std::string name)
{
	// Check that the name is not already attributed
	for (vector<Optics*>::iterator it = m_optics.begin(); it != m_optics.end(); it++)
		if ((*it)->name() == name)
			return;

	m_optics[index]->setName(name);
	emitChange(0, nOptics()-1);
}

void OpticsBench::opticsPropertyChanged(int /*index*/)
{
	computeBeams();
}

void OpticsBench::setInputBeam(const Beam& beam)
{
	if (m_optics.size() <= 0)
		addOptics(new CreateBeam(180e-6, 10e-3, 1., "w0"), 0);

	CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
	createBeam->setWaist(beam.waist());
	setOpticsPosition(0, beam.waistPosition(), false);
	computeBeams();
}

void OpticsBench::setBeam(const Beam& beam, int index)
{
	*m_beams[index] = beam;
	computeBeams(index, true);
}

void OpticsBench::setTargetBeam(const TargetBeam& beam)
{
	m_targetBeam = beam;
	for (std::list<OpticsBenchNotify*>::const_iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchTargetBeamChanged();
}

void OpticsBench::computeBeams(int changedIndex, bool backward)
{
	if (m_optics.size() == 0)
		return;

	Beam beam;

	if (backward)
	{
		beam = *m_beams[changedIndex];
		for (int i = changedIndex + 1; i < nOptics(); i++)
			*m_beams[i] = beam = m_optics[i]->image(beam);
		beam = *m_beams[changedIndex];
		for (int i = changedIndex - 1; i >= 0; i--)
			*m_beams[i] = beam = m_optics[i+1]->antecedent(beam);
		CreateBeam* createBeam = dynamic_cast<CreateBeam*>(m_optics[0]);
		createBeam->setBeam(beam);
	}
	else
	{
		if (changedIndex == 0)
			beam.setWavelength(wavelength());
		else
			beam = *m_beams[changedIndex - 1];

		for (int i = changedIndex; i < nOptics(); i++)
			*m_beams[i] = beam = m_optics[i]->image(beam);
	}

	OpticsFunction function(m_optics, m_wavelength);
	function.setOverlapBeam(*m_beams.back());
	function.setCheckLock(false);

	m_sensitivity = function.curvature(function.currentPosition())/2.;

	emitChange(changedIndex, nOptics()-1);
}

void OpticsBench::emitChange(int startOptics, int endOptics) const
{
	for (std::list<OpticsBenchNotify*>::const_iterator it = m_notifyList.begin(); it != m_notifyList.end(); it++)
		(*it)->OpticsBenchDataChanged(startOptics, endOptics);
}

bool OpticsBench::magicWaist()
{
	OpticsFunction function(m_optics, m_wavelength);
	function.setOverlapBeam(m_targetBeam);
	function.setCheckLock(true);

	vector<int> opticsMovable;
	for (int i = 0; i < nOptics(); i++)
		if (!optics(i)->absoluteLock() && !optics(i)->relativeLockParent())
			opticsMovable.push_back(i);

	if (opticsMovable.empty())
		return false;

	const int nTry = 500000;
	bool found = false;

	const double minPos = m_leftBoundary;
	const double maxPos = m_rightBoundary;

	vector<double> positions = function.currentPosition();

	for (int i = 0; i < nTry; i++)
	{
		/// @todo find a suitable RNG
		// Randomly moves a random optics
		int index = rand() % nOptics();
		double position = double(rand())/double(RAND_MAX)*(maxPos - minPos) + minPos;
		positions[index] = position;
//		positions = function.localMaximum(positions);

		// Check waist
		Beam beam = function.beam(positions);
		if ((m_targetBeam.overlapCriterion() &&
			(Beam::overlap(beam, m_targetBeam) > m_targetBeam.minOverlap())) ||
			((!m_targetBeam.overlapCriterion()) &&
			(fabs(beam.waist() - m_targetBeam.waist()) < m_targetBeam.waistTolerance()*m_targetBeam.waist()) &&
		    (fabs(beam.waistPosition() - m_targetBeam.waistPosition()) < m_targetBeam.positionTolerance()*m_targetBeam.rayleigh())))
		{
			cerr << "found waist : " << beam.waist() << " @ " << beam.waistPosition() << " // try = " << i << endl;
			found = true;
			break;
		}
	}

	positions = function.localMaximum(positions);

	if (found)
	{
		for (unsigned int i = 0; i < positions.size(); i++)
			m_optics[i]->setPositionCheckLock(positions[i]);
		sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
		computeBeams();
    }
	else
		cerr << "Beam not found !!!" << endl;

	return found;
}

bool OpticsBench::localOptimum()
{
	OpticsFunction function(m_optics, m_wavelength);
	function.setOverlapBeam(m_targetBeam);
	function.setCheckLock(true);
	vector<double> positions = function.localMaximum(function.currentPosition());

	if (!function.optimizationSuccess())
		return false;

	for (unsigned int i = 0; i < positions.size(); i++)
		m_optics[i]->setPositionCheckLock(positions[i]);
	sort(m_optics.begin() + 1, m_optics.end(), less<Optics*>());
	computeBeams();

	return true;
}
