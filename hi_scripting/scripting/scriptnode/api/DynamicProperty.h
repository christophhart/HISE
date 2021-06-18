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

struct dynamic_base
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	dynamic_base(parameter::dynamic& obj_);;

	dynamic_base();

	virtual ~dynamic_base() {};

	virtual void call(double value)
	{
		lastValue = value;
		f(obj, value);
	}

	virtual void setUIValue(double value)
	{
		lastValue = value;
	}

	virtual void callWithDuplicateIndex(int index, double value) 
	{

	}

	virtual void updateUI()
	{
		if (dataTree.isValid())
			dataTree.setProperty(PropertyIds::Value, lastValue, nullptr);
	};

	virtual void setDelta(double v)
	{

	}

	virtual int getNumDuplicates() const { return 1; }

	void setDataTree(ValueTree d) { dataTree = d; }

	static dynamic_base* createFromConnectionTree(const ValueTree& v, parameter::dynamic& callback, bool allowRange=true);

	parameter::dynamic::Function f;
	void* obj;

	ValueTree dataTree;

	virtual double getDisplayValue() const 
	{ 
		return lastValue; 
	}

	virtual void setParentNumVoiceListener(DupliListener* ) {}

protected:

	double lastValue = 0.0;

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(dynamic_base);
};




struct dynamic_base_holder
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	void call(double v)
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		lastValue = v;

		if (base != nullptr)
			base->call(v);
	}

	void setUIValue(double v)
	{
		lastValue = v;

		if (base != nullptr)
			base->setUIValue(v);
	}

	void call(int index, double v)
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		lastValue = v;

		if (base != nullptr)
			base->callWithDuplicateIndex(index, v);
	}

	int getNumVoices() const
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		if (base != nullptr)
			return base->getNumDuplicates();

		return 1;
	}


	void setDelta(double v)
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		if (base != nullptr)
			base->setDelta(v);
	}

	void updateUI()
	{
		if (base != nullptr)
			base->updateUI();
	}

	void setParameter(dynamic_base* b)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);
			base = b;
		}
		

		if (base != nullptr)
			setParentNumVoiceListener(parentNumVoiceListener);

		call(lastValue);
	}

	bool isConnected() const
	{
		return base != nullptr;
	}

	double getDisplayValue() const
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		if (displaySource.get() != nullptr)
			return displaySource->getDisplayValue();

		return lastValue;
	}

	void setDisplaySource(dynamic_base* src)
	{
		SimpleReadWriteLock::ScopedWriteLock sl(connectionLock);

		displaySource = src;
	}

	void setParentNumVoiceListener(DupliListener* l)
	{
		SimpleReadWriteLock::ScopedReadLock sl(connectionLock);

		parentNumVoiceListener = l;

		if (isConnected())
			base->setParentNumVoiceListener(l);
	}

	WeakReference<dynamic_base> displaySource;

	WeakReference<DupliListener> parentNumVoiceListener;

	ScopedPointer<dynamic_base> base;
	double lastValue = 0.0;
	
private:

	mutable SimpleReadWriteLock connectionLock;
};

struct dynamic_inv : public dynamic_base
{
	dynamic_inv(parameter::dynamic& obj) :
		dynamic_base(obj)
	{}

	void call(double v) final override
	{
		auto inv = 1.0 - v;
		lastValue = inv;
		f(obj, inv);
	}
};

struct dynamic_from0to1_inv : public dynamic_base
{
	dynamic_from0to1_inv(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{}

	void call(double v) final override
	{
		setUIValue(v);
		f(obj, lastValue);
	}

	void setUIValue(double v) final override
	{
		auto inv = 1.0 - v;
		lastValue = range.convertFrom0to1(inv);
	}

	const NormalisableRange<double> range;
};

struct dynamic_step : public dynamic_base
{
	dynamic_step(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{};

	void setUIValue(double v) final override
	{
		lastValue = range.convertFrom0to1(v);
		lastValue = range.snapToLegalValue(lastValue);
	}

	void call(double v) final override
	{
		setUIValue(v);
		f(obj, lastValue);
	}

	NormalisableRange<double> range;
};

struct dynamic_step_inv : public dynamic_base
{
	dynamic_step_inv(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{};

	void setUIValue(double v) final override
	{
		auto inv = 1.0 - v;
		lastValue = range.convertFrom0to1(inv);
		lastValue = range.snapToLegalValue(lastValue);
	}


	void call(double v) final override
	{
		setUIValue(v);
		f(obj, lastValue);
	}

	NormalisableRange<double> range;
};

struct dynamic_from0to1 : public dynamic_base
{
	dynamic_from0to1(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{}

	void setUIValue(double v) final override
	{
		lastValue = range.convertFrom0to1(v);
	}

	void call(double v) final override
	{
		setUIValue(v);
		f(obj, lastValue);
	}

	const NormalisableRange<double> range;
};



struct dynamic_to0to1 : public dynamic_base
{
	dynamic_to0to1(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{}

	void setUIValue(double v) final override
	{
		lastValue = range.convertTo0to1(v);
	}

	void call(double v) final override
	{
		setUIValue(v);
		f(obj, lastValue);
	}

	const NormalisableRange<double> range;
};

#if HISE_INCLUDE_SNEX
struct dynamic_expression : public dynamic_base
{
	dynamic_expression(parameter::dynamic& obj, snex::JitExpression* p_) :
		dynamic_base(obj),
		p(p_)
	{
		if (!p->isValid())
			p = nullptr;
	}

	void setUIValue(double v) final override
	{
		if (p != nullptr)
			lastValue = p->getValueUnchecked(v);
		else
			lastValue = v;
	}

	void call(double v) final override
	{
		setUIValue(v);
		f(obj, lastValue);
	}

	snex::JitExpression::Ptr p;
};
#endif

struct dynamic_chain : public dynamic_base
{
	dynamic_chain() :
		dynamic_base()
	{};

	bool isEmpty() const { return targets.isEmpty(); }

	void addParameter(dynamic_base* p)
	{
		targets.add(p);
	}

	dynamic_base* getFirstIfSingle()
	{
		if (targets.size() == 1 && (RangeHelpers::isIdentity(inputRange2) || scaleInput))
			return targets.removeAndReturn(0);

		return nullptr;
	}

	void call(double v)
	{
		if (scaleInput)
			v = inputRange2.convertTo0to1(v);

		for (auto& t : targets)
			t->call(v);
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

	void updateUI() override
	{
		for (auto& t : targets)
			t->updateUI();
	}

	bool scaleInput = true;
	OwnedArray<dynamic_base> targets;
	NormalisableRange<double> inputRange2;
};



}

}
