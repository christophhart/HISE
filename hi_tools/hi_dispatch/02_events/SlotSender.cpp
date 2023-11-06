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
		data.setSize(newNumSlots + 1);
		numSlots = newNumSlots;
		const auto ptr = (uint8*)data.getObjectPtr();
		ptr[0] = index;
	}
}

bool SlotSender::flush(NotificationType n)
{
	if(!pending)
		return false;
			
	obj.getParentSourceManager().sendSlotChanges(obj, static_cast<uint8*>(data.getObjectPtr()), numSlots + 1, n);
	memset((uint8*)data.getObjectPtr() + 1, 0, numSlots);
	pending = false;
	return true;
}

bool SlotSender::sendChangeMessage(int indexInSlot, NotificationType notify)
{
	jassert(isPositiveAndBelow(indexInSlot, numSlots));

	// data[0] points to the slotIndex
	const auto ptr = static_cast<uint8*>(data.getObjectPtr()) + 1;

	if(ptr[indexInSlot])
		return false;
			
	ptr[indexInSlot] = true;
	pending = true;
		
	if(notify == sendNotificationSync)
		flush();
			
	return true;
}

void SlotSender::flushAsyncChanges()
{
	if(pending)
		flush(sendNotificationAsync);
}
} // dispatch
} // hise