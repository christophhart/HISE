

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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
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


void DebugableObject::Helpers::gotoLocation(Component* ed, const Location& location)
{
	ScriptingEditor* editor = dynamic_cast<ScriptingEditor*>(ed);

	JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(editor->getProcessor());

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
}
