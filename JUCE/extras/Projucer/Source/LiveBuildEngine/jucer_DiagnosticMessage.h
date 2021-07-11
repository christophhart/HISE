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
struct DiagnosticMessage
{
    DiagnosticMessage() = default;

    DiagnosticMessage (const DiagnosticMessage& other)
       : associatedDiagnostic (createCopyIfNotNull (other.associatedDiagnostic.get())),
         message (other.message),
         mainFile (other.mainFile),
         range (other.range),
         type (other.type)
    {
    }

    DiagnosticMessage& operator= (const DiagnosticMessage& other)
    {
        associatedDiagnostic.reset (createCopyIfNotNull (other.associatedDiagnostic.get()));
        message = other.message;
        mainFile = other.mainFile;
        range = other.range;
        type = other.type;

        return *this;
    }

    enum Type
    {
        error = 0,
        warning = 1,
        note = 2
    };

    std::unique_ptr<DiagnosticMessage> associatedDiagnostic;
    String message;
    String mainFile;
    SourceCodeRange range;
    Type type;

    bool isError() const noexcept       { return type == error; }
    bool isWarning() const noexcept     { return type == warning; }
    bool isNote() const noexcept        { return type == note; }

    String toString() const
    {
        // todo: copy recursively from root
        String res;

        switch (type)
        {
            case error:   res << "error: "; break;
            case warning: res << "warning: "; break;
            case note:    res << "note: "; break;
            default:      break;
        };

        res << mainFile << ": ";
        res << message << "\n";

        return res;
    }

    ValueTree toValueTree() const
    {
        ValueTree v (MessageTypes::DIAGNOSTIC);
        v.setProperty (Ids::text, message, nullptr);
        v.setProperty (Ids::file, mainFile, nullptr);
        v.setProperty (Ids::range, range.toString(), nullptr);
        v.setProperty (Ids::type, (int) type, nullptr);

        if (associatedDiagnostic != nullptr)
            v.addChild (associatedDiagnostic->toValueTree(), 0, nullptr);

        return v;
    }

    static DiagnosticMessage fromValueTree (const ValueTree& v)
    {
        DiagnosticMessage d;
        d.message = v[Ids::text];
        d.mainFile = v[Ids::file];
        d.range = SourceCodeRange (v [Ids::range]);
        d.type = (Type) static_cast<int> (v[Ids::type]);

        auto associated = v.getChild (0);

        if (associated.isValid())
            d.associatedDiagnostic.reset (new DiagnosticMessage (fromValueTree (associated)));

        return d;
    }

    bool operator== (const DiagnosticMessage& other) const noexcept
    {
        return range == other.range
                && message == other.message
                && mainFile == other.mainFile;
    }

    bool operator!= (const DiagnosticMessage& other) const noexcept    { return ! operator== (other); }
};

//==============================================================================
struct DiagnosticList
{
    // after some research, it seems that notes never come on their own
    // i.e. they always have a warning / error preceding them
    // so we can keep notes and their associated diagnostics
    // together by keeping track of the last message
    DiagnosticMessage lastMessage;

    ValueTree list { MessageTypes::DIAGNOSTIC_LIST };

    void clear()
    {
        list = ValueTree { MessageTypes::DIAGNOSTIC_LIST };
        lastMessage = DiagnosticMessage();
    }

    void add (DiagnosticMessage m)
    {
        if (m.isNote())
        {
            if (lastMessage.message.isEmpty())
                return; // seems to happen sometimes, but with seemingly duplicated messages (?)

            m.associatedDiagnostic.reset (new DiagnosticMessage (lastMessage));
        }
        else
        {
            lastMessage = m;
        }

        list.appendChild (m.toValueTree(), nullptr);
    }

    void add (const DiagnosticList& l)
    {
        jassert (l.list != list);

        for (int i = 0; i < l.list.getNumChildren(); ++i)
            list.appendChild (l.list.getChild(i).createCopy(), nullptr);
    }

    void remove (DiagnosticMessage m)
    {
        auto n = m.toValueTree();

        for (int i = 0; i < list.getNumChildren(); ++i)
        {
            if (list.getChild (i).isEquivalentTo (n))
            {
                list.removeChild (i, nullptr);
                return;
            }
        }

        jassertfalse;
    }

    bool hasRecoveryWarning (DiagnosticMessage m) const
    {
        auto n = m.toValueTree();

        for (int i = 0; i < list.getNumChildren(); ++i)
            if (list.getChild (i).isEquivalentTo (n))
                return true;

        return false;
    }

    const ValueTree& toValueTree() const noexcept
    {
        return list;
    }

    void loadFromChildOfValueTree (ValueTree& parent)
    {
        list = parent.getChildWithName (MessageTypes::DIAGNOSTIC_LIST).createCopy();
    }
};
