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


/** CHANGE:
 *
 * - remove icon on the left
 * - add folding
 * - move add bar to top
 * - change assign to text buttons
 * - autofold layers that are not assigning
 * - coallascate selection & assign button
 */

#include "hi_core/hi_core/PresetHandler.h"

namespace hise {
using namespace juce;

int ComponentWithGroupManagerConnection::SimpleFlexbox::apply(std::vector<Rectangle<int>>& elements,
	Rectangle<int> availableSpace)
{
	if(availableSpace.getWidth() == 0 || elements.empty())
		return 0;

	auto xPos = availableSpace.getX();
	auto yPos = availableSpace.getY();

	auto rowHeight = 0;

	int lastRowStart = 0;
	int idx = 0;

	bool isOddRow = false;

	auto centerPreviousRow = [&]()
	{
		if(idx <= 0)
			return;

		if(justification.getFlags() & (int)Justification::horizontallyCentred)
		{
			auto left = elements[lastRowStart].getX();

			if(isOddRow)
				left -= oddRowOffset;

			auto right = elements[idx - 1].getRight();
			auto rowWidth = right - left;

			auto deltaX = (availableSpace.getWidth() - rowWidth) / 2;

			for(int i = lastRowStart; i < idx; i++)
				elements[i].translate(deltaX, 0);
		}
	};

	for(auto& e: elements)
	{
		auto w = e.getWidth();

		if(xPos + w > availableSpace.getRight())
		{
			xPos = availableSpace.getX() + (isOddRow ? oddRowOffset : 0);

			rowHeight = jmax(rowHeight, e.getHeight());

			yPos += rowHeight + padding;

			centerPreviousRow();

			e.setPosition(xPos, yPos);
			
			isOddRow = !isOddRow;

			rowHeight = 0;
			lastRowStart = idx;
		}
		
		e.setPosition(xPos, yPos);

		rowHeight = jmax(rowHeight, e.getHeight());
		xPos += w + padding;
		idx++;
	}

	centerPreviousRow();

	yPos = elements.back().getBottom();

	auto thisHeight = yPos - availableSpace.getY();

	if(justification.getFlags() & (int)Justification::verticallyCentred)
	{
		auto deltaY = (availableSpace.getHeight() - thisHeight) / 2;

		if(deltaY > 0)
		{
			for(auto& e: elements)
				e.translate(0, deltaY);

			return availableSpace.getHeight();
		}
	}

	return thisHeight;
}

ComponentWithGroupManagerConnection::ComponentWithGroupManagerConnection(ModulatorSampler* s):
	ControlledObject(s->getMainController()),
	sampler(s)
{}




void ComplexGroupManagerComponent::Helpers::drawSelection(Graphics& g, Rectangle<int> bounds)
{
	g.setColour(Colours::white.withAlpha(0.25f));
	Path p;
	p.addRoundedRectangle(bounds.toFloat(), LayerComponent::TopBarHeight / 2.0f);
	Path dp;
	float l[2] = { 4.0f, 4.0f };
	PathStrokeType(1.0f).createDashedStroke(dp, p, l, 2);
	g.fillPath(dp);

	g.setColour(Colours::white.withAlpha(0.05f));

	g.fillRoundedRectangle(bounds.toFloat().reduced(2.0f), LayerComponent::TopBarHeight / 2.0f);
}

void ComplexGroupManagerComponent::Helpers::drawRuler(Graphics& g, Rectangle<int>& bounds)
{
	bounds.removeFromTop(LayerComponent::BodyMargin/2-1);
	g.setColour(Colours::white.withAlpha(0.1f));
	g.fillRect(bounds.removeFromTop(1));
	bounds.removeFromTop(LayerComponent::BodyMargin/2);
}

void ComplexGroupManagerComponent::Helpers::drawLabel(Graphics& g, Rectangle<float> bounds, const String& text, Justification j)
{
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawText(text, bounds, j);
}

Path ComplexGroupManagerComponent::Helpers::getPath(LogicType lt)
{
	auto n = getLogicTypeName(lt).toLowerCase();
	Factory f;
	return f.createPath(n);
}

Colour ComplexGroupManagerComponent::Helpers::getLayerColour(const ValueTree& v)
{
	if(v.getType() == groupIds::Layers)
		return Colours::transparentBlack;

	auto lt = getLogicType(v);
	return MultiOutputDragSource::getFadeColour((int)(lt) - 1, (int)LogicType::numLogicTypes - 1);
}



ComplexGroupManagerComponent::LayerComponent::LayerComponent(ComponentWithGroupManagerConnection& parent, const ValueTree& d_, LogicType lt):
	ComponentWithGroupManagerConnection(parent.getSampler()),
	type(lt),
	clearButton("more", nullptr, f),
	data(d_)
{
	p = Helpers::getPath(type);
	typeName = Helpers::getLogicTypeName(type);
	setSize(0, DefaultHeight + TopBarHeight);

	clearButton.setTooltip("Remove this layer.");

	clearButton.onClick = [this]()
	{
		PopupLookAndFeel plaf;
		PopupMenu m;

		m.addItem(1, "Clear layer");
		m.addItem(2, "Change layer groups");
		m.addSeparator();
		m.addItem(3, "Purgable", true, this->data[groupIds::purgable]);
		m.addItem(4, "Ignorable", true, this->data[groupIds::ignorable]);
		m.addItem(5, "Cacheable", true, this->data[groupIds::cached]);

		auto before = data.createCopy();

		auto r = m.show();

		bool rebuild = r == 1 || r == 2 || r == 4;
		
		// deleting / modifying the last layer will not invalidate the bitmasks so this should be fine
		rebuild &= data.getParent().indexOf(data) != (data.getParent().getNumChildren() - 1);

		if(rebuild)
		{
			if(PresetHandler::showYesNoWindow("Confirm layer change", "Changing this property of this layer will invalidate the bitmask of every sample.  \n> Press OK to reset the layer assignments of all currently mapped samples."))
			{
				ModulatorSampler::SoundIterator iter(getSampler());

				while(auto s = iter.getNextSound())
					s->setBitmask(0);
			}
			else
			{
				return;
			}
		}

		try
		{
			if(r == 1)
			{
				findParentComponentOfClass<Content>()->removeLayer(data);
			}
			if(r == 2)
			{
				auto tokens = this->data[groupIds::tokens].toString();
				auto newTokens = PresetHandler::getCustomName(tokens, "Enter a comma separated list of group tokens");

				if(newTokens.isNotEmpty() && newTokens != tokens)
				{
					data.setProperty(groupIds::tokens, newTokens, getUndoManager());
				}
			}
			if(r == 3)
			{
				this->data.setProperty(groupIds::purgable, !(bool)data[groupIds::purgable], getUndoManager());
			}
			if(r == 4)
			{
				this->data.setProperty(groupIds::ignorable, !(bool)data[groupIds::ignorable], getUndoManager());
			}
			if(r == 5)
			{
				this->data.setProperty(groupIds::cached, !(bool)data[groupIds::cached], getUndoManager());
			}
		}
		catch(Result& r)
		{
			PresetHandler::showMessageWindow("Error at changing property", r.getErrorMessage(), PresetHandler::IconType::Error);
			data.copyPropertiesAndChildrenFrom(before, getUndoManager());

		}
	};

	addAndMakeVisible(clearButton);
}

ComplexGroupManagerComponent::LayerComponent::~LayerComponent()
{}

void ComplexGroupManagerComponent::LayerComponent::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();

	g.setColour(Colour(0xFF1D1D1D));
	g.fillRoundedRectangle(b.reduced(1.0f), 3.0f);

	g.setColour(Colour(0xFF444444));
	g.drawRoundedRectangle(b.reduced(1.0f), 3.0f, 1.0f);

	auto c = Helpers::getLayerColour(data);

	if(c.isTransparent())
	{
		GlobalHiseLookAndFeel::drawFake3D(g, b.toNearestInt());
	}
	else
	{
		g.setColour(c);
	}

	g.setColour(c.withAlpha(0.3f));
	g.fillPath(p);

	auto topBar = b.removeFromTop(TopBarHeight);
		
	g.fillRect(topBar.reduced(3));

	if(typeName.isNotEmpty())
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(typeName, topBar, Justification::centred);
	}

	drawLabelsAndRulers(g);
}

void ComplexGroupManagerComponent::LayerComponent::resized()
{
	clearRulersAndLabels();

	auto b = getLocalBounds();

	auto top = b.removeFromTop(TopBarHeight);
	clearButton.setBounds(top.removeFromRight(top.getHeight()).reduced(3));
		
}

void ComplexGroupManagerComponent::LayerComponent::updateHeight(int newHeight)
{
	if(getHeight() != newHeight)
	{
		setSize(getWidth(), newHeight);
		findParentComponentOfClass<Content>()->updateSize();
	}
}

ComplexGroupManagerComponent::LogicTypeComponent::Parser::Parser(ComponentWithGroupManagerConnection& parent,
	const ValueTree& v):
	FileTokenSelector(parent, Helpers::getTokens(v), v[groupIds::ignorable], false)
{}

ComplexGroupManagerComponent::LogicTypeComponent::LogicTypeComponent(ComponentWithGroupManagerConnection& parent, const ValueTree& d):
	LayerComponent(parent, d, Helpers::getLogicType(d)),
	items(Helpers::getTokens(d, getSampler())),
	lockButton("lock", this, f),
	midiButton("midi", this, f),
	parser(parent, d)
{
	addAndMakeVisible(lockButton);
	addAndMakeVisible(midiButton);

	addAndMakeVisible(unassignedLabel);
	addAndMakeVisible(ignoreLabel);

	unassignedLabel.setLookAndFeel(&laf);
	ignoreLabel.setLookAndFeel(&laf);

	unassignedLabel.setEditable(false, false);
	ignoreLabel.setEditable(false, false);

	unassignedLabel.setText("Unassigned", dontSendNotification);
	unassignedLabel.setTooltip("The number of samples that are not assigned on this layer");
	ignoreLabel.setText("Ignored", dontSendNotification);
	ignoreLabel.setTooltip("The number of samples that are ignored by this layer");

	
	lockButton.setTooltip("Lock the current state of this layer regardless of the MIDI input.");
	midiButton.setTooltip("Make the UI display follow the last active state of this layer");

	lockButton.setToggleModeWithColourChange(true);
	midiButton.setToggleModeWithColourChange(true);
	
	addChildComponent(parser);

	typeName << " (" << String(items.size()) << " " << Helpers::getItemName(type, items) << ")";

	addAndMakeVisible(body = new BodyBase(*this));

	setSize(10, getHeightToUse());

	auto layerIndex = Helpers::getLayerIndex(data);
	getComplexGroupManager()->addPlaystateListener(*this, layerIndex, onPlaystateUpdate);

	showSelectors(false);
}

