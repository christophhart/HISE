/*
  ==============================================================================

    ProcessorViews.h
    Created: 20 May 2015 10:41:52pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PROCESSORVIEWS_H_INCLUDED
#define PROCESSORVIEWS_H_INCLUDED




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

#endif  // PROCESSORVIEWS_H_INCLUDED
