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

namespace hise { using namespace juce;

MouseCallbackComponent::MouseCallbackComponent() :
callbackLevel(CallbackLevel::NoCallbacks),
callbackLevels(getCallbackLevels()),
constrainer(new RectangleConstrainer())
{
	initMacroControl(dontSendNotification);

	for(int i = 0; i < (int)Action::Nothing; i++)
		clickInformation[i] = new DynamicObject();
}



StringArray MouseCallbackComponent::getCallbackLevels(bool getFileCallbacks)
{
	StringArray sa;

	if (getFileCallbacks)
	{
		sa.add("No Callbacks");
		sa.add("Drop Only");
		sa.add("Drop & Hover");
		sa.add("All Callbacks");
	}
	else
	{
		sa.add("No Callbacks");
		sa.add("Context Menu");
		sa.add("Clicks Only");
		sa.add("Clicks & Hover");
		sa.add("Clicks, Hover & Dragging");
		sa.add("All Callbacks");
	}

	return sa;
}

StringArray MouseCallbackComponent::getCallbackPropertyNames()
{
	StringArray sa;
	
	sa.add("mouseDownX");
	sa.add("mouseDownY");
	sa.add("x");
	sa.add("y");
	sa.add("clicked");
    sa.add("doubleClick");
	sa.add("rightClick");
	sa.add("mouseUp");
	sa.add("drag");
	sa.add("isDragOnly");
	sa.add("dragX");
	sa.add("dragY");
	sa.add("insideDrag");
	sa.add("hover");
	sa.add("result");
	sa.add("itemText");
    sa.add("shiftDown");
    sa.add("cmdDown");
    sa.add("altDown");
    sa.add("ctrlDown");
	

	return sa;
}

void MouseCallbackComponent::setPopupMenuItems(const StringArray &newItemList)
{
	itemList.clear();
	itemList.addArray(newItemList);
}



juce::PopupMenu MouseCallbackComponent::parseFromStringArray(const StringArray& itemList, Array<int> activeIndexes, LookAndFeel* laf)
{
	return SubmenuComboBox::parseFromStringArray(itemList, activeIndexes, laf);
}

void MouseCallbackComponent::setUseRightClickForPopup(bool shouldUseRightClickForPopup)
{
	useRightClickForPopup = shouldUseRightClickForPopup;
}

void MouseCallbackComponent::alignPopup(bool shouldBeAligned)
{
	popupShouldBeAligned = shouldBeAligned;
}

void MouseCallbackComponent::setDraggingEnabled(bool shouldBeEnabled)
{
	draggingEnabled = shouldBeEnabled;
}

void MouseCallbackComponent::setDragBounds(Rectangle<int> newDraggingBounds, RectangleConstrainer::Listener* listener)
{
	constrainer->draggingBounds = newDraggingBounds;
	constrainer->addListener(listener);
}

void MouseCallbackComponent::addMouseCallbackListener(Listener *l)
{
	listenerList.addIfNotAlreadyThere(l);
}

void MouseCallbackComponent::removeCallbackListener(Listener *l)
{
	listenerList.removeAllInstancesOf(l);
}

void MouseCallbackComponent::removeAllCallbackListeners()
{
	listenerList.clear();
}

void MouseCallbackComponent::mouseDoubleClick(const MouseEvent &event)
{
    if (callbackLevel < CallbackLevel::ClicksOnly) return;
    
    sendMessage(event, Action::DoubleClicked);
}

void MouseCallbackComponent::setEnableFileDrop(const String& newCallbackLevel, const String& allowedWildcards)
{
	if (newCallbackLevel.isEmpty() || allowedWildcards.isEmpty())
	{
		fileCallbackLevel = FileCallbackLevel::NoCallbacks;
		fileDropExtensions.clear();
		return;
	}

	fileCallbackLevel = (FileCallbackLevel)getCallbackLevels(true).indexOf(newCallbackLevel);

	if (fileCallbackLevel > FileCallbackLevel::NoCallbacks)
	{
		fileDropExtensions.clear();
		fileDropExtensions.addTokens(allowedWildcards, ";,", "\"'");
		fileDropExtensions.trim();
		fileDropExtensions.removeEmptyStrings();
	}
}

void MouseCallbackComponent::mouseDown(const MouseEvent& event)
{
	CHECK_MIDDLE_MOUSE_DOWN(event);

	ignoreMouseUp = false;
	startTouch(event.getMouseDownPosition());

	if (midiLearnEnabled && event.mods.isRightButtonDown())
	{
#if USE_FRONTEND
		enableMidiLearnWithPopup();
#else
		const bool isOnPreview = findParentComponentOfClass<FloatingTilePopup>() != nullptr;

		if (isOnPreview)
			enableMidiLearnWithPopup();
#endif

		return;
	}

	if (draggingEnabled)
	{
		dragger.startDraggingComponent(this, event);
		setAlwaysOnTop(true);
	}

	if (callbackLevel < CallbackLevel::PopupMenuOnly) return;

	if (itemList.size() != 0)
	{
		if (event.mods.isRightButtonDown() == useRightClickForPopup)
		{
			fillPopupMenu(event);

			return;
		}
	}

	if (callbackLevel > CallbackLevel::PopupMenuOnly)
	{
		sendMessage(event, Action::Clicked);
	}

	if (jsonPopupData.isObject())
	{
		if(findParentComponentOfClass<FloatingTilePopup>() == nullptr) // Deactivate this function in popups...
		{
			if (currentPopup.getComponent() != nullptr)
			{
				findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(nullptr, this, popupSize.getPosition());
				currentPopup = nullptr;
			}
			else
			{
#if USE_BACKEND
				auto mc = GET_BACKEND_ROOT_WINDOW(this)->getBackendProcessor();
#else
				auto mc = dynamic_cast<MainController*>(findParentComponentOfClass<FrontendProcessorEditor>()->getAudioProcessor());
#endif

				FloatingTile *t = new FloatingTile(mc, nullptr, jsonPopupData);
				t->setOpaque(false);

				t->setName(t->getCurrentFloatingPanel()->getBestTitle());

				t->setSize(popupSize.getWidth(), popupSize.getHeight());
				currentPopup = findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(t, this, popupSize.getPosition());
			}
		}
	}
}

void MouseCallbackComponent::touchAndHold(Point<int> downPosition)
{
	ignoreMouseUp = true;

	if (midiLearnEnabled)
	{
#if USE_FRONTEND
		enableMidiLearnWithPopup();
#else
		const bool isOnPreview = findParentComponentOfClass<FloatingTilePopup>() != nullptr;

		if (isOnPreview)
			enableMidiLearnWithPopup();
#endif
		return;
	}

	Point<float> newPos((float)downPosition.getX(), (float)downPosition.getY());
	ModifierKeys mods = ModifierKeys(ModifierKeys::rightButtonModifier);
	MouseEvent e(Desktop::getInstance().getMainMouseSource(), newPos, mods, 0.0f, 0.0, 0.0, 0.0, 0.0f, this, this, Time(), newPos, Time(), 1, false);

	if (callbackLevel < CallbackLevel::PopupMenuOnly) return;

	if (itemList.size() != 0)
	{
		if (useRightClickForPopup)
		{
			fillPopupMenu(e);

			return;
		}
	}

	if (callbackLevel > CallbackLevel::PopupMenuOnly)
	{
		sendMessage(e, Action::Clicked);
	}
}



void MouseCallbackComponent::fillPopupMenu(const MouseEvent &event)
{
	auto& plaf = getProcessor()->getMainController()->getGlobalLookAndFeel();

	auto m = parseFromStringArray(itemList, { activePopupId }, &plaf);
	
	ScopedValueSetter<bool>(currentlyShowingPopup, true, false);

	auto result = PopupLookAndFeel::showAtComponent(m, this, popupShouldBeAligned);
    
	String itemName = result != 0 ? itemList[result - 1] : "";

	auto obj = new DynamicObject();

	static const Identifier r("result");
	static const Identifier itemText("itemText");
	static const Identifier rightClick("rightClick");

	obj->setProperty(rightClick, event.mods.isRightButtonDown());
	obj->setProperty(r, result);
	obj->setProperty(itemText, itemName);

	sendToListeners(var(obj));
}

bool MouseCallbackComponent::isInterestedInFileDrag(const StringArray& files)
{
	if (fileCallbackLevel == FileCallbackLevel::NoCallbacks)
		return false;
	
	if (fileDropExtensions.isEmpty())
		return false;

	if (files.size() > 1)
		return false;

	File f(files[0]);

	for (auto& ex : fileDropExtensions)
	{
		if (files[0].matchesWildcard(ex, true))
			return true;
	}

	return false;
}

void MouseCallbackComponent::fileDragEnter(const StringArray& files, int x, int y)
{
	sendFileMessage(Action::FileEnter, files[0], Point<int>(x, y));
}

void MouseCallbackComponent::fileDragMove(const StringArray& files, int x, int y)
{
	sendFileMessage(Action::FileMove, files[0], Point<int>(x, y));
}

void MouseCallbackComponent::fileDragExit(const StringArray& files)
{
	sendFileMessage(Action::FileExit, files[0], Point<int>());
}

void MouseCallbackComponent::filesDropped(const StringArray& files, int x, int y)
{
	sendFileMessage(Action::FileDrop, files[0], Point<int>(x, y));
}

void MouseCallbackComponent::setAllowCallback(const String &newCallbackLevel) noexcept
{
	const int index = callbackLevels.indexOf(newCallbackLevel);

	callbackLevel = index != -1 ? (CallbackLevel)index : CallbackLevel::NoCallbacks;
}

MouseCallbackComponent::CallbackLevel MouseCallbackComponent::getCallbackLevel() const
{
	return callbackLevel;
}

void MouseCallbackComponent::mouseMove(const MouseEvent& event)
{
	if (callbackLevel < CallbackLevel::AllCallbacks) return;

	sendMessage(event, Action::Moved, EnterState::Nothing);
}



void MouseCallbackComponent::mouseDrag(const MouseEvent& event)
{
	CHECK_MIDDLE_MOUSE_DRAG(event);

	if (draggingEnabled)
	{
		dragger.dragComponent(this, event, constrainer);
	}

	if (callbackLevel < CallbackLevel::Drag) return;

	sendMessage(event, Action::Dragged);
}

void MouseCallbackComponent::mouseEnter(const MouseEvent &event)
{
	if (callbackLevel < CallbackLevel::ClicksAndEnter) return;

	sendMessage(event, Action::Moved, Entered);
}

void MouseCallbackComponent::mouseExit(const MouseEvent &event)
{
	if (callbackLevel < CallbackLevel::ClicksAndEnter) return;

	sendMessage(event, Action::Moved, Exited);
}

void MouseCallbackComponent::mouseUp(const MouseEvent &event)
{
	CHECK_MIDDLE_MOUSE_UP(event);

	abortTouch();

	if (draggingEnabled)
	{
		setAlwaysOnTop(false);
	}

	if (isTouchEnabled() && ignoreMouseUp)		   return;
	if (currentlyShowingPopup) return;
	if (callbackLevel < CallbackLevel::ClicksOnly) return;

	sendMessage(event, Action::MouseUp);
}

void MouseCallbackComponent::sendFileMessage(Action a, const String& f, Point<int> pos)
{
	FileCallbackLevel requiredLevel = FileCallbackLevel::NoCallbacks;

	switch (a)
	{
	case Action::FileDrop: requiredLevel = FileCallbackLevel::DropOnly; break;
	case Action::FileEnter:
	case Action::FileExit: requiredLevel = FileCallbackLevel::DropHover; break;
	case Action::FileMove: requiredLevel = FileCallbackLevel::AllCallbacks; break;
    default: break;
	}

	if (fileCallbackLevel < requiredLevel)
		return;

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier hover("hover");
	static const Identifier drop("drop");
	static const Identifier file("fileName");

	auto e = new DynamicObject();
	var fileInformation(e);

	e->setProperty(x, pos.getX());
	e->setProperty(y, pos.getY());
	e->setProperty(hover, a != Action::FileExit);
	e->setProperty(drop, a == Action::FileDrop);
	e->setProperty(file, f);

	

	for(auto l: listenerList)
	{
		l->fileDropCallback(fileInformation);
	}
}



void MouseCallbackComponent::fillMouseCallbackObject(var& clickInformation, Component* c, const MouseEvent& event, CallbackLevel callbackLevel, Action action, EnterState state)
{
	auto e = clickInformation.getDynamicObject();

	if(e == nullptr)
	{
		auto e = new DynamicObject();
		clickInformation = var(e);
	}
	
	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier clicked("clicked");
	static const Identifier doubleClick("doubleClick");
	static const Identifier rightClick("rightClick");
	static const Identifier drag("drag");
	static const Identifier isDragOnly("isDragOnly");
	static const Identifier dragX("dragX");
	static const Identifier dragY("dragY");
	static const Identifier insideDrag("insideDrag");
	static const Identifier hover("hover");
	static const Identifier mouseDownX("mouseDownX");
	static const Identifier mouseDownY("mouseDownY");
	static const Identifier mouseUp("mouseUp");
	static const Identifier shiftDown("shiftDown");
	static const Identifier cmdDown("cmdDown");
	static const Identifier altDown("altDown");
	static const Identifier ctrlDown("ctrlDown");

	if (callbackLevel >= CallbackLevel::ClicksOnly)
	{
		e->setProperty(clicked, action == Action::Clicked);
		e->setProperty(doubleClick, action == Action::DoubleClicked);
		e->setProperty(rightClick, ((action == Action::Clicked || action == Action::Dragged || action == Action::DoubleClicked) && event.mods.isRightButtonDown()) ||
			(action == Action::MouseUp && event.mods.isRightButtonDown()));
		e->setProperty(mouseUp, action == Action::MouseUp);
		e->setProperty(mouseDownX, event.getMouseDownX());
		e->setProperty(mouseDownY, event.getMouseDownY());
		e->setProperty(x, event.getPosition().getX());
		e->setProperty(y, event.getPosition().getY());
		e->setProperty(shiftDown, event.mods.isShiftDown());
		e->setProperty(cmdDown, event.mods.isCommandDown());
		e->setProperty(altDown, event.mods.isAltDown());
		e->setProperty(ctrlDown, event.mods.isCtrlDown());
	}

	if (callbackLevel >= CallbackLevel::ClicksAndEnter)
	{
		e->setProperty(hover, state != Exited);
	}

	if (callbackLevel >= CallbackLevel::Drag)
	{
		const bool isIn = c->getLocalBounds().contains(event.position.toInt());

		e->setProperty(insideDrag, isIn ? 1 : 0);
		e->setProperty(drag, action == Action::Dragged);
		e->setProperty(isDragOnly, (event.getDistanceFromDragStartX() != 0) || (event.getDistanceFromDragStartY() != 0));
		e->setProperty(dragX, event.getDistanceFromDragStartX());
		e->setProperty(dragY, event.getDistanceFromDragStartY());
	}
}

void MouseCallbackComponent::sendMessage(const MouseEvent &e, Action action, EnterState state)
{
	if (callbackLevel == CallbackLevel::NoCallbacks) 
		return;

	dispatch::StringBuilder n;

	n << "panel mouse callback for " << Component::getName() << ": [" << getCallbackLevelAsIdentifier(callbackLevel) << ", " << getActionAsIdentifier(action) << "]";
	
	TRACE_EVENT("component", DYNAMIC_STRING_BUILDER(n));

	fillMouseCallbackObject(clickInformation[(int)action], this, e, callbackLevel, action, state);

	sendToListeners(clickInformation[(int)action]);
}

void MouseCallbackComponent::sendToListeners(var clickInformation)
{
	ScopedLock sl(listenerList.getLock());

	for (int i = 0; i < listenerList.size(); i++)
	{
		if (listenerList[i].get() != nullptr)
		{
			listenerList[i]->mouseCallback(clickInformation);
		}
	}
}

bool DrawActions::PostActionBase::needsStackData() const
{ return false; }

DrawActions::ActionBase::ActionBase()
{}

DrawActions::ActionBase::~ActionBase()
{}

bool DrawActions::ActionBase::wantsCachedImage() const
{ return false; }

bool DrawActions::ActionBase::wantsToDrawOnParent() const
{ return false; }

void DrawActions::ActionBase::setCachedImage(Image& actionImage_, Image& mainImage_)
{ actionImage = actionImage_; mainImage = mainImage_; }

void DrawActions::ActionBase::setScaleFactor(float sf)
{ scaleFactor = sf; }

DrawActions::MarkdownAction::MarkdownAction(const MarkdownLayout::StringWidthFunction& f):
	renderer("", f)
{}

void DrawActions::MarkdownAction::perform(Graphics& g)
{
	ScopedLock sl(lock);
	renderer.draw(g, area);
}

DrawActions::ActionLayer::ActionLayer(bool drawOnParent_):
	ActionBase(),
	drawOnParent(drawOnParent_)
{}

bool DrawActions::ActionLayer::wantsCachedImage() const
{ 
	if(postActions.size() > 0)
		return true;

	for (auto a : internalActions)
	{
		if (a->wantsCachedImage())
			return true;
	}

	return false;
}

bool DrawActions::ActionLayer::wantsToDrawOnParent() const
{ return drawOnParent; }

void DrawActions::ActionLayer::setCachedImage(Image& actionImage_, Image& mainImage_)
{ 
	ActionBase::setCachedImage(actionImage_, mainImage_);

	// do not propagate the main image
	for (auto a : internalActions)
		a->setCachedImage(actionImage_, actionImage_);
}

void DrawActions::ActionLayer::setScaleFactor(float sf)
{ 
	ActionBase::setScaleFactor(sf);

	for (auto a : internalActions)
		a->setScaleFactor(sf);
}

void DrawActions::ActionLayer::perform(Graphics& g)
{
	for (auto action : internalActions)
		action->perform(g);
			
	if (postActions.size() > 0)
	{
		PostGraphicsRenderer r(stack, actionImage, scaleFactor);
		int numDataRequired = 0;

		for (auto p : postActions)
		{
			if (p->needsStackData())
				numDataRequired++;
		}
				
		r.reserveStackOperations(numDataRequired);

		for (auto p : postActions)
			p->perform(r);
	}
}

void DrawActions::ActionLayer::addDrawAction(ActionBase* a)
{
	internalActions.add(a);
}

void DrawActions::ActionLayer::addPostAction(PostActionBase* a)
{
	postActions.add(a);
}

DrawActions::BlendingLayer::BlendingLayer(gin::BlendMode m, float alpha_):
	ActionLayer(true),
	blendMode(m),
	alpha(alpha_)
{

}

bool DrawActions::BlendingLayer::wantsCachedImage() const
{ return true; }

void DrawActions::NoiseMapManager::drawNoiseMap(Graphics& g, Rectangle<int> area, float alpha, bool monochrom,
	float scale)
{
	auto originalArea = area;

    //scale *= scaleFactor;
    
	if(scale != 1.0f)
		area = area.transformed(AffineTransform::scale(scale));

	const auto& m = getNoiseMap(area, monochrom);

    g.saveState();
    
	g.setColour(Colours::black.withAlpha(alpha));

    g.setImageResamplingQuality(Graphics::ResamplingQuality::lowResamplingQuality);
    
	if (scale != 1.0f)
		g.drawImageWithin(m.img, originalArea.getX(), originalArea.getY(), originalArea.getWidth(), originalArea.getHeight(), RectanglePlacement::stretchToFit);
	else
		g.drawImageAt(m.img, area.getX(), area.getY());
    
    g.restoreState();
}

DrawActions::NoiseMapManager::NoiseMap& DrawActions::NoiseMapManager::getNoiseMap(Rectangle<int> area, bool monochrom)
{
	for (auto m : maps)
	{

		if (area.getWidth() == m->width &&
			area.getHeight() == m->height &&
			monochrom == m->monochrom)
		{
			return *m;
		}
	}

    dispatch::StringBuilder n;
    n << "create noisemap [" << area.getWidth() << ", " << area.getHeight() << "]";
    
    TRACE_EVENT("drawactions", DYNAMIC_STRING_BUILDER(n));
    
	maps.add(new NoiseMap(area, monochrom));

	return *maps.getLast();
}

DrawActions::Handler::Iterator::Iterator(Handler* handler_):
	handler(handler_)
{
	if (handler != nullptr)
	{
		actionsInIterator.ensureStorageAllocated(handler->nextActions.size());
		SpinLock::ScopedLockType sl(handler->lock);

		actionsInIterator.addArray(handler->nextActions);
	}
}

DrawActions::ActionBase::Ptr DrawActions::Handler::Iterator::getNextAction()
{
	if (index < actionsInIterator.size())
		return actionsInIterator[index++];

	return nullptr;
}

bool DrawActions::Handler::Iterator::wantsCachedImage() const
{
	for (auto action : actionsInIterator)
		if (action != nullptr && action->wantsCachedImage())
			return true;

	return false;
}

bool DrawActions::Handler::Iterator::wantsToDrawOnParent() const
{
	for (auto action : actionsInIterator)
		if (action != nullptr && action->wantsToDrawOnParent())
			return true;

	return false;
}

DrawActions::Handler::Listener::~Listener()
{}

DrawActions::Handler::~Handler()
{
	cancelPendingUpdate();
}

void DrawActions::Handler::beginDrawing()
{
	currentActions.clear();
}

void DrawActions::Handler::beginLayer(bool drawOnParent)
{
	auto newLayer = new ActionLayer(drawOnParent);

	addDrawAction(newLayer);
	layerStack.insert(-1, newLayer);
}

DrawActions::ActionLayer::Ptr DrawActions::Handler::getCurrentLayer()
{
	return layerStack.getLast();
}

void DrawActions::Handler::endLayer()
{
	layerStack.removeLast();
}

void DrawActions::Handler::addDrawAction(ActionBase* newDrawAction)
{
	if (layerStack.getLast() != nullptr)
		layerStack.getLast()->addDrawAction(newDrawAction);
	else
		currentActions.add(newDrawAction);
}

void DrawActions::Handler::flush(uint64_t perfettoTrackId)
{
	{
		SpinLock::ScopedLockType sl(lock);

		nextActions.swapWith(currentActions);
		currentActions.clear();
		layerStack.clear();
	}

	if(perfettoTrackId != 0)
		flowManager.continueFlow(perfettoTrackId, "flush draw handler");

	triggerAsyncUpdate();
}

void DrawActions::Handler::logError(const String& message)
{
	if (errorLogger)
		errorLogger(message);
}

void DrawActions::Handler::addDrawActionListener(Listener* l)
{ listeners.addIfNotAlreadyThere(l); }

void DrawActions::Handler::removeDrawActionListener(Listener* l)
{ listeners.removeAllInstancesOf(l); }

void DrawActions::Handler::setGlobalBounds(Rectangle<int> gb, Rectangle<int> tb, float sf)
{
	globalBounds = gb;
	topLevelBounds = tb;
	scaleFactor = sf;
}

Rectangle<int> DrawActions::Handler::getGlobalBounds() const
{ return globalBounds; }

float DrawActions::Handler::getScaleFactor() const
{ return scaleFactor; }

DrawActions::NoiseMapManager* DrawActions::Handler::getNoiseMapManager()
{ return &noiseManager.getObject(); }

void DrawActions::Handler::handleAsyncUpdate()
{
	auto x = flowManager.flushAllButLastOne("flush draw handler", {});

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->newPaintActionsAvailable(x);
	}
}

