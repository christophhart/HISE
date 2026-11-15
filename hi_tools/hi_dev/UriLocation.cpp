#include "UriLocation.h"
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
using namespace juce;


var hise::UriLocation::toJSON() const
{
	auto no = new DynamicObject();

	if(line != -1)
		no->setProperty("line", line);

	if(line != -1)
		no->setProperty("column", col);

	if(line != -1)
		no->setProperty("charIndex", charIndex);

	no->setProperty("filePath", uri);
	no->setProperty("processorId", processorId);
	return var(no);
}

UriLocation UriLocation::fromEncodedString(const String& base64String)
{
	MemoryBlock mb;

	auto trimmed = base64String.trim().trimCharactersAtStart("{").trimCharactersAtEnd("}");

	MemoryOutputStream out;
	

	if (Base64::convertFromBase64(out, trimmed))
	{
		auto decoded = out.toString();
		auto tokens = StringArray::fromTokens(decoded, "|", "");

		jassert(tokens.size() == 5);

		UriLocation loc;
		loc.processorId = tokens[0];
		loc.uri = tokens[1];
		loc.charIndex = tokens[2].getIntValue();
		loc.col = tokens[3].getIntValue();
		loc.line = tokens[4].getIntValue();

		return loc;
	}
	
	return {};
}

void UriLocation::fillLocation(const var& uriObj)
{
	line = (int)uriObj["line"];
	col = (int)uriObj["column"];
	charIndex = (int)uriObj["charIndex"];
	uri = uriObj["filePath"].toString();
	processorId = uriObj["processorId"].toString();
}

bool UriLocation::fillLineAndCol(const juce::CodeDocument& doc)
{
	if (!hasResolvedLineAndColNumbers() && charIndex != -1)
	{
		juce::CodeDocument::Position p(doc, charIndex);
		fillLocation(p);
		return true;
	}

	return false;
}

bool UriLocation::hasResolvedLineAndColNumbers() const
{
	return line != -1 && col != -1;
}

File UriLocation::getFile(const File& rootDir) const
{
	jassert(getUriType() == UriType::ExternalFile);

	if (File::isAbsolutePath(uri))
		return File(uri);

	return rootDir.getChildFile(uri);
}

UriLocation::UriType UriLocation::getUriType() const
{
	if (uri.isEmpty() || uri.startsWith("onInit"))
		return UriType::onInit;

	if (File::isAbsolutePath(uri))
		return UriType::ExternalFile;

	if (uri.containsChar('('))
		return UriType::OtherCallback;

	return UriType::ExternalFile;
}

bool UriLocation::matchesFile(const File& f) const
{
	if (getUriType() != UriType::ExternalFile)
		return false;

	UriLocation otherLoc;
	otherLoc.processorId = processorId;
	otherLoc.uri = f.getFullPathName();

	auto rp1 = otherLoc.toLocationString(UriFormat::RelativePath);
	auto rp2 = toLocationString(UriFormat::RelativePath);

	return rp1 == rp2;
}

bool UriLocation::matches(const UriLocation& other) const
{
	return other.charIndex == charIndex &&
		other.uri == uri &&
		other.processorId == processorId;
}

bool UriLocation::sameUriAndProcessorId(const UriLocation& other) const
{
	return processorId == other.processorId &&
		   toLocationString(UriFormat::ConnectionId) == other.toLocationString(UriFormat::ConnectionId);
}

