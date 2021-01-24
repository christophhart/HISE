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

	data::data(const String& id_)
	{
		info.setId(id_);
		info.setRange({ 0.0, 1.0 });
		info.ok = id_.isNotEmpty();
	}

	data::data(const String& id_, NormalisableRange<double> r)
	{
		info.setId(id_);
		info.setRange(r);
		info.ok = id_.isNotEmpty();
	}

	data::data()
	{

	}

	juce::ValueTree data::createValueTree() const
	{
		ValueTree p(PropertyIds::Parameter);

		RangeHelpers::storeDoubleRange(p, false, info.toRange(), nullptr);

		p.setProperty(PropertyIds::ID, info.getId(), nullptr);
		p.setProperty(PropertyIds::Value, info.defaultValue, nullptr);

		return p;
	}

	data data::withRange(NormalisableRange<double> r)
	{
		data copy(*this);
		copy.info.setRange(r);
		return copy;
	}

	void data::setParameterValueNames(const StringArray& valueNames)
	{
		parameterNames = valueNames;

		if (valueNames.size() > 1)
			setRange({ 0.0, (double)valueNames.size() - 1.0, 1.0 });
	}

	void data::init()
	{
		callback(info.defaultValue);
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



	encoder::encoder(ValueTree& v)
	{
		for (auto c : v)
			items.add(pod(c));
	}

	encoder::encoder(const MemoryBlock& m)
	{
		parseItems(m);
	}

	void encoder::parseItems(const MemoryBlock& mb)
	{
		MemoryInputStream mis(mb, true);

		while (!mis.isExhausted())
		{
			pod item(mis);

			if (item.ok)
				items.add(item);
		}
	}

	MemoryBlock encoder::writeItems()
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

	pod::pod(const ValueTree& v)
	{
		clearParameterName();

		index = v.getParent().indexOf(v);
		ok = setId(v[PropertyIds::ID].toString());

		auto range = RangeHelpers::getDoubleRange(v);
		min = (DataType)range.start;
		max = (DataType)range.end;
		skew = (DataType)range.skew;
		interval = (DataType)range.interval;
		defaultValue = (DataType)v[PropertyIds::Value];
	}

	pod::pod(MemoryInputStream& mis)
	{
		clearParameterName();

		auto safe = mis.readByte();

		if (safe == 91)
		{
			ok = true;
			index = mis.readInt();
			auto s = mis.readString();

			ok = setId(s);

			min = mis.readFloat();
			max = mis.readFloat();
			defaultValue = mis.readFloat();
			skew = mis.readFloat();
			interval = mis.readFloat();
		}
	}

	String pod::toString()
	{
		String s;
		String nl;

		s << "index: " << index << nl;
		s << "id: " << parameterName << nl;
		s << "min: " << min << nl;
		s << "max: " << max << nl;

		return s;
	}

	void pod::setRange(const NormalisableRange<double>& r)
	{
		min = r.start;
		max = r.end;
		interval = r.interval;
		skew = r.skew;
	}

	juce::NormalisableRange<double> pod::toRange() const
	{
		NormalisableRange<double> r;
		r.start = min;
		r.end = max;
		r.skew = skew;
		r.interval = interval;

		return r;
	}

	bool pod::setId(const String& id)
	{
		if (id.isNotEmpty() && isPositiveAndBelow(id.length(), MaxParameterNameLength))
		{
			memcpy(parameterName, id.getCharPointer().getAddress(), id.length());
			return true;
		}
		else
			clearParameterName();

		return false;
	}

	void pod::writeToStream(MemoryOutputStream& b)
	{
		b.writeByte(91);
		b.writeInt(index);

		String id(parameterName);

		b.writeString(id);
		b.writeFloat(min);
		b.writeFloat(max);
		b.writeFloat(defaultValue);
		b.writeFloat(skew);
		b.writeFloat(interval);
	}


}

}
