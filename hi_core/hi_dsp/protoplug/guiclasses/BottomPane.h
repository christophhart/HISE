#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../LuaCodeTokeniser.h"
#include "HintedFeel.h"
#include "DarkSplitter.h"
#include "ParameterPanel.h"
#include "CustomGuiPanel.h"
#include "LuaEditor.h"
#include "Dockable.h"
#include "ProtoTabButton.h"
#include <map>

class ProtoWindow;

// isn't this what labels are supposed to be?
class LabelOfOtherComponent : public Label, public MouseListener
{
public:
	LabelOfOtherComponent() { tgt = 0; }
	void setTarget(Component *_tgt) { tgt = _tgt; }
	void mouseUp (const MouseEvent &event) { if (tgt) tgt->grabKeyboardFocus(); }
private:
	Component *tgt;
};

class BottomPane	:	public Component,
						public Button::Listener,
						public TextEditor::Listener
{
public:
	BottomPane (ProtoWindow *protoWin);

	void resized();
	void buttonClicked (Button *b);
	void textEditorReturnKeyPressed (TextEditor &t);
	void setCompileVisible(bool hue);
	void updateLog();
	void scrollLog();

private:
	ProtoWindow *protoWin;
	
	TextEditor log;
	TextButton compileButton;
	LabelOfOtherComponent prompt;
	TooltipWindow tooltip; // is this used?

	TextEditor input;
	//TextButton runButton; // meh, at this point your hand is over the enter key
};