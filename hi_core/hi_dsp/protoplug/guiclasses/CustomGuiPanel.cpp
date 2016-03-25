#include "CustomGuiPanel.h"

void CustomGuiPanel::paint (Graphics& g)
{ luli->paint(g); }

void CustomGuiPanel::resized ()
{ luli->resized(); }

void CustomGuiPanel::mouseMove (const MouseEvent& event)
{ luli->mouseMove(event); }
void CustomGuiPanel::mouseEnter (const MouseEvent& event)
{ luli->mouseEnter(event); }
void CustomGuiPanel::mouseExit (const MouseEvent& event)
{ luli->mouseExit(event); }
void CustomGuiPanel::mouseDown (const MouseEvent& event)
{ luli->mouseDown(event); }
void CustomGuiPanel::mouseDrag (const MouseEvent& event)
{ luli->mouseDrag(event); }
void CustomGuiPanel::mouseUp (const MouseEvent& event)
{ luli->mouseUp(event); }
void CustomGuiPanel::mouseDoubleClick (const MouseEvent& event)
{ luli->mouseDoubleClick(event); }
void CustomGuiPanel::mouseWheelMove (const MouseEvent& event, const MouseWheelDetails& wheel)
{ luli->mouseWheelMove(event, wheel); }

bool CustomGuiPanel::keyPressed (const KeyPress &key, Component *originatingComponent)
{ return luli->keyPressed(key, originatingComponent); }
bool CustomGuiPanel::keyStateChanged (bool isKeyDown, Component *originatingComponent)
{ return luli->keyStateChanged(isKeyDown, originatingComponent); }

void CustomGuiPanel::modifierKeysChanged (const ModifierKeys &modifiers)
{ luli->modifierKeysChanged(modifiers); }
void CustomGuiPanel::focusGained (FocusChangeType cause)
{ luli->focusGained(cause); }
void CustomGuiPanel::focusLost (FocusChangeType cause)
{ luli->focusLost(cause); }