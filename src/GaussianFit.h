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

#ifndef GAUSSIANFIT_H
#define GAUSSIANFIT_H

#include "GaussianBeam.h"

#include <vector>

/**
* Type of data measured by the user for beam fitting
*/
enum FitDataType {Radius_e2 = 0, Diameter_e2, standardDeviation, FWHM, HWHM};

/**
* Find the waist radius and position for a given set of radii measurement of a Gaussian beam
* It fits the given data with a linear fit, and finds the only hyperbola
* that is tangent to the resulting line
*/
class Fit
{
public:
	/// Constructor
	Fit(int nData = 0, std::string name = "");

public:
	/// @return the number of points in the fit
	int size() const { return m_positions.size(); }
	/// @return the number of points with non zero measured value in the fit
	int nonZeroSize() const;
	/// @return the user name given to the fit
	std::string name() const { return m_name; }
	/// Set the user name of the fit
	void setName(std::string name) { m_name = name; }
	/// @return the type of measured data
	FitDataType dataType() const { return m_dataType; }
	/// Set the type of mesured data
	void setDataType(FitDataType dataType) { m_dataType = dataType; }
	/// @return the RGB color associated to the fit
	unsigned int color() const { return m_color; }
	/// Set the RGB color accociated to the fit
	void setColor(unsigned int color) { m_color = color; }
	/// @return the position of date number @p index
	double position(unsigned int index) const { return m_positions[index]; }
	/// @return the measured value number @p index
	double value(unsigned int index) const { return m_values[index]; }
	/// @return the measured beam radius at 1/e² computed from the measured data
	double radius(unsigned int index) const;
	/// Add a data point @p value , measured at position @p position to the fit
	void addData(double position, double value);
	/// Set data point number @p index to @p value at position @p position
	void setData(unsigned int index, double position, double value);
	/// Remove data point number @p index
	void removeData(unsigned int index);
	/// Remove all data in the fit
	void clear();
	/// @return the best beam adjusted to the data points
	const Beam& beam(double wavelength) const;
	/// @return the correlation coefficient of the fit
	double rho2(double wavelength) const;

private:
	void fitBeam(double wavelength) const;

private:
	std::string m_name;
	FitDataType m_dataType;
	std::vector<double> m_positions;
	std::vector<double> m_values;
	unsigned int m_color;

	mutable bool m_dirty;
	mutable Beam m_beam;
	mutable double m_rho2;
	mutable double m_lastWavelength;

	static int m_fitCount;
};

#endif