BorderPanel::BorderPanel(DrawActions::Handler* handler_) :
borderColour(Colours::black),
drawHandler(handler_),
c1(Colours::white),
c2(Colours::white),
borderRadius(0.0f),
borderSize(1.0f)
{
	addAndMakeVisible(closeButton);
	
	drawHandler->addDrawActionListener(this);

	closeButton.addListener(this);

	static const unsigned char popupData[] =
	{ 137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,48,0,0,0,48,8,6,0,0,0,87,2,249,135,0,0,0,4,103,65,77,65,0,0,177,143,11,252,97,5,0,0,0,32,99,72,82,77,0,0,122,38,0,0,128,132,0,0,250,0,0,0,128,232,0,0,117,48,0,0,234,96,0,0,58,152,0,0,23,112,156,186,
		81,60,0,0,0,6,98,75,71,68,0,0,0,0,0,0,249,67,187,127,0,0,0,9,112,72,89,115,0,0,11,18,0,0,11,18,1,210,221,126,252,0,0,10,86,73,68,65,84,104,222,213,154,125,76,84,103,22,135,159,247,206,7,87,113,168,85,70,28,88,69,86,171,91,235,224,160,75,21,65,210,53,
		139,180,118,77,154,173,145,53,88,106,130,109,108,218,77,91,181,146,40,182,238,150,110,45,141,77,235,198,255,176,17,108,249,104,194,134,186,180,1,187,73,141,8,152,90,190,89,88,108,171,142,148,65,190,169,99,117,152,97,238,221,63,230,14,139,20,144,177,84,
		179,39,185,201,29,238,157,119,158,223,57,231,61,239,23,240,127,110,98,58,218,56,125,250,180,105,249,242,229,9,6,131,33,81,167,211,61,44,132,120,8,8,3,102,105,239,220,0,186,84,85,253,198,235,245,182,122,60,158,138,150,150,150,138,141,27,55,58,1,245,126,
		8,16,241,241,241,186,162,162,162,100,89,150,159,21,66,252,1,152,17,96,27,183,84,85,45,117,185,92,185,41,41,41,229,149,149,149,222,187,17,19,168,0,1,72,29,29,29,127,12,10,10,202,20,66,68,3,40,138,162,212,212,212,180,126,254,249,231,61,245,245,245,114,
		79,79,207,92,183,219,29,170,170,234,76,0,33,196,77,163,209,216,107,54,155,251,108,54,155,107,211,166,77,230,213,171,87,63,44,73,146,4,160,170,106,227,208,208,208,95,35,34,34,74,0,37,16,33,129,8,144,234,234,234,150,46,88,176,224,239,66,136,223,3,116,118,
		118,118,102,103,103,183,85,85,85,45,85,20,37,60,16,79,72,146,228,88,183,110,221,197,125,251,246,45,179,88,44,22,77,200,191,218,219,219,255,28,19,19,115,81,19,50,45,2,4,160,187,114,229,202,159,76,38,211,49,32,100,112,112,176,127,207,158,61,205,141,141,
		141,107,1,99,128,81,28,107,238,232,232,232,243,71,142,28,89,49,123,246,236,57,192,117,167,211,249,210,162,69,139,10,128,59,166,149,116,39,248,168,168,40,163,195,225,200,50,153,76,39,129,144,194,194,194,234,228,228,100,26,27,27,19,167,1,30,192,216,216,
		216,152,152,156,156,76,65,65,65,53,16,98,50,153,242,28,14,71,86,84,84,148,241,78,78,158,236,161,136,138,138,50,86,87,87,191,111,48,24,118,121,189,222,225,244,244,244,170,230,230,230,196,105,128,158,208,86,172,88,113,246,248,241,227,235,116,58,157,222,
		227,241,228,196,197,197,189,116,249,242,101,55,19,68,98,34,1,2,48,116,116,116,252,77,150,229,61,46,151,235,230,211,79,63,253,239,174,174,174,216,95,18,222,111,97,97,97,23,138,139,139,31,145,101,121,166,203,229,58,18,17,17,177,31,240,140,39,98,188,20,
		18,128,161,181,181,53,85,150,229,61,94,175,119,120,203,150,45,205,247,10,30,160,171,171,43,118,203,150,45,205,94,175,119,88,150,229,61,109,109,109,169,128,129,113,28,62,158,0,93,113,113,241,242,121,243,230,189,15,144,158,158,94,125,237,218,181,71,239,
		21,188,223,174,93,187,246,104,122,122,122,53,64,104,104,232,7,197,197,197,203,1,221,157,4,72,128,156,144,144,112,4,8,201,207,207,175,110,106,106,90,175,170,42,247,227,106,106,106,90,255,209,71,31,85,1,38,141,73,30,203,44,198,220,27,107,107,107,83,34,
		35,35,115,7,7,7,251,147,146,146,80,20,101,206,189,246,254,109,128,66,12,156,62,125,90,153,51,103,206,92,187,221,254,236,170,85,171,138,128,145,78,61,90,141,100,50,153,130,23,44,88,176,23,224,229,151,95,110,190,223,240,0,170,170,62,248,234,171,175,182,
		0,44,92,184,112,175,201,100,10,30,205,237,191,17,128,161,180,180,52,73,146,36,107,71,71,135,163,185,185,57,238,126,195,251,173,185,185,121,109,71,71,135,67,8,97,253,236,179,207,54,50,170,67,251,5,72,192,140,37,75,150,164,0,100,103,103,95,84,85,213,48,
		149,60,77,76,76,164,161,161,129,55,222,120,195,239,177,73,47,33,4,89,89,89,212,213,213,17,31,31,63,213,254,96,200,206,206,190,8,176,120,241,226,173,248,38,142,210,104,1,186,164,164,36,179,44,203,79,40,138,162,84,86,86,46,155,170,119,142,29,59,70,72,72,
		8,59,118,236,32,43,43,11,33,38,30,27,37,73,226,240,225,195,164,166,166,50,123,246,108,142,30,61,58,229,40,84,86,86,46,83,20,69,145,101,249,137,164,164,36,51,90,69,146,180,80,4,189,242,202,43,107,0,185,166,166,166,85,81,20,203,84,27,254,244,211,79,71,
		238,83,83,83,39,20,225,135,223,186,117,235,200,223,74,74,74,166,44,64,81,20,75,77,77,77,43,32,107,172,65,128,144,52,17,65,139,23,47,126,20,224,212,169,83,61,83,110,21,200,204,204,164,168,168,104,82,17,227,193,231,229,229,241,230,155,111,6,242,83,148,
		150,150,246,0,104,172,65,128,164,215,4,200,33,33,33,15,1,212,215,215,203,170,58,245,117,133,170,170,100,100,100,0,144,146,146,50,34,66,85,85,50,51,51,17,66,140,11,255,250,235,175,19,200,239,0,212,214,214,202,0,26,171,236,23,160,7,100,131,193,16,5,208,
		211,211,19,26,80,171,19,136,216,190,125,59,0,70,163,113,90,224,71,179,105,172,50,160,215,227,235,12,70,33,68,40,128,219,237,190,171,218,63,153,136,233,128,31,205,166,177,26,1,157,164,9,48,8,33,102,105,32,179,238,170,245,81,34,10,11,11,127,242,44,55,55,
		247,103,193,143,102,211,88,13,128,78,143,182,226,26,13,241,115,76,8,129,182,212,189,205,116,58,221,72,93,159,38,211,161,85,33,1,72,170,170,222,208,0,110,220,109,139,146,36,145,157,157,125,91,206,251,109,251,246,237,188,245,214,91,147,142,19,83,112,206,
		13,0,141,85,242,11,0,192,235,245,246,1,24,141,198,254,233,130,207,205,205,165,160,160,96,218,68,248,217,252,172,224,171,64,42,160,12,13,13,217,245,122,253,67,102,179,185,183,189,189,125,201,116,192,31,60,120,112,228,243,182,109,219,70,68,168,170,202,
		129,3,7,2,78,39,179,217,220,11,44,25,26,26,178,163,109,191,72,154,0,111,119,119,183,29,96,229,202,149,174,64,225,223,125,247,221,113,225,253,57,159,145,145,113,91,36,158,121,230,153,187,138,68,76,76,140,11,64,99,245,250,5,120,1,79,69,69,197,55,0,155,
		55,111,14,104,28,120,251,237,183,39,132,247,219,68,34,14,29,58,20,144,128,205,155,55,155,1,52,86,15,224,21,248,102,118,17,145,145,145,171,106,107,107,115,21,69,49,174,94,189,186,107,170,243,161,150,150,22,76,38,19,0,39,78,156,152,180,84,10,33,120,231,
		157,119,70,210,169,183,183,151,152,152,152,41,193,75,146,212,89,83,83,19,38,73,146,123,213,170,85,207,218,237,246,90,160,67,2,134,1,151,221,110,31,232,235,235,251,82,146,36,41,62,62,190,109,170,203,190,93,187,118,49,56,56,72,78,78,14,7,15,30,68,81,148,
		9,223,85,20,133,125,251,246,145,151,151,71,127,127,63,47,188,240,194,148,151,151,241,241,241,109,146,36,73,3,3,3,95,218,237,246,1,192,5,12,251,199,128,7,129,37,135,15,31,222,248,220,115,207,253,197,225,112,116,62,254,248,227,115,153,158,141,171,233,48,
		119,89,89,89,95,120,120,184,37,39,39,231,80,70,70,70,57,240,45,48,32,225,235,205,67,192,245,3,7,14,124,227,241,120,90,194,195,195,45,54,155,237,252,253,166,246,155,213,106,61,31,30,30,110,241,120,60,45,251,247,239,191,8,92,215,152,21,127,21,26,2,156,
		94,175,183,183,176,176,240,20,192,209,163,71,87,8,33,238,106,76,152,78,19,66,244,31,59,118,108,5,64,81,81,209,63,181,49,192,169,49,171,163,247,89,4,16,84,86,86,166,123,241,197,23,205,33,33,33,203,130,131,131,235,42,43,43,23,220,79,1,187,119,239,174,91,
		187,118,237,210,27,55,110,84,37,39,39,151,3,87,128,110,124,125,64,245,143,196,10,112,11,232,3,186,118,238,220,89,10,56,211,210,210,214,69,71,71,87,220,47,120,171,213,90,177,99,199,142,117,128,243,249,231,159,47,5,186,52,198,91,26,243,200,154,88,197,87,
		87,175,3,157,95,124,241,197,213,143,63,254,248,56,192,137,19,39,214,88,44,150,175,238,53,188,197,98,249,42,55,55,119,13,64,126,126,254,135,229,229,229,118,160,83,99,28,217,39,253,201,198,22,16,10,44,6,150,159,57,115,38,209,106,181,110,115,185,92,55,159,
		124,242,201,150,238,238,238,223,222,11,248,176,176,176,11,165,165,165,143,200,178,60,179,169,169,169,224,177,199,30,59,11,180,0,223,1,189,76,176,177,229,143,194,15,128,3,184,186,97,195,134,175,47,93,186,244,15,89,150,103,150,151,151,175,180,90,173,21,
		191,244,118,162,213,106,173,40,43,43,179,201,178,60,243,202,149,43,37,27,54,108,248,26,184,170,49,253,192,152,93,234,177,19,119,5,95,231,232,3,236,138,162,92,94,179,102,205,87,13,13,13,249,122,189,222,144,159,159,191,254,181,215,94,171,146,36,105,218,
		171,147,36,73,253,123,247,238,173,202,207,207,95,175,215,235,13,13,13,13,249,177,177,177,231,21,69,185,12,216,53,38,23,99,142,158,116,227,180,165,226,155,31,13,3,94,85,85,201,205,205,117,207,159,63,255,91,155,205,182,204,102,179,45,73,73,73,185,85,83,
		83,115,161,171,171,203,50,65,27,129,152,59,58,58,186,242,147,79,62,153,23,23,23,183,20,184,126,242,228,201,15,183,109,219,214,166,170,234,119,90,218,116,226,59,170,29,30,251,229,73,15,56,128,7,128,112,224,215,192,162,196,196,68,75,94,94,94,130,201,100,
		138,3,112,56,28,157,89,89,89,23,207,157,59,183,76,81,148,249,1,122,252,90,66,66,66,91,102,102,230,210,240,240,112,11,128,211,233,172,78,75,75,59,119,246,236,217,78,124,229,242,18,19,164,206,157,4,140,22,17,130,239,208,58,18,88,8,88,178,178,178,66,211,
		211,211,127,103,52,26,31,6,223,49,235,133,11,23,90,75,74,74,122,234,235,235,229,238,238,238,185,110,183,219,172,170,106,48,128,16,226,71,163,209,216,51,111,222,188,62,155,205,230,122,234,169,167,204,177,177,177,35,199,172,110,183,187,229,248,241,227,
		103,50,51,51,123,53,111,95,213,210,166,139,49,85,39,16,1,163,69,4,3,115,181,104,252,10,176,72,146,52,103,247,238,221,33,59,119,238,252,141,217,108,78,192,183,205,17,136,185,122,122,122,206,229,228,228,252,231,189,247,222,187,174,40,74,191,6,255,189,230,
		245,62,224,199,201,224,167,34,192,255,142,78,3,124,0,95,153,181,104,81,9,5,30,8,13,13,157,153,150,150,54,99,211,166,77,161,145,145,145,97,38,147,41,66,175,215,135,10,33,130,1,84,85,253,113,120,120,184,207,233,116,126,223,222,222,222,85,90,90,218,155,
		151,151,119,171,183,183,247,166,150,30,189,154,183,59,181,251,31,240,117,216,59,30,179,6,116,208,173,69,99,38,190,180,154,171,9,152,131,111,54,59,75,123,102,212,222,211,241,191,42,167,104,48,30,124,53,252,38,190,78,57,0,244,107,208,125,248,210,229,166,
		246,222,180,29,116,143,125,223,47,100,134,6,109,210,4,153,70,137,8,210,132,140,22,224,198,55,1,243,195,59,53,96,167,246,249,214,40,240,95,228,95,13,198,19,162,211,96,131,240,165,152,60,65,4,84,124,37,112,88,19,225,210,174,33,237,242,6,10,238,183,255,
		2,195,248,174,88,205,91,102,85,0,0,0,0,73,69,78,68,174,66,96,130,0,0 };

	Image img = ImageCache::getFromMemory(popupData, sizeof(popupData));

	closeButton.setImages(false, true, true, img, 1.0f, Colour(0),
		img, 1.0f, Colours::white.withAlpha(0.05f),
		img, 1.0f, Colours::white.withAlpha(0.1f));

	WeakReference<BorderPanel> safeThis(this);

	MessageManager::callAsync([safeThis]()
	{
		if (safeThis != nullptr)
			safeThis.get()->registerToTopLevelComponent();
	});
}

