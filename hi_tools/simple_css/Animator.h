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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/
namespace hise {
namespace simple_css
{
using namespace juce;


struct Animator: public Timer
{
	struct ScopedComponentSetter
	{
		ScopedComponentSetter(Component* c);
		~ScopedComponentSetter();

		Component::SafePointer<Component> prev;
		Animator* a = nullptr;
	};

	Component::SafePointer<Component> currentlyRenderedComponent;

	struct Item
	{
		Item() = default;
		Item(Animator& parent, StyleSheet::Ptr css_, Transition tr_);;

		bool timerCallback(double deltaMs);

		Component::SafePointer<Component> target;

		StyleSheet::Ptr css;
		Transition transitionData;

		PropertyKey startValue;
		PropertyKey endValue;
		
		double currentProgress = 0.0;
		bool reverse = false;
		int waitCounter = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Item);
	};

	Animator();

	void timerCallback() override;

	double lastCallbackTime = 0.0;
	OwnedArray<Item> items;
};

struct StateWatcher
{
	using TextData = std::tuple<String, Justification, Rectangle<float>>;

	StateWatcher(Animator& animator_):
	  animator(animator_)
	{};

	Animator& animator;

	struct Item
	{
		std::pair<bool, int> changed(int stateFlag);

		void renderShadow(Graphics& g, const TextData& textData, const std::vector<melatonin::ShadowParameters>& parameters, bool wantsInset);
		void renderShadow(Graphics& g, const Path& p, const std::vector<melatonin::ShadowParameters>& parameters, bool wantsInset);

		Component::SafePointer<Component> c;
		int currentState = 0;
		
		melatonin::DropShadow dropShadow;
		melatonin::InnerShadow innerShadow;
		melatonin::DropShadow dropShadowText;
		melatonin::InnerShadow innerShadowText;
	};

	template <typename RenderObject> void renderShadow(Graphics& g, const RenderObject& p, Component* c, const std::vector<melatonin::ShadowParameters>& parameters, bool wantsInset)
	{
		if(parameters.empty())
			return;

		if(c == nullptr)
		{
			noComponentItem.renderShadow(g, p, parameters, wantsInset);
			return;
		}

		for(auto& item: items)
		{
			if(item.c == c)
			{
				item.renderShadow(g, p, parameters, wantsInset);
				break;
			}
		}
	}

	void checkChanges(Component* c, StyleSheet::Ptr ss, int currentState);

	std::pair<bool, int> changed(Component* c, int stateFlag);

	void registerComponentToUpdate(Component* c);

	Array<Item> items;
	
	struct UpdatedComponent
	{
		bool operator==(const UpdatedComponent& other) const { return target.getComponent() == other.target.getComponent(); }

		Component::SafePointer<Component> target;

		void update(StyleSheet::Ptr ss, int currentState);

		bool initialised = false;
	};

	Array<UpdatedComponent> updatedComponents;
	Item noComponentItem;
};

	
} // namespace simple_css
} // namespace hise