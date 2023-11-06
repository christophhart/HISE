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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace hise {
namespace dispatch {
namespace dummy {
using namespace juce;

// BEGIN ACTIONS =====================================================================

struct Processor::AddAction: public Action
{
	AddAction(MainController* mc):
      Action(mc, ActionTypes::add_processor),
      handler(mc->processorHandler)
	{
		b << "add processor ";
	};

	void perform() override
	{
		ownedProcessor = new Processor(getMainController(), id);
		ownedProcessor->dispatcher.setNumAttributes(numParameters);
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);
		id = obj[ActionIds::id].toString();
		numParameters = (int)obj[ActionIds::num_parameters];
	}

	var toJSON() const override
	{
		auto obj = Action::toJSON();
		obj.getDynamicObject()->setProperty(ActionIds::id, id.toString());
		obj.getDynamicObject()->setProperty(ActionIds::num_parameters, numParameters);
		return obj;
	}

	Processor* getProcessor() { return ownedProcessor.get(); }

    library::ProcessorHandler& handler;
    Identifier id;
    ScopedPointer<Processor> ownedProcessor;
	int numParameters;
};

struct Processor::ActionBase: public Action
{
	ActionBase(AddAction* a, const Identifier & type):
	  Action(a->getMainController(), type),
	  addAction(a)
	{};

	AddAction* getOwner() const
	{
		return dynamic_cast<AddAction*>(addAction.get());
	}

	Processor* getProcessor() const
	{
		auto p = getOwner()->getProcessor();
		jassert(p != nullptr);
		return p;
	}

private:

	Action::Ptr addAction;
};

struct Processor::SetAttributeAction: public Processor::ActionBase
{
	using Owner = Processor::AddAction;

    SetAttributeAction(Owner* a):
      ActionBase(a, ActionTypes::set_attribute)
	{
		b << getOwner()->id << "::setAttribute";
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);
		wait = (float)obj[ActionIds::wait];
		parameterIndex = (int)obj[ActionIds::index];
	}

	var toJSON() const override
	{
		auto obj = Action::toJSON();
		obj.getDynamicObject()->setProperty(ActionIds::wait, wait);
		obj.getDynamicObject()->setProperty(ActionIds::index,	parameterIndex);
		return obj;
	}

    void perform() override
	{
		getProcessor()->setAttribute(parameterIndex, wait, sendNotificationSync);

    	if(wait != 0)
			Helpers::busyWait(wait);
	}

	int parameterIndex = 0;
	float wait = 0.0f;
};

struct Processor::RemoveAction: public ActionBase
{
	RemoveAction(AddAction* a):
      ActionBase(a, ActionTypes::rem_processor)
	{
		b << "remove processor " << getOwner()->id;
	}

	void perform() override
	{
		getOwner()->ownedProcessor = nullptr;
	}
};

// BEGIN ACTIONS =====================================================================

Action::List Processor::Builder::createActions(const var& jsonData)
{
	Action::List list;

	AddAction* addAction = nullptr;

	if(jsonData.isArray())
	{
		for(const auto& obj: *jsonData.getArray())
		{
			Identifier type(obj[ActionIds::type].toString());

			if(type == ActionTypes::add_processor)
			{
				expectOrThrow(addAction == nullptr, "only one add per list");
				addAction = new AddAction(getMainController());
				list.add(addAction);
			}
			if(type == ActionTypes::rem_processor)
			{
				expectOrThrow(addAction != nullptr, "must add processor first");
				list.add(new RemoveAction(addAction));
			}
			if(type == ActionTypes::set_attribute)
			{
				expectOrThrow(addAction != nullptr, "must add processor first");
				list.add(new SetAttributeAction(addAction));
			}

			list.getLast()->fromJSON(obj);
		}
	}

	jassert(addAction == list.getFirst());

	return list;
}



void Processor::setAttribute(int parameterIndex, float newValue, NotificationType n)
{
	dispatcher.setAttribute(parameterIndex, newValue, n);
}

void Processor::setBypassed(bool shouldBeBypassed, NotificationType n)
{
	dispatcher.setBypassed(shouldBeBypassed);
}

Processor::Processor(MainController* mc, HashedCharPtr id):
	ControlledObject(mc),
	dispatcher(mc->processorHandler, *this, id)
{
	dispatcher.setNumAttributes(30);
	Helpers::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
}

Processor::~Processor()
{
	Helpers::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
}
} // dummy
} // dispatch
} // hise