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


#include "InputComponents.h"


namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

LabelledComponent::LabelledComponent(Dialog& r, int width, const var& obj, Component* c):
	PageBase(r, width, obj),
	component(c)
{
	addAndMakeVisible(c);
	label = obj[mpid::Text].toString();
        
	setSize(width, 32);
}

void LabelledComponent::postInit()
{
	init();
}

void LabelledComponent::paint(Graphics& g)
{        
	auto b = getLocalBounds();
	auto df = Dialog::getDefaultFont(*this);
            
	g.setFont(df.first);
	g.setColour(df.second);
            
	g.drawText(label, b.toFloat().reduced(8.0f, 0.0f), Justification::left);
}

void LabelledComponent::resized()
{
	auto b = getLocalBounds();
        
	if(helpButton != nullptr)
	{
		helpButton->setBounds(b.removeFromRight(32).withSizeKeepingCentre(24, 24));
		b.removeFromRight(5);
	}

	b.removeFromLeft(getWidth() / 4);
        
	component->setBounds(b);
}

void Tickbox::createEditorInfo(Dialog::PageInfo* rootList)
{
	rootList->addChild<Type>({
        { mpid::ID, "Type"},
        { mpid::Type, "Tickbox"},
        { mpid::Help, "A Tickbox lets you enable / disable a boolean option or can be grouped together with other tickboxes to create a radio group with an exclusive selection option" }
    });       

	rootList->addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Help, "The ID for the element (used as key in the state `var`.\n> If you have multiple tickboxes on the same page with the same ID, they will be put into a radio group and can be selected exclusively (also the ID value will be the integer index of the button in the group list in the order of appearance." }
	});
        
	rootList->addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label next to the tickbox" }
	});
        
	rootList->addChild<Tickbox>({
		{ mpid::ID, "Required" },
		{ mpid::Text, "Required" },
		{ mpid::Help, "If this is enabled, the tickbox must be selected in order to proceed to the next page, otherwise it will show a error message.\n> This is particularly useful for eg. accepting TOC" }
	});
        
}



Tickbox::Tickbox(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new ToggleButton())
{
	if(obj.hasProperty(mpid::Required))
	{
		required = true;
		requiredOption = obj[mpid::Required];
	}
}

void Tickbox::buttonClicked(Button* b)
{
    for(auto tb: groupedButtons)
        tb->setToggleState(b == tb, dontSendNotification);
}

Choice::Choice(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new ComboBox())
{
	auto& combobox = getComponent<ComboBox>();
	auto s = obj[mpid::Items].toString();
	combobox.addItemList(StringArray::fromLines(s), 1);
	hise::GlobalHiseLookAndFeel::setDefaultColours(combobox);
}

void Choice::postInit()
{
	auto t = getValueFromGlobalState().toString();
	getComponent<ComboBox>().setText(t, dontSendNotification);
}

Result Choice::checkGlobalState(var globalState)
{
	return Result::ok();
}

ColourChooser::ColourChooser(Dialog& r, int w, const var& obj):
	LabelledComponent(r, w, obj, new ColourSelector(ColourSelector::ColourSelectorOptions::showColourspace | ColourSelector::showColourAtTop, 2, 0))
{
	auto& selector = getComponent<ColourSelector>();
	selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
	selector.setLookAndFeel(&laf);

	setSize(w, 130);
}

void ColourChooser::postInit()
{
	init();

	auto& selector = getComponent<ColourSelector>();
	auto colour = (uint32)(int64)getValueFromGlobalState();
	selector.setCurrentColour(Colour(colour));
}

void ColourChooser::resized()
{
	LabelledComponent::resized();
	auto b = getComponent<Component>().getBoundsInParent();

	b.removeFromRight((b.getWidth() - b.getHeight()) * 0.75);

	getComponent<Component>().setBounds(b);
}

Result ColourChooser::checkGlobalState(var globalState)
{
	auto& selector = getComponent<ColourSelector>();
        
	writeState((int64)selector.getCurrentColour().getARGB());
        
	return Result::ok();
}

