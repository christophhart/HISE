/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once


//==============================================================================
const char* getPreferredLineFeed();
String joinLinesIntoSourceFile (StringArray& lines);
String replaceLineFeeds (const String& content, const String& lineFeed);
String getLineFeedForFile (const String& fileContent);

var parseJUCEHeaderMetadata (const File&);

String trimCommentCharsFromStartOfLine (const String& line);

String createAlphaNumericUID();
String createGUID (const String& seed); // Turns a seed into a windows GUID

String escapeSpaces (const String& text); // replaces spaces with blackslash-space
String addQuotesIfContainsSpaces (const String& text);

StringPairArray parsePreprocessorDefs (const String& defs);
StringPairArray mergePreprocessorDefs (StringPairArray inheritedDefs, const StringPairArray& overridingDefs);
String createGCCPreprocessorFlags (const StringPairArray& defs);

StringArray getCleanedStringArray (StringArray);
StringArray getSearchPathsFromString (const String& searchPath);
StringArray getCommaOrWhitespaceSeparatedItems (const String&);

void setValueIfVoid (Value value, const var& defaultValue);

bool fileNeedsCppSyntaxHighlighting (const File& file);

void writeAutoGenWarningComment (OutputStream& outStream);

StringArray getJUCEModules() noexcept;
bool isJUCEModule (const String& moduleID) noexcept;

StringArray getModulesRequiredForConsole() noexcept;
StringArray getModulesRequiredForComponent() noexcept;
StringArray getModulesRequiredForAudioProcessor() noexcept;

bool isPIPFile (const File&) noexcept;
int findBestLineToScrollToForClass (StringArray, const String&, bool isPlugin = false);

bool isValidJUCEExamplesDirectory (const File&) noexcept;

bool isJUCEModulesFolder (const File&);
bool isJUCEFolder (const File&);

//==============================================================================
int indexOfLineStartingWith (const StringArray& lines, const String& text, int startIndex);

void autoScrollForMouseEvent (const MouseEvent& e, bool scrollX = true, bool scrollY = true);

//==============================================================================
struct PropertyListBuilder
{
    void add (PropertyComponent* propertyComp)
    {
        components.add (propertyComp);
    }

    void add (PropertyComponent* propertyComp, const String& tooltip)
    {
        propertyComp->setTooltip (tooltip);
        add (propertyComp);
    }

    void addSearchPathProperty (const Value& value, const String& name, const String& mainHelpText)
    {
        add (new TextPropertyComponent (value, name, 16384, true),
             mainHelpText + " Use semi-colons or new-lines to separate multiple paths.");
    }

    void addSearchPathProperty (ValueWithDefault& value, const String& name, const String& mainHelpText)
    {
        add (new TextPropertyComponent (value, name, 16384, true),
             mainHelpText + " Use semi-colons or new-lines to separate multiple paths.");
    }

    void setPreferredHeight (int height)
    {
        for (int j = components.size(); --j >= 0;)
            components.getUnchecked(j)->setPreferredHeight (height);
    }

    Array<PropertyComponent*> components;
};

//==============================================================================
// A ValueSource which takes an input source, and forwards any changes in it.
// This class is a handy way to create sources which re-map a value.
class ValueSourceFilter   : public Value::ValueSource,
                            private Value::Listener
{
public:
    ValueSourceFilter (const Value& source)  : sourceValue (source)
    {
        sourceValue.addListener (this);
    }

protected:
    Value sourceValue;

private:
    void valueChanged (Value&) override      { sendChangeMessage (true); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ValueSourceFilter)
};
