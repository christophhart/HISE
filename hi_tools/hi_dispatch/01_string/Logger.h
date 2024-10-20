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



class Logger final : public  PooledUIUpdater::SimpleTimer,
                      public Queueable
{
public:

	Logger(RootObject& root, size_t numAllocated);;
	~Logger()
	{
		cleared();
	}

	void flush();

	HashedCharPtr getDispatchId() const override { return "logger"; }

	void printRaw(const char* rawMessage, size_t numBytes);
	void printString(const String& message);
	void log(Queueable* source, EventType l, const void* data, size_t numBytes);
	void printQueue(Queue& queue);
    
private:

	void timerCallback() override;
    
	static bool logToDebugger(const Queue::FlushArgument& f);

	Queue messageQueue;
};

} // dispatch
} // hise