/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace jdsp
{

void jcompressor::createParameters(ParameterDataList& d)
{
	{
		parameter::data p("Treshold", { -100.0, 0.0 });
		p.setSkewForCentre(-12.0);
		registerCallback<0>(p);
		p.setDefaultValue(0.0);
		d.add(p);
	}
	{
		parameter::data p("Ratio", { 1.0, 32.0 });
		p.setSkewForCentre(8.0);
		registerCallback<1>(p);
		p.setDefaultValue(1.0);
		d.add(p);
	}
	{
		parameter::data p("Attack", { 0.0, 300.0 });
		p.setSkewForCentre(50.0);
		registerCallback<2>(p);
		p.setDefaultValue(1.0);
		d.add(p);
	}
	{
		parameter::data p("Release", { 0.0, 300.0 });
		p.setSkewForCentre(50.0);
		registerCallback<3>(p);
		p.setSkewForCentre(10.0);
		p.setDefaultValue(100.0);
		d.add(p);
	}
}

void jchorus::createParameters(ParameterDataList& d)
{
	{
		parameter::data p("CentreDelay", { 0.0, 100.0 });
		registerCallback<0>(p);
		p.setDefaultValue(7.0);
		d.add(p);
	}
	{
		parameter::data p("Depth", { 0.0, 1.0 });
		registerCallback<1>(p);
		p.setDefaultValue(0.25);
		d.add(p);
	}
	{
		parameter::data p("Feedback", { -1.0, 1.0 });
		registerCallback<2>(p);
		p.setDefaultValue(0.0);
		d.add(p);
	}
	{
		parameter::data p("Rate", { 0.0, 100.0 });
		registerCallback<3>(p);
		p.setSkewForCentre(10.0);
		p.setDefaultValue(1.0);
		d.add(p);
	}
	{
		parameter::data p("Mix", { 0.0, 1.0 });
		registerCallback<4>(p);
		p.setDefaultValue(0.5);
		d.add(p);
	}
}

void jlinkwitzriley::createParameters(ParameterDataList& d)
{
	{
		parameter::data p("Frequency", { 20.0, 20000.0 });
		registerCallback<0>(p);
		p.setSkewForCentre(1000.0);
		p.setDefaultValue(2000.0);
		d.add(p);
	}
	{
		parameter::data p("Type");
		registerCallback<1>(p);
		p.setParameterValueNames({ "LP", "HP", "AP" });
		p.setDefaultValue(0.0);
		d.add(p);
	}
}







} // namespace scriptnode

}