void ComplexGroupManagerComponent::LogicTypeComponent::buttonClicked(Button* b)
{
	
	if(b == &lockButton)
	{
		auto idx = Helpers::getLayerIndex(data);
		getComplexGroupManager()->setLockLayer(idx, lockButton.getToggleState(), body->currentIndex);
	}
	
}

ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::ButtonBase::ButtonBase()
{
	setRepaintsOnMouseActivity(true);
	setTooltip("Click to show only samples from this group.");
}

bool ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::ButtonBase::isDisplayed() const
{
	auto base = findParentComponentOfClass<BodyBase>();

	if(isPositiveAndBelow(base->currentIndex - 1, base->buttons.size()))
	{
		return base->buttons[base->currentIndex-1] == this;
	}

	return true;
}

Colour ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::ButtonBase::getButtonColour(bool on) const
{
	auto alpha = isDisplayed() ? 0.8f : 0.3f;

	if(isMouseOver())
		alpha += 0.1f;

	if(isMouseButtonDown())
		alpha += 0.1f;

	auto c = on ? Colour(0xFFAAAAAA) : Colour(0xFF222222);

	return c.withAlpha(alpha);
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::ButtonBase::mouseDown(const MouseEvent& e)
{
	auto body = findParentComponentOfClass<BodyBase>();

	auto idx = body->buttons.indexOf(this) + 1;

	if(body->currentIndex == idx)
		body->currentIndex = 0;
	else
		body->currentIndex = idx;

	body->repaint();

	auto p = findParentComponentOfClass<ComplexGroupManagerComponent>();

	auto layerIndex = Helpers::getLayerIndex(body->data);

	auto lp = findParentComponentOfClass<LogicTypeComponent>();

	if(lp->lockButton.getToggleState())
		lp->getComplexGroupManager()->setLockLayer(layerIndex, true, body->currentIndex);

	p->setDisplayFilter(layerIndex, body->currentIndex);
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::ButtonBase::setActive(bool shouldBeActive)
{
	active = shouldBeActive;
	repaint();
}

ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::KeyswitchButton::KeyswitchButton(const String& n,
                                                                                             bool addPowerButton):
	name(n)
{
	if(addPowerButton)
	{
		addAndMakeVisible(purgeButton = new HiseShapeButton("purge", nullptr, f));
		purgeButton->setToggleModeWithColourChange(true);
	}
}

int ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::KeyswitchButton::getWidthToUse() const
{
	auto w = GLOBAL_BOLD_FONT().getStringWidthFloat(name) + LayerComponent::TopBarHeight;

	if(purgeButton != nullptr)
		w += LayerComponent::TopBarHeight / 2;

	return w;
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::KeyswitchButton::resized()
{
	if(purgeButton != nullptr)
		purgeButton->setBounds(getLocalBounds().removeFromLeft(getHeight()).reduced(4));
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::KeyswitchButton::paint(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());

	auto on = getButtonColour(true);
	auto off = getButtonColour(false);

	auto b = getLocalBounds().toFloat().reduced(3.0f);

	g.setColour(active ? on : off);
	g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

	g.setColour((active ? off : on).withAlpha(0.3f));
					
	g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);

	g.setColour((active ? off : on).withAlpha(1.0f));

	if(purgeButton != nullptr)
		b.removeFromLeft(LayerComponent::TopBarHeight / 2);

	g.drawText(name, b, Justification::centred);
}

ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::SelectionTag(LogicTypeComponent& parent,
	Component* other, uint8 specialValue):
	layerIndex(Helpers::getLayerIndex(parent.data)),
	target(other),
	layerValue(specialValue)
{
	setMouseCursor(MouseCursor::PointingHandCursor);
	target->addComponentListener(this);
	refreshNumSamples();
	setRepaintsOnMouseActivity(true);

	auto t = target->findParentComponentOfClass<ComplexGroupManagerComponent>();

	String msg;

	if(layerValue == 0)
	{
		msg << "Select all samples that are not assigned to any group within the " ;
	}
	else
	{
		msg << "Select all samples that are ignored by the ";
	}

	msg << t->getComplexGroupManager()->getLayerId(layerIndex);
	msg << " layer";
	
	setTooltip(msg);
}

ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::SelectionTag(ButtonBase* attachedButton,
                                                                                       uint8 layerIndex_, uint8 layerValue_):
	target(attachedButton),
	layerIndex(layerIndex_),
	layerValue(layerValue_)
{
	setMouseCursor(MouseCursor::PointingHandCursor);
	attachedButton->addComponentListener(this);
	refreshNumSamples();

	auto t = target->findParentComponentOfClass<ComplexGroupManagerComponent>();

	String msg;
	msg << "Select all samples that are assigned to ";
	msg << t->getComplexGroupManager()->getLayerValueAsToken(layerIndex, layerValue);
	setTooltip(msg);
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::mouseDown(const MouseEvent& e)
{
	findParentComponentOfClass<ComplexGroupManagerComponent>()->addSelectionFilter({layerIndex, layerValue}, e.mods);
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::refreshNumSamples()
{
	if(auto t = target->findParentComponentOfClass<ComponentWithGroupManagerConnection>())
	{
		if(layerValue == 0 || layerValue == ComplexGroupManager::IgnoreFlag)
		{
			auto s = t->getComplexGroupManager()->getNumUnassignedAndIgnored(layerIndex);

			if(layerValue == 0)
				numSamples = s.first;
			else
				numSamples = s.second;
		}
		else
		{
			numSamples = t->getComplexGroupManager()->getNumFiltered(layerIndex, layerValue, false);
		}

		

		refreshSize();
	}
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::refreshSize()
{
	auto b = target->getBoundsInParent();
	auto p = b.getTopRight();
	auto f = GLOBAL_FONT();
	auto w = f.getStringWidth(String(numSamples)) + SelectionSize;

	auto nb = Rectangle<int>(p, p).withSizeKeepingCentre(w, SelectionSize);
	setBounds(nb);
	repaint();
}

ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::~SelectionTag()
{
	if(target != nullptr)
		target->removeComponentListener(this);
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::paint(Graphics& g)
{
	auto active = findParentComponentOfClass<ComplexGroupManagerComponent>()->isFilterSelected({layerIndex, layerValue});

	auto b = getLocalBounds().toFloat().reduced(1.0f);

	auto alpha = 0.8f;

	if(isMouseButtonDown() || isMouseOver())
		alpha = 1.0f;

	if(active)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillRoundedRectangle(b, b.getHeight() / 2.0f);
		g.setColour(Colours::black.withAlpha(alpha));

	}
	else
	{
		g.setColour(Colours::black.withAlpha(0.7f));	
		g.fillRoundedRectangle(b, b.getHeight() / 2.0f);
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(alpha));
		g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);
	}
	
	g.setFont(GLOBAL_FONT());
	g.drawText(String(numSamples), b, Justification::centred);

}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::SelectionTag::componentMovedOrResized(
	Component& component, bool wasMoved, bool wasResized)
{
	refreshSize();
}

ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::BodyBase(LogicTypeComponent& parent):
	ComponentWithGroupManagerConnection(parent.getSampler()),
	lt(parent.type),
	tokens(parent.items),
	data(parent.data)
{
	getComplexGroupManager()->lockBroadcaster.addListener(*this, onLockUpdate);
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::showSelectors(bool shouldShow)
{
	auto showSelectors = selectors.size() == buttons.size();

	if(shouldShow != showSelectors)
	{
		selectors.clear();

		if(shouldShow)
		{
			auto layerIndex = Helpers::getLayerIndex(data);
			uint8 layerValue = 1;
						
			for(auto b: buttons)
				addAndMakeVisible(selectors.add(new SelectionTag(b, layerIndex, layerValue++)));
		}
	}
	else
	{
		for(auto s: selectors)
			s->refreshNumSamples();
	}
}

int ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::getCentreXPositionForToken(uint8 tokenIndex) const
{
	auto b = getLocalBounds();

	if(tokens.size() == 0)
		return b.getCentreX();

	auto normX = (float)tokenIndex / (float)(tokens.size() - 1);
	return roundToInt(normX * (float)b.getWidth()) + b.getX();
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::onPlaystateUpdate(uint8 value)
{
	currentLayerValue = value;

	for(int i = 0; i < buttons.size(); i++)
		buttons[i]->setActive(i == (value - 1));

	if(auto p = findParentComponentOfClass<LogicTypeComponent>())
	{
		if(p->midiButton.getToggleState())
		{
			auto idx = Helpers::getLayerIndex(p->data);
			p->findParentComponentOfClass<ComplexGroupManagerComponent>()->setDisplayFilter(idx, value);
			currentIndex = value;
			repaint();
		}
	}

	if(lockEnabled)
	{
		refreshLock();
	}
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::calculateXPositionsForItems(
	Array<int>& positions) const
{
	uint8 idx = 0;

	for(auto& t: tokens)
	{
		ignoreUnused(t);
		positions.add(getCentreXPositionForToken(idx++) + PaddingLeft);
	}
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::resized()
{
	RectangleList<int> buttonBounds;

	for(auto b: buttons)
		buttonBounds.add(b->getBoundsInParent());

	allBounds = buttonBounds.getBounds();

	refreshLock();
}

void ComplexGroupManagerComponent::LogicTypeComponent::BodyBase::paint(Graphics& g)
{
	Rectangle<int> b;

	if(currentIndex == 0)
		b = allBounds;
	else if(isPositiveAndBelow((int)currentIndex-1, buttons.size()))
	{
		b = buttons[(int)currentIndex-1]->getBoundsInParent();
	}
	
	Helpers::drawSelection(g, b.expanded(2));

	if(!currentLock.isEmpty())
	{
		auto pb = currentLock.withSizeKeepingCentre(10, 10).translated(0, LayerComponent::TopBarHeight / 2 + 5).toFloat();

		Factory f;
		auto p = f.createPath("lock");
		f.scalePath(p, pb);
		g.setColour(Colours::white.withAlpha(0.4f));
		g.fillPath(p);

	}
}

void ComplexGroupManagerComponent::LogicTypeComponent::setBody(BodyBase* b)
{
	addAndMakeVisible(body = b);
	setSize(10, getHeightToUse());
}

#if 0
ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::CountButton::CountButton(uint8 layerIndex_, uint8 layerValue_):
	numSamples(0),
	layerIndex(layerIndex_),
	layerValue(layerValue_) 
{
	setMouseCursor(MouseCursor::PointingHandCursor);
	setRepaintsOnMouseActivity(true);
}

int ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::CountButton::getMinWidth() const
{
	auto tw = GLOBAL_BOLD_FONT().getStringWidth(String(numSamples));
	return jmax(tw + 10, 30);
}

void ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::CountButton::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();

	auto c = Colours::white;

	g.setColour(c.withAlpha(0.05f));

	auto ar = b.reduced(2.0f);

	g.fillRoundedRectangle(ar, 5.0f);

	auto alpha = 0.4f;

	if(isMouseOver())
		alpha += 0.1f;

	if(isMouseButtonDown())
		alpha += 0.2f;

	g.setColour(c.withAlpha(alpha));

	if(findParentComponentOfClass<ComplexGroupManagerComponent>()->isFilterSelected({layerIndex, layerValue}))
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRoundedRectangle(ar, 5.0f, 1.0f);
	}
	
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText(String(numSamples), getLocalBounds().toFloat(), Justification::centred);
}

void ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::CountButton::mouseDown(const MouseEvent& e)
{
	if(e.mods.isRightButtonDown())
	{
						
	}
	else
	{
		findParentComponentOfClass<ComplexGroupManagerComponent>()->addSelectionFilter({layerIndex, layerValue}, e.mods);
	}
}

ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::SampleCountComponent(LogicTypeComponent& parent):
	ComponentWithGroupManagerConnection(parent.getSampler())
{
	auto numTokens = parent.items.size();

	auto layerIndex = Helpers::getLayerIndex(parent.data);

	for(int i = 0; i < numTokens; i++)
	{
		auto nb = new CountButton(layerIndex, i+1);
		buttons.add(nb);
		addAndMakeVisible(nb);
	}

	addAndMakeVisible(ignoredButton = new CountButton(layerIndex, ComplexGroupManager::IgnoreFlag));
	addAndMakeVisible(unassignedButton = new CountButton(layerIndex, 0));
}

int ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::getHeightToUse() const
{
	if(showSecondRow())
		return 3 * TopBarHeight + BodyMargin;
	else
		return TopBarHeight + BodyMargin;
}

void ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::applyXPositions(
	Array<int>& positionsAndCount)
{
	auto y = BodyMargin;
	auto h = TopBarHeight;

	for(int i = 0; i < positionsAndCount.size(); i++)
	{
		auto nb = buttons[i];
		auto p = positionsAndCount[i];
		auto w = nb->getMinWidth();
		auto x = p - w/2;
		nb->setBounds(x, y, w, h);
	}

	unassignedButton->setVisible(showSecondRow());
	ignoredButton->setVisible(showSecondRow());

	if(showSecondRow())
	{
		auto b = getLocalBounds().reduced(0, BodyMargin);
		b.removeFromLeft(PaddingLeft);
		b.removeFromTop(TopBarHeight);

		auto ub = b.removeFromTop(TopBarHeight);
		unassignedButton->setBounds(ub.removeFromLeft(unassignedButton->getMinWidth()));
		ub = b.removeFromTop(TopBarHeight);
		ignoredButton->setBounds(ub.removeFromLeft(ignoredButton->getMinWidth()));
	}
}

bool ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::showSecondRow() const
{
	return ignoredButton->numSamples != 0 || unassignedButton->numSamples != 0;
}

void ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::updateNumSamples()
{
	auto gm = getComplexGroupManager();
	auto layerIndex = Helpers::getLayerIndex(data);
	uint8 idx = 1;

	auto filters = gm->getAllFiltersForLayer(layerIndex);
	
	for(auto b: buttons)
	{
		b->numSamples = gm->getNumFiltered(layerIndex, idx++, false);
		b->repaint();
	}
				
	auto num = gm->getNumUnassignedAndIgnored(layerIndex);

	ignoredButton->numSamples = num.second;
	ignoredButton->repaint();

	unassignedButton->numSamples = num.first;
	unassignedButton->repaint();
}

void ComplexGroupManagerComponent::LogicTypeComponent::SampleCountComponent::paint(Graphics& g)
{
	auto lb = getLocalBounds();
	Helpers::drawRuler(g, lb);
	auto b = getLocalBounds().toFloat().reduced(0, LayerComponent::BodyMargin);

	auto topRow = b.removeFromTop(LayerComponent::TopBarHeight);
	Helpers::drawLabel(g, topRow, "Assigned:");
	
	if(showSecondRow())
	{
		Helpers::drawLabel(g, b.removeFromTop(LayerComponent::TopBarHeight), "Unassigned");
		Helpers::drawLabel(g, b.removeFromTop(LayerComponent::TopBarHeight), "Ignored");
	}
}
#endif

ComplexGroupManagerComponent::FileTokenSelector::FileTokenSelector(ComponentWithGroupManagerConnection& parent, const StringArray& layerTokens_, bool showIgnoreButton, bool addMode_):
	ComponentWithGroupManagerConnection(parent.getSampler()),
	layerTokens(layerTokens_),
	applyButton("apply", nullptr, f),
	ignoreable(showIgnoreButton),
	addMode(addMode_)
{
	addAndMakeVisible(applyButton);

	applyButton.onClick = [this]()
	{
		this->performAction();
	};


	if(addMode)
	{
		currentAction = Action::ParseFileToken;

	}
	else
	{
		modeSelector.setUseCustomPopup(true);
		rebuildComboBox();
		addAndMakeVisible(modeSelector);
		modeSelector.setLookAndFeel(&laf);
		modeSelector.setSelectedItemIndex(0, dontSendNotification);
	}

	modeSelector.onChange = [this]()
	{
		currentAction = (Action)modeSelector.getSelectedItemIndex();
		updateSelection(currentFileTokens);
	};

	updateSelection({});
	
	getSampleEditHandler()->selectionBroadcaster.addListener(*this, [](FileTokenSelector& s, ModulatorSamplerSound::Ptr fs, int)
	{
		auto fileTokens = Helpers::getFileTokens(fs.get());

		s.updateSelection(fileTokens);
	});

}

void ComplexGroupManagerComponent::FileTokenSelector::checkApplyState()
{
	auto enabled = currentAction != Action::DoNothing;

	enabled &= getSampleEditHandler()->getNumSelected() > 0;
	applyButton.setEnabled(enabled);

	applyButton.setAlpha(enabled ? 1.0 : 0.4f);

	for(auto t: tokens)
	{
		t->setEnabled(enabled);
		t->setAlpha(enabled ? 1.0 : 0.4f);
	}
}

void ComplexGroupManagerComponent::SelectorLookAndFeel::drawComboBox(Graphics& g, int i, int i1, bool isButtonDown,
                                                                     int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb)
{
	auto b = cb.getLocalBounds();

	if(cb.isEnabled())
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRoundedRectangle(b.reduced(2).toFloat(), 3.0f);
	}
			
	Path p;
	g.setColour(Colours::white.withAlpha(0.5f));
	p.addTriangle({0.0f, 0.0f}, {1.0f, 0.0f}, { 0.5f, 1.0f} );
	PathFactory::scalePath(p, b.removeFromRight(b.getHeight()).withSizeKeepingCentre(8, 6).toFloat());

	if(cb.isEnabled())
		g.fillPath(p);
}

void ComplexGroupManagerComponent::SelectorLookAndFeel::drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& cb,
	Label& label)
{
	Helpers::drawLabel(g, cb.getLocalBounds().toFloat().reduced(10, 0), cb.getTextWhenNothingSelected());
}

void ComplexGroupManagerComponent::SelectorLookAndFeel::drawLabel(Graphics& g, Label& l)
{
	auto b = l.getLocalBounds();
	Helpers::drawLabel(g, b.toFloat(), l.getText());
}

ComplexGroupManagerComponent::FileTokenSelector::FileToken::FileToken(const String& t):
	token(t)
{
	f = GLOBAL_MONOSPACE_FONT().withHeight(16.0f);

	setMouseCursor(MouseCursor::PointingHandCursor);
	setRepaintsOnMouseActivity(true);
}

void ComplexGroupManagerComponent::FileTokenSelector::FileToken::mouseDown(const MouseEvent& e)
{
	if(!ok)
		return;

	auto p = findParentComponentOfClass<FileTokenSelector>();

	if(active)
		p->setActiveToken(-1);
	else
		p->setActiveToken(p->tokens.indexOf(this));

	if(!ok)
		active = false;
}

void ComplexGroupManagerComponent::FileTokenSelector::FileToken::paint(Graphics& g)
{
	g.setFont(f);

	float alpha = 0.8f;

	if(ok)
	{
		if(isMouseOver())
		alpha += 0.1f;

		if(isMouseButtonDown())
			alpha += 0.1f;
	}
	
	Colour bg = Colours::white.withAlpha(active ? 0.3f : (ok ? 0.1f : 0.05f));
	Colour txt = (active && ok) ? Colour(SIGNAL_COLOUR).withAlpha(alpha) : Colours::white.withAlpha(alpha * (ok ? 0.7f : 0.4f));

	g.setColour(bg);
	g.fillRect(getLocalBounds().toFloat().reduced(4.0f));
	g.setColour(txt.withAlpha(0.7f));
	g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(2.0f), 3.0f, 1);
	g.setColour(txt);
	g.drawText(token, getLocalBounds().toFloat(), Justification::centred);
}

void ComplexGroupManagerComponent::FileTokenSelector::FileToken::setActive(bool shouldBeActive,
	bool isOk)
{
	if(shouldBeActive)
	{
		active = true;
		ok = isOk;
	}
	else
	{
		active = false;
		ok = true;
	}

	repaint();
}

void ComplexGroupManagerComponent::FileTokenSelector::rebuildComboBox()
{
	checkApplyState();

	auto idx = jmax(0, modeSelector.getSelectedItemIndex());
	modeSelector.clear(dontSendNotification);

	canUseFileTokens = false;

	auto validNames = getValidTokens();

	for(int i = 0; i < currentFileTokens.size(); i++)
	{
		auto match = validNames.contains(currentFileTokens[i]);

		if(match && currentAction == Action::ParseFileToken)
		{
			setActiveToken(i);
		}

		canUseFileTokens |= match;
	}

	auto getDisabledString = [](const String& s, bool enabled)
	{
		return enabled ? s.replace("~", "") : s;
	};

	modeSelector.addItemList({ "Skip", getDisabledString("~~Parse file token~~", canUseFileTokens), "Set fix group", getDisabledString("~~Set ignore flag~~", ignoreable), "Unassign"}, 1);
	modeSelector.rebuildPopupMenu();
	modeSelector.setSelectedItemIndex(idx, dontSendNotification);
	repaint();
}

void ComplexGroupManagerComponent::FileTokenSelector::setActiveToken(int idx)
{
	switch(currentAction)
	{
	case Action::ParseFileToken:
	{
		auto vnames = getValidTokens();

		for(auto t: tokens)
			t->setActive(idx == tokens.indexOf(t), vnames.contains(t->token));

		break;
	}
	case Action::SetToFixGroup:

		for(auto t: tokens)
			t->setActive(idx == tokens.indexOf(t), true);
		
		break;
	case Action::DoNothing:
	case Action::SetIgnoreFlag:
	case Action::Unassign:
	case Action::numActions:
	default: ;

		for(auto t: tokens)
			t->setActive(false, true);
	
		break;
	}

	ok = false;

	for(auto t: tokens)
	{
		ok |= (t->active && t->ok);
	}

	ok |= idx == ComplexGroupManager::IgnoreFlag;

	if(onTokenChange)
		onTokenChange(idx);

	repaint();
}

void ComplexGroupManagerComponent::FileTokenSelector::updateSelection(const StringArray& fileTokens)
{
	tokens.clear();
	delimiters.clear();

	currentFileTokens = fileTokens;

	switch(currentAction)
	{
	case Action::DoNothing:
		setActiveToken(-2);
		break;
	case Action::ParseFileToken:
	{
		for(auto t: currentFileTokens)
		{
			addAndMakeVisible(tokens.add(new FileToken(t)));
			tokens.getLast()->setTooltip(addMode ? "Create the layer groups from this token" : "Parse the layer value from this file token.");
		}

		break;
	}
	case Action::SetToFixGroup:
		for(auto t: layerTokens)
		{
			addAndMakeVisible(tokens.add(new FileToken(t)));
			tokens.getLast()->setTooltip("Parse the layer value from this file token.");
		}
			
		break;
	case Action::SetIgnoreFlag:
		setActiveToken(ComplexGroupManager::IgnoreFlag);
		break;
	case Action::Unassign:
		setActiveToken(-1);
		break;
	case Action::numActions:
		break;
	default: ;
	}

	rebuildComboBox();
	resized();
}

void ComplexGroupManagerComponent::FileTokenSelector::paint(Graphics& g)
{
	auto b = getLocalBounds();

	Helpers::drawRuler(g, b);

	//Helpers::drawLabel(g, tokenLabel, canUseFileTokens ? "File tokens:" : "Set to group:");

	g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_MONOSPACE_FONT());

	if(!tokens.isEmpty())
	{
		for(auto t: delimiters)
		{
			if(useFileTokens() && t == delimiters.getLast())
			{
				g.drawText(".wav", t.reduced(5, 0).withWidth(200), Justification::left);
			}
			else
			{
				g.drawText(useFileTokens() ? "_" : "||", t, Justification::centred);
			}
		}
	}
}

void ComplexGroupManagerComponent::FileTokenSelector::resized()
{
	auto b = getLocalBounds();
	b.removeFromTop(LogicTypeComponent::BodyMargin);
	auto tokenArea = b.removeFromTop(LogicTypeComponent::TopBarHeight);
	tokenLabel = tokenArea.removeFromLeft(LogicTypeComponent::PaddingLeft * 3 / 2).toFloat();

	modeSelector.setBounds(tokenLabel.toNearestInt());

	if(addMode)
		applyButton.setVisible(false);
	else
		applyButton.setBounds(tokenArea.removeFromRight(tokenArea.getHeight()).reduced(4));

	SimpleFlexbox fb;
	fb.padding = 30;

	Array<Component*> componentsToPosition;

	for(auto t: tokens)
	{
		t->setSize(t->getBestWidth(), LogicTypeComponent::TopBarHeight);
		componentsToPosition.add(t);
	}

	auto pos = fb.createBoundsListFromComponents(componentsToPosition);

	static constexpr int TokenMargin = 30;

	if(auto h = fb.apply(pos, tokenArea))
	{
		height = h;

		fb.applyBoundsListToComponents(componentsToPosition, pos);

		

		if(getHeightToUse() != getHeight())
		{
			if(auto c = findParentComponentOfClass<Content>())
				c->updateSize();
		}
	}

	delimiters.clear();

	for(auto p: tokens)
	{
		delimiters.add(p->getBoundsInParent().withX(p->getRight()).withWidth(TokenMargin).toFloat());
	}

}

bool ComplexGroupManagerComponent::FileTokenSelector::useFileTokens() const
{
	return currentAction == Action::ParseFileToken;
}

int ComplexGroupManagerComponent::FileTokenSelector::getHeightToUse() const
{
	return height + LogicTypeComponent::BodyMargin;
}

uint8 ComplexGroupManagerComponent::FileTokenSelector::getCurrentTokenValue(const String& filename) const
{
	if(currentAction == Action::SetIgnoreFlag)
		return ComplexGroupManager::IgnoreFlag;

	if(ok)
	{
		int tokenIndex;

		if(useFileTokens())
		{
			auto thisTokens = Helpers::getFileTokens(filename);

			auto validTokens = getValidTokens();

			for(auto t: tokens)
			{
				if(t->active)
				{
					auto idx = tokens.indexOf(t);
					auto thisValue = thisTokens[idx];
					tokenIndex = validTokens.indexOf(thisValue);

					if(legatoMode)
					{
						// note names are 128 - 255 so we need to wrap them to the original index
						tokenIndex %= 128;
					}

					break;
				}
			}
		}
		else
		{
			for(auto t: tokens)
			{
				if(t->active)
				{
					tokenIndex = tokens.indexOf(t);
					break;
				}
			}
		}

		if(legatoMode)
		{
			// now we need to fetch the key range to calculate the group layer
			ValueTree v(groupIds::Layer);
			v.setProperty(groupIds::type, "Legato", nullptr);
			auto kr = ComplexGroupManager::Helpers::getKeyRange(v, getSampler());

			auto startNote = kr.getFirstSetBit();
			tokenIndex -= startNote;
		}

		tokenIndex += 1; // one-based!
		return tokenIndex;
	}

	return 0;
}

uint8 ComplexGroupManagerComponent::FileTokenSelector::getSelectedTokenIndex() const
{
	if(currentAction == Action::SetIgnoreFlag)
		return ComplexGroupManager::IgnoreFlag;

	for(auto b: tokens)
	{
		if(b->active)
			return tokens.indexOf(b) + 1;
	}

	return 0;
}

int ComplexGroupManagerComponent::LogicTypeComponent::getHeightToUse() const
{
	auto h = TopBarHeight + 2 * BodyMargin;

	if(body != nullptr)
		h += body->getHeightToUse();

	if(parser.isVisible())
		h += parser.getHeightToUse();

	return h;
}

void ComplexGroupManagerComponent::LogicTypeComponent::refreshHeight()
{
	updateHeight(getHeightToUse());
}

void ComplexGroupManagerComponent::LogicTypeComponent::resized()
{
	LayerComponent::resized();

	auto b = getLocalBounds();

	auto topBar = b.removeFromTop(TopBarHeight);
			
	lockButton.setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(4));
	midiButton.setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(4));

	b = b.reduced(BodyMargin);

	auto bb = b.removeFromTop(body->getHeightToUse());

	auto leftArea = bb.removeFromLeft(PaddingLeft);

	ignoreLabel.setBounds(leftArea.removeFromBottom(AddComponent::LabelHeight));
	unassignedLabel.setBounds(leftArea.removeFromTop(AddComponent::LabelHeight));

	PathFactory::scalePath(p, leftArea.withSizeKeepingCentre(PaddingLeft, PaddingLeft).reduced(15).toFloat());

	body->setBounds(bb);
	body->resized();

	if(auto pc = findParentComponentOfClass<ComplexGroupManagerComponent>())
	{
		auto visible = pc->parseButton.getToggleState();
		parser.setVisible(visible);
		if(visible)
			parser.setBounds(b.removeFromTop(parser.getHeightToUse()));
	}

	updateSampleCounters();
}

