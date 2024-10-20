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

namespace scriptnode
{
using namespace hise;
using namespace juce;

void drawPlug(Graphics& g, Rectangle<float> area, Colour c)
{
    static const unsigned char pathData[] = { 110,109,233,38,145,63,119,190,39,64,108,0,0,0,0,227,165,251,63,108,0,0,0,0,20,174,39,63,108,174,71,145,63,0,0,0,0,108,174,71,17,64,20,174,39,63,108,174,71,17,64,227,165,251,63,108,115,104,145,63,119,190,39,64,108,115,104,145,63,143,194,245,63,98,55,137,
145,63,143,194,245,63,193,202,145,63,143,194,245,63,133,235,145,63,143,194,245,63,98,164,112,189,63,143,194,245,63,96,229,224,63,152,110,210,63,96,229,224,63,180,200,166,63,98,96,229,224,63,43,135,118,63,164,112,189,63,178,157,47,63,133,235,145,63,178,
157,47,63,98,68,139,76,63,178,157,47,63,84,227,5,63,43,135,118,63,84,227,5,63,180,200,166,63,98,84,227,5,63,14,45,210,63,168,198,75,63,66,96,245,63,233,38,145,63,143,194,245,63,108,233,38,145,63,119,190,39,64,99,101,0,0 };

    Path plug;

    plug.loadPathFromData(pathData, sizeof(pathData));
    
    PathFactory::scalePath(plug, area.reduced(1.5f));
    
    
    
    if(c.isTransparent())
        c = Colours::white;
    
    g.setColour(Colours::black.withAlpha(0.5f));
    g.strokePath(plug, PathStrokeType(1.0f));
    
    g.setColour(c.withAlpha(0.5f));
    g.fillPath(plug);
    
}

DspNodeList::ParameterItem::ParameterItem(DspNetwork* parent, int parameterIndex):
	Item(parent->getCurrentParameterHandler()->getParameterId(parameterIndex).toString()),
	SimpleTimer(parent->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	ptree(parent->getRootNode()->getParameterTree().getChild(parameterIndex)),
	dragButton("drag", nullptr, *this),
	network(parent),
	pIndex(parameterIndex),
	dragListener(&dragButton, [parameterIndex](DspNetworkGraph* g){ return DspNetworkListeners::MacroParameterDragListener::findSliderComponent(g, parameterIndex); })
{
	pname.getTextValue().referTo(ptree.getPropertyAsValue(PropertyIds::ID, parent->getUndoManager()));

	auto value = (double)ptree[PropertyIds::Value];
	valueSlider.setValue(value, dontSendNotification);

	valueSlider.getValueObject().referTo(ptree.getPropertyAsValue(PropertyIds::Value, parent->getUndoManager()));

	addAndMakeVisible(valueSlider);
	addAndMakeVisible(dragButton);
	addAndMakeVisible(pname);

			

	valueSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
	valueSlider.setTextBoxStyle(juce::Slider::TextEntryBoxPosition::NoTextBox, false, 0, 0);

	valueSlider.setLookAndFeel(&laf);
			

	pname.setFont(GLOBAL_BOLD_FONT());
	pname.setEditable(false, true);
	pname.setColour(Label::textColourId, Colours::white.withAlpha(0.8f));
	pname.setColour(TextEditor::ColourIds::textColourId, Colours::white.withAlpha(0.8f));

	rangeUpdater.setCallback(ptree, RangeHelpers::getRangeIds(false), valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(ParameterItem::updateRange));

	start();
}

void DspNodeList::ParameterItem::timerCallback()
{
	if(auto p = network->getRootNode()->getParameterFromIndex(pIndex))
	{
		if(p->getValue() != valueSlider.getValue())
			valueSlider.setValue(p->getValue(), dontSendNotification);
	}
}

DspNodeList::NodeItem::NodeItem(DspNetwork* parent, const String& id):
	Item(id),
	node(dynamic_cast<NodeBase*>(parent->get(id).getObject())),
	label(),
	powerButton("on", this, f),
	dragButton("drag", nullptr, f) 
{
    auto vt = node->getValueTree();
    
	label.setText(vt[PropertyIds::Name].toString(), dontSendNotification);
	usePopupMenu = false;

	if(node->getValueTree().getChildWithName(PropertyIds::ModulationTargets).isValid())
	{
		auto ntree = node->getValueTree();

		dragListener = new DspNetworkListeners::MacroParameterDragListener(&dragButton, [ntree](DspNetworkGraph* g)
		{
			return DspNetworkListeners::MacroParameterDragListener::findModulationDragComponent(g, ntree);
		});

		addAndMakeVisible(dragButton);
	}

	addAndMakeVisible(powerButton);
	addAndMakeVisible(label);
	label.addListener(this);

	label.setFont(GLOBAL_BOLD_FONT());
	label.setColour(Label::ColourIds::textColourId, Colours::white);
	label.setInterceptsMouseClicks(false, true);
	label.refreshWithEachKey = false;
	label.addMouseListener(this, true);

            
	label.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
	label.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
	label.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
	label.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
	label.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
            
	powerButton.setToggleModeWithColourChange(true);

	idListener.setCallback(node->getValueTree(), { PropertyIds::Name }, valuetree::AsyncMode::Asynchronously,
	                       BIND_MEMBER_FUNCTION_2(NodeItem::updateId));

	bypassListener.setCallback(node->getValueTree(), { PropertyIds::Bypassed }, valuetree::AsyncMode::Asynchronously,
	                           BIND_MEMBER_FUNCTION_2(NodeItem::updateBypassState));

	auto fid = node->getValueTree()[PropertyIds::FactoryPath].toString();

	searchKeywords << ";" << fid;


	if(fid.startsWith("container.") && fid != "container.chain")
	{
		scriptnode::NodeComponentFactory f;
		icon = f.createPath(fid.fromFirstOccurrenceOf("container.", false, false));
	}

}

void DspNodeList::NodeItem::updateBypassState(Identifier id, var newValue)
{
	powerButton.setToggleStateAndUpdateIcon(!newValue);
	label.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(!newValue ? 0.8f : 0.3f));
	repaint();
}

int DspNodeList::NodeItem::getIntendation() const
{
	auto networkData = node->getRootNetwork()->getValueTree();
            
	auto nTree = node->getValueTree();
            
	int index = 0;
            
	while(nTree.isValid() && nTree != networkData)
	{
		index++;
		nTree = nTree.getParent();
	}
            
	return index;
}

void DspNodeList::NodeItem::paint(Graphics& g)
{
    if (node != nullptr)
    {
		bool selected = node->getRootNetwork()->isSelected(node);
		
        auto ca = area.withWidth(4);
        
        
        auto colour = node->getColour();
        paintItemBackground(g, area.toFloat());
        
        Colour pColour;
        
        auto parent = node->getParentNode();
        
        float parentX = (float)area.getX() - 3.0f;
        
        float alpha = 1.0f;
        
        while(parent != nullptr)
        {
            pColour = parent->getColour();
            
            if(pColour != Colours::transparentBlack)
            {
                if(!parent->isBypassed())
                {
                    g.setColour(pColour.withAlpha(alpha));
                    
                    Rectangle<float> line(parentX, 0.0f, 1.0f, (float)getHeight());
                    g.fillRect(line);
                    
                    g.setColour(pColour.withAlpha(0.06f));
                    g.fillRoundedRectangle(area.toFloat(), 2.0f);
                }
                
                alpha -= 0.2f;
                
                if(alpha == 0.0f)
                    break;
            }
            
            parent = parent->getParentNode();
            parentX -= 4.0f;
        }
        
        auto pTree = node->getValueTree().getChildWithName(PropertyIds::Parameters);
        
        for(auto p: pTree)
        {
            if(p[PropertyIds::Automated] && !dragButton.isVisible())
            {
                auto copy = area.toFloat();
                copy = copy.removeFromRight(copy.getHeight()).reduced(3.0f);
                drawPlug(g, copy, colour);
                break;
            }
        }
        
        
        g.setColour(colour);
        g.fillRoundedRectangle(ca.toFloat(), 2.0f);
        
        if(!icon.isEmpty())
        {
            g.setColour(label.findColour(Label::ColourIds::textColourId));
            g.fillPath(icon);
        }
        
        if(selected)
        {
            g.setColour(Colour(SIGNAL_COLOUR));
            g.drawRoundedRectangle(area.toFloat(), 2.0f, 1.0f);
        }
        
        
    }
}

void DspNodeList::NodeItem::resized()
{
	auto b = getLocalBounds().reduced(1);
	b.removeFromLeft(getIntendation()*2);
	area = b;
	b.removeFromLeft(5);

	if(dragButton.isVisible())
		dragButton.setBounds(b.removeFromRight(b.getHeight()).reduced(1));
            
	powerButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(2));
            
