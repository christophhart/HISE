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

namespace hise { using namespace juce;

#define GET_OBJECT(x) (dynamic_cast<ScriptingApi::x*> (args.thisObject.getObject()))

#if ENABLE_SCRIPTING_SAFE_CHECKS

#define CHECK_VALID_ARGUMENTS() if(thisObject->checkValidArguments(args) != -1) return var();
#define CHECK_ARGUMENTS(callName, expectedArguments) if (!thisObject->checkArguments(callName, args.numArguments, expectedArguments)) return var(); CHECK_VALID_ARGUMENTS();
#define CHECK_IF_DEFERRED(x) if(!thisObject->checkIfSynchronous(x))return var();

#else

#define CHECK_VALID_ARGUMENTS() 
#define CHECK_ARGUMENTS(callName, expectedArguments)
#define CHECK_IF_DEFERRED(x)

#endif

var DynamicScriptingObject::Wrappers::checkExists(const var::NativeFunctionArgs& args)
{
	if(DynamicScriptingObject* thisObject = dynamic_cast<DynamicScriptingObject*>(args.thisObject.getObject()))
	{
		return thisObject->checkValidObject();
	}
	return var();
}

struct ScriptingApi::Content::Wrapper
{
	static var addButton(const var::NativeFunctionArgs& args);
	static var addKnob(const var::NativeFunctionArgs& args);
	static var addLabel(const var::NativeFunctionArgs& args);
	static var addComboBox(const var::NativeFunctionArgs& args);
	static var addTable(const var::NativeFunctionArgs& args);
	static var addImage(const var::NativeFunctionArgs& args);
	static var addViewport(const var::NativeFunctionArgs& args);
	static var addPanel(const var::NativeFunctionArgs& args);
	static var addAudioWaveform(const var::NativeFunctionArgs& args);
	static var addSliderPack(const var::NativeFunctionArgs& args);
	static var addWebView(const var::NativeFunctionArgs& args);
	static var addFloatingTile(const var::NativeFunctionArgs& args);
	static var getComponent(const var::NativeFunctionArgs& args);
	static var getAllComponents(const var::NativeFunctionArgs& args);
	static var set(const var::NativeFunctionArgs& args);
	static var get(const var::NativeFunctionArgs& args);
	static var addToMacroControl(const var::NativeFunctionArgs& args);
	static var setRange(const var::NativeFunctionArgs& args);
	static var setMode(const var::NativeFunctionArgs& args);
	static var setStyle(const var::NativeFunctionArgs& args);
	static var setPropertiesFromJSON(const var::NativeFunctionArgs& args);
	static var setValuePopupData(const var::NativeFunctionArgs& args);
	static var storeAllControlsAsPreset(const var::NativeFunctionArgs& args);
	static var restoreAllControlsFromPreset(const var::NativeFunctionArgs& args);
	static var setMidPoint(const var::NativeFunctionArgs& args);
	static var setValue(const var::NativeFunctionArgs& args);
	static var setPosition(const var::NativeFunctionArgs& args);
	static var setHeight(const var::NativeFunctionArgs& args);
	static var setWidth(const var::NativeFunctionArgs& args);
	static var setName(const var::NativeFunctionArgs& args);
    static var makeFrontInterface(const var::NativeFunctionArgs& args);
	static var makeFullScreenInterface(const var::NativeFunctionArgs& args);
	static var addItem(const var::NativeFunctionArgs& args);
	static var setColour(const var::NativeFunctionArgs& args);
	static var setTooltip(const var::NativeFunctionArgs& args);
	static var setContentTooltip(const var::NativeFunctionArgs& args);
	static var setToolbarProperties(const var::NativeFunctionArgs& args);
	static var setUseHighResolutionForPanels(const var::NativeFunctionArgs& args);
	static var isCtrlDown(const var::NativeFunctionArgs& args);

	static var getCurrentTooltip(const var::NativeFunctionArgs& args);

	static var createScreenshot(const var::NativeFunctionArgs& args);

	static var createLocalLookAndFeel(const var::NativeFunctionArgs& args);

	static var addVisualGuide(const var::NativeFunctionArgs& args);

	static var getScreenBounds(const var::NativeFunctionArgs& args);

