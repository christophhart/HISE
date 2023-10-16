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

#pragma once

namespace hise {
using namespace juce;





#if !HISE_USE_NEW_CODE_EDITOR
class MarkdownEditor : public CodeEditorComponent
{
public:

	enum AdditionalCommands
	{
		LoadFile = 759,
		SaveFile,
		numCommands
	};

	MarkdownEditor(CodeDocument& doc, CodeTokeniser* tok) :
		CodeEditorComponent(doc, tok)
	{
		setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFAAAAAA));
		setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.2f));
		setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
		setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF333333));
		setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
		setFont(GLOBAL_MONOSPACE_FONT().withHeight(18.0f));
	}

	void addPopupMenuItems(PopupMenu& menuToAddTo,
		const MouseEvent* mouseClickEvent);

	void performPopupMenuAction(int menuItemID);

	File currentFile;
};
#else
using MarkdownEditor = mcl::FullEditor;
#endif

class MarkdownEditorPanel : public FloatingTileContent,
	public Component,
	public ButtonListener
{
public:

	class Factory : public PathFactory
	{
		String getId() const override { return "Markdown Editor"; }

		Path createPath(const String& id) const override;
	};

	SET_PANEL_NAME("Markdown Editor");

	MarkdownEditorPanel(FloatingTile* root);
	~MarkdownEditorPanel() override;

	bool updatePreview();
	void buttonClicked(Button* b) override;
	void setPreview(MarkdownPreview* p);
	bool keyPressed(const KeyPress& key) override;
	void loadText(const String& s);
	void loadFile(File f);
	File getRootFile();
	void resized() override;

	Factory f;

	HiseShapeButton livePreview;
	HiseShapeButton newButton;
	HiseShapeButton openButton;
	HiseShapeButton saveButton;
	HiseShapeButton urlButton;
	HiseShapeButton imageButton;
	HiseShapeButton tableButton;
	HiseShapeButton settingsButton;

	Component::SafePointer<CallOutBox> currentBox;

	File currentFile;

	hise::GlobalHiseLookAndFeel laf;

	CodeDocument doc;
	MarkdownParser::Tokeniser tokeniser;
	Component::SafePointer<MarkdownPreview> preview;

	mcl::TextDocument tdoc;

	MarkdownEditor editor;
	MarkdownDataBase* database = nullptr;
    
    ScopedPointer<mcl::MarkdownPreviewSyncer> syncer;
};

}
