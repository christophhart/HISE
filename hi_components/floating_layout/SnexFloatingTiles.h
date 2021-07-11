/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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

#pragma once

namespace hise {
using namespace juce;

struct SnexTileBase : public Component,
	public FloatingTileContent
{
	enum SpecialPanelIds
	{
		FileName = (int)FloatingTileContent::PanelPropertyId::numPropertyIds, ///< the content of the text editor
		numSpecialPanelIds
	};

	SnexTileBase(FloatingTile* parent) :
		FloatingTileContent(parent)
	{};

	var toDynamicObject() const override
	{
		var obj = FloatingTileContent::toDynamicObject();
		storePropertyInObject(obj, SpecialPanelIds::FileName, currentFile.getFullPathName(), String());
		return obj;
	}

	void setCurrentFile(const File& f);

	void fromDynamicObject(const var& object) override
	{
		FloatingTileContent::fromDynamicObject(object);

		auto fileName = getPropertyWithDefault(object, SpecialPanelIds::FileName).toString();

		if (fileName.isNotEmpty())
			setCurrentFile(File(fileName));
	}

	virtual void workbenchChanged(snex::ui::WorkbenchData::Ptr newWorkbench) = 0;

	int getNumDefaultableProperties() const override { return numSpecialPanelIds; };
	Identifier getDefaultablePropertyId(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultablePropertyId(index);

		RETURN_DEFAULT_PROPERTY_ID(index, SpecialPanelIds::FileName, "FileName");

		jassertfalse;
		return{};
	}

	var getDefaultProperty(int index) const override
	{
		if (index < (int)PanelPropertyId::numPropertyIds)
			return FloatingTileContent::getDefaultProperty(index);

		RETURN_DEFAULT_PROPERTY(index, SpecialPanelIds::FileName, var(""));

		jassertfalse;
		return{};
	}

	File currentFile;
};

/** A Wrapper around a SnexWorkbenchComponent. 

	- Must derive from snex::ui::WorkbenchComponent
	- must have SET_PANEL_ID
*/
template <typename C> struct SnexWorkbenchPanel : public FloatingTileContent,
							public Component,
							public snex::ui::WorkbenchData::Listener,
							public snex::ui::WorkbenchManager::WorkbenchChangeListener
{
	using Ptr = snex::ui::WorkbenchData::Ptr;

	SnexWorkbenchPanel(FloatingTile* parent):
		FloatingTileContent(parent)
	{
		auto wb = static_cast<snex::ui::WorkbenchManager*>(getMainController()->getWorkbenchManager());
		static_assert(std::is_base_of<snex::ui::WorkbenchComponent, C>(), "not a workbench component");
		wb->addListener(this);
		setWorkbench(wb->getCurrentWorkbench());
	}

	~SnexWorkbenchPanel()
	{
		auto wb = static_cast<snex::ui::WorkbenchManager*>(getMainController()->getWorkbenchManager());
		wb->removeListener(this);
	}

	void workbenchChanged(snex::ui::WorkbenchData::Ptr newWorkbench) override
	{
		auto wb = static_cast<snex::ui::WorkbenchManager*>(getMainController()->getWorkbenchManager());

		if (newWorkbench == wb->getRootWorkbench() || newWorkbench == nullptr)
			setWorkbench(newWorkbench);
	}

	void recompiled(Ptr p) override
	{
		
	}

	bool showTitleInPresentationMode() const override { return forceShowTitle; }


	int getFixedHeight() const override
	{
		if (content != nullptr)
			return content->getFixedHeight();

		return 0;
	}

	void setWorkbench(Ptr wb)
	{
		content = nullptr;

		if (wb != nullptr)
		{
			content = new C(wb);
			auto asComponent = static_cast<Component*>(content.get());

			asComponent->setLookAndFeel(&getMainController()->getGlobalLookAndFeel());
			addAndMakeVisible(asComponent);
		}

		resized();
	}

#if 0
	void workbenchChanged(Ptr oldWorkBench, Ptr newWorkbench) override
	{
		setWorkbench(newWorkbench);
	}
#endif

	static Identifier getPanelId() { return C::getId(); };
	
	Identifier getIdentifierForBaseClass() const override { return getPanelId(); };

	void resized() override
	{
		if(content != nullptr)
			content->setBounds(getParentShell()->getContentBounds());
	}

	bool forceShowTitle = true;

	ScopedPointer<C> content;
};

struct SnexEditorPanel : public Component,
						 public FloatingTileContent,
						 public snex::ui::WorkbenchData::Listener,
						 public snex::ui::WorkbenchManager::WorkbenchChangeListener
{
	using Ptr = snex::ui::WorkbenchData::Ptr;

	SnexEditorPanel(FloatingTile* parent);;

	~SnexEditorPanel();

	void workbenchChanged(Ptr newWorkbench) override
	{
		setWorkbenchData(newWorkbench);
	}

	void setWorkbenchData(snex::ui::WorkbenchData::Ptr newWorkbench)
	{
		if (wb != nullptr)
			wb->removeListener(this);

		wb = newWorkbench;

		if (wb != nullptr)
		{
			addAndMakeVisible(playground = new snex::jit::SnexPlayground(newWorkbench, false));
			playground->setFullTokenProviders();
			wb->addListener(this);
		}

		resized();
	}

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	void recompiled(snex::ui::WorkbenchData::Ptr);

	SET_PANEL_NAME("SnexEditTab");

	void resized() override
	{
		auto b = FloatingTileContent::getParentShell()->getContentBounds();
		if (playground != nullptr)
			playground->setBounds(b);
	}

	void paint(Graphics& g) override;

	WeakReference<snex::ui::WorkbenchData> wb;

	ScopedPointer<snex::jit::SnexPlayground> playground;
};





}