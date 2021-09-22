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

using DupliListener = wrap::duplicate_sender::Listener;

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
		f(obj, lastValue);
	}

	virtual void callWithDuplicateIndex(int index, double value) 
	{

	}

	virtual void setDelta(double v)
	{

	}

	virtual int getNumDuplicates() const { return 1; }

	static dynamic_base::Ptr createFromConnectionTree(const ValueTree& v, parameter::dynamic& callback, bool allowRange=true);

	parameter::dynamic::Function f;
	void* obj;

	virtual double getDisplayValue() const 
	{ 
		return lastValue; 
	}

	InvertableParameterRange getRange() const { return range; }

	virtual void setParentNumVoiceListener(DupliListener* ) {}

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

	virtual int getNumDuplicates() const
	{
		jassertfalse;
		return 0;
	}

	void setDelta(double v) final override
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		if (base != nullptr)
			base->setDelta(v);
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

		{
			SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);
			base = b;
		}

		if (base != nullptr)
			setParentNumVoiceListener(parentNumVoiceListener);

		call(getDisplayValue());
	}

	bool isConnected() const
	{
		return base != nullptr;
	}

	void setParentNumVoiceListener(DupliListener* l)
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		parentNumVoiceListener = l;

		if (isConnected())
			base->setParentNumVoiceListener(l);
	}

	WeakReference<DupliListener> parentNumVoiceListener;

	dynamic_base::Ptr base;
	
protected:

	mutable SimpleReadWriteLock connectionLock;
};



template <bool ScaleInput> struct dynamic_chain : public dynamic_base
{
	using Ptr = ReferenceCountedObjectPtr<dynamic_chain>;

	dynamic_chain() :
		dynamic_base()
	{};

	bool isEmpty() const { return targets.isEmpty(); }

	void addParameter(dynamic_base* p)
	{
		targets.add(p);
	}

	void call(double v)
	{
		setDisplayValue(v);
		auto nv = ScaleInput ? getRange().convertTo0to1(v) : v;

		for (auto& t : targets)
		{
			auto tv = ScaleInput ? t->getRange().convertFrom0to1(nv) : v;
			t->call(tv);
		}
	}

	int getNumDuplicates() const override
	{
		if (targets.isEmpty())
			return 0;

		return targets[0]->getNumDuplicates();
	}

	void callWithDuplicateIndex(int index, double value) final override
	{
		for (auto& t : targets)
			t->callWithDuplicateIndex(index, value);
	}

	void setDelta(double v)
	{
		for (auto& t : targets)
			t->setDelta(v);
	}

	void setParentNumVoiceListener(DupliListener* l)
	{
		for (auto& t : targets)
			t->setParentNumVoiceListener(l);
	}

	dynamic_base::List targets;
};





}

}
