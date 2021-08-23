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



class MarkdownHelpButton : public ShapeButton,
	public ButtonListener,
	public ComponentListener
{
public:

	enum AttachmentType
	{
		Overlay,
		OverlayLeft,
		OverlayRight,
		TopRight,
		Left,
		numAttachmentTypes
	};

	MarkdownHelpButton();

	~MarkdownHelpButton()
	{
		if (ownerComponent != nullptr)
			ownerComponent->removeComponentListener(this);
	}

	void setup()
	{
		parser = new MarkdownRenderer("");
		parser->setTextColour(Colours::white);
		parser->setDefaultTextSize(fontSizeToUse);
		parser->setStyleData(sd);
	}

	MarkdownParser* getParser() { return parser; }

	void addImageProvider(MarkdownParser::ImageProvider* newImageProvider)
	{
		if (parser != nullptr)
		{
			parser->setImageProvider(newImageProvider);
		}
		else
			jassertfalse; // you need to call setup before that.
	}

	template <class ProviderType = MarkdownParser::ImageProvider> void setHelpText(const String& markdownText)
	{
		if (parser == nullptr)
			setup();

		parser->setNewText(markdownText);
		parser->setImageProvider(new ProviderType(parser));
		parser->setStyleData(sd);
		parser->parse();
	}

	void setPopupWidth(int newPopupWidth)
	{
		popupWidth = newPopupWidth;
	}

	void setFontSize(float fontSize)
	{
		fontSizeToUse = fontSize;
	}

	void buttonClicked(Button* b) override;

	void attachTo(Component* componentToAttach, AttachmentType attachmentType_)
	{
		if (ownerComponent != nullptr)
			ownerComponent->removeComponentListener(this);

		ownerComponent = componentToAttach;
		attachmentType = attachmentType_;

		if (ownerComponent != nullptr)
		{
			jassert(getParentComponent() == nullptr);

			if (auto parent = ownerComponent->getParentComponent())
			{
				parent->addAndMakeVisible(this);
			}
			else
				jassertfalse; // You tried to attach a help button to a component without a parent...

			setVisible(ownerComponent->isVisible());
			ownerComponent->addComponentListener(this);
			componentMovedOrResized(*ownerComponent, true, true);
		}
	}

	void componentMovedOrResized(Component& c, bool /*wasMoved*/, bool /*wasResized*/) override
	{
		auto cBounds = c.getBoundsInParent();

		switch (attachmentType)
		{
		case Overlay:
		{
			setBounds(cBounds.withSizeKeepingCentre(16, 16));
			break;
		}
		case OverlayLeft:
		{
			auto square = cBounds.removeFromLeft(20);

			setBounds(square.withSizeKeepingCentre(16, 16));

			break;
		}
		case OverlayRight:
		{
			auto square = cBounds.removeFromRight(20);

			setBounds(square.withSizeKeepingCentre(16, 16));

			break;
		}
		case Left:
		{
			setBounds(cBounds.getX() - 20, cBounds.getY() + 2, 16, 16);
			break;
		}
		case TopRight:
		{
			Rectangle<int> r(cBounds.getRight() - 16, cBounds.getY() - 16, 16, 16);
			setBounds(r);
		}
		default:
			break;
		}
	}

	void componentVisibilityChanged(Component& c) override
	{
		setVisible(c.isVisible());
	}

	void setIgnoreKeyStrokes(bool shouldIgnoreKeyStrokes)
	{
		setWantsKeyboardFocus(shouldIgnoreKeyStrokes);
		ignoreKeyStrokes = shouldIgnoreKeyStrokes;

	}

	static MarkdownHelpButton* createAndAddToComponent(Component* c, const String& s, int popupWidth = 400)
	{
		auto h = new MarkdownHelpButton();

		h->attachTo(c, MarkdownHelpButton::TopRight);
		h->setHelpText(s);
		h->setPopupWidth(popupWidth);
		return h;
	}

	void componentBeingDeleted(Component& component) override
	{
		component.removeComponentListener(this);

		getParentComponent()->removeChildComponent(this);

		delete this;
	}

	void setStyleData(const MarkdownLayout::StyleData& newStyleData)
	{
		sd = newStyleData;

		if (parser != nullptr)
		{
			parser->setStyleData(sd);
			parser->parse();
		}
	}

	static Path getPath();

private:

	MarkdownLayout::StyleData sd;

	bool ignoreKeyStrokes = false;
	float fontSizeToUse = 17.0f;
	Component::SafePointer<CallOutBox> currentPopup;
	ScopedPointer<MarkdownRenderer> parser;
	int popupWidth = 400;
	Component::SafePointer<Component> ownerComponent;
	AttachmentType attachmentType;
	struct MarkdownHelp;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownHelpButton);
};

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
	public CodeDocument::Listener,
	public Timer,
	public ButtonListener,
	public ViewportWithScrollCallback::Listener
{
public:

	class Factory : public PathFactory
	{
		String getId() const override { return "Markdown Editor"; }

		Path createPath(const String& id) const override
		{
			Path p;

			auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

			LOAD_PATH_IF_URL("live-preview", EditorIcons::swapIcon);
			LOAD_PATH_IF_URL("new-file", EditorIcons::newFile);
			LOAD_PATH_IF_URL("open-file", EditorIcons::openFile);
			LOAD_PATH_IF_URL("save-file", EditorIcons::saveFile);
			LOAD_PATH_IF_URL("create-link", EditorIcons::urlIcon);
			LOAD_PATH_IF_URL("create-image", EditorIcons::imageIcon);
			LOAD_PATH_IF_URL("create-table", EditorIcons::tableIcon);

#if USE_BACKEND
			LOAD_PATH_IF_URL("show-settings", BackendBinaryData::ToolbarIcons::settings);
#endif

			return p;
		}
	};

	SET_PANEL_NAME("Markdown Editor");

	MarkdownEditorPanel(FloatingTile* root);

	~MarkdownEditorPanel()
	{
		doc.removeListener(this);

		if (preview.getComponent() != nullptr)
		{
			preview->viewport.removeListener(this);
		}

	}

	static mcl::FoldableLineRange::List createLineRange(const CodeDocument& doc);

	void scrolled(Rectangle<int> /*visibleArea*/)
	{
		if (editor.hasKeyboardFocus(true))
			return;

		if (updatePreview())
		{
			auto ratio = (float)preview->viewport.getViewPositionY() / (float)preview->internalComponent.getHeight();

			int l = (int)(ratio * (float)doc.getNumLines());

			editor.editor.scrollToLine(l, true);
		}
	}

	bool updatePreview()
	{
		if (preview.getComponent() != nullptr)
			return true;

		auto p = dynamic_cast<MarkdownPreview*>(getMainController()->getCurrentMarkdownPreview());

		if (p != nullptr)
		{
			setPreview(p);
			return true;
		}
			
		return false;
	}

	void buttonClicked(Button* b) override;

	void setPreview(MarkdownPreview* p)
	{
		if (p != nullptr)
		{
			preview = p;
			preview->viewport.addListener(this);
		}
			
	}

	void codeDocumentTextDeleted(int , int ) override
	{
		if(livePreview.getToggleState())
			startTimer(300);
	}

	void codeDocumentTextInserted(const String&, int ) override
	{
		if (livePreview.getToggleState())
			startTimer(300);
	}

	bool keyPressed(const KeyPress& key) override
	{
		if (key == KeyPress::F5Key)
		{
			startTimer(300);
			return true;
		}
		if ((key.getKeyCode() == 's' ||
			key.getKeyCode() == 'S') && key.getModifiers().isCommandDown())
		{
			saveButton.triggerClick();
			return true;
		}

		return false;
	}

	void loadText(const String& s);

	void loadFile(File f);

	void timerCallback() override
	{
		if (updatePreview())
		{
			preview->renderer.clearCurrentLink();

			preview->setNewText(doc.getAllContent().replace("\r\n", "\n"), currentFile);

		}

		stopTimer();
	}

	File getRootFile();

	void resized() override
	{
		auto area = getParentShell()->getContentBounds();

		auto top = area.removeFromTop(46);

		int button_margin = 10;

		livePreview.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));

		top.removeFromLeft(20);

		newButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
		openButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
		saveButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));

		top.removeFromLeft(20);

		urlButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
		imageButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));
		tableButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(button_margin));

		settingsButton.setBounds(top.removeFromRight(top.getHeight()).reduced(button_margin));

		editor.setBounds(area);
	}

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
};

}