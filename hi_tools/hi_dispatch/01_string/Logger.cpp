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


Logger::Logger(RootObject& root, size_t numAllocated):
Queueable(root),
SimpleTimer(root.getUpdater()),
messageQueue(root, numAllocated)
{
    messageQueue.setLogger(this);
}

void Logger::flush()
{
    TRACE_DISPATCH("flush logger");
    messageQueue.flush(logToDebugger, Queue::FlushType::Flush);
}

void Logger::printRaw(const char* rawMessage, size_t numBytes)
{
    TRACE_DISPATCH("push raw message");
    messageQueue.push(this, EventType::LogRawBytes, reinterpret_cast<const uint8*>(rawMessage), numBytes);
}

void Logger::printString(const String& message)
{
    TRACE_DISPATCH("push string message");
    messageQueue.push(this, EventType::LogString, reinterpret_cast<uint8*>(message.getCharPointer().getAddress()), message.length());
}

void Logger::log(Queueable* source, EventType l, const void* data, size_t numBytes)
{
    StringBuilder b;
    b << source->getDispatchId() << l;
    TRACE_DISPATCH(perfetto::DynamicString(b.get(), b.length()));
    messageQueue.push(source, l, data, numBytes);
}

void Logger::printQueue(Queue& queue)
{
    uint8 counter = 0;
    printString("queue content");
    queue.flush([&](const Queue::FlushArgument& f)
                {
        StringBuilder s;
        s << "  q[" << String(counter++) << "] = " << f;
        log(this, EventType::LogString, s.get(), s.length());
        return true;
    }, Queue::FlushType::KeepData);
    printString("end of queue");
    flush();
}

void Logger::timerCallback()
{
    flush();
}

bool Logger::logToDebugger(const Queue::FlushArgument& f)
{
    StringBuilder s;
    s << f;
    DBG(s.toString());
    return true;
}


} // dispatch
} // hise