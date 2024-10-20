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
      Action(mc, ActionTypes::add),
      handler(*mc->processorHandler)
	{
		b << "add processor ";
	};

	~AddAction()
	{
		jassert(ownedProcessor == nullptr);
		cleanup();
	}

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

	static AddAction* getFrom(MainController* mc, const Identifier& id);

	var toJSON() const override
	{
		auto obj = Action::toJSON();
		obj.getDynamicObject()->setProperty(ActionIds::id, id.toString());
		obj.getDynamicObject()->setProperty(ActionIds::num_parameters, numParameters);
		return obj;
	}

	void cleanup()
	{
		jassert(ownedProcessor != nullptr);
		ownedProcessor = nullptr;
	}

	Processor* getProcessor() { return ownedProcessor.get(); }

    library::ProcessorHandler& handler;
    Identifier id;

private:

    ScopedPointer<Processor> ownedProcessor;
	int numParameters;
};

Processor::AddAction* Processor::AddAction::getFrom(MainController* mc, const Identifier& id)
{
	for(auto a: mc->allActions)
	{
		if(auto typed = dynamic_cast<AddAction*>(a))
		{
			if(typed->id == id)
				return typed;
		}
	}

	return nullptr;
}

struct Processor::ActionBase: public Action
{
	ActionBase(Action::Ptr a, const Identifier & type):
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

struct Processor::SetBypassedAction: public Processor::ActionBase
{
    SetBypassedAction(Action::Ptr a):
      ActionBase(a, ActionTypes::set_bypassed)
	{
		b << getOwner()->id << "::setBypassed";
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);
		wait = (float)obj[ActionIds::duration];
		value = (bool)obj[ActionIds::value];
		n = Helpers::getDispatchFromJSON(obj);
		expectAfter(getOwner(), false);
	}

	var toJSON() const override
	{
		auto obj = Action::toJSON();
		obj.getDynamicObject()->setProperty(ActionIds::duration, wait);
		obj.getDynamicObject()->setProperty(ActionIds::value,	value);
		Helpers::writeDispatchString(n, obj);

		return obj;
	}

    void perform() override
	{
		getProcessor()->setBypassed(value, n);

    	if(wait != 0)
			Helpers::busyWait(wait);
	}

	bool value = false;
	float wait = 0.0f;
	DispatchType n = DispatchType::sendNotification;
};

struct Processor::SetAttributeAction: public Processor::ActionBase
{
    SetAttributeAction(Action::Ptr a):
      ActionBase(a, ActionTypes::set_attribute)
	{
		b << getOwner()->id << "::setAttribute";
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);
		wait = (float)obj[ActionIds::duration];
		parameterIndex = (int)obj[ActionIds::index];
		n = Helpers::getDispatchFromJSON(obj);
		expectAfter(getOwner(), false);
	}

	var toJSON() const override
	{
		auto obj = Action::toJSON();
		obj.getDynamicObject()->setProperty(ActionIds::duration, wait);
		obj.getDynamicObject()->setProperty(ActionIds::index,	parameterIndex);

		Helpers::writeDispatchString(n, obj);

		return obj;
	}

    void perform() override
	{
		auto owner = getOwner();

		expectOrThrowRuntimeError(owner != nullptr, "owner deleted");

		auto p = owner->getProcessor();

		expectOrThrowRuntimeError(p != nullptr, "processor removed");

		p->setAttribute(parameterIndex, wait, n);

    	if(wait != 0)
			Helpers::busyWait(wait);
	}

	DispatchType n = DispatchType::sendNotification;
	int parameterIndex = 0;
	float wait = 0.0f;
};

struct Processor::RemoveAction: public ActionBase
{
	RemoveAction(Action::Ptr a):
      ActionBase(a, ActionTypes::rem)
	{
		b << "remove processor " << getOwner()->id;
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);
		expectAfter(getOwner(), true);
	}

	void perform() override
	{
		getOwner()->cleanup();
	}
};



Action::Ptr Processor::Builder::createAction(const Identifier& type)
{
	if(type == ActionTypes::add)
	{
		Helpers::expectOrThrow(addAction == nullptr, "only one add per list");
		return new AddAction(getMainController());
	}
	if(type == ActionTypes::rem)
	{
		Helpers::expectOrThrow(addAction != nullptr, "must add processor first");
		return new RemoveAction(addAction);
	}
	if(type == ActionTypes::set_bypassed)
	{
		Helpers::expectOrThrow(addAction != nullptr, "must add processor first");
		return new SetBypassedAction(addAction);
	}
	if(type == ActionTypes::set_attribute)
	{
		Helpers::expectOrThrow(addAction != nullptr, "must add processor first");
		return new SetAttributeAction(addAction);
	}

	return nullptr;
}

// END ACTIONS =====================================================================



Processor::Processor(MainController* mc, HashedCharPtr id):
	ControlledObject(mc),
	dispatcher(*mc->processorHandler, *this, id)
{
	dispatcher.setNumAttributes(30);
	Helpers::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
}

Processor::~Processor()
{
	Helpers::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
}

void Processor::setAttribute(int parameterIndex, float newValue, DispatchType n)
{
	dispatcher.setAttribute(parameterIndex, newValue, n);
}

void Processor::setBypassed(bool shouldBeBypassed, DispatchType n)
{
	dispatcher.setBypassed(shouldBeBypassed, n);
}

struct ProcessorListener::AddAction: public Action
{
	AddAction(MainController* mc):
	  Action(mc, ActionTypes::add)
	{
		b << "add listener to: ";
	};

	~AddAction() override
	{
		jassert(listener == nullptr);
		cleanup();
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);

		processorId = obj[ActionIds::source].toString();

		b << processorId.toString();

		auto attributeValue = obj[ActionIds::attributes];

