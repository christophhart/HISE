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

#if HISE_MULTIPAGE_INCLUDE_EDIT
void addOnValueCodeEditor(const var& infoObject, Dialog::PageInfo& rootList)
{
    auto code = infoObject[mpid::Code].toString();

    
    
    
}

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

    rootList.addChild<Button>({
        { mpid::ID, "Enabled" },
        { mpid::Text, "Enabled" },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::Enabled) },
        { mpid::Help, "Whether to allow user input for the element" }
    });

    

    auto& vlist = rootList.addChild<List>({
        { mpid::Text, "Value Properties"},
        { mpid::ID, "valueList"},
        { mpid::Foldable, true }
    });

    auto& tlist = rootList.addChild<List>({
        { mpid::Text, "Text Properties"},
        { mpid::ID, "textList"},
        { mpid::Foldable, true }
    });

    auto& clist = rootList.addChild<List>({
        { mpid::Text, "CSS Properties"},
        { mpid::Foldable, true }
    });

    vlist.addChild<TextInput>({
		{ mpid::ID, "InitValue" },
		{ mpid::Text, "InitValue" },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::InitValue) },
		{ mpid::Help, "The initial value for the UI element" }
	});

    

    vlist.addChild<Button>({
        { mpid::ID, "UseInitValue" },
        { mpid::Text, "UseInitValue" },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::UseInitValue) },
        { mpid::Help, "Whether to initialise the state object with the init value" }
    });

    vlist.addChild<TextInput>({
		{ mpid::ID, mpid::Code.toString() },
		{ mpid::Text, mpid::Code.toString() },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::Code) },
        { mpid::Items, "{BIND::functionName}" },
		{ mpid::Help, "The callback that is executed using the syntax `{BIND::myMethod}`" }
	});

    auto& col = rootList;

    tlist.addChild<TextInput>({
		{ mpid::ID, "Text" },
		{ mpid::Text, "Text" },
		{ mpid::Help, "The label next to the " + T::getStaticId() }
	});

    tlist.addChild<TextInput>({
        { mpid::ID, "Help" },
        { mpid::Text, "Help" },
        { mpid::Multiline, true },
        { mpid::Help, "The markdown content that will be shown when you click on the help button" }
    });

    tlist.addChild<TextInput>({
		{ mpid::ID, mpid::Tooltip.toString() },
		{ mpid::Text, mpid::Tooltip.toString() },
		{ mpid::Help, "The tooltip that will be applied when hovering the element" }
	});

    clist.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::Class) },
		{ mpid::Help, "The CSS class selectors for the UI element" }
	});

    clist.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::Style) },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});

    clist.addChild<Choice>({
		{ mpid::ID, mpid::Visibility.toString() },
		{ mpid::Text, mpid::Visibility.toString() },
        { mpid::Items, Dialog::PageBase::getVisibilityNames().joinIntoString("\n") },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::Visibility) },
		{ mpid::Help, "Whether to show or hide the element" }
	});

    clist.addChild<Button>({
		{ mpid::ID, mpid::NoLabel.toString() },
		{ mpid::Text, mpid::NoLabel.toString() },
        { mpid::Value, obj.getPropertyFromInfoObject(mpid::NoLabel) },
		{ mpid::Help, "Whether to attach a label to the UI element and wrap it into a div.  \n> If this is disabled, the style / class will be applied directly to the UI element, otherwise it will be applied to the wrapper div" }
	});
}
#endif

LabelledComponent::LabelledComponent(Dialog& r, int width, const var& obj, Component* c):
	PageBase(r, width, obj),
	component(c),
    showLabel((bool)!obj[mpid::NoLabel])

{
    if(showLabel)
    {
	    Helpers::setFallbackStyleSheet(*this, "display: flex; flex-direction: row; width: 100%; height: auto; gap: 10px;");

        setTextElementSelector(simple_css::ElementType::Label);

	    label = obj[mpid::Text].toString();
	    addTextElement({  }, label);
        addFlexItem(*c);
	    Helpers::setFallbackStyleSheet(*c, "flex-grow: 1; height: 32px;");
    }
    else
    {
        addFlexItem(*c);
        setIsInvisibleWrapper(true);
        updateStyleSheetInfo(true);
	    Helpers::setFallbackStyleSheet(*c, "flex-grow: 1; height: 32px;width: 100%;");

        changeClass(simple_css::Selector(".no-label"), true);
    }
    
    if(!obj.hasProperty(mpid::Enabled))
        obj.getDynamicObject()->setProperty(mpid::Enabled, true);
	
    required = obj[mpid::Required];
    
    setWantsKeyboardFocus(false);
    setInterceptsMouseClicks(false, true);

    //auto h = positionInfo.getHeightForComponent(32);

	//setSize(width, h);
}

