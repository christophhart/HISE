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




class NodeComponent : public Component,
					  public DspNetwork::SelectionListener
{
public:

	enum class MenuActions
	{
		ExportAsCpp = 1,
		ExportAsCppProject,
		ExportAsSnippet,
		EditProperties,
		UnfreezeNode,
		FreezeNode,
		WrapIntoChain,
		WrapIntoSplit,
		WrapIntoMulti,
		WrapIntoFrame,
		WrapIntoOversample4,
		SurroundWithFeedback,
		SurroundWithMSDecoder,
		numMenuActions
	};

	struct Factory : public PathFactory
	{
		String getId() const { return "PowerButton"; };

		Path createPath(const String& id) const override;
	};

	struct Header : public Component,
		public ButtonListener,
		public DragAndDropTarget
	{
		Header(NodeComponent& parent_);

		void buttonClicked(Button* b) override;

		String getPowerButtonId(bool getOff) const;

		void updatePowerButtonState(Identifier id, var newValue);

		void paint(Graphics& g) override;
		void resized() override;

		void mouseDoubleClick(const MouseEvent& event) override;
		void mouseDown(const MouseEvent& e) override;
		void mouseUp(const MouseEvent& e) override;
		void mouseDrag(const MouseEvent& e) override;
		
		bool isInterestedInDragSource(const SourceDetails&) override
		{
			return true;
		}

		void itemDragEnter(const SourceDetails& dragSourceDetails) override
		{
			auto b = powerButton.getLocalBounds();
			isHoveringOverBypass = b.contains(dragSourceDetails.localPosition);

			repaint();
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
				parent.node->addConnectionToBypass(dragSourceDetails.description);
			}

			isHoveringOverBypass = false;
			repaint();
		}
		
		

		NodeComponent& parent;
		Factory f;

		Colour colour = Colours::transparentBlack;

		valuetree::PropertyListener powerButtonUpdater;
		valuetree::PropertyListener colourUpdater;
		HiseShapeButton powerButton;
		HiseShapeButton deleteButton;
		HiseShapeButton parameterButton;
		
		bool isDragging = false;

		ComponentDragger d;

		bool isHoveringOverBypass = false;
	};

	NodeComponent(NodeBase* b);;
	~NodeComponent();

	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;
	void resized() override;

	void selectionChanged(const NodeBase::List& selection) override;

	virtual void fillContextMenu(PopupMenu& m);

	virtual void handlePopupMenuResult(int result);

	Colour getOutlineColour() const;

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
	NodeBase::Ptr node;
	Header header;

	valuetree::PropertyListener repaintListener;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeComponent);
};




struct DeactivatedComponent : public NodeComponent
{
	DeactivatedComponent(NodeBase* b);

	void paint(Graphics& g) override;
	void resized() override;
};

}