		if(attributeValue.isArray())
		{
			if(attributeValue.size() == 1)
				attributes.add((uint8)(int)attributeValue[0]);
			else
			{
				for(auto& v: *attributeValue.getArray())
					attributes.add((uint8)(int)v);
			}
		}
		else if (attributeValue.isInt() || attributeValue.isInt64())
		{
			attributes.add((uint8)(int)attributeValue);
		}

		n = Helpers::getDispatchFromJSON(obj, sendNotificationAsync);

		addBypassed = (bool)obj[ActionIds::bypassed];
	}

	

	void cleanup()
	{
		auto processorAddAction = Processor::AddAction::getFrom(getMainController(),processorId);

		expectOrThrowRuntimeError(processorAddAction != nullptr, "Can't find add action");

		auto pr = processorAddAction->getProcessor();

		expectOrThrowRuntimeError(pr != nullptr, "processor was deleted or never added");

		{
			auto& p = processorAddAction->getProcessor()->dispatcher;

			p.removeNameAndColourListener(&listener->idListener);
			p.removeAttributeListener(&listener->attributeListener);
			p.removeBypassListener(&listener->bypassListener);
			
		}

		jassert(listener != nullptr);
		listener = nullptr;
	}

	void perform() override
	{
		auto processorAddAction = Processor::AddAction::getFrom(getMainController(),processorId);

		expectOrThrowRuntimeError(processorAddAction != nullptr, "can't find processor");

		auto pr = processorAddAction->getProcessor();

		expectOrThrowRuntimeError(pr != nullptr, "processor wasn't created yet");

		auto& p = pr->dispatcher;

		listener = new ProcessorListener(getMainController());

		if(!attributes.isEmpty())
		{
			auto& l = listener->attributeListener;
			p.addAttributeListener(&l, attributes.getRawDataPointer(), attributes.size(), n);
		}
		if(addBypassed)
		{
			p.addBypassListener(&listener->bypassListener, n);
		}
	}

	ProcessorListener* getListener() const
	{
		return listener.get();
	}

	Identifier processorId;

	Array<uint16> attributes;
	DispatchType n;

	bool addBypassed = false;

private:

	ScopedPointer<ProcessorListener> listener;
};

struct ProcessorListener::CountAction: public Action
{
	CountAction(Action::Ptr addAction_):
	  Action(addAction_->getMainController(), ActionTypes::count),
	  addAction(addAction_)
	{
		b << "callback counter";
	};

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);

		if(obj.hasProperty(ActionIds::attributes))
			numExpectedAttributes = (int)obj[ActionIds::attributes];

		if(obj.hasProperty(ActionIds::bypassed))
			numExpectedBypassed = (int)obj[ActionIds::bypassed];

		expectAfter(addAction, true);
	}

	ProcessorListener* getListener() { return dynamic_cast<AddAction*>(addAction.get())->getListener(); }

	void perform() override
	{
		if(auto l = getListener())
		{
			if(numExpectedAttributes != -1)
			{
				auto numActual = (int)l->numAttributeCallbacks;
				getMainController()->currentTest->expectEquals(numActual, numExpectedAttributes, "attribute counter mismatch");
				getMainController()->numChecksPerformed++;
			}
			if(numExpectedBypassed != -1)
			{
				auto numActual = (int)l->numBypassedCallbacks;
				getMainController()->currentTest->expectEquals(numActual, numExpectedBypassed, "bypass counter mismatch");
				getMainController()->numChecksPerformed++;
			}
		}
		else
		{
			getMainController()->currentTest->expect(false, "Can't find listener");
		}
	}

	~CountAction()
	{};

	int numExpectedAttributes = -1;
	int numExpectedBypassed = -1;

	Action::Ptr addAction;
};

struct ProcessorListener::RemoveAction: public Action
{
	RemoveAction(Action::Ptr addAction_):
	  Action(addAction_->getMainController(), ActionTypes::rem),
	  addAction(addAction_)
	{
		b << "remove listener from " << getOwner()->processorId;
	}

	void fromJSON(const var& obj) override
	{
		Action::fromJSON(obj);
		expectAfter(addAction, true);
	}

	AddAction* getOwner() { return dynamic_cast<AddAction*>(addAction.get()); }

	void perform() override
	{
		getOwner()->cleanup();
	}

	Action::Ptr addAction;
};

ProcessorListener::Builder::Builder(MainController* mc):
	Action::Builder(mc)
{}

Action::Ptr ProcessorListener::Builder::createAction(const Identifier& type)
{
	if(type == ActionTypes::add)
	{
		Helpers::expectOrThrow(addAction == nullptr, "only one add per list");
		return new AddAction(getMainController());
	}
	if(type == ActionTypes::count)
	{
		Helpers::expectOrThrow(addAction != nullptr, "must add processor first");
		return new CountAction(addAction);
	}
	if(type == ActionTypes::rem)
	{
		Helpers::expectOrThrow(addAction != nullptr, "must add processor first");
		return new RemoveAction(addAction);
	}

	Helpers::expectOrThrow(false, "Can't create type: " + type.toString());
	return nullptr;
}

ProcessorListener::ProcessorListener(MainController* mc):
	ControlledObject(mc),
	attributeListener(mc->root, *this, BIND_MEMBER_FUNCTION_2(ProcessorListener::onAttribute)),
	bypassListener(mc->root, *this, BIND_MEMBER_FUNCTION_2(ProcessorListener::onBypassed)),
	idListener(mc->root, *this, BIND_MEMBER_FUNCTION_1(ProcessorListener::onIdOrColourChange))
{
	Helpers::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
}

ProcessorListener::~ProcessorListener()
{
	Helpers::busyWait(Random::getSystemRandom().nextFloat() * 4.0);
}
} // dummy
} // dispatch
} // hise