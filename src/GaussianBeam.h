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

#ifndef GAUSSIANBEAM_H
#define GAUSSIANBEAM_H

#include <complex>

namespace Property
{
enum  Type {BeamPosition = 0, BeamRadius, BeamDiameter, BeamCurvature, BeamGouyPhase, BeamDistanceToWaist,
            BeamParameter, BeamWaist, BeamWaistPosition, BeamRayleigh, BeamDivergence, Index,
            OpticsType, OpticsPosition, OpticsRelativePosition, OpticsProperties, OpticsName,
            OpticsLock, OpticsSensitivity};
}

/**
* This class represents a Gaussian beam, defined by its waist and position
*/
class Beam
{
public:
	/// Default constructor. Produced an invalid Beam. @todo this should disappear
	Beam() {}
	/// Constructor
	Beam(double waist, double waistPosition, double wavelength, double index, double M2);
	/// Construct a Gaussian beam from a given complex beam parapeter @p q at position @p z
	Beam(const std::complex<double>& q, double z, double wavelength, double index, double M2);

public:

	/// @return the waist position
	double waistPosition() const { return m_waistPosition; }
	/// Set the waist position to @p waistPosition
	void setWaistPosition(double waistPosition) { m_waistPosition = waistPosition; }
	/// @return the waist radius at 1/e²
	double waist() const { return m_waist; }
	/// Set the waist radius at 1/e² to @p waist
	void setWaist(double waist) { m_waist = waist; }
	/// @return the beam divergence (half angle)
	double divergence() const;
	/// Set the beam divergence to @p divergence
	void setDivergence(double divergence);
	/// @return the Rayleigh range
	double rayleigh() const;
	/// Set the rayleigh range to @p rayleigh
	void setRayleigh(double rayleigh);
	/// @return the beam wavelength
	double wavelength() const { return m_wavelength; }
	/// Set the beam wavelength to @p wavelength
	void setWavelength(double wavelength) { m_wavelength = wavelength; }
	/// @return the index of the medium in which the beam propagates
	double index() const { return m_index; }
	/// Set the index of the medium in which the beam propagates to @p index
	void setIndex(double index) { m_index = index; }
	/// @return the beam quality factor M²
	double M2() const { return m_M2; }
	/// Set the beam quality factor M² to @p M2
	void setM2(double M2) { m_M2 = M2; }

	// Position dependent properties
	/// @return the beam radius at 1/e² at position @p z
	double radius(double z) const;
	/// @return the derivative of the beam radius at 1/e² at position @p z
	double radiusDerivative(double z) const;
	/// @return the second derivative of the beam radius at 1/e² at position @p z
	double radiusSecondDerivative(double z) const;
	/// @return the curvature radius of the beam at position @p z
	double curvature(double z) const;
	/// @return the Gouy phase at position @p z
	double gouyPhase(double z) const;
	/// @return the beam complex parameter \f$ q = (z-z_w) + iz_0 \f$ at position @p z
	std::complex<double> q(double z) const;

	/**
	* Compute the intensity overlap between beams @p beam1 and @p beam2 at position @p z
	* This overlap does not depend on @p z if both beams have the same wavelength,
	* hence the default value for z
	* @todo this is only implented for beams of identical wavelength
	*/
	static double overlap(const Beam& beam1, const Beam& beam2, double z = 0.);

private:
	inline double zred(double z) const { return (z - waistPosition())/rayleigh(); }

private:
	double m_waist;
	double m_waistPosition;
	double m_wavelength;
	double m_index;
	double m_M2;
};

/**
* Target beam
*/
class TargetBeam : public Beam
{
public:
	TargetBeam() : Beam() {}
	TargetBeam(double waist, double waistPosition, double wavelength, double index, double M2)
		: Beam(waist, waistPosition, wavelength, index, M2)
		, m_overlapCriterion(true)
		, m_minOverlap(0.98)
		, m_waistTolerance(0.05)
		, m_positionTolerance(0.1) {}

public:
	bool overlapCriterion() const { return m_overlapCriterion; }
	void setOverlapCriterion(double overlapCriterion) { m_overlapCriterion =  overlapCriterion; }
	double minOverlap() const { return m_minOverlap; }
	void setMinOverlap(double minOverlap) { m_minOverlap = minOverlap; }
	double waistTolerance() const { return m_waistTolerance; }
	void setWaistTolerance(double waistTolerance) { m_waistTolerance = waistTolerance; }
	double positionTolerance() const { return m_positionTolerance; }
	void setPositionTolerance(double positionTolerance) { m_positionTolerance = positionTolerance; }

private:
	// True for tolerance on overlap, false for tolerance on waist and position
	bool m_overlapCriterion;
	double m_minOverlap;
	double m_waistTolerance;
	double m_positionTolerance;
};

#endif