	if(!icon.isEmpty())
	{
                
                
		PathFactory::scalePath(icon, b.removeFromLeft(b.getHeight() - 4).reduced(2).toFloat());
	}
            
	label.setBounds(b);
}

void DspNodeList::NodeCollection::paint(Graphics& g)
{
	auto b = getLocalBounds();
	auto top = b.removeFromTop(30);
	g.setColour(Colours::white.withAlpha(0.8f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(getName(), top.toFloat(), Justification::centred);
}

void DspNodeList::NodeCollection::addItems(const StringArray& idList, bool isCable)
{
	for (const auto& id : idList)
	{
		SearchableListComponent::Item* newItem;
		if(isCable)
		{
			newItem = new CableItem(network.get(), id);
		}
		else
		{
			newItem = new NodeItem(network.get(), id);
		}

		addAndMakeVisible(newItem);
		items.add(newItem);
	}
}
}

namespace hise { using namespace juce;



ApiCollection::ApiCollection(BackendRootWindow* window) :
SearchableListComponent(window),
apiTree(ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize)))
{
	setOpaque(true);
	setName("API Browser");

	setFuzzyness(0.6);
}



ApiCollection::MethodItem::MethodItem(const ValueTree &methodTree_, const String &className_) :
Item(String(className_ + "." + methodTree_.getProperty(Identifier("name")).toString()).toLowerCase()),
methodTree(methodTree_),
name(methodTree_.getProperty(Identifier("name")).toString()),
description(methodTree_.getProperty(Identifier("description")).toString()),
arguments(methodTree_.getProperty(Identifier("arguments")).toString()),
className(className_)
{
    searchKeywords = searchKeywords.replaceCharacter('.', ';');
    
	setSize(410 - 16 - 8 - 24, ITEM_HEIGHT);

	auto extendedHelp = ExtendedApiDocumentation::getMarkdownText(className, name);

	if(extendedHelp.isEmpty())
	{
		AttributedString help;

		const String name = methodTree_.getProperty(Identifier("name")).toString();
		const String arguments = methodTree_.getProperty(Identifier("arguments")).toString();
		const String description = methodTree_.getProperty(Identifier("description")).toString().trim();
		const String returnType = methodTree_.getProperty("returnType", "void");

		extendedHelp << "#### `" << className_ << "." << name << arguments << "`  \n";

		if(!returnType.isEmpty())
			extendedHelp << "**Return Type**: `" << returnType << "`  \n";

		extendedHelp << "> " << description << "  \n";
		extendedHelp << "**[F1]** - open in docs **[Enter]** - paste in editor";
	}

	if (extendedHelp.isNotEmpty())
	{
		parser = new MarkdownRenderer(extendedHelp);

		auto bd = MarkdownLayout::StyleData::createBrightStyle();
		bd.fontSize = 15.5f;
		parser->setStyleData(bd);
	}

	setWantsKeyboardFocus(true);
}




