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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

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

namespace parameter
{

data::data(const String& id_) :
	id(id_)
{

}

data::data(const String& id_, NormalisableRange<double> r) :
	id(id_),
	range(r)
{

}

juce::ValueTree data::createValueTree() const
{
	ValueTree p(PropertyIds::Parameter);

	RangeHelpers::storeDoubleRange(p, false, range, nullptr);

	p.setProperty(PropertyIds::ID, id, nullptr);
	p.setProperty(PropertyIds::Value, defaultValue, nullptr);

	return p;
}

data data::withRange(NormalisableRange<double> r)
{
	data copy(*this);
	copy.range = r;
	copy.isUsingRange = !RangeHelpers::isIdentity(r);
	return copy;
}

void data::setParameterValueNames(const StringArray& valueNames)
{
	parameterNames = valueNames;

	if (valueNames.size() > 1)
		range = { 0.0, (double)valueNames.size() - 1.0, 1.0 };
}

void data::init()
{
	jassertfalse; // check this
	dbNew(defaultValue);
}

dynamic::dynamic() :
	f(nullptr),
	obj(nullptr)
{

}

void dynamic::call(double v) const
{
	if (f != nullptr && obj != nullptr)
		f(getObjectPtr(), v);
}

void dynamic::referTo(void* p, Function f_)
{
	f = f_;
	obj = p;
}

void dynamic::operator()(double v) const
{
	call(v);
}

void* dynamic::getObjectPtr() const
{
	return obj;
}

}


ParameterEncoder::ParameterEncoder(ValueTree& v)
{
	for (auto c : v)
		items.add(ParameterEncoder::Item(c));
}

ParameterEncoder::ParameterEncoder(const MemoryBlock& m)
{
	parseItems(m);
}

void ParameterEncoder::parseItems(const MemoryBlock& mb)
{
	MemoryInputStream mis(mb, true);

	while (!mis.isExhausted())
	{
		Item item(mis);

		if (item.id.isNotEmpty())
			items.add(item);
	}
}

MemoryBlock ParameterEncoder::writeItems()
{
	MemoryBlock data;

	MemoryOutputStream mos(data, false);

	for (auto& d : items)
		d.writeToStream(mos);

	int numToPad = 4 - mos.getPosition() % 4;

	for (int i = 0; i < numToPad; i++)
		mos.writeByte(0);

	mos.flush();

	return data;
}

ParameterEncoder::Item::Item(const ValueTree& v)
{
	index = v.getParent().indexOf(v);
	id = v[PropertyIds::ID].toString();

	auto range = RangeHelpers::getDoubleRange(v);

	min = (DataType)range.start;
	max = (DataType)range.end;
	skew = (DataType)range.skew;
	interval = (DataType)range.interval;
	defaultValue = (DataType)v[PropertyIds::Value];
}

ParameterEncoder::Item::Item(MemoryInputStream& mis)
{
	index = mis.readInt();
	id = mis.readString();
	min = mis.readFloat();
	max = mis.readFloat();
	defaultValue = mis.readFloat();
	skew = mis.readFloat();
	interval = mis.readFloat();
}

String ParameterEncoder::Item::toString()
{
	String s;
	String nl;

	s << "index: " << index << nl;
	s << "id: " << id << nl;
	s << "min: " << min << nl;
	s << "max: " << max << nl;

	return s;
}

void ParameterEncoder::Item::writeToStream(MemoryOutputStream& b)
{
	b.writeInt(index);
	b.writeString(id);
	b.writeFloat(min);
	b.writeFloat(max);
	b.writeFloat(defaultValue);
	b.writeFloat(skew);
	b.writeFloat(interval);
}




}
