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
	no->setProperty("line", line);
	no->setProperty("column", col);
	no->setProperty("charIndex", charIndex);
	no->setProperty("filePath", uri);
	return var(no);
}

bool UriLocation::fillLineAndCol(const juce::CodeDocument& doc)
{
	if (!hasResolvedLineAndColNumbers() && charIndex != -1)
	{
		juce::CodeDocument::Position p(doc, charIndex);
		line = p.getLineNumber();
		col = p.getIndexInLine();
		return true;
	}

	return false;
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

}