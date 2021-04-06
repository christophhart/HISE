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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;


DspNetworkGraphPanel::DspNetworkGraphPanel(FloatingTile* parent) :
	NetworkPanel(parent)
{

}


void DspNetworkGraphPanel::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF1D1D1D));
}

Component* DspNetworkGraphPanel::createComponentForNetwork(DspNetwork* p)
{
	return new DspNetworkGraph::WrapperWithMenuBar(p);
}

NodePropertyPanel::NodePropertyPanel(FloatingTile* parent):
	NetworkPanel(parent)
{

}

struct NodePropertyContent : public Component,
							 public DspNetwork::SelectionListener
{
	NodePropertyContent(DspNetwork* n):
		network(n)
	{
		addAndMakeVisible(viewport);
		viewport.setViewedComponent(&content, false);
		n->addSelectionListener(this);
	}

	~NodePropertyContent()
	{
		if(network != nullptr)
			network->removeSelectionListener(this);
	}

	void resized() override
	{
		viewport.setBounds(getLocalBounds());
		content.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), content.getHeight());
	}

	void selectionChanged(const NodeBase::List& selection)
	{
		editors.clear();

		auto y = 0;

		for (auto n : selection)
		{
			PropertyEditor* pe = new PropertyEditor(n, false, n->getValueTree());
			editors.add(pe);
			pe->setTopLeftPosition(0, y);
			pe->setSize(content.getWidth(), pe->getHeight());
			content.addAndMakeVisible(pe);
			y = pe->getBottom();
		}

		content.setSize(content.getWidth(), y);
	}

	WeakReference<DspNetwork> network;
	Component content;
	Viewport viewport;
	OwnedArray<PropertyEditor> editors;
};

juce::Component* NodePropertyPanel::createComponentForNetwork(DspNetwork* p)
{
	return new NodePropertyContent(p);
}



#if 0
SnexPopupEditor::SnexPopupEditor(const String& name, SnexSource* src, bool isPopup) :
	d(doc),
	Component(name),
	source(src),
	editor(d),
	popupMode(isPopup),
	corner(this, nullptr),
	compileButton("compile", this, f),
	resetButton("reset", this, f),
	asmButton("asm", this, f),
	optimiseButton("optimize", this, f),
	asmView(asmDoc, &tokeniser)
{
	

	optimizations = source->s.getOptimizationPassList();

	codeValue.referTo(source->expression.asJuceValue());
	codeValue.addListener(this);

	d.getCodeDocument().replaceAllContent(codeValue.getValue().toString());
	d.getCodeDocument().clearUndoHistory();

	for (auto& o : snex::OptimizationIds::getAllIds())
		s.addOptimization(o);

	s.addDebugHandler(this);

	d.getCodeDocument().addListener(this);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0x33888888));
	d.getCodeDocument().clearUndoHistory();
	editor.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	editor.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
	editor.setShowNavigation(false);

	asmView.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xFF333333));
	asmView.setFont(GLOBAL_MONOSPACE_FONT());
	asmView.setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF));
	asmView.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
	asmView.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	asmView.setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));
	asmView.setReadOnly(true);
	asmView.setOpaque(false);
	asmDoc.replaceAllContent("; no assembly generated");


	auto numLinesToShow = jmin(20, d.getCodeDocument().getNumLines());
	auto numColToShow = jlimit(60, 90, d.getCodeDocument().getMaximumLineLength());

	auto w = (float)numColToShow * editor.getFont().getStringWidth("M");
	auto h = editor.getFont().getHeight() * (float)numLinesToShow * 1.5f;

	setSize(roundToInt(w), roundToInt(h));

	addAndMakeVisible(editor);

	addAndMakeVisible(asmView);
	asmView.setVisible(false);

	addAndMakeVisible(corner);

	addAndMakeVisible(asmButton);
	addAndMakeVisible(compileButton);
	addAndMakeVisible(resetButton);
	addAndMakeVisible(optimiseButton);

	recompile();
}