BorderPanel::~BorderPanel()
{
	if (drawHandler != nullptr)
		drawHandler->removeDrawActionListener(this);
}

void BorderPanel::newOpenGLContextCreated()
{
}

void BorderPanel::renderOpenGL()
{
		
}

void BorderPanel::openGLContextClosing()
{
}

void BorderPanel::newPaintActionsAvailable(uint64_t flowId)
{
	flowManager.continueFlow(flowId, "repaint request");
	repaint();
}

void BorderPanel::registerToTopLevelComponent()
{
#if 0
		if (srs == nullptr)
		{
			if (auto tc = findParentComponentOfClass<TopLevelWindowWithOptionalOpenGL>())
				srs = new TopLevelWindowWithOptionalOpenGL::ScopedRegisterState(*tc, this);
		}
#endif
}

void BorderPanel::resized()
{
	registerToTopLevelComponent();

	if (isPopupPanel)
	{
		closeButton.setBounds(getWidth() - 24, 0, 24, 24);
	}
	else
		closeButton.setVisible(false);
		
}

#if HISE_INCLUDE_RLOTTIE
void BorderPanel::setAnimation(RLottieAnimation::Ptr newAnimation)
{
	animation = newAnimation;
}
#endif

void BorderPanel::buttonClicked(Button* /*b*/)
{
	auto contentComponent = findParentComponentOfClass<ScriptContentComponent>();

	if (contentComponent != nullptr)
	{
		auto c = dynamic_cast<ScriptingApi::Content::ScriptPanel*>(contentComponent->getScriptComponentFor(this));

		if (c != nullptr)
		{
			c->closeAsPopup();
		}
	}
}

