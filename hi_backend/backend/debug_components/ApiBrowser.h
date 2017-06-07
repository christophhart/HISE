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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef APIBROWSER_H_INCLUDED
#define APIBROWSER_H_INCLUDED



class ApiCollection : public SearchableListComponent
{
public:

	ApiCollection(BackendRootWindow *window);

	SET_GENERIC_PANEL_ID("ApiCollection");

	class MethodItem : public SearchableListComponent::Item
	{
	public:

		MethodItem(const ValueTree &methodTree_, const String &className_);

		int getPopupHeight() const override	{ return 150; }

		void paintPopupBox(Graphics &g) const
		{
			help.draw(g, Rectangle<float>(10.0f, 10.0f, 280.0f, (float)getPopupHeight() - 20));
		}

		void mouseEnter(const MouseEvent&) override { repaint(); }
		void mouseExit(const MouseEvent&) override { repaint(); }
		void mouseDoubleClick(const MouseEvent&) override;

		bool keyPressed(const KeyPress& key) override;

		void paint(Graphics& g) override;

	private:

		void insertIntoCodeEditor();

		AttributedString help;

		String name;
		String description;
		String className;
		String arguments;

		const ValueTree methodTree;
	};

	class ClassCollection : public SearchableListComponent::Collection
	{
	public:
		ClassCollection(const ValueTree &api);;

		void paint(Graphics &g) override;
	private:

		String name;

		const ValueTree classApi;
	};

	int getNumCollectionsToCreate() const override { return apiTree.getNumChildren(); }

	Collection *createCollection(int index) override
	{
		return new ClassCollection(apiTree.getChild(index));
	}

	ValueTree apiTree;

private:

	BaseDebugArea *parentArea;

};

#endif  // APIBROWSER_H_INCLUDED
