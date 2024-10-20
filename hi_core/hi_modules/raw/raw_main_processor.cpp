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
using namespace juce;

namespace raw
{


MainProcessor::AsyncHiseEventListener::AsyncHiseEventListener(MainController* mc) :
	ConnectedObject()
{
	setup(mc);
	getMainProcessor()->addAsyncHiseEventListener(this);
}

MainProcessor::AsyncHiseEventListener::~AsyncHiseEventListener()
{
	getMainProcessor()->removeAsyncHiseEventListener(this);
}

MainProcessor::MainProcessor(MainController* mc) :
	MidiProcessor(mc, "Interface"),
	asyncHandler(*this)
{

}

void MainProcessor::addAsyncHiseEventListener(AsyncHiseEventListener* l)
{
	eventListeners.addIfNotAlreadyThere(l);
}

void MainProcessor::removeAsyncHiseEventListener(AsyncHiseEventListener* l)
{
	eventListeners.removeAllInstancesOf(l);
}

MainProcessor::~MainProcessor()
{
	parameters.clear();
}

void MainProcessor::registerCallback(Processor* p, int parameterIndex, const Callback& f, ExecutionType executionType /*= Synchronously*/)
{
	if (isPositiveAndBelow(parameterIndex, parameters.size()))
		parameters[parameterIndex]->registerCallback(p, f, executionType);
}

juce::Identifier MainProcessor::getIdentifierForParameterIndex(int parameterIndex) const
{
	if (isPositiveAndBelow(parameterIndex, parameters.size()))
		return parameters[parameterIndex]->getId();

	jassertfalse;
	return {};
}

void MainProcessor::setInternalAttribute(int parameterIndex, float newValue)
{
	if (isPositiveAndBelow(parameterIndex, parameters.size()))
		parameters[parameterIndex]->update(newValue);
}

float MainProcessor::getAttribute(int parameterIndex) const
{
	if (isPositiveAndBelow(parameterIndex, parameters.size()))
		return parameters[parameterIndex]->getCurrentValue();

	jassertfalse;
	return 0.0f;
}

void MainProcessor::processHiseEvent(HiseEvent &e)
{
	processSync(e);
	asyncHandler.pushEvent(e);
}


struct MainProcessor::ParameterBase::CallbackWithProcessor
{
	CallbackWithProcessor(Processor* p_, const Callback& f_) :
		p(p_),
		f(f_)
	{};

	~CallbackWithProcessor()
	{
		p = nullptr;
	}

	WeakReference<hise::Processor> p;
	Callback f;

	void operator()(float newValue) const
	{
		if (p != nullptr)
			f(newValue);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CallbackWithProcessor);
};


MainProcessor::ParameterBase::ParameterBase(MainProcessor* p, const Identifier& id_) :
	SuspendableAsyncUpdater(p->getMainController()),
	LambdaStorage<float>(id_, p),
	id(id_),
	currentValue(0.0f)
{
	auto tmp = &currentValue;
	saveFunction = [tmp]()
	{
		return *tmp;
	};

	loadFunction = [this](float newValue)
	{
		this->update(newValue);
		jassertfalse;
		this->p->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Attribute);
	};
}

MainProcessor::ParameterBase::~ParameterBase()
{
	asynchronousCallbacks.clear();
	synchronousCallbacks.clear();
}

void MainProcessor::ParameterBase::update(float newValue)
{
	if (newValue != currentValue)
	{
		currentValue = newValue;

		for (const auto& f : synchronousCallbacks)
			(*f)(newValue);

		if (!asynchronousCallbacks.isEmpty())
			triggerAsyncUpdate();
	}
}

void MainProcessor::ParameterBase::registerCallback(Processor* pr, const Callback& f, ExecutionType type)
{
	if (type == ExecutionType::Asynchronously)
		asynchronousCallbacks.add(new CallbackWithProcessor(pr, f));
	else
		synchronousCallbacks.add(new CallbackWithProcessor(pr, f));
}

void MainProcessor::ParameterBase::handleAsyncUpdate()
{
	for (const auto& f : asynchronousCallbacks)
		(*f)(currentValue);
}

MainProcessor::AsyncMessageHandler::AsyncMessageHandler(MainProcessor& parent_) :
	parent(parent_),
	events(8192)
{
	addChangeListener(this);
	enablePooledUpdate(parent.getMainController()->getGlobalUIUpdater());
}

MainProcessor::AsyncMessageHandler::~AsyncMessageHandler()
{
	removeChangeListener(this);
}

void MainProcessor::AsyncMessageHandler::pushEvent(HiseEvent& e)
{
	if (parent.eventListeners.isEmpty())
		return;

	events.push(std::move(e));
	sendPooledChangeMessage();
}

void MainProcessor::AsyncMessageHandler::changeListenerCallback(SafeChangeBroadcaster *)
{
	HiseEvent e;

	while (events.pop(e))
	{
		for (auto l : parent.eventListeners)
		{
			if (l != nullptr)
				l->hiseEventCallback(e);
		}
	}
}

}
}