void ComplexGroupManagerComponent::LogicTypeComponent::updateSampleCounters()
{
	callRecursive<BodyBase::SelectionTag>(this, [](BodyBase::SelectionTag* s)
	{
		s->refreshNumSamples();
		return false;
	});
}

void ComplexGroupManagerComponent::LogicTypeComponent::showSelectors(bool shouldShow)
{
	selectors.clear();

	ignoreLabel.setVisible(shouldShow && data[groupIds::ignorable]);
	unassignedLabel.setVisible(shouldShow);

	if(shouldShow)
	{
		if(ignoreLabel.isVisible())
			selectors.add(new BodyBase::SelectionTag(*this, &ignoreLabel, ComplexGroupManager::IgnoreFlag));

				
		selectors.add(new BodyBase::SelectionTag(*this, &unassignedLabel, 0));

		auto numSamples = selectors.getLast()->numSamples;

		if(numSamples == 0)
		{
			selectors.removeLast();
			unassignedLabel.setVisible(false);
		}
				
		for(auto s: selectors)
			addAndMakeVisible(s);
	}

	body->showSelectors(shouldShow);
}

void ComplexGroupManagerComponent::LogicTypeComponent::onPlaystateUpdate(LogicTypeComponent& l, uint8 value)
{
	l.body->onPlaystateUpdate(value);
}

