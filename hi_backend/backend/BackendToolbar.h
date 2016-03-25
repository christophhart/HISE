/*
  ==============================================================================

    BackendToolbar.h
    Created: 17 Sep 2014 10:16:46am
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef BACKENDTOOLBAR_H_INCLUDED
#define BACKENDTOOLBAR_H_INCLUDED

class BackendProcessorEditor;


class MainToolbarFactory: public ToolbarItemFactory
{
public:

	MainToolbarFactory(BackendProcessorEditor *parentEditor):
		editor(parentEditor)
	{

	};

	
	void getAllToolbarItemIds(Array<int> &ids) override;
	
	void getDefaultItemSet(Array<int> &ids) override { getAllToolbarItemIds(ids); };

	ToolbarItemComponent * createItem(int itemId);
	
	struct MainToolbarPaths
	{
		static Drawable *createPath(int id, bool isOn);
	};

private:

	BackendProcessorEditor *editor;

	

};





#endif  // BACKENDTOOLBAR_H_INCLUDED
