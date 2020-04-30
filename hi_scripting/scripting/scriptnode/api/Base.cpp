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








ParameterDataImpl::ParameterDataImpl(const String& id_) :
	id(id_)
{

}

ParameterDataImpl::ParameterDataImpl(const String& id_, NormalisableRange<double> r) :
	id(id_),
	range(r)
{

}

juce::ValueTree ParameterDataImpl::createValueTree() const
{
	ValueTree p(PropertyIds::Parameter);

	RangeHelpers::storeDoubleRange(p, false, range, nullptr);

	p.setProperty(PropertyIds::ID, id, nullptr);
	p.setProperty(PropertyIds::Value, defaultValue, nullptr);

	return p;
}

ParameterDataImpl ParameterDataImpl::withRange(NormalisableRange<double> r)
{
	ParameterDataImpl copy(*this);
	copy.range = r;
	copy.isUsingRange = !RangeHelpers::isIdentity(r);
	return copy;
}

#if 0
void ParameterDataImpl::operator()(double newValue) const
{
	if (isUsingRange)
		callWithRange(newValue);
	else
		callUnscaled(newValue);
}
#endif

void ParameterDataImpl::setParameterValueNames(const StringArray& valueNames)
{
	parameterNames = valueNames;

	if(valueNames.size() > 1)
		range = { 0.0, (double)valueNames.size() - 1.0, 1.0 };
}

void HiseDspBase::ParameterData::init()
{
	db(defaultValue);
}

}

