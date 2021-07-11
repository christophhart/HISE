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
class TextPropertyComponentWithEnablement    : public TextPropertyComponent,
                                               private Value::Listener
{
public:
    TextPropertyComponentWithEnablement (ValueWithDefault& valueToControl,
                                         ValueWithDefault valueToListenTo,
                                         const String& propertyName,
                                         int maxNumChars,
                                         bool isMultiLine)
        : TextPropertyComponent (valueToControl, propertyName, maxNumChars, isMultiLine),
          valueWithDefault (valueToListenTo),
          value (valueWithDefault.getPropertyAsValue())
    {
        value.addListener (this);
        setEnabled (valueWithDefault.get());
    }

    ~TextPropertyComponentWithEnablement() override    { value.removeListener (this); }

private:
    ValueWithDefault valueWithDefault;
    Value value;

    void valueChanged (Value&) override       { setEnabled (valueWithDefault.get()); }
};

//==============================================================================
class ChoicePropertyComponentWithEnablement    : public ChoicePropertyComponent,
                                                 private Value::Listener
{
public:
    ChoicePropertyComponentWithEnablement (ValueWithDefault& valueToControl,
                                           ValueWithDefault valueToListenTo,
                                           const String& propertyName,
                                           const StringArray& choiceToUse,
                                           const Array<var>& correspondingValues)
        : ChoicePropertyComponent (valueToControl, propertyName, choiceToUse, correspondingValues),
          valueWithDefault (valueToListenTo),
          value (valueToListenTo.getPropertyAsValue())
    {
        value.addListener (this);
        valueChanged (value);
    }

    ChoicePropertyComponentWithEnablement (ValueWithDefault& valueToControl,
                                           ValueWithDefault valueToListenTo,
                                           const Identifier& multiChoiceID,
                                           const String& propertyName,
                                           const StringArray& choicesToUse,
                                           const Array<var>& correspondingValues)
        : ChoicePropertyComponentWithEnablement (valueToControl, valueToListenTo, propertyName, choicesToUse, correspondingValues)
    {
        jassert (valueToListenTo.get().getArray() != nullptr);

        isMultiChoice = true;
        idToCheck = multiChoiceID;

        valueChanged (value);
    }

    ChoicePropertyComponentWithEnablement (ValueWithDefault& valueToControl,
                                           ValueWithDefault valueToListenTo,
                                           const String& propertyName)
        : ChoicePropertyComponent (valueToControl, propertyName),
          valueWithDefault (valueToListenTo),
          value (valueToListenTo.getPropertyAsValue())
    {
        value.addListener (this);
        valueChanged (value);
    }

    ~ChoicePropertyComponentWithEnablement() override    { value.removeListener (this); }

private:
    ValueWithDefault valueWithDefault;
    Value value;

    bool isMultiChoice = false;
    Identifier idToCheck;

    bool checkMultiChoiceVar() const
    {
        jassert (isMultiChoice);

        auto v = valueWithDefault.get();

        if (auto* varArray = v.getArray())
            return varArray->contains (idToCheck.toString());

        jassertfalse;
        return false;
    }

    void valueChanged (Value&) override
    {
        if (isMultiChoice)
            setEnabled (checkMultiChoiceVar());
        else
            setEnabled (valueWithDefault.get());
    }
};

//==============================================================================
class MultiChoicePropertyComponentWithEnablement    : public MultiChoicePropertyComponent,
                                                      private Value::Listener
{
public:
    MultiChoicePropertyComponentWithEnablement (ValueWithDefault& valueToControl,
                                                ValueWithDefault valueToListenTo,
                                                const String& propertyName,
                                                const StringArray& choices,
                                                const Array<var>& correspondingValues)
        : MultiChoicePropertyComponent (valueToControl,
                                        propertyName,
                                        choices,
                                        correspondingValues),
          valueWithDefault (valueToListenTo),
          value (valueToListenTo.getPropertyAsValue())
    {
        value.addListener (this);
        valueChanged (value);
    }

    ~MultiChoicePropertyComponentWithEnablement() override    { value.removeListener (this); }

private:
    void valueChanged (Value&) override       { setEnabled (valueWithDefault.get()); }

    ValueWithDefault valueWithDefault;
    Value value;
};
