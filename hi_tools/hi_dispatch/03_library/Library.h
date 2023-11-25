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
using namespace juce;


template <typename T> class SingleValueSource: public Source
{
public:

	class Listener final : private dispatch::Listener
	{
	public:

		using Callback = std::function<void(int, T)>;

		Listener(RootObject& r, ListenerOwner& o, const Callback& c):
		  dispatch::Listener(r, o),
		  f(c)
		{};

	private:

		void slotChanged(const ListenerData& d) override
		{
			jassert(d.t == EventType::ListenerWithoutData);
			jassert(f);
			jassert(d.s != nullptr);
			
			auto cd = static_cast<SingleValueSource*>(d.s);

#if PERFETTO
			StringBuilder b;
			b << "listener callback for " << cd->getDispatchId();
			TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
#endif

			f(cd->index, cd->value);
		}

		friend class SingleValueSource;

		Callback f;
	};

	SingleValueSource(SingleValueSourceManager<T>& parent, SourceOwner& owner, int index_, const HashedCharPtr& id):
	  Source(parent, owner, id),
	  index(index_),
	  valueSender(*this, 0, "value")
	{
		valueSender.setNumSlots(1);
	};

	void addValueListener(Listener* l, bool initialiseValue, DispatchType n)
	{
		l->addListenerWithoutData(this, 0, n);

		//valueSender.getListenerQueue(n)->push(l, EventType::ListenerWithoutData, nullptr, 0);

		if(initialiseValue)
		{
			StringBuilder n;
			n << "init call " << getDispatchId();
			TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(n));
			dispatch::Listener::ListenerData d;
			d.t = EventType::ListenerWithoutData;
			d.s = this;
			d.slotIndex = 0;
			l->slotChanged(d);
		}
	}

	void removeValueListener(Listener* l, DispatchType n=DispatchType::sendNotification)
	{
		l->removeListener(*this, n);
	}

#if 0
	template <typename ListenerType> int getNumListenersWithClass(DispatchType n=DispatchType::sendNotification) const
	{
		int numListeners = 0;

		const_cast<SingleValueSource*>(this)->forEachListenerQueue(n, [&numListeners](uint8, DispatchType, ListenerQueue* q)
		{
			jassertfalse;
			
#if 0
			q->flush([&numListeners](const Queue::FlushArgument& f)
			{
				auto& l = f.getTypedObject<dispatch::Listener>();

				auto t = dynamic_cast<ListenerType*>(&l.getOwner<ListenerOwner>());

				if(t != nullptr)
					numListeners++;

				return true;
			}, Queue::FlushType::KeepData);
#endif
		});

		return numListeners;
	}
#endif

	void setValue(T v, DispatchType n)
	{
#if PERFETTO
		StringBuilder b;
		b << getDispatchId() << "(" << index << ")";
		TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
#endif

		value = v;
		valueSender.sendChangeMessage(0, n);
	}

	int getNumSlotSenders() const override { return 1; }
	SlotSender* getSlotSender(uint8) override { return &valueSender; }

private:

	T value = T();
	const int index;
	SlotSender valueSender;
};


namespace library
{

class CustomAutomationSourceManager: public SingleValueSourceManager<float>
{
public:

	CustomAutomationSourceManager(RootObject& r):
	  SingleValueSourceManager<float>(r, "automation")
	{};
};

using CustomAutomationSource = SingleValueSource<float>;

class MacroControlSourceManager: public SingleValueSourceManager<float>
{
public:

	MacroControlSourceManager(RootObject& r):
	  SingleValueSourceManager<float>(r, "macros")
	{};
};

using MacroControlSource = SingleValueSource<float>;

class GlobalCableSourceManager: public SingleValueSourceManager<double>
{
	GlobalCableSourceManager(RootObject& r):
	  SingleValueSourceManager<double>(r, "cables")
	{};
};

using GlobalCableSource = SingleValueSource<float>;

}

} // dispatch
} // hise