String UriLocation::toLocationString(UriFormat f) const
{
	String msg;

	if (f == UriFormat::Callback)
	{
		if (uri.isEmpty())
			msg << "onInit()";
		else
			msg << uri;
	}
	if (f == UriFormat::CallbackId)
	{
		if (uri.isEmpty())
			msg << "onInit";
		else if (uri.containsChar('('))
			msg << uri.removeCharacters("()");
		else
			msg << uri;
	}
	if (f == UriFormat::Compact)
	{
		msg << toLocationString(UriFormat::ConnectionId) << "(" << String(line) << ")";
	}
	if (f == UriFormat::LineAndCol)
	{
		if(getUriType() == UriType::ExternalFile)
			msg << toLocationString(UriFormat::RelativePath) << " (" << String(line) << ")";
		else
			msg << "Line " << String(line) << ", column " << String(col);
	}
	if (f == UriFormat::ConnectionId)
	{
		if (getUriType() == UriType::ExternalFile)
			return toLocationString(UriFormat::RelativePath);
		else
			return toLocationString(UriFormat::CallbackId);
	}
	if (f == UriFormat::RelativePath)
	{
		if (getUriType() == UriType::ExternalFile)
		{
			auto cleaned = uri.replaceCharacter('\\', '/');

			if (File::isAbsolutePath(uri))
			{
				if (cleaned.contains("Scripts/"))
					return cleaned.fromLastOccurrenceOf("Scripts/", false, false);

				return File(cleaned).getFileName();
			}

			return cleaned;
		}

		return uri;
	}
	if (f == UriFormat::Base64)
	{
		jassert(processorId.isNotEmpty());

		msg << processorId << "|";
		msg << toLocationString(UriFormat::RelativePath) << "|";

		msg << String(charIndex) << "|";
		msg << String(col) << "|";
		msg << String(line);

		return "{{" + Base64::toBase64(msg) + "}}";
	}
	if (f == UriFormat::FilePath)
	{
		msg << uri.replaceCharacter('\\', '/');
	}
	if (f == UriFormat::GotoString)
	{
		msg << "goto ";
		msg << toLocationString(UriFormat::CallbackId);
		msg << "@" << String(charIndex);
	}

	return msg;
}

UriLocation::ResolveState UriLocation::getResolveState() const
{
	int x = 0;

	if (processorId.isNotEmpty())
		x |= (int)(ResolveState::PathResolved);

	if (charIndex != -1)
		x |= (int)(ResolveState::RawCharIndex);

	if (line != -1 && col != -1)
		x |= (int)(ResolveState::LineResolved);

	return (ResolveState)x;
}

void UriLocation::fillLocation(const UriLocation& other)
{
	line = other.line;
	col = other.col;
	charIndex = other.charIndex;
	uri = other.uri;
	processorId = other.processorId;
}

void UriLocation::fillLocation(const CodeDocument::Position& pos)
{
	line = pos.getLineNumber() + 1;
	col = pos.getIndexInLine() + 1;
	charIndex = pos.getPosition();
}





Colour UriError::getColour(ErrorType t)
{
	switch (t)
	{
	case OK:      return Colour(HISE_OK_COLOUR);
	case Unknown: return Colours::grey;
	case Error:	  return Colour(HISE_ERROR_COLOUR);
	case Warning: return Colour(HISE_WARNING_COLOUR);
	case Info:
	case Hint:	  return Colour(HISE_HINT_COLOUR);
	case Breakpoint:
	case Blink:
	case BreakBlink: return Colour(HISE_BREAKPOINT_COLOUR);
	}
}

bool UriError::sourceAndErrorTypeMatches(SourceType s, ErrorType e)
{
	if (s == SourceType::Compilation)
		return (e & OK) || (e & Error);
	else if (s == SourceType::Diagnostics)
		return (e & OK) || (e & Error) || (e & Warning) || (e & Hint) || (e & Info);
	else if (s == SourceType::Runtime)
		return (e & Error) || (e & Warning);
	else if (s == SourceType::Debug)
		return (e & Breakpoint) || (e & Log) || (e & Blink);
}

UriError UriError::ok()
{
	return UriError();
}

UriError UriError::fail(const String& error)
{
	jassert(!error.startsWith("Line"));
	
	UriError e;
	e.errorType = ErrorType::Error;
	e.errorMessage = error;
	return e;
}

