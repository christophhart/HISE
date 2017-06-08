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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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
