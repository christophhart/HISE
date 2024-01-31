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

#include "MultiPageDialog.h"
#include "MultiPageDialog.h"
#include "MultiPageDialog.h"
#include "MultiPageDialog.h"
#include "MultiPageDialog.h"
#include "MultiPageDialog.h"
#include "MultiPageDialog.h"


namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

template <typename T> void addBasicComponents(T& obj, Dialog::PageInfo& rootList, const String& typeHelp)
{
	rootList.addChild<Type>({
        { mpid::ID, "Type"},
        { mpid::Type, T::getStaticId().toString() },
        { mpid::Help, typeHelp }
    });

    rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
        { mpid::Value, "" },
		{ mpid::Help, "The ID for the element (used as key in the state `var`.\n>" }
	});

    rootList.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label next to the " + T::getStaticId() }
	});

    rootList.addChild<Choice>({
        { mpid::ID, "LabelPosition" },
        { mpid::Text, "LabelPosition" },
        { mpid::Items, Dialog::PositionInfo::getLabelPositionNames().joinIntoString("\n") },
        { mpid::Value, obj.getDefaultPositionName() },
        { mpid::Help, "The position of the text label. This overrides the value of the global layout data. " }
    });
}

LabelledComponent::LabelledComponent(Dialog& r, int width, const var& obj, Component* c):
	PageBase(r, width, obj),
	component(c)
{
	addAndMakeVisible(c);
	label = obj[mpid::Text].toString();

    positionInfo = r.getPositionInfo(obj);

    required = obj[mpid::Required];

    if(obj.hasProperty(mpid::LabelPosition))
    {
        auto np = positionInfo.getLabelPositionNames().indexOf(obj[mpid::LabelPosition].toString());

        if(np != Dialog::PositionInfo::LabelPositioning::Default)
        {
            positionInfo.LabelPosition = np;
	        inheritsPosition = false;
        }
    }
    
    setInterceptsMouseClicks(false, true);
	setSize(width, positionInfo.getHeightForComponent(32));
}

void LabelledComponent::postInit()
{
	init();
}

void LabelledComponent::paint(Graphics& g)
{
    auto b = getLocalBounds();
    using Pos = Dialog::PositionInfo::LabelPositioning;

    switch((Pos)positionInfo.LabelPosition)
    {
    	case Pos::Left:  b = b.removeFromLeft(positionInfo.getWidthForLabel(getWidth())).reduced(8, 0); break;
		case Pos::Above: b = b.removeFromTop(positionInfo.LabelHeight); break;
		case Pos::None:  b = {}; break;
    }

    auto df = Dialog::getDefaultFont(*this);

    if(!b.isEmpty())
    {
		g.setFont(df.first);
		g.setColour(df.second);

        g.drawText(label, b.toFloat(), Justification::left);
    }

    if(rootDialog.isEditModeEnabled() && id.isValid() && findParentComponentOfClass<Dialog::ModalPopup>() == nullptr)
    {
        g.setFont(GLOBAL_MONOSPACE_FONT());
        g.setColour(df.second.withAlpha(0.5f));
        g.drawText(id.toString() + " ", b.toFloat(), Justification::right);
    }
}

void LabelledComponent::resized()
{
    PageBase::resized();
    
	auto b = getLocalBounds();

    using Pos = Dialog::PositionInfo::LabelPositioning;

    switch((Pos)positionInfo.LabelPosition)
    {
    	case Pos::Left:  b.removeFromLeft(positionInfo.getWidthForLabel(getWidth())); break;
		case Pos::Above: b.removeFromTop(positionInfo.LabelHeight); break;
		case Pos::None:  break;
    }

	if(helpButton != nullptr)
	{
		helpButton->setBounds(b.removeFromRight(32).withSizeKeepingCentre(24, 24));
		b.removeFromRight(5);
	}

    component->setBounds(b);
}

void LabelledComponent::editModeChanged(bool isEditMode)
{
	PageBase::editModeChanged(isEditMode);

	if(overlay != nullptr)
	{
        auto pos = positionInfo;

		overlay->localBoundFunction = [pos](Component* c)
		{
			auto b = c->getLocalBounds();

			return b.removeFromLeft(pos.getWidthForLabel(c->getWidth()));
		};

		overlay->setOnClick([this]()
		{
			auto& l = rootDialog.createModalPopup<List>();

            l.setStateObject(infoObject);
            createEditor(l);

            l.setCustomCheckFunction([this](PageBase* b, var obj)
		    {
                auto ok = this->loadFromInfoObject(obj);
				this->repaint();
		        return ok;
		    });

            rootDialog.showModalPopup(true);

		});
	}
}