UriError UriError::fromResult(const Result& r, Owner* o)
{
	jassert(!r.getErrorMessage().startsWith("Line"));

	UriError e;
	e.errorType = ErrorType::Error;
	e.errorMessage = r.getErrorMessage();

	if (r.failed() && o != nullptr)
		e.processorId = o->getUriProcessorId();

	return e;
}

bool UriError::wasOk() const
{
	return errorMessage.isEmpty();
}

bool UriError::failed() const
{
	return errorMessage.isNotEmpty();
}

UriError UriError::withProcessorId(Owner* owner) const
{
	if (owner->getUriProcessorId() == processorId)
		return *this;

	auto copy = *this;
	copy.processorId = owner->getUriProcessorId();

	for (auto& s : copy.callstack)
		s.second.processorId = copy.processorId;

	return copy;
}

String UriError::toConsoleMessage() const
{
	if (wasOk())
		return {};

	String msg;

	msg << toLocationString(UriFormat::LineAndCol) << ": ";
	msg << errorMessage << " " << toLocationString(UriFormat::Base64);

	return msg;
}

String UriError::toFullCallStack() const
{
	const String nl = "\n";

	auto s = toConsoleMessage();

	if (!callstack.empty())
	{
		s << nl;

		for (int i = callstack.size() - 1; i >= 0; i--)
		{
			auto entry = callstack[i];
			entry.second.processorId = this->processorId;

			s << ":\t\t\t" << entry.first.upToFirstOccurrenceOf("(", false, false) << "() - ";
			s << entry.second.toLocationString(UriLocation::UriFormat::Base64) << nl;
		}
	}

	return s;
}

var UriError::toJSON() const
{
	auto obj = UriLocation::toJSON();

	if (auto m = metadata.getDynamicObject())
	{
		for (const auto& nv : m->getProperties())
			obj.getDynamicObject()->setProperty(nv.name, nv.value);
	}

	return obj;
}

UriError UriError::withBaseCallstack(const UriLocation::WithExpression& rootStack) const
{
	auto copy = *this;

	copy.callstack.emplace(copy.callstack.begin(), rootStack);

	for (auto& c : copy.callstack)
	{
		c.second.uri = copy.uri;
		c.second.processorId = copy.processorId;
	}

	return copy;
}

void UriError::forInvertedCallstack(const std::function<void(int, const UriLocation::WithExpression&)>& f)
{
	for (int i = callstack.size() - 1; i >= 0; i--)
	{
		f(i, callstack[i]);
	}
}

UriErrorLogger::UriErrorLogger()
{}

UriErrorLogger::~UriErrorLogger()
{
	clearUriErrors(SourceType::All, dontSendNotification);
}

void UriErrorLogger::clearUriErrors(SourceType s, NotificationType n)
{
	{
		ScopedWriteLock sl(lock);

		if (s == SourceType::All)
			data.clear();
		else
			data[s].clear();
	}

	{
		ScopedReadLock sl(lock);

		for (auto l : listeners)
		{
			if (l != nullptr && l->getSourceTypeMask() & s)
				l->onClear(s);
		}

		if (n != sendNotification)
		{
			if (n == sendNotificationSync)
				handleAsyncUpdate();
			else
				triggerAsyncUpdate();
		}
	}
}

void UriErrorLogger::logUriError(SourceType s, const UriError& e, NotificationType n)
{
	jassert(e.sourceType == UriError::SourceType::None);
	jassert(s != SourceType::All && s != SourceType::None);

	std::pair<UriError, int> messageError;

	{
		ScopedWriteLock sl(lock);

		for (auto& ex : data[s])
		{
			jassert(ex.first.sourceType == s);

			if (ex.first.matches(e))
			{
				ex.second++;
				messageError = ex;
				break;
			}
		}

		if (messageError.second == 0)
		{
			messageError = { e, 1 };
			messageError.first.sourceType = s;
			data[s].push_back(messageError);
		}
	}
	
	{
		ScopedReadLock sl(lock);

		for (auto l : listeners)
		{
			if (l != nullptr && l->getSourceTypeMask() & s)
				l->onError(messageError.first, messageError.second);
		}

		if (n != dontSendNotification)
		{
			if (n == sendNotificationSync)
				handleAsyncUpdate();
			else
				triggerAsyncUpdate();
		}
	}
}