void ApiCollection::MethodItem::mouseDoubleClick(const MouseEvent& e)
{
	
}

bool ApiCollection::MethodItem::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::returnKey))
	{
		insertIntoCodeEditor();
		return true;
	}

	if (key.isKeyCode(KeyPress::escapeKey))
	{
		findParentComponentOfClass<ApiCollection>()->onPopupClose(FocusChangeType::focusChangedByMouseClick);
		return true;
	}

	if(key.isKeyCode(KeyPress::upKey))
	{
		findParentComponentOfClass<SearchableListComponent>()->selectNext(false, this);
		return true;
	}
	if(key.isKeyCode(KeyPress::downKey))
	{
		findParentComponentOfClass<SearchableListComponent>()->selectNext(true, this);
		
		return true;
	}

	return false;
}

void ApiCollection::MethodItem::insertIntoCodeEditor()
{
	ApiCollection *parent = findParentComponentOfClass<ApiCollection>();

	parent->getRootWindow()->getMainSynthChain()->getMainController()->insertStringAtLastActiveEditor(className + "." + name + arguments, arguments != "()");
}

void ApiCollection::MethodItem::paint(Graphics& g)
{
	if (getWidth() <= 8)
		return;

	Colour c(0xFF000000);

	const float h = (float)getHeight();
	const float w = (float)getWidth() - 4;

	ColourGradient grad(c.withAlpha(0.1f), 0.0f, .0f, c.withAlpha(0.2f), 0.0f, h, false);

	g.setGradientFill(grad);

	g.fillRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f);

	g.setColour(hasKeyboardFocus(true) ? Colours::white : c.withAlpha(0.5f));

	g.drawRoundedRectangle(2.0f, 2.0f, w - 4.0f, h - 4.0f, 3.0f, 2.0f);

	auto wd = getWidth() - 20;

	if (wd > 40)
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_MONOSPACE_FONT());
		g.drawText(name, 10, 0, wd, ITEM_HEIGHT, Justification::centredLeft, true);
	}
}


