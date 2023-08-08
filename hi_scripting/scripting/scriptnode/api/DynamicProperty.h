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

	virtual ~dynamic_base();;

	virtual void call(double value);

	static dynamic_base::Ptr createFromConnectionTree(const ValueTree& v, parameter::dynamic& callback, bool allowRange=true);

	parameter::dynamic::Function f;
	void* obj;

	virtual double getDisplayValue() const;

	virtual InvertableParameterRange getRange() const;

	virtual void updateRange(const ValueTree& v);

protected:

	void setDisplayValue(double v);

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

	virtual InvertableParameterRange getRange() const final override
	{
		if (base != nullptr)
			return base->getRange();

		return {};
	}

	virtual void updateRange(const ValueTree& v) override
	{
        if(!allowForwardToParameter)
        {
            // If we do not allow forwarding of parameters
            // we need to at least forward the range call
            if(base != nullptr)
                base->updateRange(v);
        }
        else
        {
            // Do nothing here because the holder is not supposed to
            // change the range.
        }
	}

	virtual double getDisplayValue() const
	{
		if (base != nullptr)
			return base->getDisplayValue();

		return dynamic_base::getDisplayValue();
	}

    /** If this parameter is assigned to another dynamic_base_holder, it will "bypass" this
        parameter and directly connect the other holder to this target. If you don't want that,
        call this function with `false`.
    */
    void setAllowForwardToParameter(bool forwardParameter)
    {
        allowForwardToParameter = forwardParameter;
    }
    
	virtual void setParameter(NodeBase* n, dynamic_base::Ptr b)
	{
		dynamic_base::Ptr old = base;

		if (auto s = dynamic_cast<dynamic_base_holder*>(b.get()))
        {
            if(s->allowForwardToParameter)
                b = s->base;
        }
			

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
    
private:
    
    bool allowForwardToParameter = true;
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
