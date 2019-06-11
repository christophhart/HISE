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








bool RangeHelpers::isRangeId(const Identifier& id)
{
	using namespace PropertyIds;
	return id == MinValue || id == MaxValue || id == StepSize || id == SkewFactor || id == Converter || id == OpType;
}


bool RangeHelpers::isIdentity(NormalisableRange<double> d)
{
	if (d.start == 0.0 && d.end == 1.0 && d.skew == 1.0)
		return true;

	return false;
}


juce::Array<juce::Identifier> RangeHelpers::getRangeIds()
{
	using namespace PropertyIds;

	return { MinValue, MaxValue, StepSize, SkewFactor, Converter, OpType };
}

bool RangeHelpers::checkInversion(ValueTree& data, ValueTree::Listener* listenerToExclude, UndoManager* um)
{
	using namespace PropertyIds;

	if (data[MinValue] > data[MaxValue])
	{
		data.setPropertyExcludingListener(listenerToExclude, Inverted, true, um);
		return true;
	}
	else
		data.setPropertyExcludingListener(listenerToExclude, Inverted, false, um);
	return false;
}

void RangeHelpers::storeDoubleRange(ValueTree& d, bool isInverted, NormalisableRange<double> r, UndoManager* um)
{
	using namespace PropertyIds;

	d.setProperty(Inverted, isInverted, um);
	d.setProperty(MinValue, r.start, um);
	d.setProperty(MaxValue, r.end, um);
	d.setProperty(LowerLimit, r.start, um);
	d.setProperty(UpperLimit, r.end, um);
	d.setProperty(StepSize, r.interval, um);
	d.setProperty(SkewFactor, r.skew, um);
}

juce::NormalisableRange<double> RangeHelpers::getDoubleRange(const ValueTree& t)
{
	using namespace PropertyIds;

	NormalisableRange<double> r;

	auto minValue = (double)t.getProperty(MinValue, 0.0);
	auto maxValue = (double)t.getProperty(MaxValue, 1.0);

	if (minValue == maxValue)
		maxValue += 0.01;

	bool inverted = (double)t.getProperty(Inverted, false) || (minValue > maxValue);

	r.start = inverted ? maxValue : minValue;
	r.end = inverted ? minValue : maxValue;
	r.interval = jlimit(0.0001, 1.0, (double)t.getProperty(StepSize, 0.01));
	r.skew = jlimit(0.001, 100.0, (double)t.getProperty(SkewFactor, 1.0));

	return r;
}

}