Array<ExtendedApiDocumentation::ClassDocumentation> ExtendedApiDocumentation::classes;

bool ExtendedApiDocumentation::inititalised = false;

ApiCollection::ClassCollection::ClassCollection(int index, const ValueTree &api) :
Collection(index),
classApi(api),
name(api.getType().toString())
{
	setWantsKeyboardFocus(true);

	for (int i = 0; i < api.getNumChildren(); i++)
	{
		items.add(new MethodItem(api.getChild(i), name));

		

		addAndMakeVisible(items.getLast());
	}
}

void ApiCollection::ClassCollection::paint(Graphics &g)
{
	g.setColour(Colours::white.withAlpha(0.9f));

	auto f = GLOBAL_MONOSPACE_FONT();
	

	g.setFont(GLOBAL_MONOSPACE_FONT().withHeight(f.getHeight() * 1.2f));
	g.drawText(name, 14, 0, getWidth() - 10, COLLECTION_HEIGHT, Justification::centredLeft, true);
}

ExtendedApiDocumentation::MethodDocumentation::MethodDocumentation(Identifier& className_, const Identifier& id) :
	DocumentationBase(id),
	className(className_.toString())
{
}

juce::String ExtendedApiDocumentation::MethodDocumentation::createMarkdownText() const
{
	String s;

	s << "## " << className << "." << id.toString() << "\n";

	s << "> `";
	s << returnType.type << " " << className << "." << id << "(";

	for (const auto& p : parameters)
		s << p.type << " " << p.id << (p == parameters.getLast() ? "" : ", ");

	s << ")`\n";
	
	s << description << "\n";


	if (codeExample.isNotEmpty())
	{
		s << "### Code Example: \n";
		s << "```javascript\n";
		s << codeExample << "\n";
		s << "```\n\n";
	}

	if (parameters.size() > 0)
	{
		s << "### Parameters\n";

		s << "| Name | Type | Description |\n";
		s << "| ---- | --- | ------------- |\n";

		for (const auto& p : parameters)
		{
			s << "| " << p.id << " | `" << p.type << "` | " << p.description << " |\n";
		}
	}

	if (returnType.description.isNotEmpty())
	{
		s << "### Returns\n";

		s << "`" << returnType.type << "`: " << returnType.description << "\n";
	}

	return s;
}

ExtendedApiDocumentation::ClassDocumentation::ClassDocumentation(const Identifier& className) :
	DocumentationBase(className)
{

}

hise::ExtendedApiDocumentation::MethodDocumentation* ExtendedApiDocumentation::ClassDocumentation::addMethod(const Identifier& methodName)
{
	methods.add(MethodDocumentation(id, methodName));

	return methods.getRawDataPointer() + methods.size() - 1;
}

juce::String ExtendedApiDocumentation::ClassDocumentation::createMarkdownText() const
{
	String s;

	s << "# Class " << id.toString() << "\n";

	s << description << "\n";

	for (const auto& m : methods)
		s << m.createMarkdownText();

	return s;
}


hise::ExtendedApiDocumentation::ClassDocumentation* ExtendedApiDocumentation::addClass(const Identifier& name)
{
	classes.add(ClassDocumentation(name));
	return classes.getRawDataPointer() + classes.size() - 1;
}

juce::String ExtendedApiDocumentation::getMarkdownText(const Identifier& className, const Identifier& methodName)
{
	for (auto& c : classes)
	{
		if (c.id == className || c.subClassIds.contains(className))
		{
			for (auto& m : c.methods)
			{
				if (m.id == methodName)
					return m.createMarkdownText();
			}
		}
	}

	return {};
}

} // namespace hise
