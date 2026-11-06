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
	no->setProperty("line", line);
	no->setProperty("column", col);
	no->setProperty("charIndex", charIndex);
	no->setProperty("filePath", uri);
	no->setProperty("processorId", processorId);
	return var(no);
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

	return f.getFullPathName() == uri || f.getFileName() == uri;
}

bool UriLocation::matches(const UriLocation& other) const
{
	return other.charIndex == charIndex &&
		other.uri == uri &&
		other.processorId == processorId;
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
			if (File::isAbsolutePath(uri))
			{
				if (uri.contains("Scripts/"))
					return uri.fromLastOccurrenceOf("Scripts/", false, false);

				return File(uri).getFileName();
			}
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

String UriError::toConsoleMessage() const
{
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

}