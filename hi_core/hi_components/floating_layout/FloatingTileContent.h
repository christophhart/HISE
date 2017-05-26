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

#ifndef FLOATINGTILECONTENT_H_INCLUDED
#define FLOATINGTILECONTENT_H_INCLUDED

class FloatingTile;
class FloatingTileContainer;

/** The base class for all components that can be put into a floating panel. 
*
*	In order to use it:
*
*	- Subclass this interface
*	- Use the macro SET_PANEL_NAME() to set it's unique ID
*	- register the class to the FloatingPanel::Factory
*	- add it to the popup menu
*
*	You can use the ValueTree methods to save / restore the state.
*/
class FloatingTileContent : public RestorableObject
{
protected:

	/** Creates a new floating panel. 
	*
	*	You must supply a FloatingShellComponent at initialisation to prevent the requirement for lazy initialisation.
	*/
	FloatingTileContent(FloatingTile* parent_) :
		parent(parent_)
	{
	}

public:

	virtual ~FloatingTileContent()
	{
		parent = nullptr;
	}

	/** Returns the parent shell. 
	*
	*	Unlike getParentComponent(), this always returns non nullptr, so you can use it in the constructor.
	*/
	FloatingTile* getParentShell()
	{
		jassert(parent != nullptr);

		return parent; 
	}

	BackendRootWindow* getRootWindow();

	const BackendRootWindow* getRootWindow() const;

	const FloatingTile* getParentShell() const 
	{
		jassert(parent != nullptr);

		return parent; 
	}

	virtual String getTitle() const { return getIdentifierForBaseClass().toString(); };
	virtual Identifier getIdentifierForBaseClass() const = 0;

	/** Returns an empty ValueTree with the correct id. */
	ValueTree exportAsValueTree() const override;

	/** does nothing. */
	void restoreFromValueTree(const ValueTree& ) override;

	static FloatingTileContent* createPanel(ValueTree& state, FloatingTile* parent);

	static FloatingTileContent* createNewPanel(const Identifier& id, FloatingTile* parent);

	/** Set a custom title to the panel that will be displayed in tabs, etc. */
	void setCustomTitle(String newCustomTitle)
	{
		customTitle = newCustomTitle;
	}

	/** If you set a custom title, this will return it. */
	String getCustomTitle() const
	{
		return customTitle;
	}

	bool hasCustomTitle() const { return customTitle.isNotEmpty(); }

	BackendProcessorEditor* getMainPanel();

	const BackendProcessorEditor* getMainPanel() const;

	class Factory
	{
	public:

		/** Register a subclass to this factory. The subclass must have a static method 'Identifier getName()'. */
		template <typename DerivedClass> void registerType()
		{
			if (std::is_base_of<FloatingTileContent, DerivedClass>::value)
			{
				ids.add(DerivedClass::getPanelId());
				functions.add(&createFunc<DerivedClass>);
			}
		}

		/** Creates a subclass instance with the registered Identifier and returns a base class pointer to this. You need to take care of the ownership of course. */
		FloatingTileContent* createFromId(const Identifier &id, FloatingTile* parent) const
		{
			const int index = ids.indexOf(id);

			if (index != -1) return functions[index](parent);
			else			 return nullptr;
		}

		/** Returns the list of all registered items. */
		const Array<Identifier> &getIdList() const { return ids; }

		void handlePopupMenu(PopupMenu& m, FloatingTile* parent);

		void registerAllPanelTypes();

	private:

		/** @internal */
		template <typename DerivedClass> static FloatingTileContent* createFunc(FloatingTile* parent) { return new DerivedClass(parent); }

		/** @internal */
		typedef FloatingTileContent* (*PCreateFunc)(FloatingTile*);

		Array<Identifier> ids;
		Array <PCreateFunc> functions;;
	};

	/** @internal */
	int getFixedSizeForOrientation() const;

	/** Overwrite this method if your component should hide the "title" when not in layout mode.
	*
	*	This will make the parent shell use the full area for this component so make sure it doesn't interfer with the shell buttons.
	*/
	virtual bool showTitleInPresentationMode() const { return true; };

protected:

	/** Overwrite this method if the component has a fixed width. */
	virtual int getFixedWidth() const { return 0; }

	/** Overwrite this method if the component has a fixed height. */
	virtual int getFixedHeight() const { return 0; }
	
private:

	Component::SafePointer<FloatingTile> parent;
	
	String customTitle;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingTileContent)
};


class FloatingFlexBoxWindow : public DocumentWindow

{
public:

	FloatingFlexBoxWindow();

	void closeButtonPressed() override;

	void resized() override
	{
		getContentComponent()->setBounds(getLocalBounds());
	}

};

struct FloatingPanelTemplates
{
	static Component* createMainPanel(FloatingTile* root);

	static void create2x2Matrix(FloatingTile* parent);

	static void create3Columns(FloatingTile* parent);

	static void create3Rows(FloatingTile* parent);

	struct Helpers;
	
};




#endif  // FLOATINGTILECONTENT_H_INCLUDED