void Tickbox::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents<Tickbox>(*this, rootList, "A Tickbox lets you enable / disable a boolean option or can be grouped together with other tickboxes to create a radio group with an exclusive selection option");

	rootList.addChild<Tickbox>({
		{ mpid::ID, "Required" },
		{ mpid::Text, "Required" },
		{ mpid::Help, "If this is enabled, the tickbox must be selected in order to proceed to the next page, otherwise it will show a error message.\n> This is particularly useful for eg. accepting TOC" }
	});

    
}


Tickbox::Tickbox(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new ToggleButton())
{
    positionInfo.setDefaultPosition(Dialog::PositionInfo::LabelPositioning::None);

	if(positionInfo.LabelPosition == Dialog::PositionInfo::LabelPositioning::None)
        getComponent<ToggleButton>().setButtonText(obj[mpid::Text]);

	loadFromInfoObject(obj);
}

void Tickbox::buttonClicked(Button* b)
{
    for(auto tb: groupedButtons)
        tb->setToggleState(b == tb, dontSendNotification);
}

Result Tickbox::loadFromInfoObject(const var& obj)
{
    auto ok = LabelledComponent::loadFromInfoObject(obj);

    if(obj.hasProperty(mpid::Required))
	{
		requiredOption = true;
	}

    return ok;
}

Choice::Choice(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new SubmenuComboBox())
{
    positionInfo.setDefaultPosition(Dialog::PositionInfo::LabelPositioning::Left);

    if(obj.hasProperty(mpid::ValueMode))
    {
	    valueMode = (ValueMode)getValueModeNames().indexOf(obj[mpid::ValueMode].toString());
    }

    loadFromInfoObject(obj);
    auto& combobox = getComponent<SubmenuComboBox>();
    custom = obj[mpid::Custom];
    combobox.setUseCustomPopup(custom);
	hise::GlobalHiseLookAndFeel::setDefaultColours(combobox);

    resized();
}

Result Choice::loadFromInfoObject(const var& obj)
{
	auto ok = LabelledComponent::loadFromInfoObject(obj);

    auto& combobox = getComponent<ComboBox>();
	auto s = obj[mpid::Items].toString();
	combobox.addItemList(StringArray::fromLines(s), 1);
    return ok;

}

void Choice::postInit()
{
    LabelledComponent::postInit();
	auto t = getValueFromGlobalState();

    auto& cb = getComponent<ComboBox>();

    switch(valueMode)
    {
    case ValueMode::Text: cb.setText(t.toString(), dontSendNotification); break;
    case ValueMode::Index: cb.setSelectedItemIndex((int)t, dontSendNotification); break;
    case ValueMode::Id: cb.setSelectedId((int)t, dontSendNotification); break;
    }
    
    getComponent<SubmenuComboBox>().refreshTickState();
}

Result Choice::checkGlobalState(var globalState)
{
    auto& cb = getComponent<ComboBox>();

    switch(valueMode)
    {
    case ValueMode::Text:  writeState(cb.getText()); break;
    case ValueMode::Index: writeState(cb.getSelectedItemIndex()); break;
    case ValueMode::Id:    writeState(cb.getSelectedId()); break;
    }
    
	return Result::ok();
}

void Choice::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents<Choice>(*this, rootList, "A Choice can be used to select multiple fixed options with a drop down menu");

    rootList.addChild<Tickbox>({
        { mpid::ID, "Custom" },
        { mpid::Text, "Custom" },
        { mpid::Value, custom },
        { mpid::Help, "If ticked, then you can use the markdown syntax to create complex popup menu configurations." }
    });

	rootList.addChild<TextInput>({
		{ mpid::ID, "Items" },
		{ mpid::Text, "Items" },
        { mpid::Multiline, true },
		{ mpid::Help, "A string with one item per line" }
	});

    rootList.addChild<Choice>({
        { mpid::ID, "ValueMode" },
        { mpid::Text, "ValueMode" },
        { mpid::Help, "Defines how to store the value in the data object (either as text, integer index or one-based integer ID" },
        { mpid::Value, getValueModeNames()[(int)valueMode] },
        { mpid::Items, getValueModeNames().joinIntoString("\n") }
    });
}

