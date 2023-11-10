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
using namespace juce;

namespace library {
using namespace juce;

ProcessorHandler::BypassListener::BypassListener(RootObject& r, ListenerOwner& owner, const Callback& f_):
	Listener(r, owner),
	f(f_)
{}


void ProcessorHandler::BypassListener::slotChanged(const ListenerData& d)
{
	jassert(d.t == EventType::SlotChange);
	jassert(f);

	auto slotType = (SlotTypes)d.slotIndex;

	if(slotType == SlotTypes::Bypassed)
	{
		jassert(d.numBytes == 1);

		TRACE_DISPATCH("onBypass");

		const auto obj = d.to_static_cast<Processor>();
		f(obj, obj->isBypassed());
	}
}

ProcessorHandler::AttributeListener::AttributeListener(RootObject& r, ListenerOwner& owner, const Callback& f_):
	Listener(r, owner),
	f(f_)
{}

void ProcessorHandler::AttributeListener::slotChanged(const ListenerData& d)
{
	jassert(d.t == EventType::SlotChange);
	jassert(f);

	auto slotType = (SlotTypes)d.slotIndex;

	if(slotType == SlotTypes::Attributes)
	{
		auto obj = d.to_static_cast<Processor>();
		d.callForEachSetValue([&](uint8 index){ f(obj, index); });
	}
}

ProcessorHandler::NameAndColourListener::NameAndColourListener(RootObject& r, ListenerOwner& owner, const Callback& f_):
	Listener(r, owner),
	f(f_)
{}

void ProcessorHandler::NameAndColourListener::slotChanged(const ListenerData& d)
{
	jassert(d.s != nullptr);
	jassert(d.t == EventType::SlotChange);
	auto slotType = (SlotTypes)d.slotIndex;

	if(slotType == SlotTypes::NameAndColour)
	{
		jassert(MessageManager::getInstance()->isThisTheMessageThread());
		jassert(d.numBytes == 2);
		jassert(f);

		TRACE_DISPATCH("onNameOrColourChange");

		auto obj = d.to_static_cast<Processor>();
		f(obj);
	}
}

ProcessorHandler::ProcessorHandler(RootObject& r):
  SourceManager(r, IDs::source::modules)
{}

Processor::Processor(ProcessorHandler& h, SourceOwner& owner, const HashedCharPtr& id):
	Source(h, owner, id),
	attributes(*this, (int)SlotTypes::Attributes, IDs::event::attribute),
	nameAndColour(*this, (int)SlotTypes::NameAndColour, IDs::event::namecolour),
	bypassed(*this, (int)SlotTypes::Bypassed, IDs::event::bypassed),
	otherChange(*this, (int)SlotTypes::OtherChange, IDs::event::other)
{
	bypassed.setNumSlots(1);
	nameAndColour.setNumSlots(2);
	otherChange.setNumSlots((int)ProcessorChangeEvent::numProcessorChangeEvents);
}

Processor::~Processor()
{
	attributes.shutdown();
	bypassed.shutdown();
	nameAndColour.shutdown();
	otherChange.shutdown();
}

void Processor::setAttribute(int parameterIndex, float, DispatchType n)
{
	if(n != dontSendNotification)
		attributes.sendChangeMessage(parameterIndex, n);
}

void Processor::setBypassed(bool value)
{
	if(value != cachedBypassValue)
	{
		StringBuilder b;
		b << getDispatchId() << "::setBypassed()";
		TRACE_DYNAMIC_DISPATCH(b);
		cachedBypassValue = value;
		bypassed.sendChangeMessage(0, sendNotificationSync);
	}
}

void Processor::setId(const String&)
{
	nameAndColour.sendChangeMessage(0, sendNotificationAsync);
}

void Processor::setColour(const Colour&)
{
	nameAndColour.sendChangeMessage(1, sendNotificationAsync);
}

void Processor::setNumAttributes(int numAttributes)
{
	attributes.setNumSlots(numAttributes);
}

void Processor::addBypassListener(BypassListener* l, DispatchType n)
{
	uint8 slotIndex = static_cast<uint8>(SlotTypes::Bypassed);
	l->addListenerToSingleSource(this, &slotIndex, 1, n);
}

void Processor::removeBypassListener(BypassListener* l)
{
	l->removeListener(*this);
}

void Processor::addAttributeListener(AttributeListener* l, const uint8* attributeIndexes, size_t numAttributes,
	DispatchType n)
{
	if(numAttributes == 1)
	{
		l->addListenerToSingleSlotIndexWithinSlot(this, (int)SlotTypes::Attributes, *attributeIndexes, n);
	}
	else
	{
		l->addListenerToSingleSourceAndSlotSubset(this, (int)SlotTypes::Attributes, attributeIndexes, numAttributes, n);
	}
}

void Processor::addNameAndColourListener(NameAndColourListener* l)
{
	uint8 slotIndex = static_cast<uint8>(SlotTypes::NameAndColour);
	l->addListenerToSingleSource(this,  &slotIndex, 1, sendNotificationAsync);
}

void Processor::removeNameAndColourListener(NameAndColourListener* l)
{
	l->removeListener(*this, sendNotificationAsync);
}

bool Processor::isBypassed() const noexcept
{
	return cachedBypassValue;	
}

Modulator::Modulator(ProcessorHandler& h, SourceOwner& owner, const HashedCharPtr& id):
	Processor(h, owner, id),
	intensity(*this, (int)ModulatorSlotTypes::Intensity, IDs::event::intensity),
	other(*this, (int)ModulatorSlotTypes::Other, IDs::event::othermod)
{
	intensity.setNumSlots(1);
	other.setNumSlots(4);
}

Modulator::~Modulator()
{
	intensity.shutdown();
	other.shutdown();
}
} // library
} // dispatch
} // hise