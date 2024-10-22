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

using namespace hise;
using namespace juce;






class NodeComponent : public ComponentWithMiddleMouseDrag,
					  public DspNetwork::SelectionListener,
					  public ComponentWithDocumentation
{
public:

	enum class MenuActions
	{
		ExportAsCpp = 1,
		ExportAsCppProject,
		ExportAsSnippet,
		ExportAsTemplate,
		CreateScreenShot,
		EditProperties,
		UnfreezeNode,
		ExplodeLocalCables,
		FreezeNode,
		WrapIntoDspNetwork,
		WrapIntoChain,
		WrapIntoSplit,
		WrapIntoMulti,
		WrapIntoFrame,
		WrapIntoFix32,
		WrapIntoMidiChain,
		WrapIntoNoMidiChain,
		WrapIntoCloneChain,
		WrapIntoSoftBypass,
		WrapIntoOversample4,
		SurroundWithFeedback,
		SurroundWithMSDecoder,
		numMenuActions
	};

	using Factory = NodeComponentFactory;

	struct Header : public Component,
		public ButtonListener,
        public SettableTooltipClient,
		public DragAndDropTarget
	{
		Header(NodeComponent& parent_);

		void buttonClicked(Button* b) override;

		String getPowerButtonId(bool getOff) const;

		void updatePowerButtonState(Identifier id, var newValue);
		void updateConnectionButton(Identifier id, var newValue);

		void paint(Graphics& g) override;
		void resized() override;

		bool keyPressed(const KeyPress& key) override
		{
			return parent.keyPressed(key);
		}

		void mouseDoubleClick(const MouseEvent& event) override;
		void mouseDown(const MouseEvent& e) override;
		void mouseUp(const MouseEvent& e) override;
		void mouseDrag(const MouseEvent& e) override;
		
		bool isInterestedInDragSource(const SourceDetails&) override;

		void itemDragEnter(const SourceDetails& dragSourceDetails) override
		{
			auto b = powerButton.getLocalBounds();
			isHoveringOverBypass = b.contains(dragSourceDetails.localPosition);

			repaint();
		}

		bool isRootHeader() const
		{
			return parent.node->getRootNetwork()->getRootNode() == parent.node.get();
		}

		void updateColour(Identifier id, var value)
		{
			colour = PropertyHelpers::getColourFromVar(value);
			repaint();
		}

		void itemDragMove(const SourceDetails& dragSourceDetails) override
		{
			auto b = powerButton.getLocalBounds();
			isHoveringOverBypass = b.contains(dragSourceDetails.localPosition);

			repaint();
		}

		void itemDragExit(const SourceDetails&) override
		{
			isHoveringOverBypass = false;
			repaint();
		}

		virtual void itemDropped(const SourceDetails& dragSourceDetails)
		{
			if (isHoveringOverBypass)
			{
				parent.node->connectToBypass(dragSourceDetails.description);
			}

			isHoveringOverBypass = false;
			repaint();
		}

		void setShowRenameLabel(bool shouldShow);

		ScopedPointer<TextEditor> renameLabel;

		NodeComponent& parent;
		Factory f;

		Colour colour = Colours::transparentBlack;

		valuetree::RecursiveTypedChildListener dynamicPowerUpdater;

		valuetree::PropertyListener powerButtonUpdater;
		valuetree::PropertyListener parameterUpdater;
		valuetree::PropertyListener colourUpdater;
		HiseShapeButton powerButton;
		HiseShapeButton deleteButton;
		HiseShapeButton parameterButton;
		HiseShapeButton freezeButton;
		
		bool isDragging = false;

		ComponentDragger d;

		bool isHoveringOverBypass = false;
	};

	struct EmbeddedNetworkBar : public Component,
							    public ButtonListener
	{
		EmbeddedNetworkBar(NodeBase* n);

		void paint(Graphics& g) override
		{
			g.setColour(Colour(0x1e000000));
			auto b = getLocalBounds().reduced(1, 0);
			g.fillRect(b);
			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawHorizontalLine(getHeight(), 1.0f, (float)getWidth() - 2.0f);
		}

		void buttonClicked(Button* b) override;

		void resized() override;

		struct Factory : public PathFactory
		{
			Path createPath(const String& url) const override;
		} f;

		void updateFreezeState(const Identifier& id, const var& newValue);

		HiseShapeButton gotoButton;
		HiseShapeButton freezeButton;
		HiseShapeButton warningButton;

		valuetree::PropertyListener freezeUpdater;

		WeakReference<NodeBase> parentNode;
		WeakReference<DspNetwork> embeddedNetwork;
	};

	NodeComponent(NodeBase* b);;
	virtual ~NodeComponent();

	struct PositionHelpers
	{
		static juce::Rectangle<int> getPositionInCanvasForStandardSliders(const NodeBase* n, Point<int> topLeft);

		static juce::Rectangle<int> createRectangleForParameterSliders(const NodeBase* n, int numColumns);
	};

	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;
	void resized() override;

	bool keyPressed(const KeyPress& key) override
	{
		if(key == KeyPress::F2Key)
		{
			header.setShowRenameLabel(true);
			return true;
		}

		return false;
	}

	MarkdownLink getLink() const override;

	void selectionChanged(const NodeBase::List& selection) override;

	struct PopupHelpers
	{
		static int isWrappable(NodeBase* n);

		static void wrapIntoNetwork(NodeBase* node, bool makeCompileable=false);

		static void wrapIntoChain(NodeBase* node, MenuActions result, String idToUse = String());
	};

	

	


	virtual void fillContextMenu(PopupMenu& m);

	virtual void handlePopupMenuResult(int result);

	virtual Colour getOutlineColour() const;

	Colour getHeaderColour() const;

	static void drawTopBodyGradient(Graphics& g, Rectangle<float> b, float alpha=0.1f, float height=15.0f);

	bool isRoot() const;
	bool isFolded() const;
	bool isDragged() const;
	bool isSelected() const;
	bool isBeingCopied() const;

	void repaintAllNodes()
	{
		repaint();

		for (int i = 0; i < getNumChildComponents(); i++)
		{
			if (auto childNode = dynamic_cast<NodeComponent*>(getChildComponent(i)))
			{
				childNode->repaintAllNodes();
			}
		}
	}

	bool wasSelected = false;
	ValueTree dataReference;

	ReferenceCountedObjectPtr<NodeBase> node;
	Header header;
	ScopedPointer<EmbeddedNetworkBar> embeddedNetworkBar;

	valuetree::PropertyListener repaintListener;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeComponent);
};




struct DeactivatedComponent : public NodeComponent
{
	DeactivatedComponent(NodeBase* b);

	void paint(Graphics& g) override;
	void resized() override;
};

struct simple_visualiser : public ScriptnodeExtraComponent<NodeBase>
{
	simple_visualiser(NodeBase*, PooledUIUpdater* u);

	NodeBase* getNode();
	double getParameter(int index);
    
    InvertableParameterRange getParameterRange(int index);
    
	Colour getNodeColour();

	void timerCallback() override;
	virtual void rebuildPath(Path& path) = 0;
	void paint(Graphics& g) override;

	Path original;
	Path gridPath;
	Path p;

	bool stroke = true;
	bool drawBackground = true;
	float thickness = 1.0f;
};

}