	static var setImageFile(const var::NativeFunctionArgs& args);
	static var setImageAlpha(const var::NativeFunctionArgs& args);
	static var showControl(const var::NativeFunctionArgs& args);
	static var getValue(const var::NativeFunctionArgs& args);
	static var getItemText(const var::NativeFunctionArgs& args);
	static var getTableValue(const var::NativeFunctionArgs& args);
	static var setEditable(const var::NativeFunctionArgs& args);
	static var clear(const var::NativeFunctionArgs& args);
	static var setValueNormalized(const var::NativeFunctionArgs& args);;
	static var getValueNormalized(const var::NativeFunctionArgs& args);;
	static var setSliderAtIndex(const var::NativeFunctionArgs& args);
	static var getSliderValueAt(const var::NativeFunctionArgs& args);
	static var setAllValues(const var::NativeFunctionArgs& args);
	static var getNumSliders(const var::NativeFunctionArgs& args);
	static var setMinValue(const var::NativeFunctionArgs& args);
	static var setMaxValue(const var::NativeFunctionArgs& args);
	static var getMinValue(const var::NativeFunctionArgs& args);
	static var getMaxValue(const var::NativeFunctionArgs& args);
	static var contains(const var::NativeFunctionArgs& args);
	static var createPath(const var::NativeFunctionArgs& args);
	static var createShader(const var::NativeFunctionArgs& args);
	static var createMarkdownRenderer(const var::NativeFunctionArgs& args);
    static var createSVG(const var::NativeFunctionArgs& args);
	static var isMouseDown(const var::NativeFunctionArgs& args);
	static var getComponentUnderMouse(const var::NativeFunctionArgs& args);
	static var callAfterDelay(const var::NativeFunctionArgs& args);
	static var refreshDragImage(const var::NativeFunctionArgs& args);
	static var getComponentUnderDrag(const var::NativeFunctionArgs& args);

};

var ScriptingApi::Content::Wrapper::addButton (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addButton(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addButton()", 3);

		return thisObject->addButton(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::addKnob (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addKnob(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addKnob()", 3);

		return thisObject->addKnob(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::addLabel (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addLabel(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addLabel()", 3);

		return thisObject->addLabel(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::addComboBox (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addComboBox(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addComboBox()", 3);

		return thisObject->addComboBox(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::addTable (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addTable(Identifier(args.arguments[0]), 0,0);
		}

		CHECK_ARGUMENTS("addTable()", 3);

		return thisObject->addTable(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::addImage (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addImage(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addImage()", 3);

		return thisObject->addImage(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};


var ScriptingApi::Content::Wrapper::addViewport(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if (args.numArguments == 1)
		{
			return thisObject->addViewport(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addViewport()", 3);

		return thisObject->addViewport(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
}



var ScriptingApi::Content::Wrapper::addPanel (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if(args.numArguments == 1)
		{
			return thisObject->addPanel(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addPanel()", 3);

		return thisObject->addPanel(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
};


var ScriptingApi::Content::Wrapper::addAudioWaveform(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if (args.numArguments == 1)
		{
			return thisObject->addAudioWaveform(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addAudioWaveform()", 3);

		return thisObject->addAudioWaveform(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::addSliderPack(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		if (args.numArguments == 1)
		{
			return thisObject->addSliderPack(Identifier(args.arguments[0]), 0, 0);
		}

		CHECK_ARGUMENTS("addSliderPack()", 3);

		return thisObject->addSliderPack(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::addWebView(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("addWebView()", 3);
		return thisObject->addWebView(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::addFloatingTile(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("addFloatingTile()", 3);
		return thisObject->addFloatingTile(Identifier(args.arguments[0]), args.arguments[1], args.arguments[2]);
	}

	return var();
}


var ScriptingApi::Content::Wrapper::getComponent(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("getComponent()", 1);

		if (args.numArguments == 1)
		{
			return thisObject->getComponent(args.arguments[0]);
		}

		return var();
	}

	return var();
}

var ScriptingApi::Content::Wrapper::getAllComponents(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("getAllComponents()", 1);

		if (args.numArguments == 1)
		{
			return thisObject->getAllComponents(args.arguments[0]);
		}

		return var();
	}

	return var();
}

var ScriptingApi::Content::Wrapper::storeAllControlsAsPreset(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("storeAllControlsAsPreset()", 1);

		thisObject->storeAllControlsAsPreset(args.arguments[0].toString(), ValueTree());
	}

	return var();
}

var ScriptingApi::Content::Wrapper::restoreAllControlsFromPreset(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("restoreAllControlsFromPreset()", 1);

		thisObject->restoreAllControlsFromPreset(args.arguments[0].toString());
	}

	return var();
}


var ScriptingApi::Content::Wrapper::set (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("set()", 2);

		thisObject->set(args.arguments[0], args.arguments[1]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::get (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("get()", 1);

		return thisObject->get(args.arguments[0]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::addToMacroControl (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("addToMacroControl()", 1);

		thisObject->addToMacroControl(args.arguments[0]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setHeight (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setHeight()", 1);

		thisObject->setHeight((int)args.arguments[0]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setContentTooltip (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setContentTooltip()", 1);

		thisObject->setContentTooltip(args.arguments[0].toString());
	}

	return var();
};



var ScriptingApi::Content::Wrapper::setUseHighResolutionForPanels(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setUseHighResolutionForPanels()", 1);

		thisObject->setUseHighResolutionForPanels(args.arguments[0]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::isCtrlDown(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("isCtrlDown()", 0);

		return thisObject->isCtrlDown();
	}

	return var();
}

var ScriptingApi::Content::Wrapper::setToolbarProperties(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setToolbarProperties()", 1);

		thisObject->setToolbarProperties(args.arguments[0]);
	}

	return var();
}


var ScriptingApi::Content::Wrapper::setWidth (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setWidth()", 1);

		thisObject->setWidth((int)args.arguments[0]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setName (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setName()", 1);

		thisObject->setName(args.arguments[0].toString());
	}

	return var();
};

var ScriptingApi::Content::Wrapper::makeFrontInterface (const var::NativeFunctionArgs& args)
{
    if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
    {
        CHECK_ARGUMENTS("setName()", 2);
        
        thisObject->makeFrontInterface((int)args.arguments[0], (int)args.arguments[1]);
    }
    
    return var();
};

var ScriptingApi::Content::Wrapper::makeFullScreenInterface(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("makeFullScreenInterface()", 0);

		thisObject->makeFullScreenInterface();
	}

	return var();
};



var ScriptingApi::Content::Wrapper::setPropertiesFromJSON (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setPropertiesFromJSON()", 1);

		thisObject->setPropertiesFromJSON(Identifier(args.arguments[0]), args.arguments[1]);
	}

	return var();
};


var ScriptingApi::Content::Wrapper::setValuePopupData(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("setValuePopupData()", 1);

		thisObject->setValuePopupData(args.arguments[0]);
	}

	return var();
}


var ScriptingApi::Content::Wrapper::setColour (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setColour()", 2);

		thisObject->setColour((int)args.arguments[0], (int)args.arguments[1]);
	}

	return var();
};



// =================================================================================================== Content Component Wrappers



var ScriptingApi::Content::Wrapper::addItem (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComboBox* thisObject = GET_OBJECT(Content::ScriptComboBox))
	{
		CHECK_ARGUMENTS("addItem()", 1);

		thisObject->addItem(args.arguments[0].toString());
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setTooltip (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setTooltip()", 1);

		thisObject->setTooltip(args.arguments[0].toString());
	}

	return var();
};



var ScriptingApi::Content::Wrapper::setImageFile (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptImage* thisObject = GET_OBJECT(Content::ScriptImage))
	{
		CHECK_ARGUMENTS("setImageFile()", 2);

		thisObject->setImageFile(args.arguments[0].toString(), (bool)args.arguments[1]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setImageAlpha (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptImage* thisObject = GET_OBJECT(Content::ScriptImage))
	{
		CHECK_ARGUMENTS("setAlpha()", 1);

		thisObject->setAlpha((float)args.arguments[0]);
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setRange (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setRange()", 3);

		thisObject->setRange((double)args.arguments[0], (double)args.arguments[1], (double)args.arguments[2] );
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setMode (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMode()", 1);

		thisObject->setMode(args.arguments[0].toString());
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setStyle (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setStyle()", 1);

		thisObject->setStyle(args.arguments[0].toString());
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setMidPoint (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMidPoint()", 1);

		thisObject->setMidPoint((double)args.arguments[0]);
	}

	return var();
};


var ScriptingApi::Content::Wrapper::setEditable (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptLabel* thisObject = GET_OBJECT(Content::ScriptLabel))
	{
		CHECK_ARGUMENTS("setEditable()", 1);

		thisObject->setEditable((bool)args.arguments[0]);
	}

	return var();
};



var ScriptingApi::Content::Wrapper::setValue (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setValue()", 1);

		thisObject->setValue(args.arguments[0]);
	}

	return var();
};



var ScriptingApi::Content::Wrapper::setValueNormalized(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setValueNormalized()", 1);

		thisObject->setValueNormalized((double)args.arguments[0]);
	}

	return var();
}



var ScriptingApi::Content::Wrapper::getValueNormalized(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("getValueNormalized()", 0);

		return thisObject->getValueNormalized();
	}

	return var();
}


var ScriptingApi::Content::Wrapper::setPosition (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("setPosition()", 4);

		thisObject->setPosition((int)args.arguments[0], (int)args.arguments[1], (int)args.arguments[2], (int)args.arguments[3]);
	}

	return var();
};


var ScriptingApi::Content::Wrapper::getItemText (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComboBox* thisObject = GET_OBJECT(Content::ScriptComboBox))
	{
		CHECK_ARGUMENTS("setItemText()", 0);

		return thisObject->getItemText();
	}

	return var();
};



var ScriptingApi::Content::Wrapper::showControl (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("showControl()", 1);

		thisObject->showControl((bool)args.arguments[0]);
	}

	return var();
};



var ScriptingApi::Content::Wrapper::getValue (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptComponent* thisObject = GET_OBJECT(Content::ScriptComponent))
	{
		CHECK_ARGUMENTS("getValue()", 0);

		return thisObject->getValue();
	}

	return var();
};

var ScriptingApi::Content::Wrapper::getTableValue (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptTable* thisObject = GET_OBJECT(Content::ScriptTable))
	{
		CHECK_ARGUMENTS("getTableValue()", 1);

		return thisObject->getTableValue((int)args.arguments[0]);
	}

	return var();
};;


var ScriptingApi::Content::Wrapper::clear (const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = dynamic_cast<ScriptingApi::Content*> (args.thisObject.getObject()))
	{
		thisObject->clear();
	}

	return var();
};

var ScriptingApi::Content::Wrapper::setSliderAtIndex(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("setSliderAtIndex()", 2);

		thisObject->setSliderAtIndex((int)args.arguments[0], (double)args.arguments[1]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::getSliderValueAt(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("getSliderValueAt()", 1);

		return thisObject->getSliderValueAt((int)args.arguments[0]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::setAllValues(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("setAllValues()", 1);

		thisObject->setAllValues((double)args.arguments[0]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::getNumSliders(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSliderPack* thisObject = GET_OBJECT(Content::ScriptSliderPack))
	{
		CHECK_ARGUMENTS("getNumSliders()", 0);

		return thisObject->getNumSliders();
	}

	return var();
}


var ScriptingApi::Content::Wrapper::setMinValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMinValue()", 1);

		thisObject->setMinValue((double)args.arguments[0]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::setMaxValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("setMaxValue()", 1);

		thisObject->setMaxValue((double)args.arguments[0]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::getMinValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("getMinValue()", 0);

		return thisObject->getMinValue();
	}

	return var();
}

var ScriptingApi::Content::Wrapper::getMaxValue(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("getMaxValue()", 0);

		return thisObject->getMaxValue();
	}

	return var();
}

var ScriptingApi::Content::Wrapper::contains(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content::ScriptSlider* thisObject = GET_OBJECT(Content::ScriptSlider))
	{
		CHECK_ARGUMENTS("contains()", 1);

		return thisObject->contains((double)args.arguments[0]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::createPath(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("createPath()", 0);

		return thisObject->createPath();
	}

	return var();
}

var ScriptingApi::Content::Wrapper::createShader(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("createShader()", 1);

		return thisObject->createShader(args.arguments[0].toString());
	}

	return var();
}


juce::var ScriptingApi::Content::Wrapper::createMarkdownRenderer(const var::NativeFunctionArgs& args)
{
	if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("createMarkdownRenderer()", 0);

		return thisObject->createMarkdownRenderer();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::createSVG(const var::NativeFunctionArgs& args)
{
    if (ScriptingApi::Content* thisObject = GET_OBJECT(Content))
    {
        CHECK_ARGUMENTS("createSVG()", 1);

        return thisObject->createSVG(args.arguments[0]);
    }

    return var();
}

var ScriptingApi::Content::Wrapper::createScreenshot(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("createScreenshot()", 3);
		thisObject->createScreenshot(args.arguments[0], args.arguments[1], args.arguments[2]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::addVisualGuide(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("addVisualGuide()", 2);
		thisObject->addVisualGuide(args.arguments[0], args.arguments[1]);
	}

	return var();
}

var ScriptingApi::Content::Wrapper::getCurrentTooltip(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("getCurrentTooltip()", 0);
		return thisObject->getCurrentTooltip();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::createLocalLookAndFeel(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("createLocalLookAndFeel()", 0);
		return thisObject->createLocalLookAndFeel();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::getScreenBounds(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		CHECK_ARGUMENTS("getScreenBounds()", 1);
		return thisObject->getScreenBounds(args.arguments[0]);
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::getComponentUnderMouse(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		return thisObject->getComponentUnderMouse();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::isMouseDown(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		return thisObject->isMouseDown();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::callAfterDelay(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		thisObject->callAfterDelay(args.arguments[0], args.arguments[1], args.numArguments == 3 ? args.arguments[2] : var());
		return thisObject->isMouseDown();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::refreshDragImage(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		return thisObject->refreshDragImage();
	}

	return var();
}

juce::var ScriptingApi::Content::Wrapper::getComponentUnderDrag(const var::NativeFunctionArgs& args)
{
	if (auto thisObject = GET_OBJECT(Content))
	{
		return thisObject->getComponentUnderDrag();
	}

	return var();
}


#undef GET_OBJECT
#undef CHECK_ARGUMENTS
#undef CHECK_IF_DEFERRED

} // namespace hise
