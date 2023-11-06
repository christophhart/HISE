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


// A object that can send slot change messages to a SourceManager
// Ownership: a member of a subclassed Source object
// Features: 
// - dynamic number of slots
// - a constant sender index (one byte)
// - no heap allocation for <16 byte senders
struct SlotSender
{		
	SlotSender(Source& s, uint8 index_, const HashedCharPtr& id_);;
	~SlotSender();

	void setNumSlots(int newNumSlots);
	bool flush(NotificationType n=sendNotification);
	bool sendChangeMessage(int indexInSlot, NotificationType notify);

	/** Flushes the change if it's not async. */
	void flushAsyncChanges();

	// Todo: clear the parent queue with all pending messages. */
	void shutdown()
	{
#if JUCE_DEBUG
		shutdownWasCalled = true;
#endif
	};

private:

#if JUCE_DEBUG
	bool shutdownWasCalled = false;
#endif
	
	Source& obj;

	const uint8 index;
	const HashedCharPtr id;

	ObjectStorage<64, 16> data;
	size_t numSlots;
	bool pending = false;
};

} // dispatch
} // hise