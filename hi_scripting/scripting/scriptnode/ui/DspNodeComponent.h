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





class DefaultParameterNodeComponent : public NodeComponent
{
public:

	DefaultParameterNodeComponent(NodeBase* node);;

	void setExtraComponent(Component* newExtraComponent)
	{
		extraComponent = newExtraComponent;
		addAndMakeVisible(extraComponent);
	}

	void updateSliders(ValueTree , bool )
	{
		sliders.clear();

		if (node == nullptr)
			return;

		for (int i = 0; i < node->getNumParameters(); i++)
		{
			auto newSlider = new ParameterSlider(node.get(), i);

			addAndMakeVisible(newSlider);
			sliders.add(newSlider);
		}

		resized();
	}

	void applyExtraComponentToBounds(Rectangle<int>& bounds)
	{
		if (extraComponent != nullptr)
		{
			bounds.setWidth(jmax(bounds.getWidth(), extraComponent->getWidth()));
			bounds.setHeight(bounds.getHeight() + extraComponent->getHeight() + UIValues::NodeMargin);
		}
	}

	void resized() override;

	ScopedPointer<Component> extraComponent;
	OwnedArray<ParameterSlider> sliders;

	valuetree::ChildListener parameterListener;
};

}
