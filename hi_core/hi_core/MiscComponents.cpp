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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



MouseCallbackComponent::MouseCallbackComponent() :
callbackLevel(CallbackLevel::NoCallbacks),
currentEvent(new DynamicObject()),
callbackLevels(getCallbackLevels()),
constrainer(new RectangleConstrainer())
{

}



StringArray MouseCallbackComponent::getCallbackLevels()
{
	StringArray sa;
	sa.add("No Callbacks");
	sa.add("Context Menu");
	sa.add("Clicks Only");
	sa.add("Clicks & Hover");
	sa.add("Clicks, Hover & Dragging");
	sa.add("All Callbacks");

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
	sa.add("rightClick");
	sa.add("mouseUp");
	sa.add("drag");
	sa.add("dragX");
	sa.add("dragY");
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

void MouseCallbackComponent::mouseDown(const MouseEvent& event)
{
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
			PopupMenu m;
			m.setLookAndFeel(&plaf);

			std::vector<SubMenuList> subMenus;

			ScopedValueSetter<bool>(currentlyShowingPopup, true, false);

			for (int i = 0; i < itemList.size(); i++)
			{
				if (itemList[i].contains("::"))
				{
					String subMenuName = itemList[i].upToFirstOccurrenceOf("::", false, false);
					String subMenuItem = itemList[i].fromFirstOccurrenceOf("::", false, false);

					if (subMenuName.isEmpty() || subMenuItem.isEmpty()) continue;

					bool subMenuExists = false;

					for (int j = 0; j < subMenus.size(); j++)
					{
						if (std::get<0>(subMenus[j]) == subMenuName)
						{
							std::get<1>(subMenus[j]).add(subMenuItem);
							subMenuExists = true;
							break;
						}
					}

					if (!subMenuExists)
					{
						StringArray sa;
						sa.add(subMenuItem);
						SubMenuList item(subMenuName, sa);
						subMenus.push_back(item);
					}
				}
			}



			if (subMenus.size() != 0)
			{
				int menuIndex = 1;

				for (int i = 0; i < subMenus.size(); i++)
				{
					PopupMenu sub;
					
					StringArray sa = std::get<1>(subMenus[i]);

					for (int j = 0; j < sa.size(); j++)
					{
						sub.addItem(menuIndex++, sa[j]);
					}
					
					m.addSubMenu(std::get<0>(subMenus[i]), sub);
				}
			}
			else
			{
				for (int i = 0; i < itemList.size(); i++)
				{
					m.addItem(i + 1, itemList[i], true, false);
				}
			}
			

			int result = 0;
				
			if (popupShouldBeAligned)
			{
				result = m.showAt(this, 0, getWidth());
			}
			else
			{
				result = m.show();
			}

			String name = result != 0 ? itemList[result-1] : "";

			DynamicObject::Ptr obj = new DynamicObject();

			static const Identifier r("result");
			static const Identifier itemText("itemText");
			static const Identifier rightClick("rightClick");

			obj->setProperty(rightClick, event.mods.isRightButtonDown());
			obj->setProperty(r, result);
			obj->setProperty(itemText, name);

			sendToListeners(var(obj));

			return;
		}
	}

	if (callbackLevel > CallbackLevel::PopupMenuOnly)
	{
		sendMessage(event, Action::Clicked);
	}
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

void MouseCallbackComponent::mouseDrag(const MouseEvent& event)
{
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
	if (draggingEnabled)
	{
		setAlwaysOnTop(false);
	}


	if (currentlyShowingPopup) return;
	if (callbackLevel < CallbackLevel::ClicksOnly) return;

	sendMessage(event, Action::MouseUp);
}

