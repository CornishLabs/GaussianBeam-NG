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

#include "GaussianBeam.h"
#include "Utils.h"

#include <iostream>
#include <cmath>

using namespace std;

Beam::Beam(double waist, double waistPosition, double wavelength, double index, double M2)
	: m_waist(waist)
	, m_waistPosition(waistPosition)
	, m_wavelength(wavelength)
	, m_index(index)
	, m_M2(M2)
{
}

Beam::Beam(const complex<double>& q, double z, double wavelength, double index, double M2)
	: m_wavelength(wavelength)
	, m_index(index)
	, m_M2(M2)
{
	const double z0 = q.imag();
	m_waist = sqrt(z0*wavelength*m_M2/(m_index*M_PI));
	m_waistPosition = z - q.real();
}

double Beam::divergence() const
{
	if (m_waist == 0.)
		return 0.;

	return atan(m_wavelength*m_M2/(m_index*M_PI*m_waist));
}

void Beam::setDivergence(double divergence)
{
	if ((divergence > 0.) && (divergence < M_PI/2.))
		m_waist = m_wavelength*m_M2/(m_index*M_PI*tan(divergence));
}

double Beam::rayleigh() const
{
	if (m_wavelength == 0.)
		return 0.;

	return m_index*M_PI*sqr(m_waist)/(m_wavelength*m_M2);
}

void Beam::setRayleigh(double rayleigh)
{
	if (rayleigh > 0.)
		m_waist = sqrt(rayleigh*m_wavelength*m_M2/(m_index*M_PI));
}

double Beam::radius(double z) const
{
	return waist()*sqrt(1. + sqr(zred(z)));
}

double Beam::radiusDerivative(double z) const
{
	return waist()/rayleigh()/sqrt(1. + 1./sqr(zred(z)));
}

double Beam::radiusSecondDerivative(double z) const
{
	return waist()/sqr(rayleigh())/pow(1. + sqr(zred(z)), 1.5);
}

double Beam::curvature(double z) const
{
	return (z - waistPosition())*(1. + 1./sqr(zred(z)));
}

double Beam::gouyPhase(double z) const
{
	return atan(zred(z));
}

complex<double> Beam::q(double z) const
{
	return complex<double>(z - waistPosition(), rayleigh());
}

double Beam::overlap(const Beam& beam1, const Beam& beam2, double z)
{
//	double w1 = beam1.radius(z);
//	double w2 = beam2.radius(z);
//	double w12 = sqr(beam1.radius(z));
//	double w22 = sqr(beam2.radius(z));
//	double k1 = 2.*M_PI/beam1.wavelength();
//	double k2 = 2.*M_PI/beam2.wavelength();
//	double R1 = beam1.curvature(z);
//	double R2 = beam2.curvature(z);
	double zred1 = beam1.zred(z);
	double zred2 = beam2.zred(z);
	double rho = sqr(beam1.radius(z)/beam2.radius(z));

	//double eta = 4./sqr(w1*w2)/(sqr(1./sqr(w1) + 1./sqr(w2)) + sqr((k1/R1 - k2/R2)/2.));
	//double eta = 4./(w12*w22)/(sqr(1./w12 + 1./w22) + sqr(zred1/w12 - zred2/w22));
	double eta = 4.*rho/(sqr(1. + rho) + sqr(zred1 - zred2*rho));

//	cerr << "Coupling = " << eta << " // " << zred1 << " " << zred2 << " " << rho << endl;

	return eta;
}