Result LabelledComponent::loadFromInfoObject(const var& obj)
{
	label = obj[mpid::Text];

    auto tooltip = obj[mpid::Tooltip].toString();

    if(auto stc = dynamic_cast<SettableTooltipClient*>(component.get()))
    {
	    stc->setTooltip(tooltip);
    }

    if(showLabel)
    {
        for(int i = 0; i < getNumChildComponents(); i++)
        {
	        if(auto st = dynamic_cast<SimpleTextDisplay*>(getChildComponent(i)))
		    {
                st->setText(label);
                setFlexChildVisibility(i, false, label.isEmpty());
                break;
		    }
        }
    }

	return Result::ok();
}

void LabelledComponent::postInit()
{
    

    if(infoObject.hasProperty(mpid::Enabled))
        enabled = infoObject[mpid::Enabled];

    loadFromInfoObject(infoObject);

    if((initValue.isVoid() || initValue.isUndefined()) && infoObject[mpid::UseInitValue] && getValueFromGlobalState(var()).isVoid())
    {
	    initValue = infoObject[mpid::InitValue];
    }

    if(!infoObject.hasProperty(mpid::Code))
        infoObject.getDynamicObject()->setProperty(mpid::Code, "");

	init();

    
    
    getComponent<Component>().setEnabled(enabled);

    //setVisible(infoObject[mpid::Visible] || rootDialog.isEditModeEnabled());
    repaint();
}

    

void LabelledComponent::resized()
{
    FlexboxComponent::resized();

    if(helpButton != nullptr)
	{
		setHiseShapeButtonColours(*helpButton);
	}
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Button::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents<Button>(*this, rootList, "A Button lets you enable / disable a boolean option or can be grouped together with other tickboxes to create a radio group with an exclusive selection option");

    auto& vlist = rootList.getChild("valueList");

    auto& tlist = rootList.getChild("textList");

	vlist.addChild<Button>({
		{ mpid::ID, "Required" },
		{ mpid::Text, "Required" },
		{ mpid::Help, "If this is enabled, the tickbox must be selected in order to proceed to the next page, otherwise it will show a error message.\n> This is particularly useful for eg. accepting TOC" }
	});

    

    rootList.addChild<Choice>({
        { mpid::ID, "ButtonType" },
        { mpid::Text, "ButtonType" },
        { mpid::Items, "Toggle\nText\nIcon" },
        { mpid::Help, "The appearance of the button. " }
    });

    vlist.addChild<Button>({
		{ mpid::ID, "Trigger" },
		{ mpid::Text, "Trigger" },
		{ mpid::Help, "If this is enabled, the button will fire the action with the same ID when you click it, otherwise it will store its value (either on/off or radio group index in the global state" }
	});

    addOnValueCodeEditor(infoObject, vlist);
}
#endif


Button::Button(Dialog& r, int width, const var& obj):
    ButtonListener(),
	LabelledComponent(r, width, obj, createButton(obj))
{
    getComponent<Component>().setWantsKeyboardFocus(false);

    auto isText = infoObject[mpid::ButtonType].toString() == "Text";

    Helpers::writeClassSelectors(getComponent<juce::Button>(), { (isText ? ".text-button" : ".toggle-button") }, true);
    
	loadFromInfoObject(obj);
}

void Button::buttonClicked(juce::Button* b)
{
    jassert(b == &getComponent<juce::Button>());
    
    if(isTrigger)
    {
        auto thisId = id;

        writeState(true);

	    Component::callRecursive<Action>(&rootDialog, [thisId](Action* a)
	    {
		    if(a->getId().toString() == thisId.toString())
		    {
			    a->perform();
                return true;
		    }

            return false;
	    });
    }
    else
    {
        if(groupedButtons.isEmpty())
        {
            writeState(b->getToggleState());
        }
        else
        {
            writeState(groupedButtons.indexOf(b));
        }

	    for(auto tb: groupedButtons)
	    {
            auto thisValue = b == tb;
		    if(auto sb = dynamic_cast<HiseShapeButton*>(tb))
		    {
			    sb->setToggleStateAndUpdateIcon(thisValue, true);
		    }
            else
            {
	            tb->setToggleState(thisValue, dontSendNotification);
            }
	    }
			
    }

    callOnValueChange("click");
}

Result Button::loadFromInfoObject(const var& obj)
{
    auto ok = LabelledComponent::loadFromInfoObject(obj);

    isTrigger = obj[mpid::Trigger];

    getComponent<juce::Button>().setClickingTogglesState(!isTrigger);
    
    if(obj.hasProperty(mpid::Required))
		requiredOption = true;

    return ok;
}

