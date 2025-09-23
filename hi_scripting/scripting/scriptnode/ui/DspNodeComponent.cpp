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


juce::Path DefaultParameterNodeComponent::createPath(const String& url) const
{
	Path p;
	LOAD_EPATH_IF_URL("tabs", ScriptnodeIcons::tabIcon);
	return p;
}

DefaultParameterNodeComponent::DefaultParameterNodeComponent(NodeBase* node):
	NodeComponent(node),
	tabButton("tabs", nullptr, *this)
{
	parameterListener.setCallback(node->getParameterTree(), valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(DefaultParameterNodeComponent::updateSliders));

	pageTree = PageInfo::createPageTree(node->getParameterTree());

	tabButton.setTooltip("Group parameters into pages & tabs");

	tabButton.onClick = [this, node]()
	{
		this->node->getValueTree().setProperty(PropertyIds::CurrentPageIndex, 0, node->getUndoManager());

		if(auto dn = findParentComponentOfClass<DspNetworkGraph>())
		{
			MessageManager::callAsync([dn]()
			{
				dn->rebuildNodes();
			});
		}
	};

	addChildComponent(tabButton);
	
	updateSliders(node->getParameterTree(), false);
}



void DefaultParameterNodeComponent::resized()
{
	
	NodeComponent::resized();

	auto b = getLocalBounds();
	b = b.reduced(UIValues::NodeMargin);
	b.removeFromTop(UIValues::HeaderHeight);

	if (extraComponent != nullptr)
	{
		extraComponent->setBounds(b.removeFromTop(extraComponent->getHeight()));
		b.removeFromTop(UIValues::NodeMargin);
	}
	
	tabButton.setVisible((int)node->getValueTree()[PropertyIds::CurrentPageIndex] == -1);

	if(tabButton.isVisible())
	{
		tabButton.toFront(false);
		auto tb = getLocalBounds().reduced(UIValues::NodeMargin);
		tabButton.setBounds(tb.removeFromRight(24).removeFromBottom(24).reduced(4));
	}
		

	if(pageComponent != nullptr)
	{
		pageComponent->setBounds(b);
	}
	else if (!singlePageGroups.empty())
	{
		setGroupLayout(b, singlePageGroups, true);
	}
	else
	{
		Array<Component::SafePointer<Component>> sliderList;

		for(auto& s: sliders)
			sliderList.add(s);

		PositionHelpers::applySliderPositions(b, sliderList);


	}
}

void DefaultParameterNodeComponent::NodePageTabComponent::onExpandTabs()
{
	if (auto dn = findParentComponentOfClass<DspNetworkGraph>())
	{
		MessageManager::callAsync([dn]()
		{
			dn->rebuildNodes();
		});
	}
}

}

