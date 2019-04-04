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

#ifndef FLOATINGINTERFACEBUILDER_H_INCLUDED
#define FLOATINGINTERFACEBUILDER_H_INCLUDED

namespace hise { using namespace juce;

/** This class can be used to create interfaces using floating panels in C++
*
*	In order to use it, just create a interface builder and use its creation methods.
*/
class FloatingInterfaceBuilder
{
public:

	enum ContainerType
	{

	};

	FloatingInterfaceBuilder(FloatingTile* root)
	{
		createdComponents.add(root);
	}

	FloatingTile* finalizeAndReturnRoot();

	/** set the given panel to the content and returns true on success. */

	template <typename ContentType> bool setNewContentType(int index)
	{
		auto panelToUse = createdComponents[index].getComponent();

		if (panelToUse == nullptr)
		{
			jassertfalse;
			return false;
		}

		panelToUse->setNewContent(ContentType::getPanelId());


		removeFirstChildOfNewContainer(panelToUse);


		return true;
	}

	void setId(int index, const String& newID);

	void setSizes(int index, Array<double> sizes, NotificationType shouldUpdateLayout= dontSendNotification);
	void setFolded(int index, Array<bool> foldStates, NotificationType shouldUpdateLayout = dontSendNotification);

	void setVisibility(int index, bool shouldBeVisible, Array<bool> childVisibleStates, NotificationType shouldUpdateLayout = dontSendNotification);

	void setFoldable(int index, bool isFoldable, Array<bool> childFoldableStates, NotificationType = dontSendNotification);

	void setDynamic(int index, bool shouldBeDynamic);

	void setCustomName(int index, const String& name, Array<String> names=Array<String>());

	void setCustomPanels(int toggleBarIndex, Array<int> panels);

	int addChild(int index, const Identifier& panelId);

	/** Adds a child with the given content to the container with the index.
	*
	*	If the panel is no container it will do nothing. It will return a index that can be used for further building
	*/

	template <typename ContentType> int addChild(int index)
	{
		return addChild(index, GET_PANEL_NAME(ContentType));
	}

	FloatingTile* getPanel(int index);

	FloatingTileContainer* getContainer(int index);

	ResizableFloatingTileContainer* getTileManager(int index);

	FloatingTileContent* getContent(int index);
	
	template <class ContentType> ContentType* getContent(int index)
	{
		return dynamic_cast<ContentType*>(getContent(index));
	}

private:

	void removeFirstChildOfNewContainer(FloatingTile* panel);

	Array<Component::SafePointer<FloatingTile>> createdComponents;
};

} // namespace hise

#endif  // FLOATINGINTERFACEBUILDER_H_INCLUDED