Path Button::IconFactory::createPath(const String& url) const
{
	Path p;

	auto b64 = obj[mpid::Icon].toString();

	if(d != nullptr)
		b64 = d->getState().loadText(b64, false);

	MemoryBlock mb;
	mb.fromBase64Encoding(b64);

	if(mb.getSize() == 0)
		mb.fromBase64Encoding("844.t01G.z.QfCheCwV..d.QfCheCwV..d.QbVhXCIV..d.QL0zSCAyTKPDV..zPCk.DDgE..MDajeuEDgE..MDajeuEDQIvVMDae.PCDQIvVMDae.PCDANH9MzXs4S7aPDk.a0Pr4S7aPDV..zProYojPDV..zProYojPDk.a0Pr4S7aPDk.a0Pi0F8dlBQTBrUCwF8dlBQXA.PCwFTSICQXA.PCwFTSICQTBrUCwF8dlBQTBrUCMVa3xzMDQIvVMDa3xzMDgE..MDa8ZOODgE..MjXQyZPDgE..MT..VDQL0zSCE.fEQDmkH1PrE.fEQD3f32PrI9++PD3f32PrI9++PDk.a0PrgKS2PDk.a0Pi0l3++CQjLPhCwV..VDQjLPhCwV..VDQbullCwl3++CQbullCwl3++CQjLPhCMVah++ODQvWjNDaA.XQDQvWjNDaA.XQDQsw0NDah++ODQsw0NDah++ODQvWjNzXsI9++PD+496PrE.fEQD+496PrE.fEQDhsq7PhE.fEQjHZQ8PQyZPDA8+aOTu1yCQP++1CwFtLcCQP++1CwFtLcCQN+IzCwl3++CQN+IzCwl3++CQ7m6uCMVaPMkLD47mPODaPMkLDA8+aODaz6YJDA8+aODaz6YJD47mPODaPMkLD47mPOzXsoYojPjyeB8ProYojPDz+u8Pr4S7aPDz+u8Pr4S7aPjyeB8ProYojPjyeB8Pi0F42aAQN+IzCwF42aAQP++1Cw1PI.AQP++1CIFLSs.QP++1CE.fGPjHZQ8PA.3ADgX6JODaA.3ADwet+NDae.PCDwet+NDae.PCD47mPODajeuED47mPOzXs8A.MPD0FW6PrE.fGPD0FW6PrE.fGPDAeQ5Pr8A.MPDAeQ5Pr8A.MPD0FW6Pi01G.z.QbullCwV..d.QbullCwV..d.QjLPhCw1G.z.QjLPhCw1G.z.QbullCMVa3QyHDAI.dNDaJpeFDwEiKNDaKXTGD4S8DNDa4+mIDoQZWNDa2m6KD4S8DNDa3UvLDwEiKNDaHtbJDAI.dNDa3UvLDAEcvNDa2m6KDg7B2NDa4+mIDItkjNDaKXTGDg7B2NDaJpeFDAEcvNDa3QyHDAI.dNzXkA");
	else
		p.loadPathFromData(mb.getData(), mb.getSize());
            
	return p;
}

String Button::getStringForButtonType() const
{
	auto c = &getComponent<Component>();

	if(dynamic_cast<const TextButton*>(c))
		return "Text";
	if(dynamic_cast<const ToggleButton*>(c))
		return "Toggle";
	if(dynamic_cast<const HiseShapeButton*>(c))
		return "Icon";

	return "Undefined";
}

juce::Button* Button::createButton(const var& obj)
{
	auto buttonType = obj[mpid::ButtonType].toString();

	if(buttonType == "Toggle")
		return new ToggleButton();
	if(buttonType == "Text")
		return new TextButton(obj[mpid::Text]);
	if(buttonType == "Icon")
	{
		IconFactory f(nullptr, obj);
		return new HiseShapeButton("icon", this, f);
	}
	
    return new ToggleButton();
}

    
void Button::postInit()
{
    LabelledComponent::postInit();

    if(auto sb = dynamic_cast<HiseShapeButton*>(&getComponent<juce::Button>()))
    {
        IconFactory f(&rootDialog, infoObject);

        auto icon = f.createPath("");

        sb->setShapes(icon, icon);

        sb->offColour = rootDialog.getStyleData().textColour;
        sb->onColour = rootDialog.getStyleData().headlineColour;
        sb->setToggleModeWithColourChange(true);
        sb->refreshShape();
		sb->refreshButtonColours();
    }
    
    auto& button = this->getComponent<juce::Button>();
    button.setButtonText(infoObject[mpid::Text].toString());
    
    Component* root = getParentComponent();
    
    while(dynamic_cast<PageBase*>(root) != nullptr)
    {
        root = root->getParentComponent();
    }
    
    Component::callRecursive<Button>(root, [&](Button* tb)
    {
        if(tb->id == id)
            groupedButtons.add(&tb->getComponent<juce::Button>());
        
        return false;
    });
    
    if(groupedButtons.size() > 1)
    {
        thisRadioIndex = groupedButtons.indexOf(&button);
        auto radioIndex = (int)getValueFromGlobalState(-1);
        
        int idx = 0;
        
        for(auto b: groupedButtons)
        {
            b->setToggleState(idx++ == radioIndex, dontSendNotification);
        }
        
        button.addListener(this);
    }
    else
    {
        groupedButtons.clear();
        button.setToggleState(getValueFromGlobalState(false), dontSendNotification);

        button.addListener(this);
    }
    
	button.setColour(ToggleButton::ColourIds::tickColourId, Dialog::getDefaultFont(*this).second);
    

}

