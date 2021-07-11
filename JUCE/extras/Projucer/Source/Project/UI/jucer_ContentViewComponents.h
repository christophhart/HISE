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
struct ContentViewHeader    : public Component
{
    ContentViewHeader (String headerName, Icon headerIcon)
        : name (headerName), icon (headerIcon)
    {
    }

    void paint (Graphics& g) override
    {
        g.fillAll (findColour (contentHeaderBackgroundColourId));

        auto bounds = getLocalBounds().reduced (20, 0);

        icon.withColour (Colours::white).draw (g, bounds.toFloat().removeFromRight (30), false);

        g.setColour (Colours::white);
        g.setFont (Font (18.0f));
        g.drawFittedText (name, bounds, Justification::centredLeft, 1);
    }

    String name;
    Icon icon;
};

//==============================================================================
class ListBoxHeader    : public Component
{
public:
    ListBoxHeader (Array<String> columnHeaders)
    {
        for (auto s : columnHeaders)
        {
            addAndMakeVisible (headers.add (new Label (s, s)));
            widths.add (1.0f / (float) columnHeaders.size());
        }

        setSize (200, 40);
    }

    ListBoxHeader (Array<String> columnHeaders, Array<float> columnWidths)
    {
        jassert (columnHeaders.size() == columnWidths.size());

        auto index = 0;
        for (auto s : columnHeaders)
        {
            addAndMakeVisible (headers.add (new Label (s, s)));
            widths.add (columnWidths.getUnchecked (index++));
        }

        recalculateWidths();

        setSize (200, 40);
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        auto width = bounds.getWidth();

        auto index = 0;
        for (auto h : headers)
        {
            auto headerWidth = roundToInt ((float) width * widths.getUnchecked (index));
            h->setBounds (bounds.removeFromLeft (headerWidth));
            ++index;
        }
    }

    void setColumnHeaderWidth (int index, float proportionOfWidth)
    {
        if (! (isPositiveAndBelow (index, headers.size()) && isPositiveAndNotGreaterThan (proportionOfWidth, 1.0f)))
        {
            jassertfalse;
            return;
        }

        widths.set (index, proportionOfWidth);
        recalculateWidths (index);
    }

    int getColumnX (int index)
    {
        auto prop = 0.0f;
        for (int i = 0; i < index; ++i)
            prop += widths.getUnchecked (i);

        return roundToInt (prop * (float) getWidth());
    }

    float getProportionAtIndex (int index)
    {
        jassert (isPositiveAndBelow (index, widths.size()));
        return widths.getUnchecked (index);
    }

private:
    OwnedArray<Label> headers;
    Array<float> widths;

    void recalculateWidths (int indexToIgnore = -1)
    {
        auto total = 0.0f;

        for (auto w : widths)
            total += w;

        if (total == 1.0f)
            return;

        auto diff = 1.0f - total;
        auto amount = diff / static_cast<float> (indexToIgnore == -1 ? widths.size() : widths.size() - 1);

        for (int i = 0; i < widths.size(); ++i)
        {
            if (i != indexToIgnore)
            {
                auto val = widths.getUnchecked (i);
                widths.set (i, val + amount);
            }
        }
    }
};

//==============================================================================
class InfoButton    : public Button
{
public:
    InfoButton (const String& infoToDisplay = {})
        : Button ({})
    {
        if (infoToDisplay.isNotEmpty())
            setInfoToDisplay (infoToDisplay);
    }

    void paintButton (Graphics& g, bool isMouseOverButton, bool isButtonDown) override
    {
        auto bounds = getLocalBounds().toFloat().reduced (2);
        auto& icon = getIcons().info;

        g.setColour (findColour (treeIconColourId).withMultipliedAlpha (isMouseOverButton || isButtonDown ? 1.0f : 0.5f));

        if (isButtonDown)
            g.fillEllipse (bounds);
        else
            g.fillPath (icon, RectanglePlacement (RectanglePlacement::centred)
                        .getTransformToFit (icon.getBounds(), bounds));
    }

    void clicked() override
    {
        auto w = std::make_unique<InfoWindow> (info);
        w->setSize (width, w->getHeight() * numLines + 10);

        CallOutBox::launchAsynchronously (std::move (w), getScreenBounds(), nullptr);
    }

    using Button::clicked;

    void setInfoToDisplay (const String& infoToDisplay)
    {
        if (infoToDisplay.isNotEmpty())
        {
            info = infoToDisplay;

            auto stringWidth = roundToInt (Font (14.0f).getStringWidthFloat (info));
            width = jmin (300, stringWidth);

            numLines += static_cast<int> (stringWidth / width);
        }
    }

    void setAssociatedComponent (Component* comp)    { associatedComponent = comp; }
    Component* getAssociatedComponent()              { return associatedComponent; }

private:
    String info;
    Component* associatedComponent = nullptr;
    int width;
    int numLines = 1;

    //==============================================================================
    struct InfoWindow    : public Component
    {
        InfoWindow (const String& s)
            : stringToDisplay (s)
        {
            setSize (150, 14);
        }

        void paint (Graphics& g) override
        {
            g.fillAll (findColour (secondaryBackgroundColourId));

            g.setColour (findColour (defaultTextColourId));
            g.setFont (Font (14.0f));
            g.drawFittedText (stringToDisplay, getLocalBounds(), Justification::centred, 15, 0.75f);
        }