void BorderPanel::changeListenerCallback(SafeChangeBroadcaster* )
{
}

struct GraphicHelpers
{
	static void quickDraw(Image& dst, const Image& src)
	{
		jassert(dst.getFormat() == src.getFormat());
		jassert(dst.getBounds() == src.getBounds());

		juce::Image::BitmapData srcData(src, juce::Image::BitmapData::readOnly);
		juce::Image::BitmapData dstData(dst, juce::Image::BitmapData::writeOnly);

		for (int y = 0; y < src.getHeight(); y++)
		{
			auto s = srcData.getLinePointer(y);
			auto d = dstData.getLinePointer(y);

			auto w = src.getWidth() * 4;

			for (int x = 0; x < w; x++)
				d[x] = jmin(255, s[x] + d[x]);
		}
	}
};

void BorderPanel::paint(Graphics &g)
{

	dispatch::StringBuilder n;

	bool hasOpenGL = false;

	if(auto c = TopLevelWindowWithOptionalOpenGL::findRoot(this))
		hasOpenGL = dynamic_cast<TopLevelWindowWithOptionalOpenGL*>(c)->isOpenGLEnabled();
	
	n << Component::getName() << "::paint()";
	TRACE_EVENT("component", DYNAMIC_STRING_BUILDER(n));
	PerfettoHelpers::setCurrentThreadName(!hasOpenGL ? "UI Render Thread (Software)" : "UI Render Thread (Open GL)");

	flowManager.flushAll("juce::Component paint routine");
	
	registerToTopLevelComponent();

	

#if HISE_INCLUDE_RLOTTIE
	if (animation != nullptr)
	{
		TRACE_EVENT("component", "rendering lottie");
		animation->render(g, { 0, 0 });
		return;
	}
#endif

	if (isUsingCustomImage)
	{
		TRACE_EVENT("component", "rendering script draw actions");

        SET_IMAGE_RESAMPLING_QUALITY();
		
		if (isOpaque())
			g.fillAll(Colours::black);

		DrawActions::Handler::Iterator it(drawHandler.get());

		it.render(g, this);
		
	}
	else
	{
        
		
		Rectangle<float> fillR(borderSize, borderSize, getWidth() - 2 * borderSize, getHeight() - 2 * borderSize);

		fillR.expand(borderSize * 0.5f, borderSize * 0.5f);


		if (isPopupPanel)
			fillR.reduce(12.0f, 12.0f);

		if (fillR.isEmpty() || fillR.getX() < 0 || fillR.getY() < 0) return;

        if(c1 != c2)
        {
            ColourGradient grad = ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false);
            g.setGradientFill(grad);
        }
        else
            g.setColour(c1);

        

        
        if(borderSize > 0)
        {
            if(borderRadius != 0)
                g.fillRoundedRectangle(fillR, borderRadius);
            else
                g.fillRect(fillR);
            
            g.setColour(borderColour);
            g.drawRoundedRectangle(fillR, borderRadius, borderSize);
        }
        else
        {
            if(borderRadius != 0)
                g.fillRoundedRectangle(fillR, borderRadius);
            else
                g.fillAll();
        }
	}
}





