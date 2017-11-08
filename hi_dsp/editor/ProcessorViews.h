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

#ifndef PROCESSORVIEWS_H_INCLUDED
#define PROCESSORVIEWS_H_INCLUDED

namespace hise { using namespace juce;

/** A ViewInfo Object contains all EditorStates of every child processor.
*	@ingroup views
*
*	It tries to handle old versions of the main processor as gracefully as possible.
*/
class ViewInfo: public RestorableObject
{
public:
	/** Creates an empty ViewInfo.
	*
	*	This is used to create a ViewInfo which will be restored using restoreFromValueTree. */
	ViewInfo(ModulatorSynthChain *synthChain):
		mainSynthChain(synthChain),
		rootProcessor(nullptr),
		changed(false)
	{};

	/** Creates a ViewInfo
	*
	*	This creates a usable ViewInfo which can be serialized and stored using exportToValueTree. */
	ViewInfo(ModulatorSynthChain *synthChain, Processor *rootProcessor_, const String &viewName_);

	void restoreFromValueTree(const ValueTree &v) override;

	ValueTree exportAsValueTree() const override;

	static void restoreViewInfoList(ModulatorSynthChain *synthChain, const String &encodedViewInfo, OwnedArray<ViewInfo> &infoArray);

	static String exportViewInfoList(const OwnedArray<ViewInfo> &infoArray);

	void restore();

	const String &getViewName() const noexcept {return viewName;};

	void setViewName(const String &name)
	{
		viewName = name;
		changed = true;
	};

	

	void markAsChanged() noexcept {changed = true;};

	void markAsUnchanged() noexcept {changed = false;};

	bool wasChanged() const {return changed; };

	Processor *getRootProcessor()
	{
		return rootProcessor.get();
	}

private:

	struct ProcessorView
	{
		String processorId;
		String parentId;
		ScopedPointer<XmlElement> editorState;
	};

	bool changed;

	ModulatorSynthChain *mainSynthChain;

	String viewName;
	OwnedArray<ProcessorView> states;
	WeakReference<Processor> rootProcessor;

};




/** A ViewManager handles all logic for restorable view configurations.
*	@ingroup views
*
*	Normally, a ModulatorSynthChain automatically uses the features of the class by inheritance,
*	but you can subclass it if you want to.
*
*	It stores views in a list and has some handy features for managing them using a "current" slot.
*	
*/
class ViewManager
{
public:
	
	virtual ~ViewManager()
	{
		viewInfos.clear();
	}

	/** Returns the view that is currently selected or nullptr if  nothing is selected. */
	ViewInfo *getCurrentViewInfo() {return (currentViewInfo == -1) ? nullptr : viewInfos[currentViewInfo];};

	/** Changes the current view index. */
	void setCurrentViewInfo(int newViewIndex)
	{
		jassert(newViewIndex < viewInfos.size());
		currentViewInfo = newViewIndex;
	}

	/** Returns the amount of available views. */
	int  getNumViewInfos() const {return viewInfos.size();};

	/** returns the ViewInfo at the specified index. */
	ViewInfo *getViewInfo(int index) {return viewInfos[index];};

	/** This marks the current view as changed. */
	void setCurrentViewChanged()
	{
		if(currentViewInfo != -1) getCurrentViewInfo()->markAsChanged();
	}

	/** Adds a new ViewInfo to the list and sets it as current ViewInfo. */
	void addViewInfo(ViewInfo *newInfo)
	{
		viewInfos.add(newInfo);
		currentViewInfo = viewInfos.size() - 1;
	};

	/** This replaces the current ViewInfo with the given ViewInfo. */
	void replaceCurrentViewInfo(ViewInfo *newInfo)
	{
		viewInfos.remove(currentViewInfo);
		viewInfos.insert(currentViewInfo, newInfo);
	}

	/** Deletes the current view info. */
	void removeCurrentViewInfo()
	{
		viewInfos.remove(currentViewInfo);
		currentViewInfo = -1;
	}
    
    void clearAllViews();


	/** Saves the views as encoded string into a existing ValueTree. */
	void saveViewsToValueTree(ValueTree &v) const
	{
		v.setProperty("views", ViewInfo::exportViewInfoList(viewInfos), nullptr);
		v.setProperty("currentView", currentViewInfo, nullptr);
	}

	

	/** Restores the views from a previously encoded ValueTree. */
	void restoreViewsFromValueTree(const ValueTree &v)
	{
		ViewInfo::restoreViewInfoList(chain, v.getProperty("views", ""), viewInfos);
		currentViewInfo = v.getProperty("currentView", -1);
	}

	void setRootProcessor(Processor *root_) { root = root_; };

	Processor *getRootProcessor() const { return root.get(); };

protected:

	/** Creates a new ViewManager. */
	ViewManager(ModulatorSynthChain *chain_, UndoManager *undoManager);

private:

	

	UndoManager *viewUndoManager;

	WeakReference<Processor> root;

	ModulatorSynthChain *chain;

	OwnedArray<ViewInfo> viewInfos;

	int currentViewInfo;
};

class ViewBrowsing : public UndoableAction
{
public:

	ViewBrowsing(ViewManager *viewManager_, Component* editor_, int scrollY_, Processor *newRoot_) :
		viewManager(viewManager_),
		newRoot(newRoot_),
		editor(editor_),
		scrollY(scrollY_)
	{}


	bool perform() override;

	bool undo() override;

private:

	ViewManager *viewManager;
	Component::SafePointer<Component> editor;
	WeakReference<Processor> newRoot;
	WeakReference<Processor> oldRoot;
	int scrollY;
};


} // namespace hise
#endif  // PROCESSORVIEWS_H_INCLUDED
