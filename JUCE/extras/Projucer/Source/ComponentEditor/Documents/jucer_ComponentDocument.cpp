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

#include "../../Application/jucer_Headers.h"
#include "jucer_ComponentDocument.h"


//==============================================================================
ComponentDocument::ComponentDocument (SourceCodeDocument* c)
    : JucerDocument (c)
{
    components.reset (new ComponentLayout());
    components->setDocument (this);

    backgroundGraphics.reset (new PaintRoutine());
    backgroundGraphics->setDocument (this);
}

ComponentDocument::~ComponentDocument()
{
}

//==============================================================================
String ComponentDocument::getTypeName() const
{
    return "Component";
}

JucerDocument* ComponentDocument::createCopy()
{
    auto newOne = new ComponentDocument (cpp);

    newOne->resources = resources;
    newOne->loadFromXml (*createXml());

    return newOne;
}

std::unique_ptr<XmlElement> ComponentDocument::createXml() const
{
    auto doc = JucerDocument::createXml();

    doc->addChildElement (backgroundGraphics->createXml());
    components->addToXml (*doc);

    return doc;
}

bool ComponentDocument::loadFromXml (const XmlElement& xml)
{
    if (JucerDocument::loadFromXml (xml))
    {
        components->clearComponents();

        for (auto* e : xml.getChildIterator())
        {
            if (e->hasTagName (PaintRoutine::xmlTagName))
                backgroundGraphics->loadFromXml (*e);
            else
                components->addComponentFromXml (*e, false);
        }

        changed();
        getUndoManager().clearUndoHistory();
        return true;
    }

    return false;
}

void ComponentDocument::applyCustomPaintSnippets (StringArray& snippets)
{
    backgroundGraphics->applyCustomPaintSnippets (snippets);
}

//==============================================================================
class NormalTestComponent   : public Component
{
public:
    NormalTestComponent (ComponentDocument* const doc, const bool fillBackground)
        : document (doc),
          alwaysFillBackground (fillBackground)
    {
        ComponentLayout* const layout = document->getComponentLayout();

        for (int i = 0; i < layout->getNumComponents(); ++i)
            addAndMakeVisible (layout->getComponent (i));
    }

    ~NormalTestComponent() override
    {
        for (int i = getNumChildComponents(); --i >= 0;)
            removeChildComponent (i);
    }

    void paint (Graphics& g) override
    {
        document->getPaintRoutine (0)->fillWithBackground (g, alwaysFillBackground);
        document->getPaintRoutine (0)->drawElements (g, getLocalBounds());
    }

    void resized() override
    {
        if (! getBounds().isEmpty())
        {
            int numTimesToTry = 10;

            while (--numTimesToTry >= 0)
            {
                bool anyCompsMoved = false;

                for (int i = 0; i < getNumChildComponents(); ++i)
                {
                    Component* comp = getChildComponent (i);

                    if (ComponentTypeHandler* const type = ComponentTypeHandler::getHandlerFor (*comp))
                    {
                        const Rectangle<int> newBounds (type->getComponentPosition (comp)
                                                        .getRectangle (getLocalBounds(),
                                                                       document->getComponentLayout()));

                        anyCompsMoved = anyCompsMoved || (comp->getBounds() != newBounds);
                        comp->setBounds (newBounds);
                    }
                }

                // repeat this loop until they've all stopped shuffling (might require a few
                // loops for all the relative positioned comps to settle down)
                if (! anyCompsMoved)
                    break;
            }
        }
    }

private:
    ComponentDocument* const document;
    const bool alwaysFillBackground;
};

Component* ComponentDocument::createTestComponent (const bool alwaysFillBackground)
{
    return new NormalTestComponent (this, alwaysFillBackground);
}

void ComponentDocument::fillInGeneratedCode (GeneratedCode& code) const
{
    JucerDocument::fillInGeneratedCode (code);
}