void ComplexGroupManagerComponent::LogicTypeComponent::paint(Graphics& g)
{
	LayerComponent::paint(g);
}



struct ComplexGroupManagerComponent::AddComponent::LogicTypeSelector: public Component,
																      public ComponentWithGroupManagerConnection
{
	static constexpr int IconWidth = 60;
	static constexpr int LogicTypeSelectorHeight = 2 * IconWidth + 2 * BodyMargin + 2 * TopBarHeight;
	
	void paint(Graphics& g) override
	{
		auto pos = getMouseXYRelative().toFloat();

		for(int i = 0; i < areas.size(); i++)
		{
			auto lt = (LogicType)(i + 1);

			auto n = Helpers::getLogicTypeName(lt);
			auto p = Helpers::getPath(lt);

			auto a = areas[i].toFloat().expanded(4, 0);

			a.removeFromBottom(-BodyMargin);

			float alpha = 0.0f;

			auto active = currentType == lt;

			if(a.contains(pos))
			{
				alpha += 0.05f;

				if(isMouseButtonDown())
				{
					alpha += 0.05f;
					a = a.reduced(1.0f);
				}
			}

			if(active)
				alpha += 0.1f;

			g.setColour(Colours::white.withAlpha(alpha));
			g.fillRoundedRectangle(a.reduced(2.0f), 3.0f);
			g.drawRoundedRectangle(a.reduced(2.0f), 3.0f, 1.0f);

			PathFactory::scalePath(p, a.removeFromTop(IconWidth).reduced(15.0f));
			
			auto colour = scriptnode::MultiOutputDragSource::getFadeColour((int)lt, (int)LogicType::numLogicTypes);

			g.setColour(colour.withMultipliedSaturation(1.3f));
			g.fillPath(p);

			Helpers::drawLabel(g, a, n, Justification::centred);
		}
	}

	LogicTypeSelector(ModulatorSampler* s):
	  ComponentWithGroupManagerConnection(s)
	{
		setRepaintsOnMouseActivity(true);
	}

	void mouseMove(const MouseEvent& event) override { repaint(); }

	void mouseDown(const MouseEvent& e) override
	{
		for(int i = 0; i < areas.size(); i++)
		{
			if(areas[i].contains(e.getPosition()))
			{
				auto newType = (LogicType)(i + 1);

				if(currentType != newType)
				{
					currentType = newType;

					if(onTypeSelection)
						onTypeSelection(currentType);

					repaint();
				}

				break;
			}
		}
	}

	int getHeightToUse() const override { return height + 2 * BodyMargin; }

	void resized() override
	{
		SimpleFlexbox fb;

		fb.justification = Justification::centredTop;
		fb.padding = 10;

		areas.clear();

		auto numTypes = (int)(LogicType::numLogicTypes) - 1;

		for(int i = 0; i < numTypes; i++)
			areas.push_back({ 0, 0, IconWidth, IconWidth + 10 });

		if(auto h = fb.apply(areas, getLocalBounds().reduced(BodyMargin, BodyMargin)))
		{
			height = h;

			if(getHeight() != getHeightToUse())
			{
				if(auto c = findParentComponentOfClass<Content>())
					c->updateSize();
			}
				
		}
	}

	

	std::function<void(LogicType)> onTypeSelection;

	LogicType currentType = LogicType::RoundRobin;
	std::vector<Rectangle<int>> areas;
	int height = IconWidth;
};

