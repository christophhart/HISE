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
namespace dispatch {	

/** The classes in this namespace will copy the existing HISE communication concept without
 *  the real logic. The actual HISE classes itself will have class members of these types
 *	which handle the actual communication.
 *
 *	The rationale behind this is that it can be incrementally changed in the HISE codebase
 *	and the modules itself (= the communication) can be tested separately.
 */
namespace library {
using namespace juce;

class Processor;

enum class ProcessorChangeEvent
{
	Any,
	Macro,
	Preset,
	Children,
	Custom,
	numProcessorChangeEvents,
	Attribute, // will be ignored by the new dispatcher
	Bypassed,
	Intensity,
	OtherUnused
};

/** This will handle the entire processor notification system.
 *  In HISE this will be a member of the MainController
 */
class ProcessorHandler: public SourceManager,
					    public ListenerOwner
{
public:

	enum class SlotTypes
	{
		NameAndColour,
		Bypassed,
		OtherChange,
		AttributeOffset,
		numSlotTypes
	};
	
	/** Add this as a member to your listener class and give it a callback
	 *  that is executed whenever the bypass state changes
	 */
	class BypassListener final: private dispatch::Listener
	{
	public:

		using Callback = std::function<void(dispatch::library::Processor*, bool)>;

		BypassListener(RootObject& r, ListenerOwner& owner, const Callback& f);;
		~BypassListener() override {};
		
	private:

		friend class Processor;

		Callback f;
		void slotChanged(const ListenerData& d) override;
	};

	class AttributeListener final: private dispatch::Listener
	{
	public:

		using Callback = std::function<void(dispatch::library::Processor*, uint16)>;

		AttributeListener(RootObject& r, ListenerOwner& owner, const Callback& f_);;
		~AttributeListener() final {};

	private:

		friend class Processor;
		const Callback f;
		
		void slotChanged(const ListenerData& d) final;
	};

	class NameAndColourListener: public dispatch::Listener
	{
	public:

		using Callback = std::function<void(Processor*)>;

		NameAndColourListener(RootObject& r, ListenerOwner& owner, const Callback& f_);;
		~NameAndColourListener() final {  };

		void callWithProcessor(Processor* p)
		{
			jassert(f);
			f(p);
		}

		void removeFromProcessorHandler(ProcessorHandler& r)
		{
			cleared();
		}

	private:

		friend class Processor;
		const Callback f;
		
		void slotChanged(const ListenerData& d) final;
	};

	class OtherChangeListener: private dispatch::Listener
	{
	public:

		using Callback = std::function<void(Processor*)>;

		OtherChangeListener(RootObject& r, ListenerOwner& owner, const Callback& f_, ProcessorChangeEvent eventToListenTo_):
		  Listener(r, owner),
		  f(f_),
		  eventToListenTo(eventToListenTo_)
		{
		}

	private:
		void slotChanged(const ListenerData& d) final
		{
			jassert(d.s != nullptr);
			auto slotType = (SlotTypes)d.slotIndex;

			if(slotType == SlotTypes::OtherChange)
			{
				auto eventType = (ProcessorChangeEvent)d.indexWithinSlot;

				jassert(MessageManager::getInstance()->isThisTheMessageThread() || 
					    MessageManager::getInstance()->currentThreadHasLockedMessageManager());

				jassert(f);

				auto match = eventToListenTo == ProcessorChangeEvent::Any ||
					         eventType == eventToListenTo;

				if(match)
				{
					TRACE_DISPATCH("otherChange");

					auto obj = d.to_static_cast<Processor>();
					f(obj);
				}
			}
		}

		const Callback f;

		ProcessorChangeEvent eventToListenTo;
		

		friend class Processor;
	};
	
	ProcessorHandler(RootObject& r);

};



/** The communication handler for the hise::Processor base class. It handles changes
 *  to attributes, bypass state and name / ID changes in separate slots
 */
class Processor: public Source
{
public:

