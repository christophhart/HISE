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

#ifndef SAMPLERPANELTYPES_H_INCLUDED
#define SAMPLERPANELTYPES_H_INCLUDED

namespace hise { using namespace juce;

class SamplerBasePanel : public PanelWithProcessorConnection,
						 public SafeChangeListener
{
public:

	SamplerBasePanel(FloatingTile* parent);;

	virtual ~SamplerBasePanel();

	void changeListenerCallback(SafeChangeBroadcaster* b);

	bool hasSubIndex() const override { return false; }

	Identifier getProcessorTypeId() const override;

    void paint(Graphics& g) override;
    
	void fillModuleList(StringArray& moduleList) override
	{
		fillModuleListWithType<ModulatorSampler>(moduleList);
	};


	void contentChanged() override;
	
private:

};

class SampleEditorPanel : public SamplerBasePanel
{
public:

	SampleEditorPanel(FloatingTile* parent);

	SET_PANEL_NAME("SampleEditor");

	Component* createContentComponent(int index) override;


};

class SampleMapEditorPanel : public SamplerBasePanel
{
public:

	SampleMapEditorPanel(FloatingTile* parent);;

	SET_PANEL_NAME("SampleMapEditor");

	Component* createContentComponent(int index) override;
};


class SamplerTablePanel : public SamplerBasePanel
{
public:
	SamplerTablePanel(FloatingTile* parent);

	SET_PANEL_NAME("SamplerTable");

	Component* createContentComponent(int index) override;

};

} // namespace hise

#endif  // SAMPLERPANELTYPES_H_INCLUDED