void Tickbox::postInit()
{
    auto& button = this->getComponent<ToggleButton>();
    
    Component* root = getParentComponent();
    
    while(dynamic_cast<PageBase*>(root) != nullptr &&
          dynamic_cast<Builder*>(root->getParentComponent()) == nullptr)
    {
        root = root->getParentComponent();
    }
    
    Component::callRecursive<Tickbox>(root, [&](Tickbox* tb)
    {
        if(tb->id == id)
            groupedButtons.add(&tb->getComponent<ToggleButton>());
        
        return false;
    });
    
    if(groupedButtons.size() > 1)
    {
        thisRadioIndex = groupedButtons.indexOf(&button);
        auto radioIndex = (int)getValueFromGlobalState(-1);
        
        int idx = 0;
        
        for(auto b: groupedButtons)
        {
            b->addListener(this);
            b->setToggleState(idx++ == radioIndex, dontSendNotification);
        }
    }
    else
    {
        groupedButtons.clear();
        
        button.setToggleState(getValueFromGlobalState(false), dontSendNotification);
    }
    
	button.setColour(ToggleButton::ColourIds::tickColourId, Dialog::getDefaultFont(*this).second);
    

}

Result Tickbox::checkGlobalState(var globalState)
{
    auto& button = getComponent<ToggleButton>();
    
	if(required)
	{
        if(thisRadioIndex == -1 && button.getToggleState() != required)
		{
			return Result::fail("You need to tick " + label);
		}
        else
        {
            auto somethingPressed = false;
            
            for(auto tb: groupedButtons)
                somethingPressed |= tb->getToggleState();
            
            if(!somethingPressed)
                return Result::fail("You need to select one option of " + id.toString());
        }
	}

    if(thisRadioIndex == -1)
        writeState(button.getToggleState());
    else if(button.getToggleState())
        writeState(thisRadioIndex);
    
	return Result::ok();
}