ColourChooser::ColourChooser(Dialog& r, int w, const var& obj):
	LabelledComponent(r, w, obj, new ColourSelector(ColourSelector::ColourSelectorOptions::showColourspace | ColourSelector::showColourAtTop | ColourSelector::showAlphaChannel, 2, 0))
{
    positionInfo.setDefaultPosition(Dialog::PositionInfo::LabelPositioning::Above);
    
	auto& selector = getComponent<ColourSelector>();
	selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
	selector.setLookAndFeel(&laf);

	setSize(w, positionInfo.getHeightForComponent(130));
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
    init();

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
        if(thisRadioIndex == -1 && button.getToggleState() != requiredOption)
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
    positionInfo.setDefaultPosition(Dialog::PositionInfo::LabelPositioning::Left);

    

    parseInputAsArray = obj[mpid::ParseArray];

    auto& editor = getComponent<TextEditor>();
	GlobalHiseLookAndFeel::setTextEditorColours(editor);

    if(obj.hasProperty(mpid::EmptyText))
    {
        emptyText = obj[mpid::EmptyText].toString();
	    editor.setTextToShowWhenEmpty(emptyText, editor.findColour(TextEditor::textColourId).withAlpha(0.5f));
    }

    setWantsKeyboardFocus(true);

    editor.setSelectAllWhenFocused(false);
    editor.setIgnoreUpDownKeysWhenSingleLine(true);
    editor.setTabKeyUsedAsCharacter(false);

    loadFromInfoObject(obj);

    editor.addListener(this);

    if(editor.isMultiLine())
        setSize(width, positionInfo.getHeightForComponent(80));
    else
        resized();
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
		editor.setIndents(8, 8);
    }

    auto text = getValueFromGlobalState("");

    if(parseInputAsArray && text.isArray())
    {
	    StringArray sa;

        for(const auto& v: *text.getArray())
			sa.add(v.toString());

        text = sa.joinIntoString(", ");
    }

	editor.setText(text);
    
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
        return Result::fail(label + " must not be empty");
	}

    auto text = editor.getText();

    if(parseInputAsArray)
    {
	    auto sa = StringArray::fromTokens(text, ",", "");
        sa.trim();

        Array<var> values;

        for(const auto& s: sa)
            values.add(var(s));

        writeState(var(values));
    }
    else
    {
	    writeState(text);
    }
    
	return Result::ok();
}

void TextInput::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents<TextInput>(*this, rootList, "A TextInput lets you enter a String");

    rootList.getChild(mpid::ID)[mpid::Value] = id.toString();
    rootList.getChild(mpid::Text)[mpid::Value] = label;

    rootList.addChild<TextInput>({
		{ mpid::ID, "EmptyText" },
		{ mpid::Text, "EmptyText" },
        { mpid::Value, emptyText },
		{ mpid::Help, "The text to show when the text field is empty." }
	});
#if 0
	
        
	rootList.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
        { mpid::Required, true },
        { mpid::Value, label },
		{ mpid::Help, "The label next to the text input." }
	});
#endif
        
	rootList.addChild<Tickbox>({
		{ mpid::ID, "Required" },
		{ mpid::Text, "Required" },
		{ mpid::Help, "If this is enabled, the text input must be a non-empty String" },
        { mpid::Value, required }
	});

    rootList.addChild<Tickbox>({
		{ mpid::ID, "ParseArray" },
		{ mpid::Text, "ParseArray" },
		{ mpid::Help, "If this is enabled, the text input will parse comma separated values as Array." },
        { mpid::Value, parseInputAsArray }
	});

    rootList.addChild<TextInput>({
		{ mpid::ID, "Items" },
		{ mpid::Text, "Items" },
        { mpid::Multiline, true },
		{ mpid::Help, "A string with one item per line that will show up in the autocomplete popup." },
        { mpid::Value, autocompleteItems.joinIntoString("\n") }
	});
}

