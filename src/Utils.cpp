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

#include "Utils.h"

using namespace std;

/////////////////////////////////////////////////
// Extend vector operations

vector<double> operator+(const vector<double>& v, double r)
{
	vector<double> result = v;
	for (vector<double>::iterator it = result.begin(); it != result.end(); it++)
		(*it) += r;
	return result;
}

vector<double> operator-(const vector<double>& v, double r)
{
	vector<double> result = v;
	for (vector<double>::iterator it = result.begin(); it != result.end(); it++)
		(*it) -= r;
	return result;
}

vector<double> operator*(const vector<double>& v, double r)
{
	vector<double> result = v;
	for (vector<double>::iterator it = result.begin(); it != result.end(); it++)
		(*it) *= r;
	return result;
}

vector<double> operator/(const vector<double>& v, double r)
{
	vector<double> result = v;
	for (vector<double>::iterator it = result.begin(); it != result.end(); it++)
		(*it) /= r;
	return result;
}

vector<double> operator+(const vector<double>& v1, const vector<double>& v2)
{
	vector<double> result = v1;
	for (unsigned int i = 0; i < ::min(v1.size(), v2.size()); i++)
		result[i] += v2[i];
	return result;
}

vector<double> operator-(const vector<double>& v1, const vector<double>& v2)
{
	vector<double> result = v1;
	for (unsigned int i = 0; i < ::min(v1.size(), v2.size()); i++)
		result[i] -= v2[i];
	return result;
}

double scalar(const vector<double>& v1, const vector<double>& v2)
{
	double result = 0;

	for (unsigned int i = 0; i < ::min(v1.size(), v2.size()); i++)
		result += v1[i]*v2[i];

	return result;
}
