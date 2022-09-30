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

#define LOAD_PATH_IF_URL(urlName, editorIconName) ids.addIfNotAlreadyThere(urlName); if (url == urlName) p.loadPathFromData(editorIconName, sizeof(editorIconName));

	/** A simple interface class. Give it a String and get a path back. */
	class PathFactory
	{
	public:

		static void scalePath(Path& p, Rectangle<float> f)
		{
			p.scaleToFit(f.getX(), f.getY(), f.getWidth(), f.getHeight(), true);
		}

		static void scalePath(Path& p, Component* c, float padding)
		{
			auto b = c->getBoundsInParent().toFloat().reduced(padding);
			scalePath(p, b);
		}

		struct KeyMapping
		{
			KeyMapping(const String& name, int keyCode, ModifierKeys::Flags mods=ModifierKeys::noModifiers);

			String url;
			KeyPress k;
		};

		struct Description
		{
			Description(const String& name, const String& description_);

			String url;
			String description;
		};

		struct GlobalPool : public DeletedAtShutdown
		{
			OwnedArray<PathFactory> allFactories;
		};

		PathFactory()
		{

		};

		virtual String getId() const { return {}; }

		virtual ~PathFactory() {};
		virtual Path createPath(const String& id) const = 0;


		virtual Array<Description> getDescription() const { return {}; }

		virtual Array<KeyMapping> getKeyMapping() const { return {}; }

		void updateCommandInfoWithKeymapping(juce::ApplicationCommandInfo& info);

		mutable StringArray ids;
	};

#define REGISTER_PATH_FACTORY_AT_GLOBAL_POOL(className) className(){ registerPathFactory<className>(); }

	class ChainBarPathFactory : public PathFactory
	{
	public:

		String getId() const override { return "Processor Icons"; }

		Path createPath(const String& id) const override;
	};


	class HiseShapeButton : public ShapeButton,
		public ButtonListener
	{
	public:

		HiseShapeButton(const String& name, ButtonListener* listener, const PathFactory& factory, const String& offName = String()) :
			ShapeButton(name, Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white)
		{
			onShape = factory.createPath(name);

			if (offName.isEmpty())
				offShape = onShape;
			else
				offShape = factory.createPath(offName);

			if (listener != nullptr)
				addListener(listener);

			refreshShape();
			refreshButtonColours();
		}

		void setToggleModeWithColourChange(bool shouldBeEnabled)
		{
			setClickingTogglesState(shouldBeEnabled);

			if (shouldBeEnabled)
				addListener(this);
			else
				removeListener(this);
		}

		void setToggleStateAndUpdateIcon(bool shouldBeEnabled, bool forceUpdate=false)
		{
			if (forceUpdate || getToggleState() != shouldBeEnabled)
			{
				setToggleState(shouldBeEnabled, dontSendNotification);
				refreshButtonColours();
				refreshShape();
			}
		}

		void buttonClicked(Button* /*b*/) override
		{
			refreshShape();
			refreshButtonColours();
		}


		void refreshButtonColours()
		{
			if (getToggleState())
			{
				setColours(onColour.withAlpha(0.8f), onColour, onColour);
			}
			else
			{
				setColours(offColour.withMultipliedAlpha(0.5f), offColour.withMultipliedAlpha(0.8f), offColour);
			}

			repaint();
		}

		bool operator==(const String& id) const
		{
			return getName() == id;
		}

		void refreshShape()
		{
			if (getToggleState())
			{
				setShape(onShape, false, true, true);
			}
			else
				setShape(offShape, false, true, true);
		}

		void refresh()
		{
			refreshShape();
			refreshButtonColours();
		}

		void toggle()
		{
			setToggleState(!getToggleState(), dontSendNotification);

			refresh();
		}

		void setShapes(Path newOnShape, Path newOffShape)
		{
			onShape = newOnShape;
			offShape = newOffShape;
		}

		void clicked(const ModifierKeys& modifiers) override
		{
			lastMods = modifiers;
			ShapeButton::clicked(modifiers);
		}

		bool wasRightClicked() const { return lastMods.isRightButtonDown(); }

		Colour onColour = Colour(SIGNAL_COLOUR);
		Colour offColour = Colours::white;
		Path onShape;
		Path offShape;

		private:

		ModifierKeys lastMods = ModifierKeys();
	};

}