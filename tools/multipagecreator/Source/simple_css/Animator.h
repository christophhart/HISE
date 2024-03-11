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

		bool timerCallback();

		Component::SafePointer<Component> target;

		StyleSheet::Ptr css;
		Transition transitionData;

		StyleSheet::PropertyKey startValue;
		StyleSheet::PropertyKey endValue;
		
		double currentProgress = 0.0;
		bool reverse = false;
		int waitCounter = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Item);
	};

	Animator()
	{
		startTimer(15);
	}

	void timerCallback() override;

	OwnedArray<Item> items;
};



struct StateWatcher
{
	StateWatcher(Animator& animator_):
	  animator(animator_)
	{};

	Animator& animator;

	struct Item
	{
		std::pair<bool, int> changed(int stateFlag)
		{
			if(stateFlag != currentState)
			{
				auto prevState = currentState;
				currentState = stateFlag;

				return { true, prevState };
			}

			return { false, currentState };
		}

		Component::SafePointer<Component> c;
		int currentState = 0;

		melatonin::DropShadow dropShadow;
		melatonin::InnerShadow innerShadow;
	};

	void renderShadow(Graphics& g, const Path& p, Component* c, const std::vector<melatonin::ShadowParameters>& parameters, bool wantsInset)
	{
		if(parameters.empty())
			return;

		for(auto& item: items)
		{
			if(item.c == c)
			{
				if(wantsInset)
				{
					for(int i = 0; i < parameters.size(); i++)
						item.innerShadow.setShadow(parameters[i], i);

					item.innerShadow.render(g, p);
				}
				else
				{
					for(int i = 0; i < parameters.size(); i++)
						item.dropShadow.setShadow(parameters[i], i);

					item.dropShadow.render(g, p);
				}

				break;
			}
		}
	}

	void checkChanges(Component* c, StyleSheet::Ptr ss, int currentState)
	{
		auto stateChanged = changed(c, currentState);

		for(int i = 0; i < updatedComponents.size(); i++)
		{
			auto& uc = updatedComponents.getReference(i);

			if(uc.target == nullptr)
			{
				updatedComponents.remove(i--);
				continue;
			}
			
			if(uc.target != c)
				continue;

			if(!uc.initialised || stateChanged.first)
			{
				uc.update(ss, currentState);
			}

		}

		if(stateChanged.first)
		{
			for(const auto& p: *ss)
			{
				if(p.values.find(stateChanged.second) != p.values.end() &&
				   p.values.find(currentState) != p.values.end())
				{
					auto prev = p.values.at(stateChanged.second);
					auto current = p.values.at(currentState);

					if(prev.transition || current.transition)
					{
						auto thisTransition = current.transition ? current.transition : prev.transition;

						StyleSheet::PropertyKey thisStartValue = { p.name, stateChanged.second };
						StyleSheet::PropertyKey thisEndValue = { p.name, currentState };

						bool found = false;

						for(auto i: animator.items)
						{
							if(i->css == ss &&
							   i->target == animator.currentlyRenderedComponent &&
							   i->startValue.name == p.name)
							{
								if(currentState == i->startValue.state)
								{
									i->reverse = !i->reverse;
									found = true;
									break;
								}
								else
								{
									i->endValue.state = currentState;
									i->transitionData = thisTransition;
									found = true;
									break;
								}
							}
						}

						if(found)
							continue;

						auto ad = new Animator::Item(animator, ss, thisTransition);
						
						ad->startValue = thisStartValue;
						ad->endValue = thisEndValue;
						
						animator.items.add(ad);
					}
				}
			}
		}
	}

	std::pair<bool, int> changed(Component* c, int stateFlag)
	{
		for(auto& i: items)
		{
			if(i.c == c)
				return i.changed(stateFlag);
		}

		items.add({ c, stateFlag });
		return { false, stateFlag };
	}

	void registerComponentToUpdate(Component* c)
	{
		updatedComponents.addIfNotAlreadyThere({ c });
	}

	Array<Item> items;
	
	struct UpdatedComponent
	{
		bool operator==(const UpdatedComponent& other) const { return target.getComponent() == other.target.getComponent(); }

		Component::SafePointer<Component> target;

		void update(StyleSheet::Ptr ss, int currentState)
		{
			ss->setupComponent(target.getComponent(), currentState);
			initialised = true;
		}

		bool initialised = false;
	};

	Array<UpdatedComponent> updatedComponents;
};

	
} // namespace simple_css
} // namespace hise