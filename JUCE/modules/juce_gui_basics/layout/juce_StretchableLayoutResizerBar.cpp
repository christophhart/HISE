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

namespace juce
{

StretchableLayoutResizerBar::StretchableLayoutResizerBar (StretchableLayoutManager* layout_,
                                                          const int index,
                                                          const bool vertical)
    : layout (layout_),
      itemIndex (index),
      isVertical (vertical)
{
    setRepaintsOnMouseActivity (true);
    setMouseCursor (vertical ? MouseCursor::LeftRightResizeCursor
                             : MouseCursor::UpDownResizeCursor);
}

StretchableLayoutResizerBar::~StretchableLayoutResizerBar()
{
}

//==============================================================================
void StretchableLayoutResizerBar::paint (Graphics& g)
{
    getLookAndFeel().drawStretchableLayoutResizerBar (g,
													  *this,
                                                      getWidth(), getHeight(),
                                                      isVertical,
                                                      isMouseOver(),
                                                      isMouseButtonDown());
}

void StretchableLayoutResizerBar::mouseDown (const MouseEvent&)
{
    mouseDownPos = layout->getItemCurrentPosition (itemIndex);
}

void StretchableLayoutResizerBar::mouseDrag (const MouseEvent& e)
{
    const int desiredPos = mouseDownPos + (isVertical ? e.getDistanceFromDragStartX()
                                                      : e.getDistanceFromDragStartY());


    if (layout->getItemCurrentPosition (itemIndex) != desiredPos)
    {
        layout->setItemPosition (itemIndex, desiredPos);
        hasBeenMoved();
    }
}

void StretchableLayoutResizerBar::hasBeenMoved()
{
    if (Component* parent = getParentComponent())
        parent->resized();
}

} // namespace juce
