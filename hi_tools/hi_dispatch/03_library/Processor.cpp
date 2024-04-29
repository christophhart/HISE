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
	jassert(f);

	auto slotType = (SlotTypes)d.slotIndex;
    ignoreUnused(slotType);
	jassert(slotType == SlotTypes::Bypassed);
	TRACE_DISPATCH("onBypass");

	const auto obj = d.to_static_cast<Processor>();
	f(obj, obj->isBypassed());
}

ProcessorHandler::AttributeListener::AttributeListener(RootObject& r, ListenerOwner& owner, const Callback& f_):
	Listener(r, owner),
	f(f_)
{}

#ifndef TRACE_DISPATCH_CALLBACK
#define TRACE_DISPATCH_CALLBACK(obj, callbackName, arg) StringBuilder n; n << (obj).getDispatchId() << "." << callbackName << "(" << (int)arg << ")"; TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(n));
#endif

void ProcessorHandler::AttributeListener::slotChanged(const ListenerData& d)
{
	auto obj = d.to_static_cast<Processor>();

	jassert(d.slotIndex >= (uint8)SlotTypes::AttributeOffset);

	auto attributeOffset = (uint16)((d.slotIndex - (uint8)SlotTypes::AttributeOffset) * SlotBitmap::getNumBits());

	if(d.t == EventType::SingleListenerSingleSlot)
	{
		auto slotIndex = (attributeOffset + d.toSingleSlotIndex());
		TRACE_DISPATCH_CALLBACK(*obj, "onAttribute", slotIndex);
		f(obj, slotIndex);
	}
	else
	{
		auto bm = d.toBitMap();

		for(int i = 0; i < bm.getNumBits(); i++)
		{
			if(bm[i])
			{
				TRACE_DISPATCH_CALLBACK(*obj, "onAttribute", attributeOffset + i);
				f(obj, attributeOffset + i);
			}
		}
	}
}

ProcessorHandler::NameAndColourListener::NameAndColourListener(RootObject& r, ListenerOwner& owner, const Callback& f_):
	Listener(r, owner),
	f(f_)
{}

void ProcessorHandler::NameAndColourListener::slotChanged(const ListenerData& d)
{
	jassert(d.s != nullptr);
	auto slotType = (SlotTypes)d.slotIndex;
    ignoreUnused(slotType);
	jassert(slotType == SlotTypes::NameAndColour);
	jassert(f);

	TRACE_DISPATCH("onNameOrColourChange");

	auto obj = d.to_static_cast<Processor>();
	f(obj);
}

ProcessorHandler::ProcessorHandler(RootObject& r):
  SourceManager(r, IDs::source::modules)
{}


Processor::Processor(ProcessorHandler& h, SourceOwner& owner, const HashedCharPtr& id):
	Source(h, owner, id),
	attributes(*this, (int)SlotTypes::AttributeOffset, IDs::event::attribute),
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
	{
		jassert(n != sendNotification);

		if(auto a = getAttributeSender(parameterIndex))
			a->sendChangeMessage(static_cast<uint8>(parameterIndex % SlotBitmap::getNumBits()), n);
		else
		{
			jassertfalse;
		}
	}
}

void Processor::setBypassed(bool value, DispatchType n)
{
	if(value != cachedBypassValue)
	{
		StringBuilder b;
		b << getDispatchId() << "::setBypassed()";
		TRACE_DYNAMIC_DISPATCH(b);
		cachedBypassValue = value;
		bypassed.sendChangeMessage(0, n);
	}
}

void Processor::setId(HashedCharPtr&& id)
{
	setSourceId(HashedCharPtr(id));
	nameAndColour.sendChangeMessage(0, sendNotificationSync);
}

void Processor::setColour(const Colour&)
{
	nameAndColour.sendChangeMessage(1, sendNotificationAsync);
}