void SnexPopupEditor::buttonClicked(Button* b)
{
	if (b == &resetButton && PresetHandler::showYesNoWindow("Do you want to reset the code", "Press OK to load the default code"))
	{

	}
	if (b == &asmButton)
	{
		bool showBoth = getWidth() > 800;

		auto visible = asmView.isVisible();

		asmView.setVisible(!visible);

		editor.setVisible(showBoth || visible);
		resized();
	}
	if (b == &compileButton)
	{
		recompile();
	}
	if (b == &optimiseButton)
	{
		auto allIds = snex::OptimizationIds::getAllIds();

		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		m.addItem(9001, "Enable All optimisations", true, allIds.size() == optimizations.size());
		m.addSeparator();

		int index = 0;
		for (auto& o : allIds)
		{
			m.addItem(index + 1, o, true, optimizations.contains(allIds[index]));
			index++;
		}

		int result = m.show() - 1;

		if (result == -1)
			return;

		source->s.clearOptimizations();

		if (result == 9000)
		{
			if (allIds.size() != optimizations.size())
				optimizations = allIds;
			else
				optimizations = {};
		}
		else
		{
			bool isActive = optimizations.contains(allIds[result]);

			if (!isActive)
				optimizations.add(allIds[result]);
			else
				optimizations.removeString(allIds[result]);
		}

		source->s.clearOptimizations();
		s.clearOptimizations();

		for (auto& o : optimizations)
		{
			source->s.addOptimization(o);
			s.addOptimization(o);
		}

		source->recompile();
		recompile();
	}
}

void SnexPopupEditor::valueChanged(Value& v)
{
	auto c = v.getValue().toString();

	auto code = d.getCodeDocument().getAllContent();

	if (c != code)
	{
		ScopedValueSetter<bool> s(internalChange, true);
		d.getCodeDocument().replaceAllContent(c);
		d.getCodeDocument().clearUndoHistory();
	}
}

void SnexPopupEditor::recompile()
{
	editor.clearWarningsAndErrors();
	auto code = d.getCodeDocument().getAllContent();

	snex::jit::Compiler compiler(s);
	compiler.setDebugHandler(this);

	source->initCompiler(compiler);

	auto obj = compiler.compileJitObject(code);

	asmDoc.replaceAllContent(compiler.getAssemblyCode());
	asmDoc.clearUndoHistory();

	Colour c;

	if (compiler.getCompileResult().wasOk())
	{
		c = Colour(0xFA181818);
		codeValue.setValue(code);
	}
	else
		c = JUCE_LIVE_CONSTANT_OFF(Colour(0xf21d1616));

	if (!popupMode)
		c = c.withAlpha(1.0f);

	editor.setColour(CodeEditorComponent::ColourIds::backgroundColourId, c);
}

void SnexPopupEditor::paint(Graphics& g)
{
	if (!popupMode)
	{
		g.fillAll(Colour(0xFF333333));
	}

	auto b = getLocalBounds();

	Rectangle<int> top;

	if (popupMode)
		top = b.removeFromTop(24);
	else
		top = b.removeFromBottom(24);

	GlobalHiseLookAndFeel::drawFake3D(g, top);
}

void SnexPopupEditor::resized()
{
	auto b = getLocalBounds();

	Rectangle<int> top;

	if (popupMode)
		top = b.removeFromTop(24);
	else
		top = b.removeFromBottom(24);

	compileButton.setBounds(top.removeFromRight(24));

	bool showBoth = getWidth() > 800;

	
	asmButton.setBounds(top.removeFromRight(24));
	resetButton.setBounds(top.removeFromRight(24));
	optimiseButton.setBounds(top.removeFromRight(24));

	if (asmView.isVisible())
	{
		asmView.setBounds(showBoth ? b.removeFromRight(550) : b);
	}
	
	editor.setBounds(b);


	if (!popupMode)
		corner.setVisible(false);

	corner.setBounds(getLocalBounds().removeFromBottom(15).removeFromRight(15));
}

juce::Path SnexPopupEditor::Icons::createPath(const String& url) const
{
	Path p;

	LOAD_PATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_PATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("asm", SnexIcons::asmIcon);
	LOAD_PATH_IF_URL("optimize", SnexIcons::optimizeIcon);

	return p;
}
#endif

}

