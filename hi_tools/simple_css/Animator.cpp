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
namespace hise
{
namespace simple_css
{

Animator::ScopedComponentSetter::ScopedComponentSetter(Component* c)
{
	auto root = dynamic_cast<CSSRootComponent*>(c);

	if(root == nullptr && c != nullptr)
		root = c->findParentComponentOfClass<CSSRootComponent>();

	if(root != nullptr)
	{
		a = &root->animator;
		prev = a->currentlyRenderedComponent;
		a->currentlyRenderedComponent = c;
	}
}

Animator::ScopedComponentSetter::~ScopedComponentSetter()
{
	if(a != nullptr)
		a->currentlyRenderedComponent = prev;
}

Animator::Item::Item(Animator& parent, StyleSheet::Ptr css_, Transition tr_):
	css(css_),
	transitionData(tr_),
	target(parent.currentlyRenderedComponent)
{
	jassert(target != nullptr);
}

bool Animator::Item::timerCallback(double delta)
{
	if(target.getComponent() == nullptr)
		return false;

	auto d = delta * 0.001;

	if(transitionData.duration > 0.0)
		d /= transitionData.duration;

	if(reverse)
		d *= -1.0;

	currentProgress += d;

	if(currentProgress > 1.0 || currentProgress < 0.0)
	{
		currentProgress = jlimit(0.0, 1.0, currentProgress);
		target->repaint();
		return false;
	}

	if(target.getComponent() != nullptr)
		target->repaint();
	else
	{
		return false;
	}

	return true;
}

Animator::Animator()
{
	startTimer(15);
}

void Animator::timerCallback()
{
	auto thisTime = Time::getMillisecondCounterHiRes();

	auto delta = thisTime - lastCallbackTime;

	for(int i = 0; i < items.size(); i++)
	{
		if(!items[i]->timerCallback(delta))
			items.remove(i--);
	}

	lastCallbackTime = thisTime;
}

std::pair<bool, int> StateWatcher::Item::changed(int stateFlag)
{
	if(stateFlag != currentState)
	{
		auto prevState = currentState;
		currentState = stateFlag;

		return { true, prevState };
	}

	return { false, currentState };
}

void StateWatcher::Item::renderShadow(Graphics& g, const TextData& textData,
	const std::vector<melatonin::ShadowParameters>& parameters, bool wantsInset)
{
	if(wantsInset)
	{
		for(int i = 0; i < parameters.size(); i++)
			innerShadowText.setShadow(parameters[i], i);

		innerShadowText.render(g, std::get<0>(textData), std::get<2>(textData), std::get<1>(textData));
	}
	else
	{
		for(int i = 0; i < parameters.size(); i++)
			dropShadowText.setShadow(parameters[i], i);

		dropShadowText.render(g, std::get<0>(textData), std::get<2>(textData), std::get<1>(textData));
	}
}

void StateWatcher::Item::renderShadow(Graphics& g, const Path& p,
	const std::vector<melatonin::ShadowParameters>& parameters, bool wantsInset)
{
	if(wantsInset)
	{
		for(int i = 0; i < parameters.size(); i++)
			innerShadow.setShadow(parameters[i], i);

		innerShadow.render(g, p);
	}
	else
	{
		for(int i = 0; i < parameters.size(); i++)
			dropShadow.setShadow(parameters[i], i);

		dropShadow.render(g, p);
	}
}

void StateWatcher::checkChanges(Component* c, StyleSheet::Ptr ss, int currentState)
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
		ss->forEachProperty(PseudoElementType::All, [&](PseudoElementType t, Property& p)
		{
			auto findPropertyValue = [&](int stateToFind)
			{
				return p.getProperty(stateToFind);
			};

			auto f1 = findPropertyValue(stateChanged.second);
			auto f2 = findPropertyValue(currentState);
				
			if( f1 || f2)
			{
				auto pt = ss->getTransitionOrDefault(t, f1.transition);
				auto ct = ss->getTransitionOrDefault(t, f2.transition);
					
				if(pt || ct)
				{
					auto thisTransition = ct ? ct : pt;

					PropertyKey thisStartValue(p.name, PseudoState(stateChanged.second).withElement(t));
					PropertyKey thisEndValue(p.name, PseudoState(currentState).withElement(t));
                    
                    

                    
					bool found = false;

					for(auto i: animator.items)
					{
						if(i->css == ss &&
							i->target == animator.currentlyRenderedComponent &&
							i->startValue.name == p.name &&
							i->startValue.state.matchesElement(t))
						{
							if(currentState == i->startValue.state.stateFlag)
							{
								i->reverse = !i->reverse;
								found = true;
								break;
							}
							else
							{
								i->currentProgress = 0.0;
								i->startValue.state = i->endValue.state;
								i->endValue.state.stateFlag = currentState;
								i->transitionData = thisTransition;
								found = true;
								break;
							}
						}
					}

					if(found)
						return false;

					auto ad = new Animator::Item(animator, ss, thisTransition);
						
					ad->startValue = thisStartValue;
					ad->endValue = thisEndValue;
						
					animator.items.add(ad);
				}
			}
			
			return false;
		});

	}
}

std::pair<bool, int> StateWatcher::changed(Component* c, int stateFlag)
{
	for(auto& i: items)
	{
		if(i.c == c)
			return i.changed(stateFlag);
	}

	items.add({ c, stateFlag });
	return { false, stateFlag };
}

void StateWatcher::registerComponentToUpdate(Component* c)
{
	updatedComponents.addIfNotAlreadyThere({ c });
}

void StateWatcher::UpdatedComponent::update(StyleSheet::Ptr ss, int currentState)
{
	ss->setupComponent(target.getComponent(), currentState);
	initialised = true;
}
}
}