        String stringToDisplay;
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfoButton)
};

//==============================================================================
class PropertyGroupComponent  : public Component,
                                private TextPropertyComponent::Listener
{
public:
    PropertyGroupComponent (String name, Icon icon, String desc = {})
        : header (name, icon),
          description (desc)
    {
        addAndMakeVisible (header);

        description.setFont ({ 16.0f });
        description.setColour (getLookAndFeel().findColour (defaultTextColourId));
        description.setLineSpacing (5.0f);
        description.setJustification (Justification::centredLeft);
    }

    void setProperties (const PropertyListBuilder& newProps)
    {
        infoButtons.clear();
        properties.clear();
        properties.addArray (newProps.components);

        for (auto* prop : properties)
        {
            addAndMakeVisible (prop);

            if (! prop->getTooltip().isEmpty())
            {
                addAndMakeVisible (infoButtons.add (new InfoButton (prop->getTooltip())));
                infoButtons.getLast()->setAssociatedComponent (prop);
                prop->setTooltip ({}); // set the tooltip to empty so it only displays when its button is clicked
            }

            if (auto* multiChoice = dynamic_cast<MultiChoicePropertyComponent*> (prop))
                multiChoice->onHeightChange = [this] { updateSize(); };

            if (auto* text = dynamic_cast<TextPropertyComponent*> (prop))
                if (text->isTextEditorMultiLine())
                    text->addListener (this);

        }
    }

    int updateSize (int x, int y, int width)
    {
        header.setBounds (0, 0, width, headerSize);
        auto height = header.getBottom() + 10;

        descriptionLayout.createLayout (description, (float) (width - 40));
        auto descriptionHeight = (int) descriptionLayout.getHeight();

        if (descriptionHeight > 0)
            height += (int) descriptionLayout.getHeight() + 25;

        for (auto* pp : properties)
        {
            auto propertyHeight = pp->getPreferredHeight() + (getHeightMultiplier (pp) * pp->getPreferredHeight());

            InfoButton* buttonToUse = nullptr;
            for (auto* b : infoButtons)
                if (b->getAssociatedComponent() == pp)
                    buttonToUse = b;

            if (buttonToUse != nullptr)
            {
                buttonToUse->setSize (20, 20);
                buttonToUse->setCentrePosition (20, height + (propertyHeight / 2));
            }

            pp->setBounds (40, height, width - 50, propertyHeight);

            if (shouldResizePropertyComponent (pp))
                resizePropertyComponent (pp);

            height += pp->getHeight() + 10;
        }

        height += 16;

        setBounds (x, y, width, jmax (height, getParentHeight()));

        return height;
    }

    void paint (Graphics& g) override
    {
        g.setColour (findColour (secondaryBackgroundColourId));
        g.fillRect (getLocalBounds());

        auto textArea = getLocalBounds().toFloat()
                                        .withTop ((float) headerSize)
                                        .reduced (20.0f, 10.0f)
                                        .withHeight (descriptionLayout.getHeight());
        descriptionLayout.draw (g, textArea);
    }

    OwnedArray<PropertyComponent> properties;

private:
    //==============================================================================
    void textPropertyComponentChanged (TextPropertyComponent* comp) override
    {
        auto fontHeight = [comp]
        {
            Label tmpLabel;
            return comp->getLookAndFeel().getLabelFont (tmpLabel).getHeight();
        }();

        auto lines = StringArray::fromLines (comp->getText());

        comp->setPreferredHeight (jmax (100, 10 + roundToInt (fontHeight * (float) lines.size())));

        updateSize();
    }

    //==============================================================================
    void updateSize()
    {
        updateSize (getX(), getY(), getWidth());

        if (auto* parent = getParentComponent())
            parent->parentSizeChanged();
    }

    //==============================================================================
    bool shouldResizePropertyComponent (PropertyComponent* p)
    {
        if (auto* textComp = dynamic_cast<TextPropertyComponent*> (p))
            return ! textComp->isTextEditorMultiLine();

        return (dynamic_cast<ChoicePropertyComponent*>  (p) != nullptr
             || dynamic_cast<ButtonPropertyComponent*>  (p) != nullptr
             || dynamic_cast<BooleanPropertyComponent*> (p) != nullptr);
    }

    void resizePropertyComponent (PropertyComponent* pp)
    {
        for (auto i = pp->getNumChildComponents() - 1; i >= 0; --i)
        {
            auto* child = pp->getChildComponent (i);

            auto bounds = child->getBounds();
            child->setBounds (bounds.withSizeKeepingCentre (child->getWidth(), pp->getPreferredHeight()));
        }
    }

    int getHeightMultiplier (PropertyComponent* pp)
    {
        auto availableTextWidth = ProjucerLookAndFeel::getTextWidthForPropertyComponent (pp);

        auto font = ProjucerLookAndFeel::getPropertyComponentFont();
        auto nameWidth = font.getStringWidthFloat (pp->getName());

        if (availableTextWidth == 0)
            return 0;

        return static_cast<int> (nameWidth / (float) availableTextWidth);
    }

    OwnedArray<InfoButton> infoButtons;
    ContentViewHeader header;
    AttributedString description;
    TextLayout descriptionLayout;
    int headerSize = 40;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PropertyGroupComponent)
};
