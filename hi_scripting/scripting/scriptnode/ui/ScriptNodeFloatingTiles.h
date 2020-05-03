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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

struct NetworkPanel : public PanelWithProcessorConnection
{
	NetworkPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

	Identifier getProcessorTypeId() const override;
	virtual Component* createComponentForNetwork(DspNetwork* parent) = 0;
	Component* createContentComponent(int index) override;
	void fillModuleList(StringArray& moduleList) override;
	virtual bool hasSubIndex() const { return true; }
	void fillIndexList(StringArray& sa);
};

struct DspNetworkGraphPanel : public NetworkPanel
{
	DspNetworkGraphPanel(FloatingTile* parent);

	SET_PANEL_NAME("DspNetworkGraph");

	Component* createComponentForNetwork(DspNetwork* p) override;
};


class NodePropertyPanel : public NetworkPanel
{
public:

	NodePropertyPanel(FloatingTile* parent);

	SET_PANEL_NAME("NodePropertyPanel");

	Component* createComponentForNetwork(DspNetwork* p) override;
};




struct SnexPopupEditor : public Component,
	public SnexDebugHandler,
	public CodeDocument::Listener,
	public Timer,
	public ButtonListener,
	public Value::Listener
{
	struct Icons : public PathFactory
	{
		Path createPath(const String& url) const override;

		String getId() const override { return ""; }
	};


	struct Parent : public ButtonListener
	{
		Parent(SnexSource* s) :
			source(s)
		{
		}

		void buttonClicked(Button* b)
		{
			auto e = new SnexPopupEditor("Edit SNEX Callback", source);
			b->findParentComponentOfClass<FloatingTile>()->showComponentInRootPopup(e, b, b->getLocalBounds().getCentre().withY(b->getHeight()));
			e->grabKeyboardFocus();
		}

		SnexSource* source;
	};

	SnexPopupEditor(const String& name, SnexSource* src, bool isPopup=true);

	void buttonClicked(Button* b) override;

	bool keyPressed(const KeyPress& key) override
	{
		if (key == KeyPress::F5Key)
		{
			compileButton.triggerClick();
			return true;
		}
	}

	void valueChanged(Value& v);

	void codeDocumentTextInserted(const String& newText, int insertIndex) override
	{
	}
	void codeDocumentTextDeleted(int startIndex, int endIndex) override
	{
	}

	void timerCallback() override
	{
		recompile();
		stopTimer();
	}

	void recompile();

	void logMessage(int level, const juce::String& s)
	{
		if (level == 0)
			editor.setError(s);
		if (level == 1)
			editor.addWarning(s);
	}

	snex::jit::GlobalScope s;

	void paint(Graphics& g) override;

	void resized() override;

	snex::AssemblyTokeniser tokeniser;
	CodeDocument asmDoc;
	CodeEditorComponent asmView;

	SnexSource* source;

	bool internalChange = false;
	CodeDocument d;
	Value codeValue;
	mcl::TextEditor editor;
	juce::ResizableCornerComponent corner;
	String lastAssembly;

	Icons f;
	HiseShapeButton compileButton;
	HiseShapeButton asmButton;
	HiseShapeButton resetButton;
	HiseShapeButton optimiseButton;
	StringArray optimizations;
	bool popupMode;
};



}
