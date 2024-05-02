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
			content = new C(wb.get());
			auto asComponent = static_cast<Component*>(content.get());

			asComponent->setLookAndFeel(&getMainController()->getGlobalLookAndFeel());
			addAndMakeVisible(asComponent);
		}

		resized();
	}

	static Identifier getPanelId() { return C::getId(); };
	
	Identifier getIdentifierForBaseClass() const override { return getPanelId(); };

	void resized() override
	{
		if(content != nullptr)
			content->setBounds(getParentContentBounds());
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

	SET_PANEL_NAME("SnexEditTab");

	SnexEditorPanel(FloatingTile* parent);;
	~SnexEditorPanel();

	void workbenchChanged(Ptr newWorkbench) override;
	void setWorkbenchData(snex::ui::WorkbenchData::Ptr newWorkbench);

	bool showTitleInPresentationMode() const override
	{
		return false;
	}

	void recompiled(snex::ui::WorkbenchData::Ptr);
	void resized() override;
	void paint(Graphics& g) override;

	WeakReference<snex::ui::WorkbenchData> wb;
	ScopedPointer<snex::jit::SnexPlayground> playground;
};





}
