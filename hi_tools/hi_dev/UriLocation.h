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
using namespace juce;

/** 

TODO: - remove RealtimeSafetyInfo
	  - use UriLocation in mcl::TextEditor warnings && errors
	  - make a dependency free error class with callstack

	    RootObject::Error => ErrorWithLocationAndCallStack
		DiagnosticResult::Item => ErrorWithLocationAndCallStack

*/
struct UriLocation
{
	using WithExpression = std::pair<String, UriLocation>;

	enum class UriType
	{
		onInit,
		ExternalFile,
		OtherCallback
	};

	enum class UriFormat
	{
		Callback,   // onInit(), onNoteOn(), fullPath
		CallbackId, // onInit, onNoteOn, fullFilePath
		FilePath,
		RelativePath,
		ConnectionId, // used by PanelWithProcessorConnection::indexList
		LineAndCol,
		Base64,
		GotoString
	};

	virtual ~UriLocation() {};

	virtual var toJSON() const;

	bool fillLineAndCol(const juce::CodeDocument& doc);

	bool hasResolvedLineAndColNumbers() const
	{
		return line != -1 && col != -1;
	}

	File getFile(const File& rootDir) const;

	UriType getUriType() const;

	bool matchesFile(const File& f) const;

	bool matches(const UriLocation& other) const
	{
		return other.charIndex == charIndex && 
			   other.uri == uri &&
			   other.processorId == processorId;
	}

	Range<int> getLineRange() const { return { line, line + 1 }; }

	String toLocationString(UriFormat f) const;

	void fillLocation(const UriLocation& other);

	operator bool() const { return line != -1 || col != -1 || charIndex != -1 || uri.isNotEmpty(); }

	int line = -1;
	int col = -1;
	int charIndex = -1;
	String uri;
	String processorId;
};

struct UriError : public UriLocation
{
	String toConsoleMessage() const
	{
		String msg;

		msg << toLocationString(UriFormat::LineAndCol) << ": ";
		msg << errorMessage << " " << toLocationString(UriFormat::Base64);

		return msg;
	}

	String toFullCallStack() const
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

	var toJSON() const override
	{
		auto obj = UriLocation::toJSON();

		if (auto m = metadata.getDynamicObject())
		{
			for (const auto& nv : m->getProperties())
				obj.getDynamicObject()->setProperty(nv.name, nv.value);
		}

		return obj;
	}

	String errorMessage;
	var metadata;
	std::vector<UriLocation::WithExpression> callstack;
};


}