Result Button::checkGlobalState(var globalState)
{
    if(isTrigger)
        return Result::ok();

    auto& button = getComponent<juce::Button>();
    
	if(required)
	{
        if(thisRadioIndex == -1)
		{
            if(button.getToggleState() != requiredOption)
                return Result::fail("You need to tick this button");
		}
        else
        {
            auto somethingPressed = false;
            
            for(auto tb: groupedButtons)
                somethingPressed |= tb->getToggleState();
            
            if(!somethingPressed)
                return Result::fail("You need to select one option");
        }
	}

    if(thisRadioIndex == -1)
        writeState(button.getToggleState());
    else if(button.getToggleState())
        writeState(thisRadioIndex);
    
	return Result::ok();
}


Choice::Choice(Dialog& r, int width, const var& obj):
	LabelledComponent(r, width, obj, new SubmenuComboBox())
{
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

    auto& combobox = getComponent<SubmenuComboBox>();
    combobox.clear(dontSendNotification);
	combobox.addItemList(getItemsAsStringArray(), 1);

    if(custom)
        combobox.rebuildPopupMenu();

    return ok;

}

void Choice::postInit()
{
    LabelledComponent::postInit();
	auto t = getValueFromGlobalState();

    auto& cb = getComponent<ComboBox>();

    cb.setTextWhenNothingSelected(infoObject[mpid::EmptyText].toString());

    cb.onChange = [this]()
    {
        const auto& cb = const_cast<const Choice*>(this)->getComponent<ComboBox>();

	    switch(valueMode)
	    {
		    {
		    case ValueMode::Text: writeState(cb.getText()); break;
		    case ValueMode::Index: writeState(cb.getSelectedItemIndex()); break;
		    case ValueMode::Id: writeState(cb.getSelectedId()); break;
		    case ValueMode::numValueModes: 
            default: break;
		    }
	    }

        callOnValueChange("change");
    };

    switch(valueMode)
    {
    case ValueMode::Text: cb.setText(t.toString(), dontSendNotification); break;
    case ValueMode::Index: cb.setSelectedItemIndex((int)t, dontSendNotification); break;
    case ValueMode::Id: cb.setSelectedId((int)t, dontSendNotification); break;
    case ValueMode::numValueModes: 
    default: break;
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
    case ValueMode::numValueModes:
    default: break;
    }
    
	return Result::ok();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Choice::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents<Choice>(*this, rootList, "A Choice can be used to select multiple fixed options with a drop down menu");

    rootList.addChild<TextInput>({
		{ mpid::ID, mpid::EmptyText.toString() },
		{ mpid::Text, mpid::EmptyText.toString() },
        { mpid::Value, infoObject[mpid::EmptyText] },
		{ mpid::Help, "A text that will be displayed if nothing is selected" }
	});

    auto& col = rootList;

	col.addChild<TextInput>({
		{ mpid::ID, "Items" },
		{ mpid::Text, "Items" },
        { mpid::Multiline, true },
		{ mpid::Help, "A string with one item per line" }
	});

    col.addChild<Button>({
        { mpid::ID, "Custom" },
        { mpid::Text, "Custom" },
        { mpid::Value, custom },
        { mpid::Help, "If ticked, then you can use the markdown syntax to create complex popup menu configurations." }
    });

    rootList.addChild<Choice>({
        { mpid::ID, "ValueMode" },
        { mpid::Text, "ValueMode" },
        { mpid::Help, "Defines how to store the value in the data object (either as text, integer index or one-based integer ID" },
        { mpid::Value, getValueModeNames()[(int)valueMode] },
        { mpid::Items, getValueModeNames().joinIntoString("\n") }
    });

    addOnValueCodeEditor(infoObject, rootList);
}

using EditorType = AllEditor;

#else
struct EditorType: public Component
{
    EditorType(const String& unused, var unused2) {};
    String syntax;
    Result compile() { return Result::fail("not implemented"); }
	CodeDocument doc;
};
#endif


CodeEditor::CodeEditor(Dialog& r, int w, const var& obj):
	LabelledComponent(r, w, obj, new EditorType(obj[mpid::Syntax].toString(), var()))
{
	Helpers::writeInlineStyle(getComponent<EditorType>(), "height: 360px;");
	setSize(w, 360);
}