void UriErrorLogger::logLocationTree(UriLocationTree::Ptr p, NotificationType n)
{
	if (lastTree != p)
	{
		lastTree = p;

		for (auto l : listeners)
		{
			if (l != nullptr)
				l->onLocationTree(lastTree);
		}

		if (n != dontSendNotification)
		{
			if (n == sendNotificationSync)
				handleAsyncUpdate();
			else
				triggerAsyncUpdate();
		}
	}
}

void UriErrorLogger::addListener(Listener* l, bool initWithErrors)
{
	bool send = initWithErrors;

	{
		ScopedWriteLock sl(lock);
		send &= listeners.addIfNotAlreadyThere(l);
	}

	if (send)
	{
		ScopedReadLock sl(lock);

		l->onLocationTree(lastTree);

		auto mask = l->getSourceTypeMask();

		for (const auto& source : data)
		{
			if (source.first & mask)
			{
				l->onClear(source.first);

				for (const auto& e : source.second)
				{
					jassert(e.first.sourceType == source.first);
					l->onError(e.first, e.second);
				}
			}
		}
	}
}

void UriErrorLogger::removeListener(Listener* l)
{
	ScopedWriteLock sl(lock);
	listeners.removeAllInstancesOf(l);
}

int UriErrorLogger::getNumErrors(UriError::SourceType s) const
{
	ScopedReadLock sl(lock);

	int num = 0;

	for (auto& st : data)
	{
		if (st.first & (int)s)
		{
			num += st.second.size();
		}
	}

	return num;
}

std::vector<std::pair<UriError, int>> UriErrorLogger::getErrorList(UriError::SourceType s) const
{
	std::vector<std::pair<UriError, int>> returnList;

	auto num = getNumErrors(s);

	returnList.reserve(num);

	{
		ScopedReadLock sl(lock);

		for (const auto& d : data)
		{
			if (d.first & (int)s)
			{
				for (auto& e : d.second)
				{
					jassert(e.first.sourceType == d.first);
					returnList.push_back(e);
				}
			}
		}
	}
	
	return returnList;
}

UriBreakpoint UriBreakpoint::fromJSON(const var& obj)
{
	UriBreakpoint bp;
	bp.fillLocation(obj);

	bp.expression = obj["condition"].toString();
	bp.enabled = obj["enabled"];
	bp.breakOnHit = obj["breakOnHit"];
	bp.blinkOnHit = obj["blinkOnHit"];

	return bp;
}

var UriBreakpoint::toJSON() const
{
	auto obj = UriLocation::toJSON();

	auto o = obj.getDynamicObject();

	o->setProperty("condition", expression);
	o->setProperty("enabled", enabled);
	o->setProperty("breakOnHit", breakOnHit);
	o->setProperty("blinkOnHit", blinkOnHit);

	return obj;
}

UriBreakpoint::Owner::~Owner()
{
	clearBreakpoints(sendNotificationSync);
	listeners.clear();
}

/** Adds a breakpoint to this owner and sends a message to its registered listeners. */
std::pair<UriBreakpoint::Owner::AttachState, UriBreakpoint> UriBreakpoint::Owner::addUriBreakpoint(const UriBreakpoint& newBreakpoint, NotificationType n)
{
	auto result = attachBreakpoint(newBreakpoint);

	if (result.first == AttachState::Duplicate)
		return result;

	for (auto& ex : currentBreakpoints)
	{
		if (ex.second.matches(result.second))
		{
			if (ex.first != result.first)
			{
				ex = result;
				sendBreakpointUpdate(ex, UriBreakpointListener::ChangeType::AttachStateChange, n);
			}

			return { AttachState::Duplicate, result.second };
		}
	}

	currentBreakpoints.add(result);
	sendBreakpointUpdate(result, UriBreakpointListener::ChangeType::Added, n);

	return result;
}

