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

#ifndef OPTICSBENCH_H
#define OPTICSBENCH_H

#include "Optics.h"
#include "GaussianBeam.h"
#include "GaussianFit.h"
#include "Cavity.h"

#include <vector>
#include <list>
#include <map>

class OpticsBench;

/**
* OpticsBenchNotify. Inherit from this class and register to the bench
* using OpticsBench::registerNotify(this) if you want to
* be notified of optics bench change events.
* Upon registration, (almost) all notifications are called for initilization
*/
class OpticsBenchNotify
{
public:
	/// Constructor
	OpticsBenchNotify(OpticsBench& opticsBench);
	/// Called when the wavevelength changes
	virtual void OpticsBenchWavelengthChanged() {};
	/// Called when Optics @p index is added
	virtual void OpticsBenchOpticsAdded(int /*index*/) {};
	virtual void OpticsBenchOpticsRemoved(int /*index*/, int /*count*/) {};
	virtual void OpticsBenchDataChanged(int /*startOptics*/, int /*endOptics*/) {};
	virtual void OpticsBenchTargetBeamChanged() {};
	virtual void OpticsBenchBoundariesChanged() {};
	virtual void OpticsBenchFitAdded(int /*index*/) {};
	virtual void OpticsBenchFitsRemoved(int /*index*/, int /*count*/) {};
	virtual void OpticsBenchFitDataChanged(int /*index*/) {};

protected:
	/// A reference to the OpticsBench to which we register;
	OpticsBench& m_bench;
};

/**
* OpticsBench
*/
class OpticsBench
{
public:
	OpticsBench();
	~OpticsBench();

public:

	// Properties

	/// @return the bench wavelength
	double wavelength() const { return m_wavelength; }
	/// Set the bench wavelength to @p wavelength
	void setWavelength(double wavelength);
	/// @return the bench left boundary
	double leftBoundary() { return m_leftBoundary; }
	/// Set the bench left boundary to @p leftBoundary
	void setLeftBoundary(double leftBoundary);
	/// @return the bench right boundary
	double rightBoundary() { return m_rightBoundary; }
	/// Set the bench right boundary to @p leftBoundary
	void setRightBoundary(double rightBoundary);

	// Optics

	/// @return the number of optics in the bench
	int nOptics() const { return m_optics.size(); }
	/**
	* @return the sorted index of optics @p optics in the optics bench
	* This index may change when an optics position is changed
	*/
	int opticsIndex(const Optics* optics) const;
	/// @return a pointer to optics situated at index @p index
	const Optics* optics(int index) const { return m_optics[index]; }
	/// Add the optics @p optics at index @p index
	void addOptics(Optics* optics, int index);
	/// Add an optics of type @p opticsType at index @p index
	void addOptics(OpticsType opticsType, int index);
	/// Remove the optics situated at index @p index
	void removeOptics(int index, int count = 1);
	/**
	* Set the optics at @p index to position @p position. Takes care of locks,
	* exclusion areas, and optics ordering.
	* @return the new index of the optics
	*/
	int setOpticsPosition(int index, double position, bool respectAbsoluteLock = true);
	/// set the name of optics at @p index to @p name
	void setOpticsName(int index, std::string name);
	/// lock the optics at index @p index to the first optics called @p opticsName
	void lockTo(int index, std::string opticsName);
	/// lock the optics at index @p index to the optics which id is @p id
	void lockTo(int index, int id);
	/**
	* Retrieve a non constant pointer to the optics @p index for property change.
	* When finished, call opticsPropertyChanged() to propagate your changes
	* If opticsBench proposes a direct function for changing a property
	* (position, name, lockTo), rather use these functions.
	*/
	Optics* opticsForPropertyChange(int index) { return m_optics[index]; }
	/// Call this function after changing the properties of an optics returned by opticsForPropertyChange( @p index )
	void opticsPropertyChanged(int index);

	/// Beams handling
	const Beam* beam(int index) const { return m_beams[index]; }
	void setInputBeam(const Beam& beam);
	void setBeam(const Beam& beam, int index);
	double sensitivity(int index) const { return m_sensitivity[index]; }

	/// Cavity
	Cavity& cavity() { return m_cavity; }
	/**
	* If you modified a Cavity class obtained by the cavity() function, call this
	* function to notify this modification
	*/
	void notifyCavityChange();

	/// Waist fit
	int nFit() const;
	Fit& addFit(unsigned int index, int nData = 0);
	Fit& fit(unsigned int index);
	void removeFit(unsigned int index);
	void removeFits(unsigned int startIndex, int n);
	/**
	* If you modified a Fit class obtained by the fit() function, call this
	* function to notify this modification
	*/
	void notifyFitChange(unsigned int index);

	/// Magic waist
	const TargetBeam* targetBeam() const { return &m_targetBeam; }
	void setTargetBeam(const TargetBeam& beam);
	bool magicWaist();
	bool localOptimum();

	/// Register modification callback
	void registerNotify(OpticsBenchNotify* notify);

	/// Debugging
	void printTree();

private:
	/// @todo on demand computing of beam, cavity and sensitity
	void computeBeams(int changedIndex = 0, bool backward = false);
	void emitChange(int startOptics, int endOptics) const;

private:
	double m_wavelength;
	std::vector<Optics*> m_optics;
	std::vector<Beam*> m_beams;
	std::vector<double> m_sensitivity;

	/// Exclusion area
	double m_leftBoundary, m_rightBoundary;

	/// Waist fit
	std::vector<Fit> m_fits;

	/// Magic waist
	TargetBeam m_targetBeam;

	/// Cavity
	Cavity m_cavity;

	/// Optics naming
	std::map<OpticsType, int> m_lastOpticsName;
	std::map<OpticsType, std::string> m_opticsPrefix;

	/// Callback
	std::list<OpticsBenchNotify*> m_notifyList;

	/// @todo it might be possible to remove this
	friend class OpticsFunction;
};

#endif
