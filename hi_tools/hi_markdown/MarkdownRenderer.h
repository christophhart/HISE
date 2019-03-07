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

namespace hise {
using namespace juce;

class MarkdownRenderer : public MarkdownParser
{
public:

	struct LayoutCache
	{
		void clear() { cachedLayouts.clear(); }

		const MarkdownLayout& getLayout(const AttributedString& s, float w);

	private:

		struct Layout
		{
			Layout(const AttributedString& s, float w);

			MarkdownLayout l;
			int64 hashCode;
			float width;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Layout);
		};

		OwnedArray<Layout> cachedLayouts;

		JUCE_DECLARE_WEAK_REFERENCEABLE(LayoutCache);
	};

	struct Listener
	{
		virtual ~Listener() {};
		virtual void markdownWasParsed(const Result& r) = 0;

		virtual void scrollToAnchor(float v) {};

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	MarkdownRenderer(const String& text, LayoutCache* c = nullptr) :
		MarkdownParser(text),
		layoutCache(c),
		uncachedLayout({}, 0.0f)
	{
		history.add(markdownCode);
		historyIndex = 0;
	};

	~MarkdownRenderer()
	{
		setTargetComponent(nullptr);
	}

	void draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea = {}) const
	{
		for (auto* e : elements)
		{
			auto heightToUse = e->getHeightForWidthCached(totalArea.getWidth());
			auto topMargin = e->getTopMargin();
			totalArea.removeFromTop(topMargin);
			auto ar = totalArea.removeFromTop(heightToUse);

			if (firstDraw || viewedArea.isEmpty() || ar.toNearestInt().intersects(viewedArea))
				e->draw(g, ar);
		}

		firstDraw = false;
	}

	float getHeightForWidth(float width);

	void parse() override;

	bool canNavigate(bool back) const
	{
		if (back)
			return historyIndex < history.size();
		else
			return historyIndex < (history.size() - 1);
	}

	void navigate(bool back)
	{
		if (back)
			historyIndex = jmax<int>(historyIndex - 1, 0);
		else
			historyIndex = jmin<int>(historyIndex + 1, history.size() - 1);

		setNewText(history[historyIndex]);
	}

	void scrollToY(float y)
	{
		for (auto l : listeners)
		{
			if (l.get() != nullptr)
				l->scrollToAnchor(y);
		}
	}

	RectangleList<float> searchInContent(const String& searchString)
	{
		RectangleList<float> positions;

		float y = 0.0f;

		for (auto e : elements)
		{
			e->searchInContent(searchString);

			y += e->getTopMargin();

			for (auto r : e->searchResults)
			{
				positions.add(r.translated(0.0f, y));
			}

			y += e->getLastHeight();
		}

		return positions;
	}

	String getAnchorForY(int y) const;



	String getSelectionContent() const
	{
		String s;

		for (auto e : elements)
		{
			if (e->selected)
			{
				s << e->getTextToCopy() << "\n";
			}
		}

		return s;
	}

	void updateSelection(Rectangle<float> area)
	{
		Range<float> yRange(area.getY(), area.getBottom());

		float y = 0.0f;

		for (auto e : elements)
		{
			float h = e->getTopMargin() + e->getLastHeight();

			e->setSelected(Range<float>(y, y + h).intersects(yRange));

			y += h;
		}
	}

	void addListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
	void removeListener(Listener* l) { listeners.removeAllInstancesOf(l); }

#if 0
	const MarkdownLayout& getTextLayoutForString(const AttributedString& s, float width)
	{
		if (layoutCache.get() != nullptr)
			return layoutCache->getLayout(s, width);

		uncachedLayout = { s, width };
		return  uncachedLayout;
	}
#endif


	void setTargetComponent(Component* newTarget)
	{
		if (targetComponent == newTarget)
			return;

		if (auto existing = targetComponent.getComponent())
		{
			for (auto e : elements)
			{
				if (auto c = e->createComponent(existing->getWidth()))
					existing->removeChildComponent(c);
			}
		}

		targetComponent = newTarget;
	}

	void updateCreatedComponents()
	{
		if (targetComponent == nullptr)
			return;

		float y = 0.0f;

		for (auto e : elements)
		{
			y += e->getTopMargin();

			if (auto c = e->createComponent(targetComponent->getWidth()))
			{
				if (c->getParentComponent() == nullptr)
					targetComponent->addAndMakeVisible(c);

				jassert(c->getWidth() > 0);
				jassert(c->getHeight() > 0);

				c->setTopLeftPosition(0, (int)y);
			}

			y += e->getLastHeight();
		}
	}

	private:

		StringArray history;
		int historyIndex;

		WeakReference<LayoutCache> layoutCache;
		MarkdownLayout uncachedLayout;

		Array<Component::SafePointer<Component>> createdComponents;
		Component::SafePointer<Component> targetComponent;

		Array<WeakReference<Listener>> listeners;

		mutable bool firstDraw = true;
		float lastHeight = -1.0f;
		float lastWidth = -1.0f;
};



}