/** Call this to remove a breakpoint from this owner. */
bool UriBreakpoint::Owner::removeUriBreakpoint(const UriBreakpoint& newBreakpoint, NotificationType n)
{
	for (int i = 0; i < currentBreakpoints.size(); i++)
	{
		if (newBreakpoint.matches(currentBreakpoints[i].second))
		{
			currentBreakpoints.remove(i);
			sendBreakpointUpdate({ AttachState::Stale, newBreakpoint }, UriBreakpointListener::ChangeType::Removed, n);
			return true;
		}
	}

	return false;
}

bool UriBreakpoint::Owner::modifyUriBreakpoint(const UriBreakpoint& bpToModify, NotificationType n)
{
	for (auto& b : currentBreakpoints)
	{
		if (b.second.matches(bpToModify))
		{
			auto shouldBeStale = b.second.charIndex != bpToModify.charIndex;
			auto isStale = b.first == AttachState::Stale;

			if (isStale != shouldBeStale)
			{
				b.first = shouldBeStale ? AttachState::Stale : AttachState::Unresolved;

				if (b.first == AttachState::Unresolved)
				{
					b = attachBreakpoint(b.second);
				}

				sendBreakpointUpdate(b, UriBreakpointListener::ChangeType::AttachStateChange, n);
				return true;
			}

			if (isStale)
				return false;

			b.second = bpToModify;
			sendBreakpointUpdate(b, UriBreakpointListener::ChangeType::Modified, n);
			return true;
		}
	}

	return false;
}



void UriBreakpoint::Owner::onBreakpointClear()
{
	for (const auto& bp : currentBreakpoints)
		onUriBreakpointChange(bp.second, UriBreakpointListener::ChangeType::Removed);
}

void UriBreakpoint::Owner::clearBreakpoints(NotificationType n)
{
	if (n != dontSendNotification)
	{
		onBreakpointClear();

		for (auto l : listeners)
			l->onUriBreakpointClear();
	}

	touchedBreakpoints.clear();
	currentBreakpoints.clear();
}

UriBreakpoint::Owner::AttachState UriBreakpoint::Owner::sendUriBreakpointEvent(const UriError& breakpointEvent)
{
	UriBreakpoint resolvedBreakpoint;

	if (breakpointEvent.errorType == UriError::ErrorType::Blink)
	{
		for (auto& bp : currentBreakpoints)
		{
			if (bp.second.matches(breakpointEvent))
			{
				resolvedBreakpoint = bp.second;
				break;
			}
		}

		if (!(bool)resolvedBreakpoint)
			return AttachState::Unresolved;

		onUriBreakpointEvent(resolvedBreakpoint, breakpointEvent);

		for (auto l : listeners)
			l->onUriBreakpointEvent(resolvedBreakpoint, breakpointEvent);

		return AttachState::OK;
	}

	if (breakpointEvent.wasOk() && currentBreakpointEvent.wasOk())
		return AttachState::Duplicate;

	if (!breakpointEvent.wasOk() && !currentBreakpointEvent.wasOk())
	{
		if (breakpointEvent.sameUriAndProcessorId(currentBreakpointEvent) &&
			breakpointEvent.charIndex == currentBreakpointEvent.charIndex)
		{
			return AttachState::Duplicate;
		}
	}

	currentBreakpointEvent = breakpointEvent;
	
	if (!currentBreakpointEvent.wasOk())
	{
		jassert(currentBreakpointEvent.processorId == getUriProcessorId());
		jassert(currentBreakpointEvent.sourceType == UriError::SourceType::Debug);
		jassert(currentBreakpointEvent.errorType & UriError::ErrorType::BreakBlink);
	}

	currentBreakpointEvent.sourceType = UriError::SourceType::Debug;

	if (!breakpointEvent.wasOk())
	{
		for (const auto& bp : currentBreakpoints)
		{
			if (bp.first == AttachState::OK)
			{
				if (bp.second.matches(breakpointEvent))
				{
					resolvedBreakpoint = bp.second;

					bool found = false;

					for (auto& ex: touchedBreakpoints)
					{
						if (bp.second.matches(ex.first))
						{
							ex.second++;
							found = true;
							break;
						}
					}

					if (!found)
						touchedBreakpoints.add({ bp.second, 1 });
					
					break;
				}
			}
		}
	}

	onUriBreakpointEvent(resolvedBreakpoint, currentBreakpointEvent);

	for (auto l : listeners)
	{
		if (!l->matchesUriBreakpointEvent(currentBreakpointEvent))
			continue;

		l->onUriBreakpointEvent(resolvedBreakpoint, currentBreakpointEvent);
	}

	return AttachState::OK;
}

