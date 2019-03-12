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

/** A markdown header is a datastructure containing metadata for a markdown document.

	It is formatted using the Front Matter YAML specification.

	The most important keys are "keywords" and "summary"
*/
struct MarkdownHeader
{
	/** Returns the value for the key "keywords". */
	StringArray getKeywords();

	/** Returns the first value for the key "keywords". */
	String getFirstKeyword();

	/** Returns the value for the key "summary". */
	String getDescription();

	String getIcon() const;

	String getKeyValue(const String& key) const;

	/** Creates a String representation of the header. */
	String toString() const;

	/** This returns a version of this header with only the header and the description. */
	MarkdownHeader cleaned() const;

	StringArray getKeyList(const String& key) const;

	void checkValid();

	static MarkdownHeader getHeaderForFile(File root, const String& url);

	static File createEmptyMarkdownFileWithMarkdownHeader(File parent, String childName, String description);

	struct Item
	{
		String toString() const;

		String key;
		StringArray values;
	};

	Array<Item> items;
};

}