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



	/** A simple interface class. Give it a String and get a path back. */
	class PathFactory
	{
	public:

		static void scalePath(Path& p, Rectangle<float> f);

		static void scalePath(Path& p, Component* c, float padding);

		static bool isValid(const Path& p, Rectangle<float> area = {});

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

		PathFactory();;

		virtual String getId() const;

		virtual ~PathFactory();;
		virtual Path createPath(const String& id) const = 0;


		virtual Array<Description> getDescription() const;

		virtual Array<KeyMapping> getKeyMapping() const;

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

		HiseShapeButton(const String& name, ButtonListener* listener, const PathFactory& factory, const String& offName = String());

		void setToggleModeWithColourChange(bool shouldBeEnabled);

		void setToggleStateAndUpdateIcon(bool shouldBeEnabled, bool forceUpdate=false);
		void buttonClicked(Button* /*b*/) override;
		void refreshButtonColours();

		bool operator==(const String& id) const
		{
			return getName() == id;
		}

		void refreshShape();
		void refresh();
		void toggle();

		void mouseDown(const MouseEvent& e) override;
		void mouseUp(const MouseEvent& e) override;
		void mouseDrag(const MouseEvent& e) override;

		void setShapes(Path newOnShape, Path newOffShape);
		void clicked(const ModifierKeys& modifiers) override;
		bool wasRightClicked() const { return lastMods.isRightButtonDown(); }

		Colour onColour = Colour(SIGNAL_COLOUR);
		Colour offColour = Colours::white;
		Path onShape;
		Path offShape;

		private:

		ModifierKeys lastMods = ModifierKeys();
	};

}