Result TextInput::loadFromInfoObject(const var& obj)
{
    auto ok = LabelledComponent::loadFromInfoObject(obj);

	auto& editor = getComponent<TextEditor>();
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

	
	
	
	if(obj.hasProperty(mpid::Items))
	{
		autocompleteItems = StringArray::fromLines(obj[mpid::Items].toString());
	}

    return ok;
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



void FileSelector::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents(*this, rootList, "Select a file using a native file browser or text input. ");
    
    rootList.addChild<Tickbox>({
		{ mpid::ID, "Directory" },
		{ mpid::Text, "Directory" },
		{ mpid::Help, "Whether the file selector should be able to select directories vs. files." }
	});

    rootList.addChild<TextInput>({
		{ mpid::ID, "Wildcard" },
		{ mpid::Text, "Wildcard" },
		{ mpid::Help, "The file wildcard for filtering selectable files" }
	});

    rootList.addChild<Tickbox>({
		{ mpid::ID, "SaveFile" },
		{ mpid::Text, "SaveFile" },
		{ mpid::Help, "Whether the selected file will be opened or saved." }
	});
}

    
struct BetterFileSelector: public Component,
						   public TextEditor::Listener
{
    BetterFileSelector(const String& name, const File& initialFile, bool unused, bool isDirectory, bool save, const String& wildcard):
      Component(name),
      browseButton("Browse"),
      currentFile(initialFile)
    {
	    addAndMakeVisible(browseButton);
        addAndMakeVisible(fileLabel);

        GlobalHiseLookAndFeel::setTextEditorColours(fileLabel);
        hise::GlobalHiseLookAndFeel::setDefaultColours(*this);

	    fileLabel.setWantsKeyboardFocus(true);
        fileLabel.setTextToShowWhenEmpty("No file selected", Colours::black.withAlpha(0.3f));
        fileLabel.setEscapeAndReturnKeysConsumed(true);
	    fileLabel.setSelectAllWhenFocused(true);
	    fileLabel.setIgnoreUpDownKeysWhenSingleLine(true);
	    fileLabel.setTabKeyUsedAsCharacter(false);

        String wc(wildcard);

        browseButton.onClick = [wc, save, this, isDirectory]()
        {
	        

            if(isDirectory)
            {
                FileChooser fc("Select directory", currentFile, wc, true);

	            if(fc.browseForDirectory())
                    setCurrentFile(fc.getResult(), sendNotificationAsync);
            }
            else
            {
	            if(save)
	            {
                    FileChooser fc("Select file to save", currentFile, wc, true);

		            if(fc.browseForFileToSave(true))
                        setCurrentFile(fc.getResult(), sendNotificationAsync);
	            }
	            else
	            {
                    FileChooser fc("Select file to open", currentFile, wc, true);

		            if(fc.browseForFileToOpen())
                        setCurrentFile(fc.getResult(), sendNotificationAsync);
	            }
            }
        };

        fileLabel.onReturnKey = [this]()
        {
	        auto t = fileLabel.getText();

            if(File::isAbsolutePath(t))
            {
	            setCurrentFile(File(t), sendNotificationAsync);
            }
            else if (t.isEmpty())
            {
	            setCurrentFile(File(), sendNotificationAsync);
            }
        };
    }

    void resized() override
    {
	    auto b = getLocalBounds();
        b.removeFromRight(5);

        browseButton.setBounds(b.removeFromRight(128));
        b.removeFromRight(10);
        fileLabel.setBounds(b);
    }


    void setCurrentFile(const File& f, NotificationType n)
    {
	    if(f != currentFile)
	    {
            currentFile = f;
            fileLabel.setText(f.getFullPathName(), dontSendNotification);
		    fileBroadcaster.sendMessage(n, f);
	    }
    }

    File getCurrentFile() const { return currentFile; }

    LambdaBroadcaster<File> fileBroadcaster;



    File currentFile;
	TextEditor fileLabel;
    TextButton browseButton;

    JUCE_DECLARE_WEAK_REFERENCEABLE(BetterFileSelector);
};

FileSelector::FileSelector(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, createFileComponent(obj)),
	fileId(obj["ID"].toString())
{
    positionInfo.setDefaultPosition(Dialog::PositionInfo::LabelPositioning::Left);

    auto& fileSelector = getComponent<BetterFileSelector>();

    fileSelector.fileBroadcaster.addListener(*this, [](FileSelector& f, File nf)
    {
        f.writeState(nf.getFullPathName());
    }, false);

	isDirectory = obj[mpid::Directory];
	addAndMakeVisible(fileSelector);
        
	setSize(width, positionInfo.getHeightForComponent(32));
    resized();
}

Component* FileSelector::createFileComponent(const var& obj)
{
	bool isDirectory = obj[mpid::Directory];
	auto name = obj[mpid::Text].toString();
	if(name.isEmpty())
		name = isDirectory ? "Directory" : "File";
	auto wildcard = obj[mpid::Wildcard].toString();
	auto save = (bool)obj[mpid::SaveFile];
        
	return new BetterFileSelector(name, File(), true, isDirectory, save, wildcard);
}

void FileSelector::postInit()
{
    LabelledComponent::postInit();

	auto v = getValueFromGlobalState();

    auto& fileSelector = getComponent<BetterFileSelector>();
    
    fileSelector.fileLabel.setFont(Dialog::getDefaultFont(*this).first);
	fileSelector.fileLabel.setIndents(8, 8);

	fileSelector.setCurrentFile(getInitialFile(v), dontSendNotification);
}

Result FileSelector::checkGlobalState(var globalState)
{
    auto& fileSelector = getComponent<BetterFileSelector>();
	auto f = fileSelector.getCurrentFile();
        
	if(f != File() && !f.isRoot() && (f.isDirectory() || f.existsAsFile()))
	{
        writeState(f.getFullPathName());

		return Result::ok();
	}
        
	String message;
	message << "You need to select a ";
	if(isDirectory)
		message << "directory";
	else
		message << "file";
        
	return Result::fail(message);
}

File FileSelector::getInitialFile(const var& path)
{
	if(path.isString())
		return File(path);
	if(path.isInt() || path.isInt64())
	{
		auto specialLocation = (File::SpecialLocationType)(int)path;
		return File::getSpecialLocation(specialLocation);
	}
        
	return File();
}




} // factory
} // multipage
} // hise