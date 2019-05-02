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


template <class InterfaceClass>
void hise::raw::MainProcessor::addAndSetup(ConnectedObject<InterfaceClass>* object, InterfaceClass* newObject)
{
	auto objAsControlledObject = dynamic_cast<hise::ControlledObject*>(object);

	// You need to call this with a object that is also derived from hise::ControlledObject
	jassert(objAsControlledObject != nullptr);

	auto mc = objAsControlledObject->getMainController();

	// Create a builder object
	raw::Builder b(mc);
	auto chain = mc->getMainSynthChain();

	// Pass it to the builder and it will be added to the chain
	b.add(newObject, chain, raw::IDs::Chains::Midi);
	newObject->setId("Interface");

	// The ownership was transferred so we need to release it here...

	object->setup(mc);
	newObject->init();
}


template <int Index>
void hise::raw::MainProcessor::addParameter(const Identifier& id)
{
	// You can't add this if the processor is already initialised...
	jassert(LockHelpers::freeToGo(getMainController()));
	jassert(!isOnAir());

	// Oops, parameter index mismatch. Make sure you call this method in the correct order.
	jassert(Index == parameters.size());

	ScopedPointer<ParameterBase> p = new Parameter<Index>(this, id);
	parameters.add(p.release());
}


}
}