	using SlotTypes = ProcessorHandler::SlotTypes;
	using BypassListener = ProcessorHandler::BypassListener;
	using AttributeListener = ProcessorHandler::AttributeListener;
	using OtherChangeListener = ProcessorHandler::OtherChangeListener;
	using NameAndColourListener = ProcessorHandler::NameAndColourListener;

	Processor(ProcessorHandler& h, SourceOwner& owner, const HashedCharPtr& id);;
	~Processor() override;

	/** Call this whenever the attributes change. You can specify a notification type
	 *
	 *	The notification will always be send out synchronously.
	 */
	void setAttribute(int parameterIndex, float, DispatchType n);

	/** Call this in the processor if the bypass state changes. This will cache the value
	 *  so that the BypassListener API is consistent with the current HISE API.
	 */
	void setBypassed(bool value, DispatchType n);

	/** Call this in the processor if the ID changes. */
	void setId(HashedCharPtr&& id);

	/** Call this in the processor if the colour changes. */
	void setColour(const Colour& );

	/** Call this if you want to notify the processor listeners about any other change. */
	void sendChangeMessage(dispatch::library::ProcessorChangeEvent eventType, DispatchType n=sendNotificationAsync)
	{
		otherChange.sendChangeMessage(static_cast<int>(eventType), n);
	}

	/** Call this in the processor if the amount of attributes change. */
	void setNumAttributes(uint16 numAttributes);

	/** Adds a Bypass listener to receive notifications. */
	void addBypassListener(BypassListener* l, DispatchType n);

	/** Removes a bypass listener from all listener queues (both sync and async). */
	void removeBypassListener(BypassListener* l);

	void addAttributeListener(AttributeListener* l, uint16 singleIndex, DispatchType n);

	void addAttributeListener(AttributeListener* l, const uint16* attributeIndexes, size_t numAttributes, DispatchType n);

	void removeAttributeListener(AttributeListener* l, DispatchType n=DispatchType::sendNotification);

	/** Add a listener to be notified about various other events. */
	void addOtherChangeListener(OtherChangeListener* l, DispatchType n)
	{
		uint8 slots = (uint8)SlotTypes::OtherChange;

		if(l->eventToListenTo == ProcessorChangeEvent::Any)
		{
			l->addListenerToSingleSource(this, &slots, 1, n);
		}
		else
		{
			l->addListenerToSingleSlotIndexWithinSlot(this, slots, (uint8)l->eventToListenTo, n);
		}

		
	}

	/** Removes a listener that is notified about other events. */
	void removeOtherChangeListener(OtherChangeListener* l)
	{
		l->removeListener(*this);
	}

	void addNameAndColourListener(NameAndColourListener* l, DispatchType n = DispatchType::sendNotificationSync);

	void removeNameAndColourListener(NameAndColourListener* l);

	bool isBypassed() const noexcept;
	
	int getNumSlotSenders() const override;

	SlotSender* getSlotSender(uint8 slotSenderIndex) override;

private:

	SlotSender* getAttributeSender(uint16 attributeIndex)
	{
		auto slotIndex = (uint8)(attributeIndex / SlotBitmap::getNumBits());
		if(slotIndex == 0)
			return &attributes;
		else
		{
			jassert(isPositiveAndBelow(slotIndex-1, additionalAttributes.size()));
			return additionalAttributes[slotIndex - 1];
		}
	}

	bool cachedBypassValue = false;
	
	SlotSender nameAndColour;
	SlotSender bypassed;
	SlotSender otherChange;

	// the first 32 attribute slots are a class member
	SlotSender attributes;

	// further attributes are stored in a dynamic array
	OwnedArray<SlotSender> additionalAttributes;
};

class Modulator final: public Processor
{
public:

	enum class ModulatorSlotTypes
	{
		Intensity,
		Other,		// monophonic / retrigger, etc
		numModulatorSlotTypes
	};

	Modulator(ProcessorHandler& h, SourceOwner& owner, const HashedCharPtr& id);;
	~Modulator() override;

	void setIntensity(float newIntensity)
	{
		intensity.sendChangeMessage(0, sendNotificationSync);
	}

	SlotSender intensity;
	SlotSender other;
};

} // library
} // dispatch
} // hise