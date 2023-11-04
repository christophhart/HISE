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

#include "JuceHeader.h"

namespace hise {
namespace dispatch {	
namespace library {
using namespace juce;

ProcessorHandler::BypassListener::BypassListener(RootObject& r, const Callback& f_):
	Listener(r),
	f(f_)
{}


void ProcessorHandler::BypassListener::slotChanged(const ListenerData& d)
{
	jassert(d.slotIndex == (uint8)SlotTypes::Bypassed);
	jassert(d.numBytes == 1);
	jassert(d.t == EventType::SlotChange);
	jassert(f);

	const auto obj = d.to_static_cast<Processor>();
	f(obj, obj->isBypassed());
}

void ProcessorHandler::AttributeListener::slotChanged(const ListenerData& d)
{
	jassert(d.slotIndex == (uint8)SlotTypes::Bypassed);
	jassert(d.numBytes == 1);
	jassert(d.t == EventType::SlotChange);
	auto obj = d.to_static_cast<Processor>();
	d.callForEachSetValue([&](uint8 index){ f(obj, index); });
}

ProcessorHandler::ProcessorHandler(RootObject& r):
  SourceManager(r, IDs::source::modules)
{}

void Processor::addBypassListener(BypassListener* l, NotificationType n)
{
	uint8 slotIndex = (uint8)SlotTypes::Bypassed;
	l->addListenerToSingleSource(this, &slotIndex, 1, n);
}

void Processor::removeBypassListener(BypassListener* l)
{
	l->removeListener(parent, sendNotificationSync);
	l->removeListener(parent, sendNotificationAsync);
}
} // library
} // dispatch
} // hise