MultilineLabel::MultilineLabel(const String &name) :
Label(name),
multiline(false)
{

}

void MultilineLabel::setMultiline(bool shouldBeMultiline)
{
	multiline = shouldBeMultiline;
}

TextEditor * MultilineLabel::createEditorComponent()
{
	TextEditor *textEditor = Label::createEditorComponent();

	textEditor->setMultiLine(multiline, true);

	textEditor->setReturnKeyStartsNewLine(multiline);
	
	if (usePasswordChar)
		textEditor->setPasswordCharacter('*');

	return textEditor;
}

void MultilineLabel::paint(Graphics& g)
{
	if (usePasswordChar)
	{
		g.fillAll(findColour(Label::backgroundColourId));

		if (!isBeingEdited())
		{

			g.setColour(findColour(Label::textColourId));
			g.setFont(getFont());

			auto text = getText().length();

			String s;
			for (int i = 0; i < text; i++)
				s << '*';

			auto b = getBorderSize().subtractedFrom(getLocalBounds());

			g.drawFittedText(s, b, getJustificationType(), 1);
		}

		g.setColour(findColour(Label::outlineColourId));
		g.drawRect(getLocalBounds(), 2);
	}
	else
		Label::paint(g);
}

ImageComponentWithMouseCallback::ImageComponentWithMouseCallback() :
image(nullptr),
alpha(1.0f),
offset(0),
scale(1.0)
{

}

