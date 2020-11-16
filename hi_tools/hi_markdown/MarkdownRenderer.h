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

class MarkdownRenderer : public MarkdownParser,
						 public ViewportWithScrollCallback::Listener
{
public:

	struct NavigationAction : public UndoableAction
	{
		NavigationAction(MarkdownRenderer* renderer, const MarkdownLink& newLink):
			currentLink(newLink),
			parent(renderer)
		{
			lastLink = renderer->getLastLink();
			lastY = renderer->currentY;
		};

		bool perform()
		{
			if (parent != nullptr)
			{
				parent->MarkdownParser::gotoLink(currentLink);
				return true;
			}

			return false;
		}

		bool undo() override
		{
			if (parent != nullptr)
			{
				parent->MarkdownParser::gotoLink(lastLink);
				parent->scrollToY(lastY);
				return true;
			}

			return false;
		}

		float lastY = 0.0f;
		
		MarkdownLink lastLink;
		MarkdownLink currentLink;
		
		WeakReference<MarkdownRenderer> parent;
	};

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

		virtual void scrollToAnchor(float v) 
		{
			ignoreUnused(v);
		};

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
			totalArea.removeFromTop((float)topMargin);
			auto ar = totalArea.removeFromTop(heightToUse);

			if (firstDraw || viewedArea.isEmpty() || ar.toNearestInt().intersects(viewedArea))
				e->draw(g, ar);
		}

		firstDraw = false;
	}

	float getHeightForWidth(float width, bool forceUpdate=false);

	void parse() override;

	bool canNavigate(bool back) const
	{
		if (back)
			return undoManager.canUndo();
		else
			return undoManager.canRedo();
	}

	void navigate(bool back)
	{
		if (back)
			undoManager.undo();
		else
			undoManager.redo();
	}

	void jumpToCurrentAnchor();

	

	RectangleList<float> searchInContent(const String& searchString)
	{
		RectangleList<float> positions;

		float y = 0.0f;

		for (auto e : elements)
		{
			e->searchInContent(searchString);

			y += e->getTopMargin();

			for (auto r : e->searchResults)
				positions.add(r.translated(0.0f, y));

			y += e->getLastHeight();
		}

		return positions;
	}

	void scrolled(Rectangle<int> visibleArea) override
	{
		currentY = (float)visibleArea.getY();
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

	bool gotoLink(const MarkdownLink& url) override
	{
		undoManager.beginNewTransaction("New Link");
		return undoManager.perform(new NavigationAction(this, url));
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

	Component* getTargetComponent() const
	{
		return targetComponent.getComponent();
	}

	void updateHeight()
	{
		float y = currentY;

		getHeightForWidth(lastWidth, true);
		
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->markdownWasParsed(getParseResult());
		}

		scrollToY(y);

	}

	void setChildComponentBounds(Rectangle<int> renderArea)
	{
		childArea = renderArea;
	}

	void updateCreatedComponents()
	{
		if (targetComponent == nullptr)
			return;

		if (targetComponent->getWidth() == 0)
			return;

		float y = (float)childArea.getY();

		auto wToUse = childArea.getWidth();

		if (wToUse == 0)
			wToUse = (int)targetComponent->getWidth();

		for (auto e : elements)
		{
			y += e->getTopMargin();

			if (auto c = e->createComponent(wToUse))
			{
				if (c->getParentComponent() == nullptr)
					targetComponent->addAndMakeVisible(c);

				jassert(c->getWidth() > 0);
				jassert(c->getHeight() > 0);

				c->setTopLeftPosition(childArea.getX(), (int)y);

				y += (float)e->getLastHeight();
			}
			else
			{
				y += e->getLastHeight();
			}
		}

		
	}

	bool navigateFromXButtons(const MouseEvent& e)
	{
		if (e.mods.isX1ButtonDown())
		{
			navigate(true);
			return true;
		}
		if (e.mods.isX2ButtonDown())
		{
			navigate(false);
			return true;
		}

		return false;
	}

	UndoManager* getUndoManager() { return &undoManager; }

private:

	float currentY;

	void scrollToY(float y)
	{
		currentY = y;

		WeakReference<MarkdownRenderer> r = this;

		auto f = [r, y]()
		{
			if (r != nullptr)
			{
				for (auto l : r->listeners)
				{
					if (l.get() != nullptr)
						l->scrollToAnchor(y);
				}
			}
			
		};

		MessageManager::callAsync(f);
	}

	JUCE_DECLARE_WEAK_REFERENCEABLE(MarkdownRenderer);

	UndoManager undoManager;

	StringArray history;
	int historyIndex;

	WeakReference<LayoutCache> layoutCache;
	MarkdownLayout uncachedLayout;

	Array<Component::SafePointer<Component>> createdComponents;
	Component::SafePointer<Component> targetComponent;

	Array<WeakReference<Listener>> listeners;

	mutable bool firstDraw = true;
	float lastHeight = -1.0f;
	Rectangle<int> childArea;
	float lastWidth = -1.0f;
};



}