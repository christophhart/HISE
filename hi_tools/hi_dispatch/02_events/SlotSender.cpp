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



SlotSender::SlotSender(Source& s, uint8 index_, const HashedCharPtr& id_):
	index(index_),
	id(id_),
	obj(s),
	numSlots(0)
{
}

SlotSender::~SlotSender()
{
#if JUCE_DEBUG

	// If this happens, you have forgot to call shutdown in the destructor
	// of your Source subclass' destructor. Make sure to call it there
	// to ensure correct order of deinitialisation.
	jassert(shutdownWasCalled);
#endif
}

void SlotSender::setNumSlots(int newNumSlots)
{
	if(numSlots < newNumSlots)
	{
		numSlots = newNumSlots;

		data.forEach([&](DataType& d)
		{
			d.first.setSize(newNumSlots+1);
			const auto ptr = (uint8*)d.first.getObjectPtr();
			ptr[0] = index;
			d.second = false;
		});
	}
}

bool SlotSender::flush(DispatchType n)
{
	// make sure to flush the other slots first
	if(n == DispatchType::sendNotificationAsync ||
	   n == DispatchType::sendNotificationAsyncHiPriority)
	{
		flush(DispatchType::sendNotificationSync);
	}

	if(n == DispatchType::sendNotificationAsync)
	{
		flush(DispatchType::sendNotificationAsyncHiPriority);
	}

	if(!data.get(n).second)
		return false;

	auto ptr = static_cast<uint8*>(data.get(n).first.getObjectPtr());
	obj.getParentSourceManager().sendSlotChanges(obj, ptr, numSlots + 1, n);
	memset(ptr + 1, 0, numSlots);
	data.get(n).second = false;
	return true;
}

bool SlotSender::sendChangeMessage(int indexInSlot, DispatchType n)
{
	StringBuilder b;
	b << "send message" << indexInSlot << ": " << n;
	TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));

	if(!isPositiveAndBelow(indexInSlot, numSlots))
	{
		jassertfalse;
		return false;
	}

	// Write the change in all slot values
	data.forEach([indexInSlot](DataType& d)
	{
		// d[0] points to the slotIndex
		const auto ptr = static_cast<uint8*>(d.first.getObjectPtr()) + 1;

		if(ptr[indexInSlot])
			return;

		ptr[indexInSlot] = true;
		d.second = true;
	});

	// If the message wasn't explicitely sent as sendNotificationAsync, it will flush the sync changes immediately
	if(n == sendNotification || n == sendNotificationSync)
	{
		flush(sendNotificationSync);
	}
	
	return true;
}


} // dispatch
} // hise