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

struct InvertableParameterRange
{
	InvertableParameterRange(double start, double end) :
		rng(start, end),
		inv(false)
	{};

	InvertableParameterRange() :
		rng(0.0, 1.0),
		inv(false)
	{};

	bool operator==(const InvertableParameterRange& other) const;

	InvertableParameterRange(const ValueTree& v);

	void store(ValueTree& v, UndoManager* um);

	InvertableParameterRange(double start, double end, double interval, double skew=1.0) :
		rng(start, end, interval, skew),
		inv(false)
	{};

	double convertFrom0to1(double input, bool applyInversion) const;

	InvertableParameterRange inverted() const
	{
		auto copy = *this;
		copy.inv = !copy.inv;
		return copy;
	}

	double convertTo0to1(double input, bool applyInversion) const;

	Range<double> getRange() const
	{
		return rng.getRange();
	}

	double snapToLegalValue(double v) const
	{
		return rng.snapToLegalValue(v);
	}

	void setSkewForCentre(double value)
	{
		rng.setSkewForCentre(value);
	}

	void checkIfIdentity();

	juce::NormalisableRange<double> rng;
	bool inv = false;

private:

	bool isIdentity = false;
};

struct RangeHelpers
{
	static bool isRangeId(const Identifier& id);

	static bool isBypassIdentity(InvertableParameterRange d);

	static bool isIdentity(InvertableParameterRange d);

	static void removeRangeProperties(ValueTree v, UndoManager* um);

	static Array<Identifier> getRangeIds(bool includeValue=false);

	/** Checks if the range should be inverted. */
	static bool isInverted(const ValueTree& v);

	/** Creates a NormalisableRange from the ValueTree properties. It doesn't update the Inverted property. */
	static InvertableParameterRange getDoubleRange(const ValueTree& t);

	static void storeDoubleRange(ValueTree& d, InvertableParameterRange r, UndoManager* um);

	static bool equalsWithError(const InvertableParameterRange& r1, const InvertableParameterRange& r2, double maxError);

	static bool isEqual(const InvertableParameterRange& r1, const InvertableParameterRange& r2)
	{
		return  r1.rng.start == r2.rng.start &&
				r1.rng.end == r2.rng.end &&
				r1.rng.skew == r2.rng.skew &&
				r1.rng.interval == r2.rng.interval &&
				r1.inv == r2.inv;
	}

	static Array<Identifier> getHiddenIds()
	{
		return { PropertyIds::NodeId, PropertyIds::ParameterId, Identifier("Enabled") };
	}
};



namespace parameter
{

#define PARAMETER_SPECS(parameterType, parameterAmount) static constexpr int size = parameterAmount; static constexpr ParameterType type = parameterType; static constexpr bool isRange() { return false; };

enum class ParameterType
{
	Single,
	Chain,
	List,
	Clone,
	CloneChain
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

struct pod
{
	static constexpr int MaxParameterNameLength = 16;

	pod()
	{
		clearParameterName();
	};

	pod(MemoryInputStream& mis);

	pod(const ValueTree& v);

	String toString();

	String getId() const { return String(parameterName); }

	void clearParameterName()
	{
		memset(parameterName, 0, MaxParameterNameLength);
	}

	void setRange(const InvertableParameterRange& r);

	InvertableParameterRange toRange() const;

	using DataType = float;
	int index = -1;

	char parameterName[MaxParameterNameLength];

	bool setId(const String& id);

	DataType min = DataType(0);
	DataType max = DataType(1);
	DataType defaultValue = DataType(0);
	DataType skew = DataType(1);
	DataType interval = DataType(0);

	bool inverted = false;
	bool ok = false;

	void writeToStream(MemoryOutputStream& b);
};

/** Used by the scriptnode interpreter. */
struct data
{
	data();
	data(const String& id_);;
	data(const String& id_, InvertableParameterRange r);
	data withRange(InvertableParameterRange r);

	ValueTree createValueTree() const;

	void setRange(const InvertableParameterRange& r)
	{
		info.setRange(r);
	}

	InvertableParameterRange toRange() const
	{
		return info.toRange();
	}
	
	void setSkewForCentre(double midPoint)
	{
		auto r = info.toRange();
		r.rng.setSkewForCentre(midPoint);
		info.skew = r.rng.skew;
	}

	void setDefaultValue(double newDefaultValue)
	{
		info.defaultValue = newDefaultValue;
	}

	operator bool() const
	{
		return info.ok;
	}

	void setParameterValueNames(const StringArray& valueNames);
	void init();

	pod info;
	dynamic callback;

	StringArray parameterNames;
};

/** A Parameter encoder encodes / decodes the parameter data of a node

	The parameter data of a node is converted from a ValueTree to a
	lightweight data structure and can be emitted as C++ literal array
	of ints that can be read by the JIT compiler and plain C++.

*/
struct encoder
{
	// Just a node that shows how it must look so you can feed it to ParameterEncoder::fromNode
	struct ExampleNode
	{
		struct metadata
		{
			SNEX_METADATA_ENCODED_PARAMETERS(9)
			{
				0x00000000, 0x61726150, 0x6574656D, 0x00003172,
					0x00000000, 0x00422000, 0x003F0000, 0x0A3F8000,
					0x003C23D7
			};
		};
	};

	template <typename NodeClass> static encoder fromNode()
	{
        typename NodeClass::MetadataClass n;

		MemoryOutputStream tm;

		for (auto& s : n.encodedParameters)
			tm.writeShort(static_cast<uint16>(s));

		auto data = tm.getMemoryBlock();

		return encoder(data);
	}

	encoder(ValueTree& v);

	encoder(const MemoryBlock& m);

	pod* begin() { return items.begin(); }
	pod* end() { return items.end();}

	const pod* begin() const { return items.begin(); }
	const pod* end() const { return items.end(); }

	MemoryBlock writeItems();

private:

	void parseItems(const MemoryBlock& mb);


	Array<pod> items;
};

}


using ParameterDataList = Array<parameter::data>;







}
