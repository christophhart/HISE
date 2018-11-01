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
		processor->addChangeListener(this);
}


template <class ProcessorType>
void hise::raw::Reference<ProcessorType>::addParameterToWatch(int parameterIndex, const ParameterCallback& callbackFunction)
{
	watchedParameters.add({ parameterIndex, processor->getAttribute(parameterIndex), callbackFunction });
}

template <class ProcessorType>
hise::raw::Reference<ProcessorType>::~Reference()
{
	if (processor != nullptr)
	{
		processor->removeChangeListener(this);
	}
}


template <class ProcessorType>
Processor* hise::raw::Reference<ProcessorType>::getProcessor()
{
	return static_cast<Processor*>(processor.get());
}

template <class ProcessorType>
const Processor* hise::raw::Reference<ProcessorType>::getProcessor() const
{
	return static_cast<const Processor*>(processor.get());
}


template <class ProcessorType>
void hise::raw::Reference<ProcessorType>::changeListenerCallback(SafeChangeBroadcaster *)
{
	for (const auto& p : watchedParameters)
	{
		if (p.lastValue != processor->getAttribute(p.index))
			p.callbackFunction(p.index, p.lastValue);
	}
}

}

} // namespace hise;