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
	struct Owner
	{
		virtual ~Owner() {};
		virtual String getUriProcessorId() const = 0;
	};

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

	operator bool() const { return line != -1 || col != -1 || charIndex != -1 || uri.isNotEmpty(); }

	virtual var toJSON() const;

	bool fillLineAndCol(const juce::CodeDocument& doc);
	bool hasResolvedLineAndColNumbers() const;

	File getFile(const File& rootDir) const;
	UriType getUriType() const;
	Range<int> getLineRange() const { return { line, line + 1 }; }
	String toLocationString(UriFormat f) const;

	bool matchesFile(const File& f) const;
	bool matches(const UriLocation& other) const;

	void fillLocation(const UriLocation& other);
	void fillLocation(const CodeDocument::Position& pos);
	void fillLocation(const var& uriObj);

	int line = -1;
	int col = -1;
	int charIndex = -1;
	String uri;
	String processorId;

	std::function<bool(Owner*, Component*)> gotoCallback;
};

struct UriBreakpoint: public UriLocation
{
	static UriBreakpoint fromJSON(const var& obj);

	var toJSON() const override;

	String expression = "true";
	bool enabled      = true;
	bool breakOnHit   = true;
	bool blinkOnHit   = false;
};

struct UriError : public UriLocation
{
	String toConsoleMessage() const;

	String toFullCallStack() const;

	var toJSON() const override;

	String errorMessage;
	var metadata;
	std::vector<UriLocation::WithExpression> callstack;
};


}