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



StringBuilder::StringBuilder(size_t numToPreallocate)
{
    if (numToPreallocate != 0)
        data.setSize(numToPreallocate);
}

StringBuilder& StringBuilder::operator<<(const char* rawLiteral)
{
	const auto num = strlen(rawLiteral);
    memcpy(getWriteHeadAndAdvance(num), rawLiteral, num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const CharPtr& p)
{
	const auto num = p.length();
    memcpy(getWriteHeadAndAdvance(num), p.get(), num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const HashedCharPtr& p)
{
	const auto num = p.length();
    memcpy(getWriteHeadAndAdvance(num), p.get(), num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(const String& s)
{
	const auto num = s.length();
    memcpy(getWriteHeadAndAdvance(num), s.getCharPointer().getAddress(), num);
    return *this;
}

StringBuilder& StringBuilder::operator<<(int number)
{
    char buffer[8];
    auto numWritten = snprintf(buffer, 8, "%d", number);
    memcpy(getWriteHeadAndAdvance(numWritten), buffer, numWritten);
    return *this;
}

StringBuilder& StringBuilder::operator<<(uint8 number)
{
	return *this << (int)number;
}

StringBuilder& StringBuilder::operator<<(size_t number)
{
	return *this << (int)number;
}

StringBuilder& StringBuilder::operator<<(const StringBuilder& other)
{
	const auto num = other.length();
    memcpy(getWriteHeadAndAdvance(num), other.get(), num);
    return *this;
}

#if ENABLE_QUEUE_AND_LOGGER
StringBuilder& StringBuilder::operator<<(const Queue::FlushArgument& f)
{
	using namespace hise::dispatch;
	auto& s = *this;
        
	s << f.source->getDispatchId() << ":\t";
	s.appendEventValues(f.eventType, f.data, f.numBytes);

	return *this;
}
#endif

StringBuilder& StringBuilder::operator<<(EventType eventType)
{
	auto& s = *this;

	switch (eventType) {
	case EventType::Nothing:		s << "nop"; break;
	case EventType::Warning:        s << "warning:  "; break;
	case EventType::LogString:		s << "log_string"; break;
	case EventType::LogRawBytes:    s << "log_rbytes"; break;
	case EventType::Add:			s << "add_source"; break;
	case EventType::Remove:			s << "rem_source"; break;
	case EventType::SlotChange:		s << "slotchange"; break;
	case EventType::AllListener:	s << "listen_all"; break;
	case EventType::numEventTypes: break;

	default: jassertfalse;
	}
	return *this;
}

StringBuilder& StringBuilder::operator<<(DispatchType notificationType)
{
	auto& s = *this;

	switch (notificationType) {
	case dontSendNotification: s << "ignore"; break;
	case sendNotification: s << "send"; break;
	case sendNotificationSync: s << "sync"; break;
	case sendNotificationAsync: s << "async"; break;
	case sendNotificationAsyncHiPriority: s << "async hiprio"; break;
	default: break;
	}
	return *this;
}

StringBuilder& StringBuilder::appendEventValues(EventType eventType, const uint8* values, size_t numBytes)
{
	auto& s = *this;

    s << eventType;
	s << " ";
    
	switch (eventType) {
	case EventType::Nothing:        break;
	case EventType::Warning:        
	case EventType::LogString:      s << String::createStringFromData(values, (int)numBytes); break;
	case EventType::LogRawBytes:    appendRawByteArray(values, numBytes); break;
	case EventType::Add:            
	case EventType::Remove:         s << (int)*values; break;
	case EventType::SlotChange:     appendRawByteArray(values, numBytes); break;
	case EventType::AllListener:    jassertfalse; break;
	case EventType::numEventTypes:  jassertfalse; break;
	
	default: ;
	}
    
	return *this;
}

StringBuilder& StringBuilder::appendRawByteArray(const uint8* values, size_t numBytes)
{
	auto& s = *this;
	s << "[ ";
	for(int i = 0; i < numBytes; i++)
	{
		s << (int)values[i];

		if(i != (numBytes-1))
			s << ", ";
	}
	s << " ] (";
	s << numBytes << " bytes)";
	return *this;
}

String StringBuilder::toString() const noexcept
{ return String(get(), length()); }

const char* StringBuilder::get() const noexcept
{ return static_cast<const char*>(data.getObjectPtr()); }

const char* StringBuilder::end() const noexcept
{ return static_cast<char*>(data.getObjectPtr()) + position; }

size_t StringBuilder::length() const noexcept
{ return position; }

void StringBuilder::ensureAllocated(size_t num)
{
    data.ensureAllocated(position + num + 1, true);
}

char* StringBuilder::getWriteHead() const
{
    return static_cast<char*>(data.getObjectPtr()) + position;
}

char* StringBuilder::getWriteHeadAndAdvance(size_t numToWrite)
{
    ensureAllocated(numToWrite);
    const auto ptr = getWriteHead();
    position += numToWrite;
    *getWriteHead() = 0;
    return ptr;
}


} // dispatch
} // hise