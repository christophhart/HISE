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

#pragma once

namespace hise { using namespace juce;

struct ComponentWithPreferredSize
{
	struct BodyFactory
	{
		BodyFactory(Component& root, BodyFactory* parent = nullptr);

		using CreateFunction = std::function<ComponentWithPreferredSize*(Component*, const var&)>;

		void registerFunction(const CreateFunction& cf);

		template <typename T> void registerWithCreate()
		{
			registerFunction(T::create);
		}

		ComponentWithPreferredSize* create(const var& v);

		BodyFactory* parentFactory;

		Component& parent;
		Array<CreateFunction> functions;
	};

	enum class Layout
	{
		NoChildren,
		ChildrenAreRows,
		ChildrenAreColumns
	};

	virtual ~ComponentWithPreferredSize() {};

	virtual int getPreferredHeight() const = 0;
	virtual int getPreferredWidth() const = 0;

	void resetSize();

	void resetRootSize();

	void resizeChildren(Component* asComponent);

	int getMaxWidthOfChildComponents(const Component* asComponent) const;
	int getMaxHeightOfChildComponents(const Component* asComponent) const;
	int getSumOfChildComponentWidth(const Component* asComponent) const;
	int getSumOfChildComponentHeight(const Component* asComponent) const;
	void addChildWithPreferredSize(ComponentWithPreferredSize* c);

	OwnedArray<ComponentWithPreferredSize> children;

	Layout childLayout = Layout::NoChildren;
    bool stretchChildren = true;
	int padding = 0;

	int marginTop = 0;
	int marginBottom = 0;
	int marginLeft = 0;
	int marginRight = 0;
};

template <typename ComponentType, int Width, int Height> struct PrefferedSizeWrapper : public Component,
public ComponentWithPreferredSize
{
	template <typename... Ps> PrefferedSizeWrapper(Ps... arguments)
	{
		addAndMakeVisible(content = dynamic_cast<Component*>(new ComponentType(arguments...)));
	}

	PrefferedSizeWrapper(ComponentType* existingType)
	{
		addAndMakeVisible(content = dynamic_cast<Component*>(existingType));
	}

	void resized() override
	{
		content->setBounds(getLocalBounds());
	}

	int getPreferredWidth() const override { return Width; }
	int getPreferredHeight() const override { return Height; }

	ScopedPointer<Component> content;
};

struct Column : public Component,
	public ComponentWithPreferredSize
{
	Column()
	{
		setInterceptsMouseClicks(false, true);
		childLayout = ComponentWithPreferredSize::Layout::ChildrenAreRows;
	}
	int getPreferredHeight() const override { return getSumOfChildComponentHeight(this); }
	int getPreferredWidth() const override { return getMaxWidthOfChildComponents(this); }
	void resized() override { resizeChildren(this); }
};

struct Row : public Component,
	public ComponentWithPreferredSize
{
	Row()
	{
		setInterceptsMouseClicks(false, true);
		childLayout = ComponentWithPreferredSize::Layout::ChildrenAreColumns;
	}

	int getPreferredHeight() const override { return getMaxHeightOfChildComponents(this); }
	int getPreferredWidth() const override { return getSumOfChildComponentWidth(this); }
	void resized() override { resizeChildren(this); }
};

} // namespace hise

