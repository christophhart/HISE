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


namespace SnexIcons
{
static const unsigned char asmIcon[] = { 110,109,176,114,148,64,8,172,122,65,98,74,12,134,64,47,221,130,65,14,45,90,64,31,133,134,65,55,137,33,64,31,133,134,65,98,156,196,144,63,31,133,134,65,0,0,0,0,170,241,122,65,0,0,0,0,240,167,100,65,98,0,0,0,0,53,94,78,65,156,196,144,63,186,73,60,65,55,
137,33,64,186,73,60,65,98,14,45,90,64,186,73,60,65,74,12,134,64,154,153,67,65,176,114,148,64,240,167,78,65,108,102,102,190,64,240,167,78,65,108,102,102,190,64,139,108,41,65,98,102,102,190,64,80,141,11,65,72,225,238,64,190,159,230,64,223,79,21,65,190,
159,230,64,108,240,167,78,65,190,159,230,64,108,240,167,78,65,176,114,148,64,98,154,153,67,65,74,12,134,64,186,73,60,65,14,45,90,64,186,73,60,65,55,137,33,64,98,186,73,60,65,156,196,144,63,53,94,78,65,0,0,0,0,240,167,100,65,0,0,0,0,98,170,241,122,65,
0,0,0,0,31,133,134,65,156,196,144,63,31,133,134,65,55,137,33,64,98,31,133,134,65,14,45,90,64,47,221,130,65,74,12,134,64,8,172,122,65,176,114,148,64,108,8,172,122,65,190,159,230,64,108,199,75,169,65,190,159,230,64,108,199,75,169,65,176,114,148,64,98,156,
196,163,65,74,12,134,64,172,28,160,65,14,45,90,64,172,28,160,65,55,137,33,64,98,172,28,160,65,156,196,144,63,246,40,169,65,0,0,0,0,211,77,180,65,0,0,0,0,98,164,112,191,65,0,0,0,0,238,124,200,65,156,196,144,63,238,124,200,65,55,137,33,64,98,238,124,200,
65,14,45,90,64,254,212,196,65,74,12,134,64,211,77,191,65,176,114,148,64,108,211,77,191,65,190,159,230,64,108,150,67,235,65,190,159,230,64,108,150,67,235,65,176,114,148,64,98,106,188,229,65,74,12,134,64,123,20,226,65,14,45,90,64,123,20,226,65,55,137,33,
64,98,123,20,226,65,156,196,144,63,197,32,235,65,0,0,0,0,162,69,246,65,0,0,0,0,98,63,181,0,66,0,0,0,0,94,58,5,66,156,196,144,63,94,58,5,66,55,137,33,64,98,94,58,5,66,14,45,90,64,102,102,3,66,74,12,134,64,209,162,0,66,176,114,148,64,108,209,162,0,66,190,
159,230,64,108,86,14,14,66,190,159,230,64,98,37,134,21,66,190,159,230,64,129,149,27,66,80,141,11,65,129,149,27,66,139,108,41,65,108,129,149,27,66,240,167,78,65,108,119,190,33,66,240,167,78,65,98,68,139,35,66,154,153,67,65,252,169,38,66,186,73,60,65,57,
52,42,66,186,73,60,65,98,168,198,47,66,186,73,60,65,205,76,52,66,53,94,78,65,205,76,52,66,240,167,100,65,98,205,76,52,66,170,241,122,65,168,198,47,66,31,133,134,65,57,52,42,66,31,133,134,65,98,252,169,38,66,31,133,134,65,68,139,35,66,47,221,130,65,119,
190,33,66,8,172,122,65,108,129,149,27,66,8,172,122,65,108,129,149,27,66,199,75,169,65,108,119,190,33,66,199,75,169,65,98,68,139,35,66,156,196,163,65,252,169,38,66,172,28,160,65,57,52,42,66,172,28,160,65,98,168,198,47,66,172,28,160,65,205,76,52,66,246,
40,169,65,205,76,52,66,211,77,180,65,98,205,76,52,66,164,112,191,65,168,198,47,66,238,124,200,65,57,52,42,66,238,124,200,65,98,252,169,38,66,238,124,200,65,68,139,35,66,254,212,196,65,119,190,33,66,211,77,191,65,108,129,149,27,66,211,77,191,65,108,129,
149,27,66,150,67,235,65,108,119,190,33,66,150,67,235,65,98,68,139,35,66,106,188,229,65,252,169,38,66,123,20,226,65,57,52,42,66,123,20,226,65,98,168,198,47,66,123,20,226,65,205,76,52,66,197,32,235,65,205,76,52,66,162,69,246,65,98,205,76,52,66,63,181,0,
66,168,198,47,66,94,58,5,66,57,52,42,66,94,58,5,66,98,252,169,38,66,94,58,5,66,68,139,35,66,102,102,3,66,119,190,33,66,209,162,0,66,108,129,149,27,66,209,162,0,66,108,129,149,27,66,162,69,10,66,98,129,149,27,66,106,188,17,66,37,134,21,66,199,203,23,66,
86,14,14,66,199,203,23,66,108,209,162,0,66,199,203,23,66,108,209,162,0,66,119,190,33,66,98,102,102,3,66,68,139,35,66,94,58,5,66,252,169,38,66,94,58,5,66,57,52,42,66,98,94,58,5,66,168,198,47,66,63,181,0,66,205,76,52,66,162,69,246,65,205,76,52,66,98,197,
32,235,65,205,76,52,66,123,20,226,65,168,198,47,66,123,20,226,65,57,52,42,66,98,123,20,226,65,252,169,38,66,106,188,229,65,68,139,35,66,150,67,235,65,119,190,33,66,108,150,67,235,65,199,203,23,66,108,211,77,191,65,199,203,23,66,108,211,77,191,65,119,
190,33,66,98,254,212,196,65,68,139,35,66,238,124,200,65,252,169,38,66,238,124,200,65,57,52,42,66,98,238,124,200,65,168,198,47,66,164,112,191,65,205,76,52,66,211,77,180,65,205,76,52,66,98,246,40,169,65,205,76,52,66,172,28,160,65,168,198,47,66,172,28,160,
65,57,52,42,66,98,172,28,160,65,252,169,38,66,156,196,163,65,68,139,35,66,199,75,169,65,119,190,33,66,108,199,75,169,65,199,203,23,66,108,8,172,122,65,199,203,23,66,108,8,172,122,65,119,190,33,66,98,47,221,130,65,68,139,35,66,31,133,134,65,252,169,38,
66,31,133,134,65,57,52,42,66,98,31,133,134,65,168,198,47,66,170,241,122,65,205,76,52,66,240,167,100,65,205,76,52,66,98,53,94,78,65,205,76,52,66,186,73,60,65,168,198,47,66,186,73,60,65,57,52,42,66,98,186,73,60,65,252,169,38,66,154,153,67,65,68,139,35,
66,240,167,78,65,119,190,33,66,108,240,167,78,65,199,203,23,66,108,223,79,21,65,199,203,23,66,98,72,225,238,64,199,203,23,66,102,102,190,64,106,188,17,66,102,102,190,64,162,69,10,66,108,102,102,190,64,209,162,0,66,108,176,114,148,64,209,162,0,66,98,74,
12,134,64,102,102,3,66,14,45,90,64,94,58,5,66,55,137,33,64,94,58,5,66,98,156,196,144,63,94,58,5,66,0,0,0,0,63,181,0,66,0,0,0,0,162,69,246,65,98,0,0,0,0,197,32,235,65,156,196,144,63,123,20,226,65,55,137,33,64,123,20,226,65,98,14,45,90,64,123,20,226,65,
74,12,134,64,106,188,229,65,176,114,148,64,150,67,235,65,108,102,102,190,64,150,67,235,65,108,102,102,190,64,211,77,191,65,108,176,114,148,64,211,77,191,65,98,74,12,134,64,254,212,196,65,14,45,90,64,238,124,200,65,55,137,33,64,238,124,200,65,98,156,196,
144,63,238,124,200,65,0,0,0,0,164,112,191,65,0,0,0,0,211,77,180,65,98,0,0,0,0,246,40,169,65,156,196,144,63,172,28,160,65,55,137,33,64,172,28,160,65,98,14,45,90,64,172,28,160,65,74,12,134,64,156,196,163,65,176,114,148,64,199,75,169,65,108,102,102,190,
64,199,75,169,65,108,102,102,190,64,8,172,122,65,108,176,114,148,64,8,172,122,65,99,109,14,45,6,66,123,20,1,66,108,14,45,6,66,127,106,76,65,108,6,129,53,65,127,106,76,65,108,6,129,53,65,123,20,1,66,108,14,45,6,66,123,20,1,66,99,101,0,0 };

static const unsigned char optimizeIcon[] = { 110,109,12,2,148,65,141,151,197,65,108,70,182,101,65,164,112,164,65,108,250,126,180,65,154,153,69,65,108,70,182,101,65,166,155,132,64,108,12,2,148,65,0,0,0,0,108,129,149,246,65,233,38,69,65,108,41,92,246,65,154,153,69,65,108,129,149,246,65,49,8,70,65,
108,12,2,148,65,141,151,197,65,99,109,166,155,132,64,141,151,197,65,108,0,0,0,0,164,112,164,65,108,174,71,3,65,154,153,69,65,108,0,0,0,0,166,155,132,64,108,166,155,132,64,0,0,0,0,108,94,186,131,65,233,38,69,65,108,6,129,131,65,154,153,69,65,108,94,186,
131,65,49,8,70,65,108,166,155,132,64,141,151,197,65,99,101,0,0 };

}


SnexPopupEditor::SnexPopupEditor(const String& name, SnexSource* src, bool isPopup) :
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

	d.replaceAllContent(codeValue.getValue().toString());
	d.clearUndoHistory();

	for (auto& o : snex::OptimizationIds::getAllIds())
		s.addOptimization(o);

	s.addDebugHandler(this);

	d.addListener(this);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);

	editor.setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0x33888888));
	d.clearUndoHistory();
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


	auto numLinesToShow = jmin(20, d.getNumLines());
	auto numColToShow = jlimit(60, 90, d.getMaximumLineLength());

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

	auto code = d.getAllContent();

	if (c != code)
	{
		ScopedValueSetter<bool> s(internalChange, true);
		d.replaceAllContent(c);
		d.clearUndoHistory();
	}
}

void SnexPopupEditor::recompile()
{
	editor.clearWarningsAndErrors();
	auto code = d.getAllContent();

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

}

