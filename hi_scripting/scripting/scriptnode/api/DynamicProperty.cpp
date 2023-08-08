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

namespace scriptnode
{
using namespace juce;
using namespace hise;

namespace parameter
{

dynamic_base::dynamic_base(parameter::dynamic& obj_) :
	obj(obj_.getObjectPtr()),
	f(obj_.getFunction())
{

}

dynamic_base::dynamic_base() :
	obj(nullptr),
	f(nullptr)
{

}

dynamic_base::~dynamic_base()
{}

void dynamic_base::call(double value)
{
	setDisplayValue(value);
        
	if(obj != nullptr && f)
		f(obj, lastValue);
}

double dynamic_base::getDisplayValue() const
{ 
	return lastValue; 
}

InvertableParameterRange dynamic_base::getRange() const
{ return range; }

void dynamic_base::updateRange(const ValueTree& v)
{
	range = RangeHelpers::getDoubleRange(v);
	range.checkIfIdentity();
}

void dynamic_base::setDisplayValue(double v)
{
	lastValue = v;
}


scriptnode::parameter::dynamic_base::Ptr dynamic_base::createFromConnectionTree(const ValueTree& c, parameter::dynamic& callback, bool allowRange)
{
	dynamic_base::Ptr b = new dynamic_base(callback);
	b->updateRange(c);

#if 0

	auto r = RangeHelpers::getDoubleRange(c);
	auto e = c[PropertyIds::Expression].toString();
	auto inv = RangeHelpers::isInverted(c);

	if (e.isNotEmpty())
	{
#if USE_BACKEND
		b = new parameter::dynamic_expression(callback, new snex::JitExpression(e));
#else
		b = new parameter::dynamic_base(callback);
#endif
	}
	else if (inv && allowRange)
	{
		if (RangeHelpers::isIdentity(r))
			b = new parameter::dynamic_inv(callback);
		else if (r.rng.interval > 0.01)
			b = new parameter::dynamic_step_inv(callback, r);
		else
			b = new parameter::dynamic_from0to1_inv(callback, r);
	}
	else if (allowRange && !RangeHelpers::isIdentity(r))
	{
		if (r.rng.interval > 0.01)
			b = new parameter::dynamic_step(callback, r);
		else
			b = new parameter::dynamic_from0to1(callback, r);
	}
	else
		b = new parameter::dynamic_base(callback);
#endif

	if (c.hasProperty(PropertyIds::Value))
	{
		b->lastValue = c[PropertyIds::Value];
	}


	return b;
}





}

}