void UriBreakpoint::Owner::addUriBreakpointListener(UriBreakpointListener* newListener)
{
	listeners.addIfNotAlreadyThere(newListener);

	if (!currentBreakpointEvent.wasOk())
		newListener->onUriBreakpointEvent({}, currentBreakpointEvent);

	for (auto cbp : currentBreakpoints)
	{
		if (newListener->matchesUriLocation(cbp.second))
		{
			newListener->onUriBreakpointChange(cbp.second, UriBreakpointListener::ChangeType::Added);
			newListener->onUriBreakpointAttachTypeChange(cbp.second, cbp.first);
		}
	}
}

void UriBreakpoint::Owner::removeUriBreakpointListener(UriBreakpointListener* bp)
{
	listeners.removeAllInstancesOf(bp);
}

bool UriBreakpoint::Owner::setBreakpointState(const std::pair<AttachState, UriBreakpoint>& newData, NotificationType n)
{
	for (auto& ex : currentBreakpoints)
	{
		if (ex.second.matches(newData.second))
		{
			ex = newData;
			sendBreakpointUpdate(newData, UriBreakpointListener::ChangeType::AttachStateChange, n);
			return true;
		}
	}

	return false;
}

Array<UriBreakpoint> UriBreakpoint::Owner::resetAttachmentAndCreateBreakpointList()
{
	Array<UriBreakpoint> list;

	currentBreakpointEvent = {};
	touchedBreakpoints.clear();

	for (auto& bp : currentBreakpoints)
	{
		bp.first = AttachState::Unresolved;
		list.add(bp.second);
		sendBreakpointUpdate(bp, UriBreakpointListener::ChangeType::AttachStateChange, sendNotificationSync);
	}

	return list;
}

void UriBreakpoint::Owner::sendBreakpointUpdate(const std::pair<AttachState, UriBreakpoint>& bp, UriBreakpointListener::ChangeType ct, NotificationType n)
{
	if (n != dontSendNotification)
	{
		this->onUriBreakpointChange(bp.second, ct);
		this->onUriBreakpointAttachTypeChange(bp.second, bp.first);

		for (auto l : listeners)
		{
			if (l != nullptr)
			{
				if (!l->matchesUriLocation(bp.second))
					continue;

				l->onUriBreakpointChange(bp.second, ct);
				l->onUriBreakpointAttachTypeChange(bp.second, bp.first);
			}
		}
	}
}

UriLocationTree::UriLocationTree(WeakPtr parent_) :
	parent(parent_),
	data(new DynamicObject())
{
	if (parent != nullptr)
	{
		processorId = parent->processorId;
		parent->children.addIfNotAlreadyThere(this);
	}
}

void UriLocationTree::setParent(WeakPtr newParent)
{
	if (parent != nullptr)
	{
		Ptr keepAlive(this);
		parent->children.removeObject(keepAlive);
		parent = newParent;
		parent->children.addIfNotAlreadyThere(keepAlive);
		return;
	}

	parent = newParent;
	parent->children.addIfNotAlreadyThere(this);
}

