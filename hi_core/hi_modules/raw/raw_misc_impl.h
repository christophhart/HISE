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

#pragma once

namespace hise {
using namespace juce;

namespace raw
{

template <class ProcessorType>
hise::raw::Reference<ProcessorType>::Reference(MainController* mc, const String& id /*= String()*/, bool addAsListener/*=false*/) :
	ControlledObject(mc)
{
	if (id.isEmpty())
		processor = ProcessorHelpers::getFirstProcessorWithType<ProcessorType>(mc->getMainSynthChain());
	else
		processor = dynamic_cast<ProcessorType*>(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), id));

	// If you hit this assertion it means that the processor you try to find can't be located.
	jassert(processor != nullptr);

	if (addAsListener && processor != nullptr)
		updater = new Updater(*this, processor);
}

template <class ProcessorType>
void hise::raw::Reference<ProcessorType>::addParameterToWatch(int parameterIndex, const ParameterCallback& callbackFunction)
{
	ParameterValue v({ parameterIndex, processor->getAttribute(parameterIndex), callbackFunction });
	watchedParameters.add(v);

	v.callbackFunction(v.lastValue);
}

template <class ProcessorType>
hise::raw::Reference<ProcessorType>::~Reference()
{
	updater = nullptr;
}

template <class ProcessorType>
ProcessorType* hise::raw::Reference<ProcessorType>::getProcessor() const noexcept
{
	return processor.get();
}


template <class ProcessorType>
void hise::raw::Reference<ProcessorType>::refresh()
{
	for (auto& p : watchedParameters)
	{
		auto thisValue = processor->getAttribute(p.index);

		if (p.lastValue != thisValue)
		{
			p.lastValue = thisValue;
			p.callbackFunction(p.lastValue);
		}
			
	}
}


template <class ComponentType, typename ValueType>
hise::raw::UIConnection::Base<ComponentType, ValueType>::Base(ComponentType* c, MainController* mc, const String& id) :
	Data<ValueType>(id),
	ControlledObject(mc),
	OtherListener(ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), id), dispatch::library::ProcessorChangeEvent::Any),
	component(c)
{
	c->addListener(this);
	//setData<Data<ValueType>::Dummy>();
}


template <class ComponentType, typename ValueType>
hise::raw::UIConnection::Base<ComponentType, ValueType>::~Base()
{
	if (component.getComponent() != nullptr)
		component->removeListener(this);
}

template <class ComponentType, typename ValueType>
void hise::raw::UIConnection::Base<ComponentType, ValueType>::otherChange(Processor* p)
{
	if (saveFunction)
	{
		auto newValue = static_cast<ValueType>(saveFunction(p));

		if (newValue != lastValue)
		{
			lastValue = newValue;
			updateUI(newValue);
		}
	}
}

template <class ComponentType, typename ValueType>
void hise::raw::UIConnection::Base<ComponentType, ValueType>::parameterChangedFromUI(ValueType newValue)
{
	if (!loadFunction)
		jassertfalse;

	if (useLoadingThread)
	{
		// TODO
		jassert(undoManager == nullptr);

		auto tmp = loadFunction;
		auto f = [tmp, newValue](Processor* p)
		{
			tmp(p, newValue);
			return SafeFunctionCall::OK;
		};

		TaskAfterSuspension::call(processor, f);
	}
	else
	{
		if(undoManager == nullptr)
			loadFunction(processor, newValue);
		else
		{
			undoManager->perform(new UndoableUIAction(*this, newValue));
		}
	}
	
}


	



template <int parameterIndex>
hise::raw::UIConnection::Slider<parameterIndex>::Slider(juce::Slider* s, MainController* mc, const String& processorID) :
	SliderBase(s, mc, processorID)
{
	setData<Data::Attribute<parameterIndex>>();
}


}

} // namespace hise;
