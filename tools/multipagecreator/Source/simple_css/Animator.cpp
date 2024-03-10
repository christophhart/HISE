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
	auto root = dynamic_cast<ComponentWithCSS*>(c);

	if(root == nullptr)
		root = c->findParentComponentOfClass<ComponentWithCSS>();

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

bool Animator::Item::timerCallback()
{
	auto d = 1.0 * 0.015;

	if(transitionData.duration > 0.0)
		d /= transitionData.duration;

	if(reverse)
		d *= -1.0;

	currentProgress += d;

	if(currentProgress > 1.0 || currentProgress < 0.0)
	{
		currentProgress = jlimit(0.0, 1.0, currentProgress);
		return false;
	}

	target->repaint();
	return true;
}

void Animator::timerCallback()
{
	for(int i = 0; i < items.size(); i++)
	{
		if(!items[i]->timerCallback())
			items.remove(i--);
	}
}
}
}