void UriLocationTree::cleanup(const std::function<bool(const UriLocationTree& item)>& f)
{
	for (int i = 0; i < children.size(); i++)
	{
		auto c = children[i];

		if (f(*c))
			children.remove(i--);
		else
			c->cleanup(f);
	}
}

bool UriLocationTree::forEach(const std::function<bool(const UriLocationTree& item)>& f) const
{
	if (f(*this))
		return true;

	for (auto c : children)
	{
		if (c->forEach(f))
			return true;
	}

	return false;
}

bool UriLocationTree::forEachParent(const std::function<bool(const UriLocationTree&)>& f) const
{
	if (f(*this))
		return true;

	if (parent != nullptr)
		return parent->forEachParent(f);

	return false;
}

bool UriLocationTree::forEachChildWithCategory(const Identifier& childCategory, const std::function<bool(const UriLocationTree&)> f) const
{
	for (auto c : children)
	{
		if (c->category == childCategory)
		{
			if (f(*c))
				return true;
		}
	}

	return false;
}

bool UriLocationTree::forEachLeafChild(const std::function<bool(const UriLocationTree& t)>& f) const
{
	for (auto c : children)
	{
		if (!c->hasRange())
		{
			if (f(*c))
				return true;
		}
	}

	return false;
}

bool UriLocationTree::containsLocation(const UriLocation& other) const
{
	if (!hasRange())
		return false;

	if (!sameUriAndProcessorId(other))
		return false;

	if ((getResolveState() & ResolveState::RawCharIndex) &&
		(other.getResolveState() & ResolveState::RawCharIndex))
	{
		Range<int> charRange(charIndex, closeLocation.charIndex);
		return charRange.contains(other.charIndex);
	}

	if ((getResolveState() & ResolveState::LineResolved) &&
		(other.getResolveState() & ResolveState::LineResolved))
	{
		Range<int> lineRange(line, closeLocation.line);
		return lineRange.contains(other.line);
	}

	return false;
}

void UriLocationTree::setToStringFunction(const std::function<String(const UriLocationTree&)>& f)
{
	toStringFunction = f;
}

String UriLocationTree::getAsString() const
{
	if (toStringFunction)
		return toStringFunction(*this);
	else
		return id.toString();
}

var UriLocationTree::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();

	if (category.isValid())
		obj->setProperty("category", category.toString());

	if (id.isValid())
		obj->setProperty("id", id.toString());

	if (toStringFunction)
		obj->setProperty("asString", toStringFunction(*this));

	if (hasRange())
	{
		UriLocation open;
		open.fillLocation(*this);
		obj->setProperty("startLocation", open.toJSON());
		obj->setProperty("closeLocation", closeLocation.toJSON());
	}
	else
	{
		auto base = UriLocation::toJSON();

		for (const auto& nv : base.getDynamicObject()->getProperties())
			obj->setProperty(nv.name, nv.value);
	}

	for (const auto& nv : data->getProperties())
		obj->setProperty(nv.name, nv.value);

	if (!children.isEmpty())
	{
		Array<var> childList;

		for (auto c : children)
			childList.add(c->toJSON());

		obj->setProperty("children", var(childList));
	}

	return var(obj.get());

}

const UriLocationTree* UriLocationTree::getRoot() const
{
	if (parent == nullptr)
		return this;

	return parent->getRoot();
}

int UriLocationTree::getNumChildren() const { return children.size(); }

UriLocationTree::Ptr UriLocationTree::getSubTree(const UriLocation& l, bool checkPosition) const
{
	if (matches(l))
		return Ptr(const_cast<UriLocationTree*>(this));

	if(!checkPosition && sameUriAndProcessorId(l))
		return Ptr(const_cast<UriLocationTree*>(this));

	for (auto c : children)
	{
		if (auto m = c->getSubTree(l, checkPosition))
			return m;
	}

	return nullptr;
}