ComplexGroupManagerComponent::AddComponent::AddComponent(ComponentWithGroupManagerConnection& parent, const ValueTree& d):
	LayerComponent(parent, d, LogicType::numLogicTypes),
	//addButton("add", this, f),
	//moreButton("more", this, f),
	okButton("Add new layer"),
	tokenSelector(parent, {}, false, true)
{
	typeName = {};

	//addAndMakeVisible(addButton);

	//addButton.setToggleModeWithColourChange(true);

	//addAndMakeVisible(moreButton);

	addAndMakeVisible(okButton);
	okButton.setLookAndFeel(&alaf);
	okButton.addListener(this);

	addAndMakeVisible(tokenSelector);

	

	addEditor(groupIds::type, "The layer mode. There are multiple predefined layer types with common use cases which can be combined.", "LogicTypeSelector");
	
	auto tokenEditor = dynamic_cast<TextEditor*>(addEditor(groupIds::tokens, "A comma separated list of tokens that will be used to determine the amount of groups in this layer.  \n> If you select some samples, you can choose the file name token and it will automatically fill out the tokens based on the sample selection.", "TextEditor"));

	auto idEditor = dynamic_cast<TextEditor*>(addEditor(groupIds::id, "This sets the ID of the layer. This is used for organisational purposes like the table column name as well as addressing them through the scripting API.", "TextEditor"));
	
	idEditor->setTextToShowWhenEmpty("Enter ID", Colours::black.withAlpha(0.3f));
	tokenEditor->setTextToShowWhenEmpty("Token1, Token2, Token3", Colours::black.withAlpha(0.3f));

	addEditor(groupIds::purgable, "Enable purging of different groups of this layer", "HiseShapeButton");
	addEditor(groupIds::ignorable, "Allow the ignore flag to be set for this layer" , "HiseShapeButton");
	addEditor(groupIds::cached, "Prefilters the groups of this layer for the voice start calculation", "HiseShapeButton");
	
	setSize(200, TopBarHeight);

	//addButton.setTooltip("Add a new layer");
	clearButton.setTooltip("Remove all layers");
	//moreButton.setTooltip("Manage layer setup");

	layerListener.setCallback(data, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(AddComponent::layerAddOrRemoved));

	groupRebuildListener.setCallback(data, { groupIds::tokens, groupIds::ignorable, groupIds::purgable }, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(AddComponent::rebuildLayers));

	tokenSelector.onTokenChange = [this, tokenEditor](uint8 idx)
	{
		auto lt = dynamic_cast<LogicTypeSelector*>(this->getEditor(groupIds::type));

		auto isLegato = lt->currentType == ComplexGroupManager::LogicType::LegatoInterval;

		// Do not update the token in the editor when in legato mode...
		if(isLegato)
		{
			tokenEditor->setText("", true);
			return;
		}
			

		StringArray allTokens;

		for(auto s: *getSampleEditHandler())
		{
			auto fileTokens = Helpers::getFileTokens(s.get(), tokenSelector.delimiter);
			auto tokenValue = fileTokens[idx].trim();
			allTokens.addIfNotAlreadyThere(tokenValue);
		}

		allTokens.sortNatural();
		tokenEditor->setText(allTokens.joinIntoString(", "), true);
	};

}

ComplexGroupManagerComponent::AddComponent::~AddComponent()
{
	
}

ComplexGroupManagerComponent::LogicTypeComponent* ComplexGroupManagerComponent::AddComponent::create(const ValueTree& l)
{
	auto nc = new LogicTypeComponent(*this, l);

	switch(nc->type)
	{
	case LogicType::Undefined: break;
	case LogicType::Custom: break;
	case LogicType::RoundRobin: 
		nc->setBody(new RRBody(*nc));
		break;
	case LogicType::Keyswitch:
		nc->setBody(new KeyswitchBody(*nc));
		break;
	case LogicType::TableFade:
		nc->setBody(new TableFadeBody(*nc));
		break;
	case LogicType::XFade:
		nc->setBody(new XFadeBody(*nc));
		break;
	case LogicType::LegatoInterval:
		nc->setBody(new LegatoBody(*nc));
		nc->parser.legatoMode = true;
		break;
	case LogicType::ReleaseTrigger: break;
	case LogicType::Choke:
		nc->setBody(new ChokeBody(*nc));
		break;
	case LogicType::numLogicTypes: break;
	default: ;
	}
			
	return nc;
}

void ComplexGroupManagerComponent::AddComponent::addNewLayer()
{
	auto tokens = StringArray::fromTokens(getEditorValue(groupIds::tokens).toString(), ",", "");
	tokens.trim();
	tokens.removeDuplicates(false);
	tokens.removeEmptyStrings(true);

	dynamic_cast<TextEditor*>(getEditor(groupIds::tokens))->setText(tokens.joinIntoString(","), dontSendNotification);

	ValueTree l(groupIds::Layer);

	Array<Identifier> ids = {
		groupIds::id,
		groupIds::type,
		groupIds::tokens,
		groupIds::ignorable,
		groupIds::cached,
		groupIds::purgable
	};

	ids.addArray(getSpecialIds());

	for(auto id: ids)
	{
		if(getEditor(id) != nullptr)
			l.setProperty(id, getEditorValue(id), nullptr);
	}

	if(l[groupIds::id].toString().isEmpty())
	{
		auto lt = Helpers::getLogicType(l);
		l.setProperty(groupIds::id, Helpers::getNextFreeId(lt, data).toString(), nullptr);
	}

	tokens = Helpers::getTokens(l, getSampler());
	
	getUndoManager()->beginNewTransaction();

	try
	{
		data.addChild(l, -1, getUndoManager());
	}
	catch(Result& r)
	{
		PresetHandler::showMessageWindow("Error at creating layer", r.getErrorMessage(), PresetHandler::IconType::Error);
		data.removeChild(l, getUndoManager());
	}

	if(tokenSelector.isVisible())
	{
		if(auto idx = tokenSelector.getSelectedTokenIndex())
		{
			tokenSelector.layerTokens = tokens;

			auto gm = getComplexGroupManager();

			auto layerId = Identifier(l[groupIds::id].toString());

			for(auto s: *getSampleEditHandler())
			{
				String filename = Helpers::getSampleFilename(s.get());

				auto idxToUse = tokenSelector.getCurrentTokenValue(filename);

				gm->setSampleId(s.get(), { { layerId, idxToUse }}, true);
			}
		}
	}
}

void ComplexGroupManagerComponent::AddComponent::buttonClicked(Button* b)
{
#if 0
	if(b == &addButton)
	{
		auto shouldShow = b->getToggleState();
		auto show = getHeight() != TopBarHeight;

		if(shouldShow != show)
			updateHeight(getHeightToUse());
	}

	if(b == &moreButton)
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Export as Base64");
		m.addItem(2, "Import from Base64");

		m.addSeparator();

		m.addItem(3, "Export as JSON");
		m.addItem(4, "Import from JSON");

		m.addSeparator();

		m.addItem(5, "Reset all samples");
		m.addItem(6, "Rebuild cache");

		if(auto r = m.show())
		{
			if(r == 1)
			{
				zstd::ZDefaultCompressor comp;
				MemoryBlock mb;
				comp.compress(data, mb);
				auto b64 = mb.toBase64Encoding();
				SystemClipboard::copyTextToClipboard(b64);
			}
			if(r == 2)
			{
				
				MemoryBlock mb;
				auto b64 = SystemClipboard::getTextFromClipboard();
				if(mb.fromBase64Encoding(b64))
				{
					ValueTree v;
					zstd::ZDefaultCompressor comp;
					comp.expand(mb, v);

					ComplexGroupManager::ScopedUpdateDelayer sds(*getComplexGroupManager());

					getUndoManager()->beginNewTransaction();
					data.removeAllChildren(getUndoManager());

					for(auto c: v)
						data.addChild(c.createCopy(), -1, getUndoManager());
				}
			}
		}

		
	}
#endif
	if(b == &okButton)
	{
		if(b == &okButton)
		{
			addNewLayer();
		}
	}
}


void ComplexGroupManagerComponent::AddComponent::paint(Graphics& g)
{
	LayerComponent::paint(g);
	
	if(!tokenSelector.isVisible() || tokenSelector.tokens.isEmpty())
	{
		auto b = tokenSelector.getBoundsInParent().toFloat();
		Helpers::drawLabel(g, b, "Select samples to create a token list", Justification::centred);
	}
}

void ComplexGroupManagerComponent::AddComponent::rebuildLayers(const ValueTree& v, const Identifier& id)
{
	auto p = findParentComponentOfClass<Content>();
	p->layerComponents.clear();

	for(auto c: data)
		layerAddOrRemoved(c, true);
	
}