struct TextInput::Autocomplete: public Component,
                         public ScrollBar::Listener,
                         public ComponentMovementWatcher
{
    struct Item
    {
        String displayString;
    };
    
    ScrollBar sb;
    ScrollbarFader fader;
       
    static constexpr int ItemHeight = 28;
    
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& details)
    {
        sb.mouseWheelMove(e, details);
    }
    
    void scrollBarMoved (ScrollBar* scrollBarThatHasMoved,
                                 double newRangeStart) override
    {
        repaint();
    }
    
    Font f;
    
    void componentMovedOrResized (bool wasMoved, bool wasResized) override
    {
        dismiss();
    }

    /** This callback happens when the component's top-level peer is changed. */
    void componentPeerChanged() {};

    /** This callback happens when the component's visibility state changes, possibly due to
        one of its parents being made visible or invisible.
    */
    void componentVisibilityChanged() override
    {
        if(getComponent()->isShowing())
            dismiss();
    }
    
    Autocomplete(TextInput& p):
      ComponentMovementWatcher(&p),
      parent(&p),
      sb(true)
    {
        f = Dialog::getDefaultFont(*parent).first;
        
        sb.addListener(this);
        addAndMakeVisible(sb);
        fader.addScrollBarToAnimate(sb);

        for(auto& i: p.autocompleteItems)
            allItems.add({i});

        sb.setSingleStepSize(0.2);
        
        auto& ed = parent->getComponent<TextEditor>();
        
        update(ed.getText());
        
        setSize(ed.getWidth() + 20, ItemHeight * 4 + 5 + 20);
        
        setWantsKeyboardFocus(true);
        parent->getTopLevelComponent()->addChildComponent(this);
        auto topLeft = ed.getTopLevelComponent()->getLocalArea(&ed, ed.getLocalBounds()).getTopLeft();
        
        setTopLeftPosition(topLeft.getX() - 10, topLeft.getY() + ed.getHeight());

        Desktop::getInstance().getAnimator().fadeIn(this, 150);
        
    }
    
    ~Autocomplete()
    {
        setComponentEffect(nullptr);
    }
    
    int selectedIndex = 0;
    
    void mouseDown(const MouseEvent& e) override
    {
        auto newIndex = sb.getCurrentRangeStart() + (e.getPosition().getY() - 15) / ItemHeight;
        
        if(isPositiveAndBelow(newIndex, items.size()))
            setSelectedIndex(newIndex);
    }
    
    void mouseDoubleClick(const MouseEvent& e) override
    {
        setAndDismiss();
    }
    
    bool inc(bool next)
    {
        auto newIndex = selectedIndex + (next ? 1 : -1);
        
        if(isPositiveAndBelow(newIndex, items.size()))
        {
            setSelectedIndex(newIndex);
            return true;
        }
        
        return false;
    }
    
    bool keyPressed(const KeyPress& k)
    {
        if(k == KeyPress::upKey)
            return inc(false);
        if(k == KeyPress::downKey)
            return inc(true);
        if(k == KeyPress::escapeKey)
            return dismiss();
        if(k == KeyPress::returnKey ||
           k == KeyPress::tabKey)
            return setAndDismiss();
        
        return false;
    }
    
    void setSelectedIndex(int index)
    {

        
        selectedIndex = index;
        
        if(!sb.getCurrentRange().contains(selectedIndex))
        {
            if(sb.getCurrentRange().getStart() > selectedIndex)
                sb.setCurrentRangeStart(selectedIndex);
            else
                sb.setCurrentRangeStart(selectedIndex - 3);
        }
        
        repaint();
    }
    
    void resized() override
    {
        sb.setBounds(getLocalBounds().reduced(10).removeFromRight(16).reduced(1));
    }
    
    void paint(Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
         
        b = b.reduced(10.0f);
        
        DropShadow sh;
        sh.colour = Colours::black.withAlpha(0.7f);
        sh.radius = 10;
        sh.drawForRectangle(g, b.toNearestInt());
            
        b.reduced(2.0f);
        g.setColour(Colour(0xFF222222));
        g.fillRoundedRectangle(b, 5.0f);
        g.setColour(Colours::white.withAlpha(0.3f));
        g.drawRoundedRectangle(b, 5.0f, 2.0f);
        
        auto r = sb.getCurrentRange();
        
        b.reduced(5.0f);
        b.removeFromLeft(10.0f);
        b.removeFromRight(sb.getWidth());

        b.removeFromTop(2.5f);
        
        g.setFont(f);
        
        if(items.isEmpty())
        {
            g.setColour(Colours::white.withAlpha(0.1f));
            g.drawText("No items found", getLocalBounds().toFloat(), Justification::centred);
        }
        else
        {
            for(int i = 0; i < 4; i++)
            {
                g.setColour(Colours::white.withAlpha(0.6f));
                auto tb = b.removeFromTop(ItemHeight);
                
                auto idx = i + roundToInt(r.getStart());
                
                if(idx == selectedIndex)
                {
                    g.fillRoundedRectangle(tb.withX(10.0f).reduced(3.0f, 1.0f), 3.0f);
                    g.setColour(Colours::black.withAlpha(0.8f));
                }
                    
                g.drawText(items[idx].displayString, tb, Justification::left);
            }
        }
    }
    
    bool setAndDismiss()
    {
        auto newTextAfterComma = items[selectedIndex].displayString;
        auto& ed = parent->getComponent<TextEditor>();
        
        String nt = ed.getText();
            
        if(nt.containsChar(','))
        {
            nt = nt.upToLastOccurrenceOf(",", false, false);
            nt << ", " << newTextAfterComma;
        }
        else
            nt = newTextAfterComma;
        
        ed.setText(nt, sendNotificationSync);
        
        return dismiss();
    }
    
    bool dismiss()
    {
        SafeAsyncCall::call<TextInput>(*parent, [](TextInput& ti)
        {
            ti.dismissAutocomplete();
            ti.getComponent<TextEditor>().grabKeyboardFocusAsync();
        });
        
        return true;
    }
    
    void update(const String& currentText)
    {
        auto search = currentText.fromLastOccurrenceOf(",", false, false).toLowerCase().trim();
        
        items.clear();
        
        for(const auto& i: allItems)
        {
            if(search.isEmpty() || i.displayString.toLowerCase().contains(search))
            {
                items.add(i);
            }
        }
        
        sb.setRangeLimits(0.0, (double)items.size());
        sb.setCurrentRange(0.0, 4.0);
        setSelectedIndex(0);
        
        if(items.isEmpty())
            dismiss();
    }
    
    Array<Item> allItems;
    Array<Item> items;
    
    WeakReference<TextInput> parent;
};

