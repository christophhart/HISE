/*
  ==============================================================================

    SamplerToolbar.h
    Created: 19 Sep 2014 1:45:14pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef SAMPLERTOOLBAR_H_INCLUDED
#define SAMPLERTOOLBAR_H_INCLUDED

class SampleMapEditor;

class SampleMapEditorToolbarFactory: public ToolbarItemFactory
{
public:

	SampleMapEditorToolbarFactory(SampleMapEditor *editor_);

	void getAllToolbarItemIds(Array<int> &ids) override;

	void getDefaultItemSet(Array<int> &ids) override { getAllToolbarItemIds(ids); };

	ToolbarItemComponent * createItem(int itemId);
	
	struct ToolbarPaths
	{
		static Drawable *createPath(int id, bool isOn);
		
	};

	

private:

	ModulatorSampler *sampler;

	SampleMapEditor *editor;

};

class SampleEditor;

class SampleEditorToolbarFactory: public ToolbarItemFactory
{
public:

	SampleEditorToolbarFactory(SampleEditor *editor_);

	void getAllToolbarItemIds(Array<int> &ids) override;

	void getDefaultItemSet(Array<int> &ids) override { getAllToolbarItemIds(ids); };

	ToolbarItemComponent * createItem(int itemId);
	
	struct ToolbarPaths
	{
		static Drawable *createPath(int id, bool isOn);
		
	};

	

private:

	ModulatorSampler *sampler;

	SampleEditor *editor;

};


#endif  // SAMPLERTOOLBAR_H_INCLUDED
