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

namespace hise
{
using namespace juce;

struct WrapperWithMenuBarBase : public Component,
	public ComboBoxListener,
	public ZoomableViewport::ZoomListener,
	public Timer
{
	struct ButtonWithStateFunction
	{
        virtual ~ButtonWithStateFunction() {};
		virtual bool hasChanged() = 0;
	};

	static Component* showPopup(FloatingTile* ft, Component* parent, const std::function<Component*()>& createFunc, bool show);

	template <typename ContentType, typename PathFactoryType> struct ActionButtonBase :
		public Component,
		public ButtonWithStateFunction,
		public SettableTooltipClient
	{
		ActionButtonBase(ContentType* parent_, const String& name) :
			Component(name),
			parent(parent_)
		{
			PathFactoryType f;

			p = f.createPath(name);
			setSize(MenuHeight, MenuHeight);
			setRepaintsOnMouseActivity(true);

			setColour(TextButton::ColourIds::buttonOnColourId, Colour(SIGNAL_COLOUR));
			setColour(TextButton::ColourIds::buttonColourId, Colour(0xFFAAAAAA));
		}

		virtual ~ActionButtonBase()
		{

		}

		bool hasChanged() override
		{
			bool changed = false;

			if (stateFunction)
			{
				auto thisState = stateFunction(*parent);
				changed |= thisState != lastState;
				lastState = thisState;
				
			}

			if (enabledFunction)
			{
				auto thisState = enabledFunction(*parent);
				changed |= thisState != lastEnableState;
				lastEnableState = thisState;
			}

			return changed;
		}

		void triggerClick(NotificationType sendNotification)
		{
			if (enabledFunction && !enabledFunction(*parent))
				return;

			if (actionFunction)
				actionFunction(*parent);

			SafeAsyncCall::repaint(this);
		}

		/** Call this function with a lambda that creates a component and it will be shown as Floating Tile popup. */
		void setControlsPopup(const std::function<Component*()>& createFunc)
		{
			stateFunction = [this](ContentType&)
			{
				return this->currentPopup != nullptr;
			};

			actionFunction = [this, createFunc](ContentType&)
			{
				auto ft = findParentComponentOfClass<FloatingTile>();

				this->currentPopup = showPopup(ft, this, createFunc, this->currentPopup == nullptr);

				return false;
			};
		}
		

		void paint(Graphics& g) override
		{
			auto on = stateFunction ? stateFunction(*parent) : false;
			auto enabled = enabledFunction ? enabledFunction(*parent) : true;
			auto over = isMouseOver(false);
			auto down = isMouseButtonDown(false);

			Colour c = findColour(on ? TextButton::ColourIds::buttonOnColourId : TextButton::ColourIds::buttonColourId);


			float alpha = 0.7f;

			if (enabled)
			{
				if (over)
					alpha += 0.2f;

				if (down)
					alpha += 0.1f;
			}
			else
				alpha = 0.3f;
			
			c = c.withAlpha(alpha);

			g.setColour(c);

			PathFactory::scalePath(p, getLocalBounds().toFloat().reduced(down && enabled ? 5.0f : 4.0f));

			g.fillPath(p);
		}

		void mouseDown(const MouseEvent& e) override
		{
			triggerClick(sendNotificationSync);
		}

		void resized() override
		{
			PathFactory::scalePath(p, this, 2.0f);
		}

		Path p;

		using Callback = std::function<bool(ContentType& g)>;

		Component::SafePointer<ContentType> parent;
		Callback stateFunction;
		Callback enabledFunction;
		Callback actionFunction;
		bool lastState = false;
		bool lastEnableState = false;

		Component::SafePointer<Component> currentPopup;

		bool isPopupShown = false;
	};

	constexpr static int MenuHeight = 24;

	struct Spacer : public Component
	{
		Spacer(int width)
		{
			setSize(width, MenuHeight);
		}
	};

	WrapperWithMenuBarBase(Component* contentComponent);

	/** Override this method and initialise the menu bar here. */
	virtual void rebuildAfterContentChange() = 0;

	virtual bool isValid() const { return true; }

	template <typename ComponentType> ComponentType* getComponentWithName(const String& id)
	{
		for (auto b : actionButtons)
		{
			if (b->getName() == id)
			{
				if (auto typed = dynamic_cast<ComponentType*>(b))
					return typed;
			}
		}

		jassertfalse;
		return nullptr;
	}
	
	void setContentComponent(Component* newContent);

	void timerCallback() override;
	void addSpacer(int width);
	void updateBookmarks(ValueTree, bool);

	virtual int bookmarkAdded() { return -1; };
	virtual void zoomChanged(float newScalingFactor) {};
	virtual void bookmarkUpdated(const StringArray& idsToShow) = 0;
	virtual ValueTree getBookmarkValueTree() = 0;
	void comboBoxChanged(ComboBox* c) override;
	void addBookmarkComboBox();
	virtual void addButton(const String& name) = 0;
	void paint(Graphics& g) override;
	void addCustomComponent(Component* c);
	void setPostResizeFunction(const std::function<void(Component*)>& f);
	void resized() override;

	std::function<void(Component*)> resizeFunction;

	ZoomableViewport canvas;
	OwnedArray<Component> actionButtons;

	GlobalHiseLookAndFeel glaf;
	ComboBox* bookmarkBox;

	valuetree::ChildListener bookmarkUpdater;
};

}
