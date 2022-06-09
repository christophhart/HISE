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


namespace parameter
{

struct dynamic_base: public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<dynamic_base>;
	using List = ReferenceCountedArray<dynamic_base>;

	PARAMETER_SPECS(ParameterType::Single, 1);

	dynamic_base(parameter::dynamic& obj_);;

	dynamic_base();

	virtual ~dynamic_base() {};

	virtual void call(double value)
	{
		setDisplayValue(value);
        
        if(obj != nullptr && f)
            f(obj, lastValue);
	}

	static dynamic_base::Ptr createFromConnectionTree(const ValueTree& v, parameter::dynamic& callback, bool allowRange=true);

	parameter::dynamic::Function f;
	void* obj;

	virtual double getDisplayValue() const 
	{ 
		return lastValue; 
	}

	InvertableParameterRange getRange() const { return range; }

	virtual void updateRange(const ValueTree& v)
	{
		range = RangeHelpers::getDoubleRange(v);
		range.checkIfIdentity();
	}

protected:

	void setDisplayValue(double v)
	{
		lastValue = v;
	}

private:

	InvertableParameterRange range;
	double lastValue = 0.0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_base);
};

struct dynamic_base_holder: public dynamic_base
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	void call(double v) final override
	{
		setDisplayValue(v);

		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		if (base != nullptr)
			base->call(v);
	}

	virtual void updateRange(const ValueTree& v) override
	{
		// Do nothing here because the holder is not supposed to 
		// change the range?
	}

	virtual double getDisplayValue() const
	{
		if (base != nullptr)
			return base->getDisplayValue();

		return dynamic_base::getDisplayValue();
	}

	virtual void setParameter(NodeBase* n, dynamic_base::Ptr b)
	{
		dynamic_base::Ptr old = base;

		auto oldValue = getDisplayValue();

		{
			SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);
			base = b;
		}

		call(oldValue);
	}

	bool isConnected() const
	{
		// this should always return true to allow setting the display value
		// when there's no connection
		return true;
	}

	dynamic_base::Ptr base;
	
protected:

	mutable SimpleReadWriteLock connectionLock;
};



template <bool ScaleInput> struct dynamic_chain : public dynamic_base
{
	static constexpr int NumMaxSlots = 32;

	using Ptr = ReferenceCountedObjectPtr<dynamic_chain>;

	dynamic_chain() :
		dynamic_base()
	{
		for (int i = 0; i < NumMaxSlots; i++)
			unscaleValue[i] = false;
	};

	bool isEmpty() const { return targets.isEmpty(); }

	void addParameter(dynamic_base::Ptr p, bool isUnscaled)
	{
		jassert(p != nullptr);
		
		if (p != nullptr)
		{
			jassert(targets.size() < NumMaxSlots);

			unscaleValue[targets.size()] = isUnscaled;
			targets.add(p);
			
		}
	}

	void call(double v)
	{
		setDisplayValue(v);
		auto nv = ScaleInput ? getRange().convertTo0to1(v, true) : v;

		int index = 0;

		for (auto& t : targets)
		{
			auto isUnscaled = (double)unscaleValue[index++];
			auto tv = ScaleInput ? t->getRange().convertFrom0to1(nv, true) : v;
			auto valueToSend = isUnscaled * v + (1.0 - isUnscaled) * tv;

			t->call(valueToSend);
		}
	}

	dynamic_base::List targets;
	bool unscaleValue[NumMaxSlots];
	
};





}

}
