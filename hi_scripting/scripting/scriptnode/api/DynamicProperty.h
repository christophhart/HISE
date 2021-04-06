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


struct dynamic_base
{
	PARAMETER_SPECS(ParameterType::Single, 1);

	dynamic_base(parameter::dynamic& obj_);;

	dynamic_base();

	virtual void call(double value)
	{
		lastValue = value;
		f(obj, value);
	}

	virtual void updateUI()
	{
		if (dataTree.isValid())
			dataTree.setProperty(PropertyIds::Value, lastValue, nullptr);
	};

	virtual void setDelta(double v)
	{

	}

	void setDataTree(ValueTree d) { dataTree = d; }

	parameter::dynamic::Function f;
	void* obj;

	ValueTree dataTree;

	virtual double getDisplayValue() const 
	{ 
		return lastValue; 
	}

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
		lastValue = v;

		if (base != nullptr)
			base->call(v);
	}

	void setDelta(double v)
	{
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
		base = b;
		call(lastValue);
	}

	bool isConnected() const
	{
		return base != nullptr;
	}

	double getDisplayValue() const
	{
		if (displaySource.get() != nullptr)
			return displaySource->getDisplayValue();

		return lastValue;
	}

	void setDisplaySource(dynamic_base* src)
	{
		displaySource = src;
	}

	WeakReference<dynamic_base> displaySource;

	ScopedPointer<dynamic_base> base;
	double lastValue = 0.0;
	
};



struct dynamic_from0to1 : public dynamic_base
{
	dynamic_from0to1(parameter::dynamic& obj, const NormalisableRange<double>& r) :
		dynamic_base(obj),
		range(r)
	{}

	void call(double v) final override
	{
		lastValue = range.convertFrom0to1(v);
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

	void call(double v) final override
	{
		lastValue = range.convertTo0to1(v);
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

	void call(double v) final override
	{
		if (p != nullptr)
		{
			lastValue = p->getValueUnchecked(v);
			f(obj, lastValue);
		}
		else
		{
			lastValue = v;
			f(obj, lastValue);
		}
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
		if (targets.size() == 1)
			return targets.removeAndReturn(0);

		return nullptr;
	}

	void call(double v)
	{
		for (auto& t : targets)
			t->call(v);
	}

	void setDelta(double v)
	{
		for (auto& t : targets)
			t->setDelta(v);
	}

	void updateUI() override
	{
		for (auto& t : targets)
			t->updateUI();
	}

	OwnedArray<dynamic_base> targets;
};



}

}