void ComplexGroupManagerComponent::AddComponent::layerAddOrRemoved(const ValueTree& v, bool wasAdded)
{
	auto c = findParentComponentOfClass<Content>();

	if(c == nullptr)
		return;

	if(wasAdded)
	{
		auto c = findParentComponentOfClass<Content>();
		auto nc = create(v);
		c->layerComponents.add(nc);
		c->addAndMakeVisible(nc);

		dynamic_cast<TextEditor*>(getEditor(groupIds::tokens))->setText("", dontSendNotification);
		dynamic_cast<TextEditor*>(getEditor(groupIds::id))->setText("", dontSendNotification);
		
		//if(addButton.getToggleState())
//			addButton.triggerClick(sendNotificationSync);

		c->updateSize();
	}
	else
	{
		for(auto l: c->layerComponents)
		{
			if(l->data == v)
			{
				c->layerComponents.removeObject(l);
				break;
			}
		}
		
		c->updateSize();
	}

	if(auto parent = findParentComponentOfClass<ComplexGroupManagerComponent>())
		parent->rebuildSampleCounters();
}



void ComplexGroupManagerComponent::AddComponent::resized()
{
	LayerComponent::resized();

	auto b = getLocalBounds();

	auto top = b.removeFromTop(TopBarHeight);

	constexpr int ButtonMargin = 5;

	b.removeFromBottom(BodyMargin);
	b.removeFromLeft(BodyMargin);
	b.removeFromRight(BodyMargin);

	//moreButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(ButtonMargin));
	//addButton.setBounds(top.withSizeKeepingCentre(top.getHeight(), top.getHeight()).reduced(ButtonMargin));
	clearButton.setVisible(false);
	clearButton.setBounds(top.removeFromRight(top.getHeight()).reduced(ButtonMargin));
	top.removeFromRight(2 * top.getHeight());

	auto showBody = true;

	if(showBody)
	{
		// set the ok padding before the left padding
		okButton.setBounds(b.removeFromBottom(TopBarHeight));
		addRuler(b.removeFromBottom(BodyMargin).reduced(BodyMargin, 0));

		b.removeFromTop(BodyMargin).reduced(BodyMargin, 0);

		addLabel(b.removeFromTop(LabelHeight), "Layer Type:");
		getEditor(groupIds::type)->setBounds(b.removeFromTop(getGroupComponent(groupIds::type)->getHeightToUse()));
		addRuler(b.removeFromTop(BodyMargin).reduced(BodyMargin, 0));

		auto textEditorArea = b.removeFromTop(LabelHeight + 28);

		auto idArea = textEditorArea.removeFromLeft(150);
		textEditorArea.removeFromLeft(BodyMargin);
		auto tokenArea = textEditorArea;

		addLabel(idArea.removeFromTop(LabelHeight), "Layer ID:");
		addLabel(tokenArea.removeFromTop(LabelHeight), "Token values:");
		
		getEditor(groupIds::id)->setBounds(idArea.removeFromTop(28));
		getEditor(groupIds::tokens)->setBounds(tokenArea.removeFromTop(28));

		tokenSelector.setBounds(b.removeFromTop(tokenSelector.getHeightToUse()));
		addRuler(b.removeFromTop(BodyMargin).reduced(BodyMargin, 0));

		{
			auto flagArea = b.removeFromTop(flagHeight);

			addLabel(flagArea.removeFromLeft(PaddingLeft), "Layer flags:");

			std::vector<Rectangle<int>> flagButtons;

			flagButtons.push_back({ 60 + TopBarHeight, TopBarHeight });
			flagButtons.push_back({ 60 + TopBarHeight, TopBarHeight });
			flagButtons.push_back({ 60 + TopBarHeight, TopBarHeight });

			SimpleFlexbox fb;

			if(auto h = fb.apply(flagButtons, flagArea))
			{
				flagHeight = h;

				getEditor(groupIds::purgable)->setBounds(flagButtons[0].removeFromLeft(TopBarHeight).reduced(ButtonMargin));
				addLabel(flagButtons[0], "Purge");
				getEditor(groupIds::ignorable)->setBounds(flagButtons[1].removeFromLeft(TopBarHeight).reduced(ButtonMargin));
				addLabel(flagButtons[1].removeFromLeft(100), "Ignore" );
				getEditor(groupIds::cached)->setBounds(flagButtons[2].removeFromLeft(TopBarHeight).reduced(ButtonMargin));
				addLabel(flagButtons[2].removeFromLeft(100), "Cache" );
			}
		}

		{

			auto specialComponents = getSpecialComponents();

			if(!specialComponents.isEmpty())
			{
				addRuler(b.removeFromTop(BodyMargin));

				addLabel(b.removeFromTop(LabelHeight), "Special Properties:");

				b.removeFromTop(BodyMargin);

				auto specialBounds = b.removeFromTop(specialHeight);

				SimpleFlexbox fb;

				auto pos = fb.createBoundsListFromComponents(specialComponents);
				if(auto h = fb.apply(pos, specialBounds))
				{
					specialHeight = h;
					fb.applyBoundsListToComponents(specialComponents, pos);

					if(getHeight() != getHeightToUse())
						findParentComponentOfClass<Content>()->updateSize();
				}
			}
		}
	}

	for(auto e: editors)
		e->setVisible(showBody);

	for(auto b: helpButtons)
		b->setVisible(showBody);

	tokenSelector.setVisible(showBody);
	okButton.setVisible(showBody);
}

int ComplexGroupManagerComponent::AddComponent::getHeightToUse() const
{
	int bodyHeight = TopBarHeight;

	getGroupComponent(groupIds::type)->resizeWithoutRecursion();

	bodyHeight += BodyMargin;
	bodyHeight += LabelHeight + getGroupComponent(groupIds::type)->getHeightToUse() + BodyMargin;
	bodyHeight += LabelHeight + getEditor(groupIds::tokens)->getHeight() + BodyMargin;
	bodyHeight += tokenSelector.getHeightToUse() + BodyMargin;
	bodyHeight += flagHeight;

	if(specialHeight != 0)
		bodyHeight += 2 * BodyMargin + LabelHeight + specialHeight;

	bodyHeight += TopBarHeight;
	bodyHeight += BodyMargin;

	return bodyHeight;
}

void ComplexGroupManagerComponent::AddComponent::addSpecialComponents(LogicType nt)
{
	auto newId = Helpers::getNextFreeId(nt, getComplexGroupManager()->getDataTree()).toString();
	dynamic_cast<TextEditor*>(getEditor(groupIds::id))->setText(newId, dontSendNotification);

	auto defaultCacheValue = ComplexGroupManager::Helpers::getDefaultValue(nt, ComplexGroupManager::Flags::FlagCached);
	auto defaultPurgeValue = ComplexGroupManager::Helpers::getDefaultValue(nt, ComplexGroupManager::Flags::FlagPurgable);
	auto defaultIgnoreValue = ComplexGroupManager::Helpers::getDefaultValue(nt, ComplexGroupManager::Flags::FlagIgnorable);

	if(defaultPurgeValue != -1)
		dynamic_cast<HiseShapeButton*>(getEditor(groupIds::purgable))->setToggleStateAndUpdateIcon((bool)defaultPurgeValue, true);

	if(defaultIgnoreValue != -1)
		dynamic_cast<HiseShapeButton*>(getEditor(groupIds::ignorable))->setToggleStateAndUpdateIcon((bool)defaultIgnoreValue, true);

	if(defaultCacheValue != -1)
		dynamic_cast<HiseShapeButton*>(getEditor(groupIds::cached))->setToggleStateAndUpdateIcon((bool)defaultCacheValue, true);

	tokenSelector.legatoMode = nt == ComplexGroupManager::LogicType::LegatoInterval;
	tokenSelector.rebuildComboBox();

	getEditor(groupIds::tokens)->setEnabled(!tokenSelector.legatoMode);

#if 0

	auto list = getSpecialComponents();

	for(auto l: list)
	{
		for(auto hb: helpButtons)
		{
			if(hb->getAttachedComponent() == l)
			{
				helpButtons.removeObject(hb);
				break;
			}
		}

		editors.removeObject(l);
	}

	auto addKeySelector = [&](const Identifier& id, const String& emptyText, const String& tooltip)
	{
		auto ko = dynamic_cast<ComboBox*>(addEditor(id, 
											tooltip, 
											"ComboBox"));

		StringArray noteNames;

		for(int i = 0; i < 128; i++)
			noteNames.add(MidiMessage::getMidiNoteName(i, true, true, 3) + " (" + String(i) + ")");

		ko->setTextWhenNothingSelected(emptyText);
		ko->addItemList(noteNames, 1);
	};

	switch(nt)
	{
	case LogicType::Keyswitch:
	{
		addKeySelector(SampleIds::LoKey, "First keyswitch", "The first note that's assigned to the keyswitches");

		auto kc = dynamic_cast<ComboBox*>(addEditor(groupIds::isChromatic, 
													"How the key switches are layed out on the keyboard", 
													"ComboBox"));

		kc->setTextWhenNothingSelected("Key layout");
		kc->addItemList({ "White keys", "Chromatic"}, 1);

		

		break;
	}
	case LogicType::Undefined: break;
	case LogicType::Custom: break;
	case LogicType::RoundRobin: break;
	case LogicType::TableFade: break;
	case LogicType::XFade: break;
	case LogicType::LegatoInterval:
	{
		addKeySelector(SampleIds::LoKey, "Lowest note", "");
		addKeySelector(SampleIds::HiKey, "Highest note", "");

		auto kc = dynamic_cast<ComboBox*>(addEditor(groupIds::useNumbers, 
													"Whether to use note names or numbers as file token", 
													"ComboBox"));

		kc->setTextWhenNothingSelected("Token format");
		kc->addItemList({ "Note name (eg. D#3)", "Number (0 - 127)"}, 1);

		break;
	}
	case LogicType::ReleaseTrigger: break;
	case LogicType::Choke: break;
	case LogicType::numLogicTypes: break;
	default:
		break;
	}

	list = getSpecialComponents();

	SimpleFlexbox fb;

	auto pos = fb.createBoundsListFromComponents(list);
#endif

	specialHeight = 0; // fb.apply(pos, getLocalBounds());



	resized();
}

Array<Component::SafePointer<Component>> ComplexGroupManagerComponent::AddComponent::getSpecialComponents() const
{
	return {};

#if 0
	auto specialIds = getSpecialIds();
	Array<Component::SafePointer<Component>> list;

	for(int i = 0; i < editors.size(); i++)
	{
		if(specialIds.contains(Identifier(editors[i]->getName())))
			list.add(editors[i]);
	}

	return list;
#endif
}