void CodeEditor::postInit()
{
	LabelledComponent::postInit();

	auto code = getValueFromGlobalState(var()).toString();

    getComponent<EditorType>().syntax = infoObject[mpid::Syntax].toString();

	getComponent<EditorType>().doc.replaceAllContent(code);
}



Result CodeEditor::compile()
{
    auto code = getComponent<EditorType>().doc.getAllContent();
    writeState(code);
    return getComponent<EditorType>().compile();
}


ColourChooser:: ColourChooser(Dialog& r, int w, const var& obj):
	LabelledComponent(r, w, obj, new ColourSelector(ColourSelector::ColourSelectorOptions::showColourspace | ColourSelector::showColourAtTop | ColourSelector::showAlphaChannel | ColourSelector::ColourSelectorOptions::editableColour, 2, 0))
{
	auto& selector = getComponent<ColourSelector>();
	selector.setColour(ColourSelector::ColourIds::backgroundColourId, Colours::transparentBlack);
	selector.setLookAndFeel(&laf);
    selector.addChangeListener(this);
    Helpers::appendToElementStyle(selector, "height: 130px");
}

ColourChooser::~ColourChooser()
{
	getComponent<ColourSelector>().removeChangeListener(this);
}

void ColourChooser::postInit()
{
	LabelledComponent::postInit();

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

void ColourChooser::changeListenerCallback(ChangeBroadcaster* source)
{
    auto& selector = getComponent<ColourSelector>();
        
	writeState((int64)selector.getCurrentColour().getARGB());
}

Result ColourChooser::checkGlobalState(var globalState)
{
	return Result::ok();
}

Result ColourChooser::loadFromInfoObject(const var& obj)
{
	return Result::ok();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void ColourChooser::createEditor(Dialog::PageInfo& info)
{
    addBasicComponents(*this, info, "A colour chooser that will store the colour as `0xAARRGGBB` value into the data object.");
        
}
#endif

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

        auto top = TopLevelWindowWithOptionalOpenGL::findRoot(parent);

        if(top == nullptr)
            top = parent->getTopLevelComponent();

        top->addChildComponent(this);
        auto topLeft = ed.getTopLevelComponent()->getLocalArea(&ed, ed.getLocalBounds()).getTopLeft();
        
        setTopLeftPosition(topLeft.getX() - 10, topLeft.getY() + ed.getHeight());

#if JUCE_DEBUG
        setVisible(true);
#else
        Desktop::getInstance().getAnimator().fadeIn(this, 150);
#endif
        
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
        
        ed.setText(nt, true);
        
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



void TextInput::timerCallback()
{
    if(callOnEveryChange)   
        callOnValueChange("change");

	if(Component::getCurrentlyFocusedComponent() == &getComponent<TextEditor>())
		showAutocomplete(getComponent<TextEditor>().getText());
        
	stopTimer();
}

void TextInput::textEditorReturnKeyPressed(TextEditor& e)
{
	if(currentAutocomplete != nullptr)
		currentAutocomplete->setAndDismiss();

    

    rootDialog.grabKeyboardFocusAsync();

    callOnValueChange("submit");
}

void TextInput::textEditorTextChanged(TextEditor& e)
{
    //check(MultiPageDialog::getGlobalState(*this, id, var()));

    if(parseInputAsArray)
        writeState(Dialog::parseCommaList(e.getText()));
    else
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
	LabelledComponent(r, width, obj, new TextEditor()),
    navigator(*this)
{
    parseInputAsArray = obj[mpid::ParseArray];

    auto& editor = getComponent<TextEditor>();
	GlobalHiseLookAndFeel::setTextEditorColours(editor);

    if(obj.hasProperty(mpid::EmptyText))
    {
        emptyText = obj[mpid::EmptyText].toString();
	    editor.setTextToShowWhenEmpty(emptyText, editor.findColour(TextEditor::textColourId).withAlpha(0.5f));
    }

    setWantsKeyboardFocus(false);

    editor.addKeyListener(&navigator);
    editor.setSelectAllWhenFocused(false);
    editor.setIgnoreUpDownKeysWhenSingleLine(true);
    editor.setTabKeyUsedAsCharacter(false);

    r.stateWatcher.registerComponentToUpdate(&editor);

    loadFromInfoObject(obj);

    editor.addListener(this);

    auto mlHeight = (int)obj.getProperty(mpid::Height, 80);

    if(editor.isMultiLine())
        Helpers::appendToElementStyle(editor, "height:" + String(mlHeight) + "px;");
    else
        resized();
}

bool TextInput::AutocompleteNavigator::keyPressed(const KeyPress& k, Component* originatingComponent)
{
    if(k == KeyPress::tabKey)
    {
        if(parent.currentAutocomplete != nullptr)
			parent.dismissAutocomplete();

	    parent.getComponent<TextEditor>().moveKeyboardFocusToSibling(true);
        return true;
    }

    if(parent.currentAutocomplete == nullptr)
		return false;
        
	if(k == KeyPress::upKey)
		return parent.currentAutocomplete->inc(false);
	if(k == KeyPress::downKey)
		return parent.currentAutocomplete->inc(true);

    
        
	return false;
}

void TextInput::postInit()
{
    LabelledComponent::postInit();
    
    callOnEveryChange = (bool)infoObject[mpid::CallOnTyping];

    auto& editor = getComponent<TextEditor>();
    
    if(editor.isMultiLine())
    {
        auto h = jmax(80, (int)infoObject[mpid::Height]);
        simple_css::FlexboxComponent::Helpers::writeInlineStyle(*this, "height:"+String(h)+"px;");
        
        String s = Helpers::getInlineStyle(editor);

    	s << "vertical-align:top;";

        if(infoObject[mpid::NoLabel])
            s << "height:"+String(h)+"px;";
        else
            s << "height:100%;";

	    simple_css::FlexboxComponent::Helpers::writeInlineStyle(editor, s);
        
    }
    else
    {
	    editor.setFont(Dialog::getDefaultFont(*this).first);
		//editor.setIndents(8, 8);

        
    }

    auto text = getValueFromGlobalState("");
    
    if(parseInputAsArray && text.isArray())
    {
	    StringArray sa;

        for(const auto& v: *text.getArray())
			sa.add(v.toString());

        text = sa.joinIntoString(", ");
    }
    else
    {
	    text = loadValueOrAssetAsText();
    }

	editor.setText(text);
    
    if(auto m = findParentComponentOfClass<Dialog>())
    {
        auto c = m->getStyleData().headlineColour;
        editor.setColour(TextEditor::ColourIds::focusedOutlineColourId, c);
        editor.setColour(Label::ColourIds::outlineWhenEditingColourId, c);
        editor.setColour(TextEditor::ColourIds::highlightColourId, c);
    }

    if(infoObject[mpid::Autofocus])
    {
        editor.selectAll();
	    editor.grabKeyboardFocusAsync();
    }

    editor.repaint();
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
        writeState(Dialog::parseCommaList(text));
    else
	    writeState(text);
    
	return Result::ok();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void TextInput::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents<TextInput>(*this, rootList, "A TextInput lets you enter a String");

    if(id.isValid())
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

    auto& col = rootList;

	col.addChild<Button>({
		{ mpid::ID, "Required" },
		{ mpid::Text, "Required" },
		{ mpid::Help, "If this is enabled, the text input must be a non-empty String" },
        { mpid::Value, required }
	});

    col.addChild<Button>({
		{ mpid::ID, "ParseArray" },
		{ mpid::Text, "ParseArray" },
		{ mpid::Help, "If this is enabled, the text input will parse comma separated values as Array." },
        { mpid::Value, parseInputAsArray }
	});
    
    col.addChild<Button>({
        { mpid::ID, "Multiline" },
        { mpid::Text, "Multiline" },
        { mpid::Help, "If this is enabled, the text editor will allow multiline editing" },
        { mpid::Value, infoObject[mpid::Multiline] }
    });

    col.addChild<Button>({
        { mpid::ID, mpid::Autofocus.toString() },
        { mpid::Text, mpid::Autofocus.toString() },
        { mpid::Help, "If this is enabled, the text editor will gain the focus when the page is loaded" },
        { mpid::Value, infoObject[mpid::Autofocus] }
    });

    rootList.addChild<TextInput>({
		{ mpid::ID, "Height" },
		{ mpid::Text, "Height" },
        { mpid::Help, "The height (in pixels) for the editor (if used in multiline mode)." },
        { mpid::Value, (int)infoObject.getProperty(mpid::Height, 80) }
	});
    
    rootList.addChild<TextInput>({
		{ mpid::ID, "Items" },
		{ mpid::Text, "Items" },
        { mpid::Multiline, true },
        { mpid::Height, 100},
		{ mpid::Help, "A string with one item per line that will show up in the autocomplete popup." },
        { mpid::Value, autocompleteItems.joinIntoString("\n") }
	});

    addOnValueCodeEditor(infoObject, rootList);

    col.addChild<Button>(DefaultProperties::getForSetting(infoObject, mpid::CallOnTyping,
        "Enables the value change callback for every key input (instea of when pressing return"));
}
#endif

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
        if(obj[mpid::Items].toString() == "{DYNAMIC}")
        {
	        useDynamicAutocomplete = true;
        }
        else
        {
            useDynamicAutocomplete = false;
	        autocompleteItems = getItemsAsStringArray();
        }
	}

    return ok;
}

void TextInput::showAutocomplete(const String& currentText)
{
    if(useDynamicAutocomplete)
    {
        if(auto hd = findParentComponentOfClass<HardcodedDialogWithState>())
            autocompleteItems = hd->getAutocompleteItems(id);
        else
            autocompleteItems = {};
    }
    
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
#if JUCE_DEBUG
    if(currentAutocomplete != nullptr)
		currentAutocomplete->setVisible(false);
#else
    if(currentAutocomplete != nullptr)
		Desktop::getInstance().getAnimator().fadeOut(currentAutocomplete, 150);
#endif
	currentAutocomplete = nullptr;
}


#if HISE_MULTIPAGE_INCLUDE_EDIT
void FileSelector::createEditor(Dialog::PageInfo& rootList)
{
    addBasicComponents(*this, rootList, "Select a file using a native file browser or text input. ");
    
    rootList.addChild<Button>({
		{ mpid::ID, "Directory" },
		{ mpid::Text, "Directory" },
		{ mpid::Help, "Whether the file selector should be able to select directories vs. files." }
	});

    rootList.addChild<TextInput>({
		{ mpid::ID, "Wildcard" },
		{ mpid::Text, "Wildcard" },
		{ mpid::Help, "The file wildcard for filtering selectable files" }
	});

    rootList.addChild<Button>({
		{ mpid::ID, "SaveFile" },
		{ mpid::Text, "SaveFile" },
		{ mpid::Help, "Whether the selected file will be opened or saved." }
	});

    rootList.addChild<Button>({
		{ mpid::ID, "Required" },
		{ mpid::Text, "Required" },
        { mpid::Value, required },
		{ mpid::Help, "Whether a file / directory must be selected" }
	});

    addOnValueCodeEditor(infoObject, rootList);
}
#endif

    
struct BetterFileSelector: public simple_css::FlexboxComponent,
						   public TextEditor::Listener
{
    BetterFileSelector(const String& name, const File& initialFile, bool unused, bool isDirectory, bool save, const String& wildcard):
      FlexboxComponent(simple_css::Selector(".fileselector")),
      browseButton("Browse"),
      currentFile(initialFile)
    {
        setName(name);

        addAndMakeVisible(fileLabel);
        addAndMakeVisible(browseButton);

        setDefaultStyleSheet("display: flex; gap: 10px; height: auto; flex-grow: 1;");
        Helpers::setFallbackStyleSheet(fileLabel, "flex-grow: 1; height: 100%;");
         
        fileLabel.setTextToShowWhenEmpty(isDirectory ? "No folder selected" : "No file selected", Colours::black.withAlpha(0.3f));
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
                {
                    auto dir = fc.getResult();
                    
                    while(dir.getNumberOfChildFiles(File::findFiles) > 0 || dir.containsSubDirectories())
                    {
                        if(NativeMessageBox::showOkCancelBox(MessageBoxIconType::QuestionIcon, "Use existing directory", "The directory you've selected is not empty. Press OK to continue anyway or cancel to choose an empty directory"))
                            break;
                                  
                        if(fc.browseForDirectory())
                        {
                            dir = fc.getResult();
                        }
                        else
                        {
                            return;
                        }
                    }
                    
                    setCurrentFile(dir, sendNotificationAsync);
                }
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

            findParentComponentOfClass<Dialog>()->grabKeyboardFocusAsync();
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

            findParentComponentOfClass<Dialog>()->grabKeyboardFocusAsync();
        };
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
	LabelledComponent(r, width, obj, createFileComponent(obj))
{
    auto& fileSelector = getComponent<BetterFileSelector>();

    r.stateWatcher.registerComponentToUpdate(&fileSelector.fileLabel);

    fileSelector.fileBroadcaster.addListener(*this, [](FileSelector& f, File nf)
    {
        f.writeState(nf.getFullPathName());
        f.callOnValueChange("submit");
    }, false);

	isDirectory = obj[mpid::Directory];
	addAndMakeVisible(fileSelector);
        
	setSize(width, 32);
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
    
    if(f == File() && !fileSelector.fileLabel.isEmpty())
    {
	    f = File(fileSelector.fileLabel.getText());    
    }
    
    if(isDirectory && !f.isDirectory() && f != File())
    {
        f.createDirectory();
    }
    
	if(f != File() && !f.isRoot() && (f.isDirectory() || f.existsAsFile()))
	{
        writeState(f.getFullPathName());
		return Result::ok();
	}

    if(required)
    {
        auto isSave = (bool)infoObject[mpid::SaveFile];
        
        auto allowNewFile = isSave && f != File();
        
        if(allowNewFile)
            return Result::ok();
        
	    String message;
		message << "You need to select a ";
		if(isDirectory)
			message << "directory";
		else
			message << "file";
	        
		return Result::fail(message);
    }

    return Result::ok();
}

File FileSelector::getInitialFile(const var& path) const
{
	if(path.isString())
	{
        auto t = MarkdownText::getString(path.toString(), rootDialog.getState());
		return File(t);
	}
	if(path.isInt() || path.isInt64())
	{
		auto specialLocation = (File::SpecialLocationType)(int)path;
		return File::getSpecialLocation(specialLocation);
	}
        
	return File();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Table::createEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Type"},
		{ mpid::Type, "Table"},
		{ mpid::Help, "A component with fix columns and a dynamic amount of rows" }
	});
            
	rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Value, infoObject[mpid::ID].toString() },
		{ mpid::Help, "The ID for the element (used as key in the state `var`." }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Columns.toString() },
		{ mpid::Text, mpid::Columns.toString() },
		{ mpid::Help, "The list of columns - one per line using a pseudo CSS syntax for `name`, `max-width`, `min-width` and `width` properties." },
		{ mpid::Multiline, true },
		{ mpid::Height, 80 },
		{ mpid::Value, itemsToString(infoObject[mpid::Columns]) }
	});
        
	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Items.toString() },
		{ mpid::Text, mpid::Items.toString() },
		{ mpid::Help, "The list of rows - one per line using `|` for separating cells" },
		{ mpid::Multiline, true },
		{ mpid::Height, 80 },
		{ mpid::Value, infoObject[mpid::Items] }
	});

	rootList.addChild<factory::Choice>({
		{ mpid::Text, mpid::ValueMode.toString() },
		{ mpid::ID,  mpid::ValueMode.toString() },
		{ mpid::Help, "How the table stores the selected item (either the zero based index of the selected row or the `[column, row]` position of the clicked cell" },
		{ mpid::Items, "Row\nGrid\nFirstColumnText" },
		{ mpid::Value, infoObject[mpid::ValueMode] },
		
	});

    rootList.addChild<factory::Button>(DefaultProperties::getForSetting(infoObject, mpid::Multiline,
        "Allows selection of multiple rows / cells"));

    rootList.addChild<TextInput>({
		{ mpid::ID, mpid::EmptyText.toString() },
		{ mpid::Text, mpid::EmptyText.toString() },
		{ mpid::Value, infoObject[mpid::EmptyText] },
        { mpid::Help, "The text that will be displayed if no table content is present." } 
	});

    rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Code.toString() },
		{ mpid::Text, mpid::Code.toString() },
        { mpid::Value, infoObject[mpid::Code] },
        { mpid::Items, "{BIND::functionName}" },
		{ mpid::Help, "The callback that is executed using the syntax `{BIND::myMethod}`" }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, "FilterFunction" },
		{ mpid::Text, "FilterFunction" },
		{ mpid::Value, infoObject[mpid::FilterFunction] },
        { mpid::Items, "{BIND::functionName}" },
        { mpid::Help, "A function that will be called to filter the items" } 
	});

    rootList.addChild<Button>({
		{ mpid::ID, mpid::SelectOnClick.toString() },
		{ mpid::Text, mpid::SelectOnClick.toString() },
		{ mpid::Value, infoObject[mpid::SelectOnClick] },
        { mpid::Help, "Whether to fire the value callback on a single mouse click or double click / return key only." } 
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
		{ mpid::Help, "The CSS class that is applied to the UI element." },
		{ mpid::Value, infoObject[mpid::Class] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
		{ mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});

	addOnValueCodeEditor(infoObject, rootList);
}

void TagList::createEditor(Dialog::PageInfo& rootList)
{
	rootList.addChild<Type>({
		{ mpid::ID, "Type"},
		{ mpid::Type, "TagList"},
		{ mpid::Help, "A dynamic list of clickable tags." }
	});
            
	rootList.addChild<TextInput>({
		{ mpid::ID, "ID" },
		{ mpid::Text, "ID" },
		{ mpid::Value, infoObject[mpid::ID].toString() },
		{ mpid::Help, "The ID for the element (used as key in the state `var`." }
	});

    rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Code.toString() },
		{ mpid::Text, mpid::Code.toString() },
        { mpid::Value, infoObject[mpid::Code] },
        { mpid::Items, "{BIND::functionName}" },
		{ mpid::Help, "The callback that is executed using the syntax `{BIND::myMethod}`" }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Items.toString() },
		{ mpid::Text, mpid::Items.toString() },
		{ mpid::Help, "The list of tags - one per line" },
		{ mpid::Multiline, true },
		{ mpid::Height, 80 },
		{ mpid::Value, infoObject[mpid::Items] }
	});
        
	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Class.toString() },
		{ mpid::Text, mpid::Class.toString() },
		{ mpid::Help, "The CSS class that is applied to the UI element." },
		{ mpid::Value, infoObject[mpid::Class] }
	});

	rootList.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
		{ mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});

    addOnValueCodeEditor(infoObject, rootList);
}
#endif

} // factory
} // multipage
} // hise
