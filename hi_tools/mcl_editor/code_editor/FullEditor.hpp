/** ============================================================================
 *
 * TextEditor.hpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


/**
 * 
 TODO:




 */


#pragma once




namespace mcl
{

struct FullEditor: public Component
{
	TextEditor editor;

	struct Navigator : public Component,
					   public ButtonListener
	{
		Navigator(TextDocument& d) :
			codeMap(d, new CPlusPlusCodeTokeniser()),
			foldMap(d),
			mapButton("Map"),
			foldButton("Tree"),
			edge(this, nullptr, ResizableEdgeComponent::leftEdge)
		{
			auto initButton = [this](TextButton& b)
			{
				addAndMakeVisible(b);
				b.addListener(this);
				b.setRadioGroupId(90);
				b.setClickingTogglesState(true);
			};

			initButton(mapButton);
			initButton(foldButton);
			initButton(treeButton);

			addAndMakeVisible(foldMap);
			addAndMakeVisible(codeMap);

			mapButton.setToggleState(true, sendNotification);

			setSize(200, 10);
		};

		void buttonClicked(Button* b) override
		{
			codeMap.setVisible(b == &mapButton);
			foldMap.setVisible(b == &foldButton);
			resized();
		}

		void resized() override
		{
			auto b = getLocalBounds();
			edge.setBounds(b.removeFromLeft(5));

			auto top = b.removeFromTop(24);

			auto bw = b.getWidth() / 2;

			mapButton.setBounds(top.removeFromLeft(bw));
			foldButton.setBounds(top.removeFromLeft(bw));

			codeMap.setBounds(b);
			foldMap.setBounds(b);
		}

		TextButton mapButton, foldButton, treeButton;
		CodeMap codeMap;
		FoldMap foldMap;

		ResizableEdgeComponent edge;

	} navigator;

	FullEditor(TextDocument& d):
		editor(d),
		navigator(d)
	{
		addAndMakeVisible(editor);
		addAndMakeVisible(navigator);

		navigator.codeMap.colourScheme = editor.colourScheme;
		navigator.codeMap.transformToUse = editor.transform;
	}

	String injectBreakpointCode(const String& s)
	{
		return editor.gutter.injectBreakPoints(s);
	}

	void setCurrentBreakline(int n)
	{
		editor.gutter.setCurrentBreakline(n);
	}

	void addBreakpointListener(GutterComponent::BreakpointListener* l)
	{
		editor.gutter.addBreakpointListener(l);
	}

	void removeBreakpointListener(GutterComponent::BreakpointListener* l)
	{
		editor.gutter.removeBreakpointListener(l);
	}

	void resized() override
	{
		auto b = getLocalBounds();
		
		navigator.setVisible(false);
		//navigator.setBounds(b.removeFromRight(navigator.getWidth()));
		editor.setBounds(b);
	}

	
};

}