Component* ComplexGroupManagerComponent::AddComponent::addEditor(const Identifier& propertyId, const String& helpText,
                                                                 const String& type)
{
	Component* nc = nullptr;

	auto at = MarkdownHelpButton::AttachmentType::TopRight;

	if(type == "LogicTypeSelector")
	{
		auto t = new LogicTypeSelector(getSampler());
		t->onTypeSelection = BIND_MEMBER_FUNCTION_1(AddComponent::addSpecialComponents); 
		nc = t;
	}
	else if (type == "ComboBox")
	{
		nc = new ComboBox();
		nc->setLookAndFeel(&tokenSelector.laf);
		nc->setSize(128, TopBarHeight);
	}
	else if(type == "TextEditor")
	{
		auto te = new TextEditor();
		GlobalHiseLookAndFeel::setTextEditorColours(*te);
		te->setSelectAllWhenFocused(true);
		nc = te;
	}
	else if (type == "HiseShapeButton")
	{
		auto hb = new HiseShapeButton(propertyId.toString(), this, f);
		hb->setToggleModeWithColourChange(true);
		hb->getProperties().set("helpOffsetX", 60);
		at = MarkdownHelpButton::AttachmentType::PropertyHelpOffsetXY;
		nc = hb;
	}
	else
		return nullptr;

	nc->setName(propertyId.toString());
	editors.add(nc);
	addAndMakeVisible(nc);

	if(helpText.isNotEmpty())
	{
		auto md = new MarkdownHelpButton();
		md->setHelpText(helpText);
		md->attachTo(nc, at);
		addAndMakeVisible(helpButtons.add(md));
	}

	return nc;
}

ComponentWithGroupManagerConnection* ComplexGroupManagerComponent::AddComponent::getGroupComponent(
	const Identifier& id) const
{
	return dynamic_cast<ComponentWithGroupManagerConnection*>(getEditor(id));
}

Component* ComplexGroupManagerComponent::AddComponent::getEditor(const Identifier& id) const
{
	for(auto e: editors)
	{
		if(e->getName() == id.toString())
			return e;
	}
	
	jassert(getSpecialIds().contains(id));

	return nullptr;
}

var ComplexGroupManagerComponent::AddComponent::getEditorValue(const Identifier& id) const
{
	if(getSpecialIds().contains(id))
	{
		if(auto cb = dynamic_cast<ComboBox*>(getEditor(id)))
		{
			return cb->getSelectedItemIndex();
		}

		jassertfalse;
		return var();
	}

	if(id == groupIds::type)
	{
		auto lt = dynamic_cast<LogicTypeSelector*>(getEditor(id))->currentType;
		return Helpers::getLogicTypeName(lt);
	}
	if(id == groupIds::purgable || id == groupIds::cached || id == groupIds::ignorable)
	{
		return dynamic_cast<HiseShapeButton*>(getEditor(id))->getToggleState();
	}
	if(id == groupIds::id || id == groupIds::tokens)
	{
		return dynamic_cast<TextEditor*>(getEditor(id))->getText();
	}

	jassertfalse;
	return var();
}

ComplexGroupManagerComponent::ParseToolbar::ParseToolbar(ComponentWithGroupManagerConnection& parent, const ValueTree& d):
	LayerComponent(parent, d, LogicType::numLogicTypes),
	applyButton("apply", this, f),
	resetButton("clear", this, f),
	tokeniser(new Console::ConsoleTokeniser()),
	console(log, tokeniser)
{
	log.setDisableUndo(true);

	addAndMakeVisible(applyButton);
	addAndMakeVisible(resetButton);
	addAndMakeVisible(console);

	applyButton.setTooltip("Assign all layers that are not set to \"Skip\"");

	console.setReadOnly(true);
	console.setLineNumbersShown(false);
	console.setColour(CodeEditorComponent::backgroundColourId, Colour(0xFF262626));
	console.setColour(CodeEditorComponent::highlightColourId, Colour(0x33FFFFFF));

	sf.addScrollBarToAnimate(console.getScrollbar(false));

	setSize(10, TopBarHeight + ConsoleHeight);
	clearButton.setVisible(false);
}

void ComplexGroupManagerComponent::ParseToolbar::buttonClicked(Button* b)
{
	if(b == &applyButton)
	{
		logToConsole("\t" + msg);

		auto p = findParentComponentOfClass<ComplexGroupManagerComponent>();

		for(auto l: p->content.layerComponents)
		{
			if(l->parser.isVisible() && l->parser.ok)
			{
				auto msg = l->parser.performAction();
				msg << "\n";
				logToConsole(msg);
			}
		}
	}
	if(b == &resetButton)
	{
		auto p = findParentComponentOfClass<ComplexGroupManagerComponent>();

		Array<Identifier> layersToClear;

		for(auto l: p->content.layerComponents)
		{
			if(l->parser.isVisible())
			{
				
				layersToClear.add(Helpers::getId(l->data));
				logToConsole("\tReset layer " + layersToClear.getLast());
			}
		}

		
	}
}

void ComplexGroupManagerComponent::ParseToolbar::paint(Graphics& g)
{
	LayerComponent::paint(g);

	auto b = getLocalBounds().removeFromTop(TopBarHeight);

	Helpers::drawLabel(g, b.toFloat(), "Batch processor", Justification::centred);

	b.removeFromLeft(TopBarHeight);
	b.removeFromRight(TopBarHeight);

	
}

void ComplexGroupManagerComponent::ParseToolbar::resized()
{
	auto b = getLocalBounds();

	console.setBounds(b.removeFromBottom(ConsoleHeight + 2 * BodyMargin).reduced(BodyMargin));

	resetButton.setBounds(b.removeFromRight(b.getHeight()).reduced(5));
	applyButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(5));
}

int ComplexGroupManagerComponent::ParseToolbar::getHeightToUse() const
{
	return ConsoleHeight + TopBarHeight + 2 * BodyMargin;
}

void ComplexGroupManagerComponent::ParseToolbar::setLayersToParse(const Array<LogicTypeComponent*>& layers,
                                                                  int numSamples)
{
	msg = "";
	msg << "Assign " << String(numSamples) << " samples to layer";

	if(layers.size() > 1)
		msg << "s";

	msg << " ";

	for(auto l: layers)
	{
		msg << Helpers::getLogicTypeName(l->type) << "[" << String(Helpers::getLayerIndex(l->data)) << "] & ";
	}

	msg = msg.upToLastOccurrenceOf(" & ", false, false);

	applyButton.setTooltip(msg);

	if(layers.isEmpty())
		log.replaceAllContent({});

	setVisible(!layers.isEmpty());
	repaint();
}

void ComplexGroupManagerComponent::ParseToolbar::logToConsole(const String& message)
{
	CodeDocument::Position pos(log, log.getNumCharacters());
	log.insertText(pos, "\n" + message);
}

ComplexGroupManagerComponent::Content::Content(ComponentWithGroupManagerConnection& parent, const ValueTree& d):
  ComponentWithGroupManagerConnection(parent.getSampler()),
  data(d),
  addComponent(parent, d),
  parseToolbar(parent, d)
{
	addChildComponent(addComponent);
	addChildComponent(parseToolbar);
	updateSize();
}

void ComplexGroupManagerComponent::Content::updateSize()
{
	height = 0;

	Array<LogicTypeComponent*> lp;

	for(auto l: layerComponents)
	{
		height += l->getHeightToUse() + ItemMargin;

		if(l->parser.isVisible())
			lp.add(l);
	}

	parseToolbar.setLayersToParse(lp, getSampleEditHandler()->getNumSelected());
	//addComponent.setVisible(!parseToolbar.isVisible());
	
	if(parseToolbar.isVisible())
		height += parseToolbar.getHeightToUse();
	else if (addComponent.isVisible())
		height += addComponent.getHeightToUse();

	setSize(getWidth(), height);
	resized();

	if(auto p = findParentComponentOfClass<ComplexGroupManagerComponent>())
	{
		if(recursion)
			return;

		p->resized();
		p->repaint();
	}
		
}

void ComplexGroupManagerComponent::Content::removeLayer(const ValueTree& d)
{
	auto layerIndex = d == data ? -1 : data.indexOf(d);

	if(layerIndex == -1 && !PresetHandler::showYesNoWindow("Clear all layers", "Do you want to clear all layers"))
		return ;

	getUndoManager()->beginNewTransaction();

	if(layerIndex == -1)
	{
		ComplexGroupManager::ScopedUpdateDelayer sds(*getComplexGroupManager());
		data.removeAllChildren(getUndoManager());
	}
	else
		data.removeChild(layerIndex, getUndoManager());
}

void ComplexGroupManagerComponent::Content::resized()
{
	auto b = getLocalBounds();

	for(auto l: layerComponents)
	{
		l->setBounds(b.removeFromTop(l->getHeightToUse()));
		b.removeFromTop(ItemMargin);
	}

	if(parseToolbar.isVisible())
	{
		parseToolbar.setBounds(b.removeFromTop(parseToolbar.getHeightToUse()));
	}
	else
		addComponent.setBounds(b.removeFromTop(addComponent.getHeightToUse()));
}

ComplexGroupManagerComponent::Logger::Logger(const ValueTree& d, CodeDocument& doc_):
	doc(doc_),
	data(d)
{
	setRootValueTree(data);
	setMillisecondsBetweenUpdate(500);
	setEnableLogging(true);
}

void ComplexGroupManagerComponent::Logger::anythingChanged(CallbackType cb)
{
	auto xml = data.createXml();
	doc.replaceAllContent(xml->createDocument(""));
}

