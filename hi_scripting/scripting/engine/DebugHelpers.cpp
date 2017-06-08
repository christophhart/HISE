

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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

String DebugInformation::varArrayToString(const Array<var> &arrayToStringify)
{
	String output;
	output << "[";
	const int maxSize = jmin<int>(4, arrayToStringify.size());

	for (int i = 0; i < maxSize - 1; i++)
		output << arrayToStringify[i].toString() << ", ";

	output << arrayToStringify[maxSize - 1].toString();

	if (maxSize != arrayToStringify.size())
		output << ", (...)]";
	else
		output << "]";

	return output;
}

StringArray DebugInformation::createTextArray()
{
	StringArray sa;
	
	sa.add(getTextForType());
	sa.add(getTextForDataType());
	sa.add(getTextForName());
	sa.add(getTextForValue());
	
	return sa;
}


String DebugInformation::getTextForRow(Row r)
{
	switch (r)
	{
	case Row::Name: return getTextForName();
	case Row::Type: return getTextForType();
	case Row::DataType: return getTextForDataType();
	case Row::Value: return getTextForValue();
    case Row::numRows:   return "";
	}

	return "";
}

String DebugInformation::toString()
{
	String output;

	output << "Name: " << getTextForRow(Row::Name) << ", ";
	output << "Type: " << getTextForRow(Row::Type) << ", ";
	output << "DataType:" << getTextForRow(Row::DataType) << ", ";
	output << "Value: " << getTextForRow(Row::Value);

	return output;
}


void DebugableObject::Helpers::gotoLocation(Component* ed, JavascriptProcessor* sp, const Location& location)
{
#if USE_BACKEND
	ScriptingEditor* editor = dynamic_cast<ScriptingEditor*>(ed);

	if (sp == nullptr && editor != nullptr)
	{
		sp = dynamic_cast<JavascriptProcessor*>(editor->getProcessor());
	}

	if (sp == nullptr)
	{
		// You have to somehow manage to pass the processor here...
		jassertfalse;
		return;
	}

	File file = File(location.fileName);

	if (file.existsAsFile())
	{
		for (int i = 0; i < sp->getNumWatchedFiles(); i++)
		{
			if (sp->getWatchedFile(i) == file)
			{
				sp->showPopupForFile(i, location.charNumber);
			}
		}
	}
	else
	{
		editor->showOnInitCallback();
		editor->gotoChar(location.charNumber);
	}
#else
	ignoreUnused(ed, sp, location);
#endif
}

