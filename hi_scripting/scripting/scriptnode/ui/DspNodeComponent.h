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

struct ModulationSourceBaseComponent : public Component,
	public PooledUIUpdater::SimpleTimer
{
	ModulationSourceBaseComponent(PooledUIUpdater* updater) :
		SimpleTimer(updater)
	{};

	ModulationSourceNode* getSourceNodeFromParent() const
	{
		if (sourceNode == nullptr)
		{
			if (auto pc = findParentComponentOfClass<NodeComponent>())
			{
				sourceNode = dynamic_cast<ModulationSourceNode*>(pc->node.get());
			}
		}

		return sourceNode;
	}

	void timerCallback() override {};

	void paint(Graphics& g) override
	{
		g.setColour(Colour(0xFF999999));
		g.drawRect(getLocalBounds(), 1);

		g.setColour(Colours::white.withAlpha(0.1f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Drag to modulation target", getLocalBounds().toFloat(), Justification::centred);
	}

	Image createDragImage()
	{
		Image img(Image::ARGB, 128, 48, true);
		Graphics g(img);

		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.4f));
		g.fillAll();
		g.setColour(Colours::black);
		g.setFont(GLOBAL_BOLD_FONT());

		g.drawText(getSourceNodeFromParent()->getId(), 0, 0, 128, 48, Justification::centred);

		return img;
	}

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& event) override
	{
		if (getSourceNodeFromParent() == nullptr)
			return;

		if (auto container = DragAndDropContainer::findParentDragContainerFor(this))
		{
			// We need to be able to drag it anywhere...
			while (auto pc = DragAndDropContainer::findParentDragContainerFor(dynamic_cast<Component*>(container)))
				container = pc;

			var d;

			DynamicObject::Ptr details = new DynamicObject();

			details->setProperty(PropertyIds::ID, sourceNode->getId());
			details->setProperty(PropertyIds::ModulationTarget, true);

			container->startDragging(var(details), this, createDragImage());
		}
	}

protected:

	mutable WeakReference<ModulationSourceNode> sourceNode;
};

struct ModulationSourcePlotter : ModulationSourceBaseComponent
{
	ModulationSourcePlotter(PooledUIUpdater* updater):
		ModulationSourceBaseComponent(updater)
	{
		setSize(0, ModulationSourceNode::ModulationBarHeight);
		buffer.setSize(1, ModulationSourceNode::RingBufferSize);
	}

	void timerCallback() override
	{
		if (getSourceNodeFromParent() != nullptr)
		{
			auto numNew = sourceNode->fillAnalysisBuffer(buffer);

			if (numNew != 0)
				rebuildPath();
		}
	}

	

	void rebuildPath()
	{
		float offset = 2.0f;
		
		float rectangleWidth = 0.5f;

		auto width = (float)getWidth() - 2.0 * offset;
		auto maxHeight = (float)getHeight() - 2.0 * offset;

		int samplesPerPixel = ModulationSourceNode::RingBufferSize / jmax((int)(width/rectangleWidth), 1);

		rectangles.clear();

		int sampleIndex = 0;

		for (float i = 0.0f; i < width; i+= rectangleWidth)
		{
			float maxValue = jlimit(0.0f, 1.0f, buffer.getMagnitude(0, sampleIndex, samplesPerPixel));
			FloatSanitizers::sanitizeFloatNumber(maxValue);
			float height = maxValue * maxHeight;
			float y = offset + maxHeight - height;

			sampleIndex += samplesPerPixel;

			rectangles.add({ i + offset, y, rectangleWidth, height });
		}
		repaint();
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF333333));

		g.setColour(Colour(0xFF999999));
		g.fillRectList(rectangles);

		ModulationSourceBaseComponent::paint(g);		
	}

	RectangleList<float> rectangles;
	AudioSampleBuffer buffer;
	
};


struct FeedbackTargetComponent : public NodeComponent,
								 public ComboBox::Listener
{
	FeedbackTargetComponent(NodeBase* n);

	void resized() override
	{
		NodeComponent::resized();
		auto b = getLocalBounds();
		b.removeFromTop(UIValues::HeaderHeight);
		sourceSelector.setBounds(b.reduced(5));
	}

	

	void comboBoxChanged(ComboBox*) override
	{
		node->setProperty(PropertyIds::Connection, sourceSelector.getText());
	}

	void updateComboBox(Identifier, var newValue)
	{
		auto t = newValue.toString();

		if (t.isEmpty())
			sourceSelector.setSelectedId(0, dontSendNotification);
		else
			sourceSelector.setText(t, dontSendNotification);
	}

	valuetree::PropertyListener comboboxUpdater;
	ComboBox sourceSelector;
};

struct DefaultParameterNodeComponent : public NodeComponent
{
	DefaultParameterNodeComponent(NodeBase* node);;

	void setExtraComponent(Component* newExtraComponent)
	{
		extraComponent = newExtraComponent;
		addAndMakeVisible(extraComponent);
		
		if (extraComponent != nullptr)
			extraWidth = extraComponent->getWidth();
	}

	void resized() override;

	int extraWidth = -1;
	ScopedPointer<Component> extraComponent;
	OwnedArray<ParameterSlider> sliders;
};

}