void ImageComponentWithMouseCallback::paint(Graphics &g)
{
	if (image.isValid())
	{
		g.setOpacity(jmax<float>(0.0f, jmin<float>(1.0f, alpha)));

		Rectangle<int> cropArea = Rectangle<int>(0,
			(int)((float)offset * scale),
			jmin<int>((int)((float)getWidth() * (float)scale), image.getWidth()),
			jmin<int>((int)((float)getHeight() * (float)scale), image.getHeight()));

		Image croppedImage = image.getClippedImage(cropArea);

        float ratio  = (float)getHeight() / (float)getWidth();
        int heightInImage = (int)((float)image.getWidth() * ratio);
        g.drawImage(image, 0, 0, getWidth(), getHeight(), 0, offset, image.getWidth(), heightInImage);
	}
}

void ImageComponentWithMouseCallback::setImage(const Image &newImage)
{
	if (newImage != image)
	{
		image = newImage;
		repaint();
	}
}

void ImageComponentWithMouseCallback::setAlpha(float newAlpha)
{
	if (alpha != newAlpha)
	{
		alpha = newAlpha;
		repaint();
	}
}

void ImageComponentWithMouseCallback::setOffset(int newOffset)
{
	if (newOffset != offset)
	{
		offset = newOffset;
		repaint();
	}
}

