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

namespace hise { using namespace juce;

void ComponentWithPreferredSize::resetSize()
{
	auto w = getPreferredWidth();
	auto h = getPreferredHeight();

	auto asComponent = dynamic_cast<Component*>(this);
	auto forceResize = asComponent->getWidth() == w && asComponent->getHeight() == h;
	asComponent->setSize(w, h);

	if (forceResize)
		asComponent->resized();
}

void ComponentWithPreferredSize::resetRootSize()
{
	ComponentWithPreferredSize* root = nullptr;
	ComponentWithPreferredSize* parentRoot = this;

	while (parentRoot != nullptr)
	{
		root = parentRoot;
		parentRoot = dynamic_cast<Component*>(root)->findParentComponentOfClass<ComponentWithPreferredSize>();
	}

	jassert(root != nullptr);
	
	root->resetSize();
}

void ComponentWithPreferredSize::resizeChildren(Component* asComponent)
{
	if (children.isEmpty())
		return;

	auto b = asComponent->getLocalBounds();

	b.removeFromLeft(marginLeft);
	b.removeFromRight(marginRight);
	b.removeFromTop(marginTop);
	b.removeFromBottom(marginBottom);

	if (childLayout == Layout::ChildrenAreColumns)
	{
		for (auto c : children)
		{
			if (!dynamic_cast<Component*>(c)->isVisible())
				continue;

            auto cb = b.removeFromLeft(c->getPreferredWidth());
            
            if(!stretchChildren)
                cb = cb.removeFromTop(c->getPreferredHeight());
            
			dynamic_cast<Component*>(c)->setBounds(cb);
			
			if (cb.getWidth() != 0)
				b.removeFromLeft(padding);
		}
	}
	else if (childLayout == Layout::ChildrenAreRows)
	{
		for (auto c : children)
		{
			if (!dynamic_cast<Component*>(c)->isVisible())
				continue;

            auto cb = b.removeFromTop(c->getPreferredHeight());
            
            if(!stretchChildren)
                cb = cb.removeFromLeft(c->getPreferredWidth());
            
			dynamic_cast<Component*>(c)->setBounds(cb);

			if (cb.getHeight() != 0)
				b.removeFromTop(padding);
		}
	}
}

int ComponentWithPreferredSize::getMaxWidthOfChildComponents(const Component* asComponent) const
{
	int w = 0;

	for (auto c : children)
	{
		if (!dynamic_cast<Component*>(c)->isVisible())
			continue;

		w = jmax(w, c->getPreferredWidth());
	}
		
	w += marginLeft + marginRight;

	return w;
}

int ComponentWithPreferredSize::getMaxHeightOfChildComponents(const Component* asComponent) const
{
	int h = 0;

	for (auto cp : children)
	{
		if (!dynamic_cast<Component*>(cp)->isVisible())
			continue;

		h = jmax(h, cp->getPreferredHeight());
	}
	
	if(h != 0)
		h += marginTop + marginBottom;

	return h;
}

int ComponentWithPreferredSize::getSumOfChildComponentWidth(const Component* asComponent) const
{
	int w = 0;

	for (auto cp : children)
	{
		if (!dynamic_cast<Component*>(cp)->isVisible())
			continue;

		auto thisWidth = cp->getPreferredWidth();

		w += thisWidth;

		if (children.getLast() != cp && thisWidth != 0)
			w += padding;
	}
		
	if (w != 0)
		w += marginLeft + marginRight;
	
	return w;
}

int ComponentWithPreferredSize::getSumOfChildComponentHeight(const Component* asComponent) const
{
	int h = 0;

	for (auto cp : children)
	{
		if (!dynamic_cast<Component*>(cp)->isVisible())
			continue;

		auto thisHeight = cp->getPreferredHeight();

		h += thisHeight;

		if (children.getLast() != cp && thisHeight != 0)
			h += padding;
	}
	
	if (h != 0)
		h += marginBottom + marginTop;

	return h;
}

void ComponentWithPreferredSize::addChildWithPreferredSize(ComponentWithPreferredSize* c)
{
	c->resetSize();
	children.add(c);
	dynamic_cast<Component*>(this)->addAndMakeVisible(dynamic_cast<Component*>(c));
}

ComponentWithPreferredSize::BodyFactory::BodyFactory(Component& m, BodyFactory* parentFactory_) :
	parent(m),
	parentFactory(parentFactory_)
{

}

void ComponentWithPreferredSize::BodyFactory::registerFunction(const CreateFunction& cf)
{
	functions.add(cf);
}

ComponentWithPreferredSize* ComponentWithPreferredSize::BodyFactory::create(const var& v)
{
	for (int i = functions.size() - 1; i >= 0; i--)
	{
		if (auto c = functions[i](&parent, v))
			return c;
	}

	if (parentFactory != nullptr)
		return parentFactory->create(v);
	else
		return nullptr;
}

} // namespace hise