UriLocationTree::Ptr UriLocationTree::getSubTreeWithId(const Identifier& idToLookFor, bool lookRecursive) const
{
	if (id.isValid() && id == idToLookFor)
		return Ptr(const_cast<UriLocationTree*>(this));

	for (auto c : children)
	{
		if (lookRecursive)
		{
			if (auto match = c->getSubTreeWithId(idToLookFor, lookRecursive))
				return match;
		}
		else
		{
			if (c->id.isValid() && c->id == idToLookFor)
				return c;
		}
	}

	return nullptr;
}

UriLocationTree::Ptr UriLocationTree::getInnerTree(const UriLocation& l) const
{
	jassert(hasRange());

	if (auto firstMatch = getSubTree(l, false))
	{
		auto inner = firstMatch;
		auto found = inner->containsLocation(l);

		//jassert(found);

		while (found)
		{
			found = false;

			for (auto c : inner->children)
			{
				if (c->containsLocation(l))
				{
					found = true;
					inner = c;
					break;
				}
			}

			if (!found)
				break;
		}

		return inner;
	}

	return nullptr;
}

Colour UriLocationTree::getIconColour() const
{
	auto c = category.toString().toUpperCase()[0];

	switch (c)
	{
	case 'R': return Colour(0xFF00AEFF);
	case 'V': return Colour(0xFF0FEF88);
	case 'N': return Colour(0xFF3E55FF);
	case 'F': return Colour(0xFFDF6469);
	case 'C': return Colour(0xFF0FEFDD);
	case 'I': return Colour(0xFFE44C7D);
	case 'P': return Colour(0xFFC2BE00);
	case 'L': return Colour(0xFF7EEF0F);
	default:  return Colours::grey;
	}
}

UriLocationTree::Ptr UriLocationTree::getChild(int index) const
{
	jassert(isPositiveAndBelow(index, children.size()));
	return children[index];
}

UriError UriLocationTree::checkSanity() const
{
	if (hasRange())
	{
		if (closeLocation.getResolveState() != ResolveState::FullyResolved)
			return createError(closeLocation, "close location must be resolved if defined");

		for (auto c : children)
		{
			if (!c->sameUriAndProcessorId(*this))
				continue;

			if (!containsLocation(*c))
				return c->createError(*c, "child element must be within location range");
		}
	}

	for (auto c : children)
	{
		auto e = c->checkSanity();

		if (!e.wasOk())
			return e;
	}

	return UriError::ok();
}

Range<int> UriLocationTree::getLineRange() const
{
	if (hasRange())
	{
		jassert(getResolveState() & ResolveState::LineResolved);
		jassert(closeLocation.getResolveState() & ResolveState::LineResolved);

		return { line, closeLocation.line };
	}

	return UriLocation::getLineRange();
}

UriError UriLocationTree::createError(const UriLocation& errorLocation, const String& errorMessage) const
{
	auto e = UriError::fail(errorMessage);
	e.fillLocation(errorLocation);
	e.sourceType = UriError::SourceType::Include;

	forEachParent([&](const UriLocationTree& t)
	{
		e.callstack.push_back({ t.id.toString(), t });
		return false;
	});

	return e;
}

hise::UriBreakpoint::Owner::UriBreakpointListener::~UriBreakpointListener()
{
	setOwner(nullptr);
}

void UriBreakpoint::Owner::UriBreakpointListener::setOwner(Owner* o)
{
	if (owner.get() != nullptr)
		owner->removeUriBreakpointListener(this);

	owner = o;

	if(owner)
		owner->addUriBreakpointListener(this);
}

bool UriBreakpoint::Owner::UriBreakpointListener::matchesUriBreakpointEvent(const UriError& e) const
{
	if (e.wasOk())
		return true;

	if (e.errorMessage.startsWith("Step"))
		return true;

	if (matchesUriLocation(e))
		return true;

	for (const auto& cs : e.callstack)
	{
		if (matchesUriLocation(cs.second))
			return true;
	}

	return false;
}

}