void ImageComponentWithMouseCallback::setScale(double newScale)
{
	if (newScale != scale)
	{
		scale = jmax<double>(0.1, newScale);

		repaint();
	}
}

MouseCallbackComponent::Listener::Listener()
{}

MouseCallbackComponent::Listener::~Listener()
{ masterReference.clear(); }

MouseCallbackComponent::RectangleConstrainer::Listener::~Listener()
{ masterReference.clear(); }

MouseCallbackComponent::~MouseCallbackComponent()
{}

void MouseCallbackComponent::setJSONPopupData(var jsonData, Rectangle<int> popupSize_)
{ 
	jsonPopupData = jsonData; 
	popupSize = popupSize_;
}

void MouseCallbackComponent::setActivePopupItem(int menuId)
{
	activePopupId = menuId;
}

void MouseCallbackComponent::updateValue(NotificationType)
{
	repaint();
}

NormalisableRange<double> MouseCallbackComponent::getRange() const
{
	return range;
}

void MouseCallbackComponent::setRange(NormalisableRange<double>& newRange)
{
	range = newRange;
}

void MouseCallbackComponent::setMidiLearnEnabled(bool shouldBeEnabled)
{
	midiLearnEnabled = shouldBeEnabled;
}

void MouseCallbackComponent::RectangleConstrainer::checkBounds(Rectangle<int> &newBounds, const Rectangle<int> &, const Rectangle<int> &, bool, bool, bool, bool)
{
	if (!draggingBounds.isEmpty())
	{
		if (newBounds.getX() < draggingBounds.getX()) newBounds.setX(draggingBounds.getX());
		if (newBounds.getY() < draggingBounds.getY()) newBounds.setY(draggingBounds.getY());
		if (newBounds.getBottom() > draggingBounds.getBottom()) newBounds.setY(draggingBounds.getBottom() - newBounds.getHeight());
		if (newBounds.getRight() > draggingBounds.getRight()) newBounds.setX(draggingBounds.getRight() - newBounds.getWidth());
	}

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() != nullptr)
		{
			listeners[i]->boundsChanged(newBounds);
		}
	}
}

