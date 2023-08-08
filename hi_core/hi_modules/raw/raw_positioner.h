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

namespace raw
{

/** A helper class for transferring the position data of a scripted interface to C++. 

	The positioning of UI elements is a rather annoying task (aka pixel-pushing). If
	your interface has a static layout (which is still the case for most plugin projects),
	you will have to shovel around the component positions, recompile, see if it matches, etc.

	The interface designer in HISE was made to improve the efficiency for this step: you have a WYSIWYG
	editor where you can select, drag and resize UI elements just like in any other graphic
	design application. However if you are using the raw API to build your final project in
	C++, you won't be able to leverage this tool, or even worse, if you have made a prototype with
	the interface designer, start from scratch or copy / paste all positions manually.

	This is where this helper class comes in. It takes a object that contains the position and hierarchy
	of every component in your scripted interface and applies it to a C++ Component. It assumes the
	same child-component hierarchy and naming.

	You can create such a Data object simply by right clicking an UI component in the Component List of
	the interface designer and choose "Copy C++ position data to clipboard". Then paste the content of
	the clipboard somewhere in your C++ project and call the apply() method in the resize() callback of the
	root component:

	\code
	// This is our main component class
	class MainInterface: public Component
	{
	public:
	    MainInterface()
		{
			// We need to set the root name
		    setName("Root");

			addAndMakeVisible(slider1);

			// The slider needs to have the exact name as in the data object.
			slider1.setName("Slider1");

			addAndMakeVisible(buttonPanel);
			buttonPanel.setName("ButtonPanel");

			// We add the buttons to the invisible component to match the hierarchy (see below).
			buttonPanel.addAndMakeVisible(leftButton);
			leftButton.setName("LeftButton");

			buttonPanel.addAndMakeVisible(leftButton);
			leftButton.setName("LeftButton");
		}

		void resized()
		{
			// This is pasted from the interface designer.
			// Note how the names and hierarchy matches the component's names and hierarchy.
			raw::Positioner positioner({
			 "Root", { 0, 0, 600, 500 },
			 {
			  { "Slider1", { 227, 96, 140, 70 }, {} },
			  {
			   "ButtonPanel", { 100, 200, 400, 60 },
			   {
				{ "LeftButton", { 64, 18, 117, 25 }, {} },
				{ "RightButton", { 240, 18, 117, 25 }, {} }
			   }
			  }
			 }
			});

			// Applies the positions from above.
			positioner.apply(*this);

			// Everything should be found here...
			positioner.printSummary();
		}
	private:

		juce::Slider slider1;
		juce::Component buttonPanel;
		juce::TextButton leftButton;
		juce::TextButton rightButton;
	};
	\endcode

*/
class Positioner
{
public:


	/** The data object. This will be created from the std::initializer_list tree that you've pasted from the clipboard. */
	struct Data
	{
		Data(const String& n, Rectangle<int> b, std::vector<Data>&& c) :
			name(n),
			bounds(b),
			children(c)
		{}

		Data(var component);

		String toString(int currentTabLevel=0) const;

		void apply(Component& c, StringArray& processedComponents) const;

		void fillNameList(StringArray& list) const;

		/** The name of the component. This must be the same String that was passed into Component::setName(). */
		String name;

		/** The bounds relative to its parent component. */
		Rectangle<int> bounds;

		/** The list of child components. This sets up a tree hierarchy that must be matched by your main Component's child component hierarchy. */
		std::vector<Data> children;
	};

	/** Creates a positioner object from a scripted component. You don't need this method in your code, it's used for the creation of the data. */
	Positioner(var component);

	/** Creates a positioner object from the given data. Just paste the exported data String from the clipboard as argument. */
	Positioner(const Data& d);

	/** Creates a string representation of the UI positioning data ready to be pasted into your C++ project.
		You won't need to call this method ever manually, just use the drop down menu entry in the Component List. */
	String toString();

	/** Applies the given UI positioning data to the component. The best place to call this is the resized() callback
		of your main component - it will iterate over all children and set their position 
		(so you can leave the child component's resized() callback alone). */
	void apply(Component& c);

	/** The same as apply(), but doesn't change the root component position. */
	void applyToChildren(Component& c);

	/** This prints out a statistic of all components that were positioned and those who could not be resolved. 
		This is helpful during development so you can quickly check which components you need to fix. */
	void printSummary();

private:

	StringArray processedComponents;

	Data data;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Positioner);
};



}

}