ComplexGroupManagerComponent::ComplexGroupManagerComponent(ModulatorSampler* s):
	ComponentWithGroupManagerConnection(s),
	SamplerSubEditor(s->getSampleEditHandler()),
	content(*this, getComplexGroupManager()->getDataTree()),
	editButton("edit", nullptr, f),
	moreButton("more", nullptr, f),
    parseButton("parse", nullptr, f),
	tagButton("count", nullptr, f) 
{
	editButton.onClick = [this]()
	{
		content.addComponent.setVisible(editButton.getToggleState());
		content.updateSize();
		this->repaint();
	};

	moreButton.onClick = [this]()
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(1, "Export as Base64");
		m.addItem(2, "Import from Base64");

		m.addSeparator();

		m.addItem(3, "Export as JSON");
		m.addItem(4, "Import from JSON");

		m.addSeparator();

		m.addItem(5, "Reset all samples");
		m.addItem(6, "Rebuild cache");
		m.addItem(7, "Remove all layers");

		if(auto r = m.show())
		{
			if(r == 1)
			{
				zstd::ZDefaultCompressor comp;
				MemoryBlock mb;
				comp.compress(content.data, mb);
				auto b64 = mb.toBase64Encoding();
				SystemClipboard::copyTextToClipboard(b64);
			}
			if(r == 2)
			{
				
				MemoryBlock mb;
				auto b64 = SystemClipboard::getTextFromClipboard();
				if(mb.fromBase64Encoding(b64))
				{
					ValueTree v;
					zstd::ZDefaultCompressor comp;
					comp.expand(mb, v);

					ComplexGroupManager::ScopedUpdateDelayer sds(*getComplexGroupManager());

					getUndoManager()->beginNewTransaction();
					content.data.removeAllChildren(getUndoManager());

					for(auto c: v)
						content.data.addChild(c.createCopy(), -1, getUndoManager());
				}
			}
			if(r == 5)
			{
				if(PresetHandler::showYesNoWindow("Confirm reset", "Do you want to clear the bitmask for all samples?"))
				{
					ModulatorSampler::SoundIterator iter(getSampler());

					while(auto s = iter.getNextSound())
						s->setBitmask(0);
				}
			}
			if(r == 6)
			{
				getComplexGroupManager()->refreshCache();
			}
			if(r == 7)
			{
				if(PresetHandler::showYesNoWindow("Confirm reset", "Do you want to remove all layers?"))
				{
					ComplexGroupManager::ScopedUpdateDelayer sds(*getComplexGroupManager());

					getUndoManager()->beginNewTransaction();
					content.data.removeAllChildren(getUndoManager());
				}
			}
		}
	};

	tagButton.onClick = [this]()
	{
		auto on = tagButton.getToggleState();
		Component::callRecursive<LogicTypeComponent>(&content, [on](LogicTypeComponent* lt)
		{
			lt->showSelectors(on);
			return false;
		});
	};

	parseButton.onClick = [this]()
	{
		auto on = parseButton.getToggleState();

		if(on)
		{
			editButton.setToggleState(false, sendNotification);
			tagButton.setToggleState(true, sendNotification);
		}

		Component::callRecursive<LogicTypeComponent>(&content, [on](LogicTypeComponent* lt)
		{
			lt->parser.setVisible(on);
			return false;
		});

		content.updateSize();
	};
	

	tagButton.setTooltip("Show / hide the number of samples that are assigned / unassigned to each layer group.");
	parseButton.setTooltip("Show / hide the tools that can be used to assign the current sample selection to layer groups.");

	addAndMakeVisible(editButton);
	addAndMakeVisible(moreButton);
	addAndMakeVisible(tagButton);
	addAndMakeVisible(parseButton);

	editButton.setToggleModeWithColourChange(true);
	tagButton.setToggleModeWithColourChange(true);
	parseButton.setToggleModeWithColourChange(true);

	s->getSampleMap()->addListener(this);

	addAndMakeVisible(viewport);
	viewport.setViewedComponent(&content, false);
	viewport.setScrollBarThickness(12);
	sf.addScrollBarToAnimate(viewport.getVerticalScrollBar());
}

ComplexGroupManagerComponent::~ComplexGroupManagerComponent()
{
	if(getSampler() != nullptr)
		getSampler()->getSampleMap()->removeListener(this);
}

void ComplexGroupManagerComponent::resized()
{
	auto b = getLocalBounds();

	auto topBar = b.removeFromTop(24);

	b.removeFromTop(viewport.getScrollBarThickness());

	editButton.setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(3));
	parseButton.setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(3));
	tagButton.setBounds(topBar.removeFromLeft(topBar.getHeight()).reduced(3));
	moreButton.setBounds(topBar.removeFromRight(topBar.getHeight()).reduced(3));

	b.removeFromLeft(viewport.getScrollBarThickness());
	viewport.setBounds(b);
	content.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), content.getHeight());
}

void ComplexGroupManagerComponent::soundsSelected(int numSelected)
{}

void ComplexGroupManagerComponent::samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id,
	const var& newValue)
{
	if(id == SampleIds::RRGroup)
		rebuildSampleCounters();

	ignoreUnused(s, id, newValue);
}

void ComplexGroupManagerComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF262626));

	auto b = getLocalBounds();

	

	if(editButton.getToggleState())
	{
		GlobalHiseLookAndFeel::draw1PixelGrid(g, this, b);
	}

	g.setColour(Colour(0xFF262626));
	auto tb = b.removeFromTop(24);
	g.fillRect(tb);
	GlobalHiseLookAndFeel::drawFake3D(g, tb);

	if(content.layerComponents.isEmpty())
	{
		g.setColour(Colour(0x33FFFFFF));
		g.setFont(GLOBAL_FONT());
		g.drawText("Click on the pen button to add logic layers", getLocalBounds().toFloat(), Justification::centred);
	}
}

void ComplexGroupManagerComponent::setXmlLogger(CodeDocument& doc)
{
	logger = new Logger(getComplexGroupManager()->getDataTree(), doc);
}

void ComplexGroupManagerComponent::setDisplayFilter(uint8 layerIndex, uint8 value)
{
	auto id = getComplexGroupManager()->getLayerId(layerIndex);

	if(value == 0)
	{
		for(int i = 0; i < displayFilters.size(); i++)
		{
			if(displayFilters[i].first == id)
			{
				displayFilters.remove(i);
				break;
			}
		}
	}
	else
	{
		bool found = false;

		for(int i = 0; i < displayFilters.size(); i++)
		{
			if(displayFilters[i].first == id)
			{
				displayFilters.getReference(i).second = value;
				found = true;
				break;
			}
		}

		if(!found)
			displayFilters.add({ id, value });
	}

	if(displayFilters.isEmpty())
	{
		getSampleEditHandler()->complexGroupBroadcaster.sendMessage(sendNotificationSync, {});
	}
	else
	{
		auto f = getComplexGroupManager()->getDisplayFilter(displayFilters);
		getSampleEditHandler()->complexGroupBroadcaster.sendMessage(sendNotificationSync, f);
	}
}

void ComplexGroupManagerComponent::rebuildSampleCounters()
{
	callRecursive<LogicTypeComponent>(this, [this](LogicTypeComponent* cb)
	{
		cb->showSelectors(tagButton.getToggleState());
		return false;
	});
}

void ComplexGroupManagerComponent::addSelectionFilter(const std::pair<uint8, uint8>& filter,
	ModifierKeys mods)
{
	auto add = mods.isShiftDown() || mods.isCommandDown();


	if(!add)
	{
		auto isSelected = isFilterSelected(filter);
		activeCountButtons.clear();

		if(!isSelected)
			activeCountButtons.add(filter);
	}
	else
	{
		auto isZero = filter.second == 0;

		for(int i = 0; i < activeCountButtons.size(); i++)
		{
			if(activeCountButtons[i].first == filter.first)
			{
				auto thisZero = activeCountButtons[i].second == 0;

				if(thisZero != isZero)
					activeCountButtons.remove(i--);
			}
		}

		activeCountButtons.addIfNotAlreadyThere(filter);
	}

	rebuildSelection();
}

bool ComplexGroupManagerComponent::isFilterSelected(const std::pair<uint8, uint8>& f) const
{
	return activeCountButtons.contains(f);
}

void ComplexGroupManagerComponent::rebuildSelection()
{
	auto& selection = getSampleEditHandler()->getSelectionReference();

	selection.deselectAll();

	if(!activeCountButtons.isEmpty())
	{
		std::map<uint8, std::vector<uint8>> filtersPerLayer;

		for(const auto& p: activeCountButtons)
		{
			filtersPerLayer[p.first].push_back(p.second);
		}

		std::map<Identifier, Array<WeakReference<SynthSoundWithBitmask>>> samplesPerLayer;

		for(const auto& p: filtersPerLayer)
		{
			auto isUnassigned = p.second.size() == 1 && p.second[0] == 0;

			auto id = getComplexGroupManager()->getLayerId(p.first);

			if(isUnassigned)
			{
				samplesPerLayer[id] = getComplexGroupManager()->getUnassignedSamples(p.first);
			}
			else
			{
				for(auto v: p.second)
					samplesPerLayer[id].addArray(getComplexGroupManager()->getFilteredSelection(p.first, v, v == ComplexGroupManager::IgnoreFlag));
			}
		}

		Array<SynthSoundWithBitmask*> newSelection;

		for(auto s: samplesPerLayer.begin()->second)
		{
			newSelection.add(s);
		}

		if(samplesPerLayer.size() >= 1)
		{
			bool first = true;

			for(const auto& sl: samplesPerLayer)
			{
				if(first)
				{
					first = false;
					continue;
				}

				for(int i = 0; i < newSelection.size(); i++)
				{
					if(!sl.second.contains(newSelection[i]))
					{
						newSelection.remove(i--);
					}
				}
			}
		}

		for(auto s: newSelection)
			selection.addToSelection(dynamic_cast<ModulatorSamplerSound*>(s));

		getSampleEditHandler()->setMainSelectionToLast();
	}

	using Count = LogicTypeComponent::BodyBase::SelectionTag;

	callRecursive<Count>(this, [](Count* c)
	{
		c->repaint();
		return false;
	});
}

ComplexGroupManagerFloatingTile::ComplexGroupActivator::ComplexGroupActivator(ModulatorSampler* s):
	b("purge", nullptr, f),
	sampler(s)
{
	addAndMakeVisible(b);
	b.setTooltip("Activate the complex group manager");

	b.onClick = [this]()
	{
		sampler->setUseComplexGroupManager(true);
		auto pc = findParentComponentOfClass<PanelWithProcessorConnection>();

		MessageManager::callAsync([pc]()
		{
			pc->refreshContent();
		});
	};
}

void ComplexGroupManagerFloatingTile::ComplexGroupActivator::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF262626));

	g.setColour(Colours::white.withAlpha(0.7f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Click to enable the complex group manager for this sampler", getLocalBounds().toFloat().reduced(10.0f), Justification::centredTop);
}

void ComplexGroupManagerFloatingTile::ComplexGroupActivator::resized()
{
	b.setBounds(getLocalBounds().withSizeKeepingCentre(48, 48));
}

Component* ComplexGroupManagerFloatingTile::createContentComponent(int)
{
	if(auto s = dynamic_cast<ModulatorSampler*>(getProcessor()))
	{
		if(auto g = s->getComplexGroupManager())
			return new ComplexGroupManagerComponent(s);
		else
			return new ComplexGroupActivator(s);
	}

	return nullptr;
}
}