void MouseCallbackComponent::RectangleConstrainer::addListener(Listener *l)
{
	listeners.add(l);
}

void MouseCallbackComponent::RectangleConstrainer::removeListener(Listener *l)
{
	listeners.removeAllInstancesOf(l);
}

bool DrawActions::Handler::beginBlendLayer(const Identifier& blendMode, float alpha)
{
	static const Array<Identifier> blendIds = {
		"Normal",
		"Lighten",
		"Darken",
		"Multiply",
		"Average",
		"Add",
		"Subtract",
		"Difference",
		"Negation",
		"Screen",
		"Exclusion",
		"Overlay",
		"SoftLight",
		"HardLight",
		"ColorDodge",
		"ColorBurn",
		"LinearDodge",
		"LinearBurn",
		"LinearLight",
		"VividLight",
		"PinLight",
		"HardMix",
		"Reflect",
		"Glow",
		"Phoenix"
	};

	auto idx = blendIds.indexOf(blendMode);

	if (idx == -1)
		return false;

	auto bm = (gin::BlendMode)idx;
	ActionLayer* newLayer = new BlendingLayer(bm, alpha);
	addDrawAction(newLayer);
	layerStack.insert(-1, newLayer);
	return true;
}

juce::Rectangle<int> DrawActions::Handler::getScreenshotBounds(Rectangle<int> shaderBounds) const
{
	shaderBounds = shaderBounds.transformedBy(AffineTransform::scale(scaleFactor));

	auto x = shaderBounds.getX() -1 * globalBounds.getX();
	
	auto y = topLevelBounds.getHeight() + globalBounds.getY() - shaderBounds.getHeight() - shaderBounds.getY();

	shaderBounds.setX(x);
	shaderBounds.setY(y);

	return shaderBounds;

}

void DrawActions::BlendingLayer::perform(Graphics& g)
{
	auto imageToBlendOn = actionImage;

	blendSource = Image(Image::ARGB, actionImage.getWidth(), actionImage.getHeight(), true);

	for (auto a : internalActions)
	{
		if (a->wantsCachedImage())
			a->setCachedImage(blendSource, actionImage);
	}

	setCachedImage(blendSource, actionImage);
	Graphics g2(blendSource);
    g2.addTransform(AffineTransform::scale(scaleFactor));

	ActionLayer::perform(g2);
	gin::applyBlend(imageToBlendOn, blendSource, blendMode, alpha);
}

void DrawActions::Handler::Iterator::render(Graphics& g, Component* c)
{
	if (handler->recursion)
		return;

	UnblurryGraphics ug(g, *c);

	auto sf = ug.getTotalScaleFactor();
	auto st = AffineTransform::scale(jmin<double>(4.0, sf));
	auto st2 = AffineTransform::scale(sf);

	auto tc = c->getTopLevelComponent();

	auto gb = c->getLocalArea(tc, c->getLocalBounds()).transformed(st2);

	handler->setGlobalBounds(gb, tc->getLocalBounds(), sf);

    auto zoomFactor = UnblurryGraphics::getScaleFactorForComponent(c, false);
    
    handler->getNoiseMapManager()->setScaleFactor(zoomFactor);

    
	if (wantsCachedImage())
	{
		// We are creating one master image before the loop
		Image cachedImg;

		if (!c->isOpaque() && (c->getParentComponent() != nullptr && wantsToDrawOnParent()))
		{
			// fetch the parent component content here...
			ScopedValueSetter<bool> sv(handler->recursion, true);
			cachedImg = c->getParentComponent()->createComponentSnapshot(c->getBoundsInParent(), true, sf);
		}
		else
		{
			// just use an empty image
			cachedImg = Image(Image::ARGB, c->getWidth() * sf, c->getHeight() * sf, true);
		}

		Graphics g2(cachedImg);
		g2.addTransform(st);

		while (auto action = getNextAction())
		{
#if PERFETTO
			dispatch::StringBuilder b;
			b << "g." << action->getDispatchId() << "()";
			TRACE_EVENT("drawactions", DYNAMIC_STRING_BUILDER(b));
#endif

			if (action->wantsCachedImage())
			{
				Image actionImage;

				if (action->wantsToDrawOnParent())
					actionImage = cachedImg; // just use the cached image
				else
				{
					actionImage = Image(cachedImg.getFormat(), cachedImg.getWidth(), cachedImg.getHeight(), true);
				}

				Graphics g3(actionImage);
                
                action->setScaleFactor(sf);
				action->setCachedImage(actionImage, cachedImg);
				action->perform(g3);

				if (!action->wantsToDrawOnParent())
                {
                    g2.drawImageAt(actionImage, 0, 0);
                }
					//GraphicHelpers::quickDraw(cachedImg, actionImage);
			}
			else
				action->perform(g2);
		}

		g.drawImageTransformed(cachedImg, st.inverted());
	}
	else
	{
		while (auto action = getNextAction())
		{
#if PERFETTO
			dispatch::StringBuilder b;
			b << "g." << action->getDispatchId() << "()";
			TRACE_EVENT("drawactions", DYNAMIC_STRING_BUILDER(b));
#endif

			action->perform(g);
		}
			
	}
}

DrawActions::NoiseMapManager::NoiseMap::NoiseMap(Rectangle<int> a, bool monochrom_) :
	width(a.getWidth()),
	height(a.getHeight()),
	img(Image::ARGB, width, height, false),
	monochrom(monochrom_)
{
	Image::BitmapData bd(img, Image::BitmapData::readWrite);
	Random r;

	if (monochrom)
	{
		for (int y = 0; y < bd.height; y++)
		{
			for (int x = 0; x < bd.width; x++)
				bd.setPixelColour(x, y, Colours::white.withBrightness(r.nextFloat()));
		}
	}
	else
	{
		for (int y = 0; y < bd.height; y++)
		{
			for (int x = 0; x < bd.width; x++)
				bd.setPixelColour(x, y, Colour((uint32)r.nextInt()));
		}
	}

	
}

} // namespace hise
