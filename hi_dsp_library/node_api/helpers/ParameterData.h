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


struct RangeHelpers
{
	static bool isRangeId(const Identifier& id);

	static bool isIdentity(NormalisableRange<double> d);

	static Array<Identifier> getRangeIds();

	/** Checks if the MinValue and MaxValue are inverted, sets the property Inverted and returns the result. */
	static bool checkInversion(ValueTree& data, ValueTree::Listener* listenerToExclude, UndoManager* um);

	/** Creates a NormalisableRange from the ValueTree properties. It doesn't update the Inverted property. */
	static NormalisableRange<double> getDoubleRange(const ValueTree& t);

	static void storeDoubleRange(ValueTree& d, bool isInverted, NormalisableRange<double> r, UndoManager* um);

	static bool isEqual(const NormalisableRange<double>& r1, const NormalisableRange<double>& r2)
	{
		return  r1.start == r2.start &&
				r1.end == r2.end &&
				r1.skew == r2.skew &&
				r1.interval == r2.interval;
	}
};



namespace parameter
{

#define PARAMETER_SPECS(parameterType, parameterAmount) static constexpr int size = parameterAmount; static constexpr ParameterType type = parameterType; static constexpr bool isRange() { return false; };

enum class ParameterType
{
	Single,
	Chain,
	List
};

/** This class wraps one of the other classes into an opaque pointer and is used by the data class. */
struct dynamic
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	using Function = void(*)(void*, double);

	static constexpr int MaxSize = 32;

	dynamic();

	template <typename T> dynamic& operator=(T&& other)
	{
		obj = other.getObjectPtr();
		f = (Function)(T::callStatic);

		return *this;
	}

	void call(double v) const;

	void referTo(void* p, Function f_);

	void operator()(double v) const;

	void* getObjectPtr() const;

	Function getFunction() { return f; }

private:

	void* obj = nullptr;
	Function f = nullptr;
};


struct data // better name: parameter::data
{
	data(const String& id_);;
	data(const String& id_, NormalisableRange<double> r);
	data withRange(NormalisableRange<double> r);

	ValueTree createValueTree() const;

	void setDefaultValue(double newDefaultValue)
	{
		defaultValue = newDefaultValue;
	}

	operator bool() const
	{
		return id.isNotEmpty();
	}

	void setParameterValueNames(const StringArray& valueNames);
	void init();

	String id;
	NormalisableRange<double> range;
	double defaultValue = 0.0;
	dynamic dbNew;

	StringArray parameterNames;

	double lastValue = 0.0;
	bool isUsingRange = true;
};

}


using ParameterDataList = Array<parameter::data>;



}
