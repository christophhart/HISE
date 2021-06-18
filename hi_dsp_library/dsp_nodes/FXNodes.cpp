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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace fx
{

reverb::reverb()
{
	auto p = r.getParameters();
	p.dryLevel = 0.0f;
	r.setParameters(p);
}

void reverb::initialise(NodeBase* )
{
	
}

void reverb::prepare(PrepareSpecs ps)
{
	r.setSampleRate(ps.sampleRate);
}

void reverb::reset() noexcept
{
	r.reset();
}

void reverb::createParameters(ParameterDataList& data)
{
	{
		DEFINE_PARAMETERDATA(reverb, Damping);
		p.setDefaultValue(0.5);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(reverb, Width);
		p.setDefaultValue(0.5);
		data.add(std::move(p));
	}

	{
		DEFINE_PARAMETERDATA(reverb, Size);
		p.setDefaultValue(0.5);
		data.add(std::move(p));
	}
}

void reverb::setDamping(double newDamping)
{
	auto p = r.getParameters();
	p.damping = jlimit(0.0f, 1.0f, (float)newDamping);
	r.setParameters(p);
}

void reverb::setWidth(double width)
{
	auto p = r.getParameters();
	p.damping = jlimit(0.0f, 1.0f, (float)width);
	r.setParameters(p);
}

void reverb::setSize(double size)
{
	auto p = r.getParameters();
	p.roomSize = jlimit(0.0f, 1.0f, (float)size);
	r.setParameters(p);
}

}
}