bool TextInput::keyPressed(const KeyPress& k)
{
	if(currentAutocomplete == nullptr)
		return false;
        
	if(k == KeyPress::upKey)
		return currentAutocomplete->inc(false);
	if(k == KeyPress::downKey)
		return currentAutocomplete->inc(true);
            
	return false;
}

void TextInput::timerCallback()
{
	if(Component::getCurrentlyFocusedComponent() == &getComponent<TextEditor>())
		showAutocomplete(getComponent<TextEditor>().getText());
        
	stopTimer();
}

void TextInput::textEditorReturnKeyPressed(TextEditor& e)
{
	if(currentAutocomplete != nullptr)
		currentAutocomplete->setAndDismiss();
}

void TextInput::textEditorTextChanged(TextEditor& e)
{
	//check(MultiPageDialog::getGlobalState(*this, id, var()));
        
	writeState(e.getText());
        
	startTimer(400);
}

void TextInput::textEditorEscapeKeyPressed(TextEditor& e)
{
	if(currentAutocomplete != nullptr)
		dismissAutocomplete();
	else
		currentAutocomplete = new Autocomplete(*this);
}

TextInput::TextInput(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new TextEditor())
{
    auto& editor = getComponent<TextEditor>();
	GlobalHiseLookAndFeel::setTextEditorColours(editor);

    setWantsKeyboardFocus(true);

    editor.setSelectAllWhenFocused(false);
    editor.setIgnoreUpDownKeysWhenSingleLine(true);
    editor.setTabKeyUsedAsCharacter(false);

    auto ml = (bool)obj[mpid::Multiline];

    if(ml)
    {
        editor.setReturnKeyStartsNewLine(true);
	    editor.setMultiLine(ml);
        editor.setFont(GLOBAL_MONOSPACE_FONT());
        editor.setTabKeyUsedAsCharacter(true);
        editor.setIgnoreUpDownKeysWhenSingleLine(false);
    }

    if(obj.hasProperty(mpid::Items))
    {
	    autocompleteItems = StringArray::fromLines(obj[mpid::Items].toString());
    }

    editor.addListener(this);
	required = obj[mpid::Required];

    if(ml)
        setSize(width, 80);
}

void TextInput::postInit()
{
    LabelledComponent::postInit();

    auto& editor = getComponent<TextEditor>();

    if(editor.isMultiLine())
    {
	    
    }
    else
    {
	    editor.setFont(Dialog::getDefaultFont(*this).first);
		editor.setIndents(4, 8);
    }
    
	editor.setText(getValueFromGlobalState(""));
    
    if(auto m = findParentComponentOfClass<Dialog>())
    {
        auto c = m->getStyleData().headlineColour;
        editor.setColour(TextEditor::ColourIds::focusedOutlineColourId, c);
        editor.setColour(Label::ColourIds::outlineWhenEditingColourId, c);
        editor.setColour(TextEditor::ColourIds::highlightColourId, c);
    }
}

Result TextInput::checkGlobalState(var globalState)
{
    auto& editor = getComponent<TextEditor>();
    
	if(required && editor.getText().isEmpty())
	{
        return Result::fail(id + " must not be empty");
	}
		
    writeState(editor.getText());
    
	return Result::ok();
}

void TextInput::showAutocomplete(const String& currentText)
{
	if(!autocompleteItems.isEmpty() && currentAutocomplete == nullptr && currentText.isNotEmpty())
	{
		currentAutocomplete = new Autocomplete(*this);
	}
	else
	{
		if(currentText.isEmpty())
			currentAutocomplete = nullptr;
		else if (currentAutocomplete != nullptr)
			currentAutocomplete->update(currentText);
	}
}

void TextInput::dismissAutocomplete()
{
	stopTimer();
	Desktop::getInstance().getAnimator().fadeOut(currentAutocomplete, 150);
	currentAutocomplete = nullptr;
}
} // factory
} // multipage
} // hise