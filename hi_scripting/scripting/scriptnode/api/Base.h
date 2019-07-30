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

class HardcodedNode;

class HiseDspBase
{
public:

	using Function = std::function<void(HiseDspBase&)>;

	virtual ~HiseDspBase() {};

	struct ParameterData
	{
		ParameterData(const String& id_);;
		ParameterData(const String& id_, NormalisableRange<double> r);
		ParameterData withRange(NormalisableRange<double> r);

		ValueTree createValueTree() const;
		
		
		void setDefaultValue(double newDefaultValue)
		{
			defaultValue = newDefaultValue;
		}

		void callUnscaled(double newValue) const;

		void operator()(double newValue) const;
		void setBypass(double newValue) const;
		void callWithRange(double value) const;
		void addConversion(const Identifier& converterId);
		void setParameterValueNames(const StringArray& valueNames);
		void init();

		void setCallback(const std::function<void(double)>& callback)
		{
			db = callback;
			db(defaultValue);
		}

		String id;
		NormalisableRange<double> range;
		double defaultValue = 0.0;
		std::function<void(double)> db;
		StringArray parameterNames;

		double lastValue = 0.0;
	};

	template <class ObjectType> class ExtraComponent : public Component,
		public PooledUIUpdater::SimpleTimer
	{
	protected:

		ExtraComponent(ObjectType* t, PooledUIUpdater* updater) :
			SimpleTimer(updater),
			object(dynamic_cast<HiseDspBase*>(t))
		{};

		ObjectType* getObject() const
		{
			return dynamic_cast<ObjectType*>(object.get());
		}

	private:

		WeakReference<HiseDspBase> object;
	};

	bool isHardcoded() const;

	virtual int getExtraWidth() const { return 0; };

	virtual void initialise(NodeBase* n)
	{
		ignoreUnused(n);
	}

	virtual void prepare(PrepareSpecs specs) {};

	virtual HardcodedNode* getAsHardcodedNode() { return nullptr; }

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	};

	

	virtual Component* createExtraComponent(PooledUIUpdater* updater)
	{
		ignoreUnused(updater);
		return nullptr;
	}

	virtual bool isPolyphonic() const { return false; }

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseDspBase);

	virtual void createParameters(Array<ParameterData>& data) = 0;

};

template <class T> class SingleWrapper : public HiseDspBase
{
public:

	inline void initialise(NodeBase* n) override
	{
		obj.initialise(n);
	}

	Component* createExtraComponent(PooledUIUpdater* updater) override
	{
		return obj.createExtraComponent(updater);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	bool isPolyphonic() const override
	{
		return obj.isPolyphonic();
	}

	HiseDspBase* getInternalT()
	{
		return dynamic_cast<HiseDspBase*>(&obj);
	}

	HardcodedNode* getAsHardcodedNode() override { return obj.getAsHardcodedNode(); }

protected:

	T obj;
};

namespace wrap
{


}




}