void Processor::setNumAttributes(uint16 numAttributes)
{
	if(isPositiveAndBelow(numAttributes, SlotBitmap::getNumBits()))
	{
		attributes.setNumSlots(numAttributes);
	}
	else
	{
		attributes.setNumSlots(static_cast<uint8>(SlotBitmap::getNumBits()));

		auto numAttributeSlotsRequired = (static_cast<size_t>(numAttributes) / SlotBitmap::getNumBits()) + 1;
		auto lastSlotAmount = static_cast<uint8>(numAttributes % SlotBitmap::getNumBits());

		auto numAdditionalSlots = (numAttributeSlotsRequired - 1);

		// add the missing slots
		for(int i = additionalAttributes.size(); i < numAdditionalSlots; i++)
			additionalAttributes.add(new SlotSender(*this, (uint8)i + (uint8)SlotTypes::numSlotTypes, IDs::event::attribute));

		// update the slot amounts so that the last one has lastSlotAmount and the other the full 32 slots...
		for(int i = 0; i < additionalAttributes.size(); i++)
		{
			auto sl = additionalAttributes[i];
			auto lastSlot = additionalAttributes.getLast() == sl;
			sl->setNumSlots(lastSlot ? lastSlotAmount : static_cast<uint8>(SlotBitmap::getNumBits()));
		}
	}
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

void Processor::addAttributeListener(AttributeListener* l, uint16 singleIndex, DispatchType n)
{
	addAttributeListener(l, &singleIndex, 1, n);
}

void Processor::addAttributeListener(AttributeListener* l, const uint16* attributeIndexes, size_t numAttributes,
                                     DispatchType n)
{
	if(numAttributes == 1)
	{
		auto a = *attributeIndexes;

		if(auto s = getAttributeSender(a))
		{
			auto indexWithinSlot = (uint8)(a % SlotBitmap::getNumBits());

			l->addListenerToSingleSlotIndexWithinSlot(this, s->getSlotIndex(), indexWithinSlot, n);
		}
		else
		{
			jassertfalse;
		}
	}
	else
	{
		// Sort the incoming indexes (might have any order)
		Array<uint16> sorted(attributeIndexes, (int)numAttributes);
		uint8 currentSlotIndex = attributes.getSlotIndex();
		sorted.sort();

		uint8 numUsed = 0;
		static constexpr size_t NumSlots = SlotBitmap::getNumBits();
		std::array<uint8, NumSlots> currentSubSet;

		memset(currentSubSet.data(), 0, sizeof(currentSubSet));

		auto addCurrent = [&]()
		{
			if(numUsed == 0)
				return;

			if(numUsed == 1)
			{
				auto v = currentSubSet[0];
				jassert(isPositiveAndBelow(v, (uint8)SlotBitmap::getNumBits()));
				l->addListenerToSingleSlotIndexWithinSlot(this, currentSlotIndex, v, n);
			}
			else
				l->addListenerToSingleSourceAndSlotSubset(this, currentSlotIndex, currentSubSet.data(), numUsed, n);
		};

		for(auto i: sorted)
		{
			if(auto attributeSlot = getAttributeSender(i))
			{
				auto thisSlot = attributeSlot->getSlotIndex();
				jassert(thisSlot != 0);

				if(currentSlotIndex != thisSlot)
				{
					addCurrent();
					memset(currentSubSet.data(), 0, sizeof(currentSubSet));
					numUsed = 0;
					currentSlotIndex = thisSlot;
				}

				currentSubSet[numUsed++] = static_cast<uint8>(i % SlotBitmap::getNumBits());
			}
		}

		addCurrent();
	}
}

void Processor::removeAttributeListener(AttributeListener* l, DispatchType n)
{
	l->removeListener(*this, n);
}

void Processor::addNameAndColourListener(NameAndColourListener* l, DispatchType n)
{
	uint8 slotIndex = static_cast<uint8>(SlotTypes::NameAndColour);
	l->addListenerToSingleSource(this,  &slotIndex, 1, n);
}

void Processor::removeNameAndColourListener(NameAndColourListener* l)
{
	l->removeListener(*this);
}

bool Processor::isBypassed() const noexcept
{
	return cachedBypassValue;	
}

int Processor::getNumSlotSenders() const
{
	return (int)SlotTypes::numSlotTypes + additionalAttributes.size();
}

SlotSender* Processor::getSlotSender(uint8 slotSenderIndex)
{
	if(isPositiveAndBelow(slotSenderIndex, SlotTypes::numSlotTypes))
	{
		switch((SlotTypes)slotSenderIndex)
		{
		case SlotTypes::AttributeOffset:		return &attributes;
		case SlotTypes::NameAndColour:  return &nameAndColour;
		case SlotTypes::Bypassed:		return &bypassed;
		case SlotTypes::OtherChange:	return &otherChange;
		default: jassertfalse;			return nullptr;
		}
	}

	return additionalAttributes[slotSenderIndex - (uint8)SlotTypes::numSlotTypes];
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
