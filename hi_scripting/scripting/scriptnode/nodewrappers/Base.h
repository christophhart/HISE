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
		ParameterData(const String& id_) :
			id(id_)
		{};

		ParameterData(const String& id_, NormalisableRange<double> r) :
			id(id_),
			range(r)
		{};

		ValueTree createValueTree() const
		{
			ValueTree p(PropertyIds::Parameter);

			RangeHelpers::storeDoubleRange(p, false, range, nullptr);

			p.setProperty(PropertyIds::ID, id, nullptr);
			p.setProperty(PropertyIds::Value, defaultValue, nullptr);

			return p;
		}

		ParameterData withRange(NormalisableRange<double> r)
		{
			ParameterData copy(*this);
			copy.range = r;
			return copy;
		}

		void operator()(double newValue) const
		{
			db(range.convertFrom0to1(newValue));
		}

		void setBypass(double newValue) const
		{
			db(range.getRange().contains(newValue) ? 0.0 : 1.0);
		}

		void callWithRange(double value)
		{
			db(range.convertFrom0to1(value));
		}

		void addConversion(const Identifier& converterId)
		{
			db = DspHelpers::wrapIntoConversionLambda(converterId, db, range, false);
			range = {};
		}

		String id;
		NormalisableRange<double> range;
		double defaultValue = 0.0;
		
		void setParameterValueNames(const StringArray& valueNames)
		{
			parameterNames = valueNames;
			range = { 0.0, (double)valueNames.size() - 1.0, 1.0 };
		}

		void init()
		{
			db(defaultValue);
		}

		std::function<void(double)> db;
		StringArray parameterNames;
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

	virtual HardcodedNode* getAsHardcodedNode() { return nullptr; }

	virtual Array<HiseDspBase*> createListOfNodesWithSameId()
	{
		return { this };
	}

	virtual Component* createExtraComponent(PooledUIUpdater* updater)
	{
		ignoreUnused(updater);
		return nullptr;
	}



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

	HardcodedNode* getAsHardcodedNode() override { return obj.getAsHardcodedNode(); }

	Array<HiseDspBase*> createListOfNodesWithSameId() override
	{
		if (std::is_base_of<HiseDspBase, T>())
			return { this, static_cast<HiseDspBase*>(&obj) };
		else
			return { this };
	}

protected:

	T obj;
};




}