void MouseCallbackComponent::sendMessage(const MouseEvent &event, Action action, EnterState state)
{
	if (callbackLevel == CallbackLevel::NoCallbacks) return;

	DynamicObject::Ptr obj = currentEvent;

	static const Identifier x("x");
	static const Identifier y("y");
	static const Identifier clicked("clicked");
	static const Identifier rightClick("rightClick");
	static const Identifier drag("drag");
	static const Identifier dragX("dragX");
	static const Identifier dragY("dragY");
	static const Identifier hover("hover");
	static const Identifier mouseDownX("mouseDownX");
	static const Identifier mouseDownY("mouseDownY");
	static const Identifier mouseUp("mouseUp");
    static const Identifier shiftDown("shiftDown");
    static const Identifier cmdDown("cmdDown");
    static const Identifier altDown("altDown");
    static const Identifier ctrlDown("ctrlDown");
    
	currentEvent->clear();

	if (callbackLevel >= CallbackLevel::ClicksOnly)
	{
		currentEvent->setProperty(clicked, action == Action::Clicked);
		currentEvent->setProperty(rightClick, (action == Action::Clicked && event.mods.isRightButtonDown()) ||
											  (action == Action::MouseUp && event.mods.isRightButtonDown()));
		currentEvent->setProperty(mouseUp, action == Action::MouseUp);
		currentEvent->setProperty(mouseDownX, event.getMouseDownX());
		currentEvent->setProperty(mouseDownY, event.getMouseDownY());
		currentEvent->setProperty(x, event.getPosition().getX());
		currentEvent->setProperty(y, event.getPosition().getY());
        currentEvent->setProperty(shiftDown, event.mods.isShiftDown());
        currentEvent->setProperty(cmdDown, event.mods.isCommandDown());
        currentEvent->setProperty(altDown, event.mods.isAltDown());
        currentEvent->setProperty(ctrlDown, event.mods.isCtrlDown());
        
	}

	if (callbackLevel >= CallbackLevel::ClicksAndEnter)
	{
		currentEvent->setProperty(hover, state != Exited);
	}

	if (callbackLevel >= CallbackLevel::Drag)
	{
		currentEvent->setProperty(drag, event.getDistanceFromDragStart() > 4);
		currentEvent->setProperty(dragX, event.getDistanceFromDragStartX());
		currentEvent->setProperty(dragY, event.getDistanceFromDragStartY());
	}

	var clickInformation(obj);

	sendToListeners(clickInformation);
}



void MouseCallbackComponent::sendToListeners(var clickInformation)
{
	for (int i = 0; i < listenerList.size(); i++)
	{
		if (listenerList[i].get() != nullptr)
		{
			listenerList[i]->mouseCallback(clickInformation);
		}
	}
}

BorderPanel::BorderPanel() :
borderColour(Colours::black),
c1(Colours::white),
c2(Colours::white),
borderRadius(0.0f),
borderSize(1.0f)
{

}



void BorderPanel::paint(Graphics &g)
{
	if (isUsingCustomImage)
	{
		g.setColour(Colours::black);
		g.setOpacity(1.0f);
		
        g.drawImageWithin(image, 0, 0, getWidth() , getHeight(), RectanglePlacement::centred);
	}
	else
	{
		ColourGradient grad = ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false);
		Rectangle<float> fillR(borderSize, borderSize, getWidth() - 2 * borderSize, getHeight() - 2 * borderSize);

		fillR.expand(borderSize * 0.5f, borderSize * 0.5f);

		if (fillR.isEmpty() || fillR.getX() < 0 || fillR.getY() < 0) return;

		g.setGradientFill(grad);
		g.fillRoundedRectangle(fillR, borderRadius);

		g.setColour(borderColour);
		g.drawRoundedRectangle(fillR, borderRadius, borderSize);
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

	return textEditor;
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
			offset,
			jmin<int>(getWidth(), image.getWidth()),
			jmin<int>(getHeight(), image.getHeight()));

		Image croppedImage = image.getClippedImage(cropArea);

		if (scale != 1.0)
		{
			croppedImage = croppedImage.rescaled((int)((double)croppedImage.getWidth() / scale), (int)((double)croppedImage.getHeight() / scale));
		}

		g.drawImageAt(croppedImage, 0, 0);
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
