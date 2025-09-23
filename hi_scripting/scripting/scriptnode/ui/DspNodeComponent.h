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





class DefaultParameterNodeComponent : public NodeComponent,
									  public PathFactory
{
public:

	Path createPath(const String& url) const override;

	DefaultParameterNodeComponent(NodeBase* node);;

	void setExtraComponent(Component* newExtraComponent)
	{
		extraComponent = newExtraComponent;
		addAndMakeVisible(extraComponent);
	}

	struct NodePageTabComponent: public hise::PageTabComponent
	{
		NodePageTabComponent(NodeBase* n):
		  PageTabComponent(n->getUndoManager()),
		  v(n->getValueTree())
		{
			pageHeightFunction = getPageHeightStatic;
			layoutFunction = setGroupLayout;
		};

		void onExpandTabs() override;

		ValueTree getNodeTree() override { return v; }

		ValueTree v;
	};

	ScopedPointer<NodePageTabComponent> pageComponent;
	std::vector<PageTabComponent::Group> singlePageGroups;

	std::unique_ptr<PageInfo::Tree> pageTree;

	bool hasSpecialLayout() const
	{
		return pageTree->hasPageLayout() || pageTree->hasGroupTags();
	}

	Component* createSlider(int parameterIndex)
	{
		auto newSlider = new ParameterSlider(node.get(), parameterIndex);
		sliders.add(newSlider);
		return newSlider;
	}

	

	static Rectangle<int> getPageBounds(PageInfo::Tree& pageTree, Point<int> topLeft)
	{
		jassert(pageTree.hasPageLayout());

		if (pageTree.hasMoreThanOnePage())
		{
			int w = 0;
			int h = 0;

			for (const auto& p : pageTree.getPageNames())
			{
				int pw = 0;
				int ph = 0;


				for (auto& g : pageTree.getGroups(p))
				{
					if (g.isNotEmpty())
						ph += UIValues::GroupHeight;

					ph += UIValues::NodeMargin;

					auto numInGroup = pageTree.getList(p, g).size();
					auto pb = PositionHelpers::getPageBounds(numInGroup);

					pw = jmax(pb.getWidth(), pw);

					ph += pb.getHeight();

					ph -= 2 * UIValues::NodeMargin;
					ph -= UIValues::HeaderHeight;
				};

				w = jmax(pw, w);
				h = jmax(h, ph);
			}

			h -= 10;
			h += UIValues::HeaderHeight;
			h += UIValues::TabHeight + UIValues::NodeMargin;
			h += 2 * UIValues::NodeMargin;

			return Rectangle<int>(w, h).withPosition(topLeft);
		}
		else
		{
			int w = 0;
			int h = 0;

			auto firstPageId = pageTree.getPageNames()[0];

			for(const auto& g: pageTree.getGroups(firstPageId))
			{
				if(g.isNotEmpty())
					h += UIValues::GroupHeight;

				auto numInGroup = pageTree.getList(firstPageId, g).size();
				auto pb = PositionHelpers::getPageBounds(numInGroup);

				w = jmax(pb.getWidth(), w);

				h += pb.getHeight();

				h -= UIValues::NodeMargin;
				h -= UIValues::HeaderHeight;
			}

			h -= 10;
			h += UIValues::HeaderHeight;
			h += UIValues::TabHeight + UIValues::NodeMargin;
			h += 2 * UIValues::NodeMargin;

			return Rectangle<int>(w, h).withPosition(topLeft);
		}
	}

	static int getPageHeightStatic(const std::vector<PageTabComponent::Group>& groups)
	{
		int h = 0;

		for(auto& g: groups)
		{
			if(g.name.isNotEmpty())
				h += UIValues::GroupHeight + UIValues::NodeMargin;

			h += 48 + UIValues::NodeMargin + 28;
		}

		return h;
	}

	static void setGroupLayout(Rectangle<int> b, const std::vector<PageTabComponent::Group>& group, bool drawGroups)
	{
		for(const auto& g: group)
		{
			if(drawGroups)
				b.removeFromTop(UIValues::GroupHeight);


			PositionHelpers::applySliderPositions(b, g.groupComponents);
		}
	}

	void updateSliders(ValueTree , bool )
	{
		sliders.clear();

		if (node == nullptr)
			return;

		auto forceNoLayout = (int)node->getValueTree()[PropertyIds::CurrentPageIndex] == -1;

		if(pageTree->hasPageLayout() && !forceNoLayout)
		{
			addAndMakeVisible(pageComponent = new NodePageTabComponent(node.get()));
			pageComponent->buildParameters(*pageTree, BIND_MEMBER_FUNCTION_1(DefaultParameterNodeComponent::createSlider));
		}
		else
		{
			pageComponent = nullptr;

			for (int i = 0; i < node->getNumParameters(); i++)
			{
				addAndMakeVisible(createSlider(i));

				if(pageTree->hasGroupTags() && !forceNoLayout)
				{
					auto gid = pageTree->getGroupIdForParameter(i);

					if(gid.isNotEmpty())
						singlePageGroups.push_back({gid});
					else if (singlePageGroups.empty())
						singlePageGroups.push_back({ String() });

					singlePageGroups.back().groupComponents.add(sliders.getLast());
				}
			}
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
	HiseShapeButton tabButton;
};

}
