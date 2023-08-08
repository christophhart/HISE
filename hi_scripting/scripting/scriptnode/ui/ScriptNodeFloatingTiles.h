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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

struct NetworkPanel : public PanelWithProcessorConnection
{
	NetworkPanel(FloatingTile* parent) :
		PanelWithProcessorConnection(parent)
	{};

    void paint(Graphics& g) override
    {
        g.setColour(Colour(0xFF262626));
        g.fillRect(getParentShell()->getContentBounds());
    }
    
	Identifier getProcessorTypeId() const override;
	virtual Component* createComponentForNetwork(DspNetwork* parent) = 0;
	Component* createContentComponent(int index) override;
	virtual Component* createEmptyComponent() { return nullptr; };
	void fillModuleList(StringArray& moduleList) override;
	virtual bool hasSubIndex() const { return true; }
	void fillIndexList(StringArray& sa);
};

struct DspNetworkGraphPanel : public NetworkPanel
{
	DspNetworkGraphPanel(FloatingTile* parent);

	SET_PANEL_NAME("DspNetworkGraph");

	void paint(Graphics& g) override;

	Component* createComponentForNetwork(DspNetwork* p) override;

	Component* createEmptyComponent() override;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetworkGraphPanel);
};


class NodePropertyPanel : public NetworkPanel
{
public:

	NodePropertyPanel(FloatingTile* parent);

	SET_PANEL_NAME("NodePropertyPanel");

	Component* createComponentForNetwork(DspNetwork* p) override;
};

class FaustEditorPanel: public NetworkPanel
{
public:
    
    FaustEditorPanel(FloatingTile* parent);
    
    SET_PANEL_NAME("FaustEditorPanel");
    Component* createComponentForNetwork(DspNetwork* p) override;
};

#if USE_BACKEND
struct WorkbenchTestPlayer : public FloatingTileContent,
	public Component,
	public WorkbenchManager::WorkbenchChangeListener,
	public WorkbenchData::Listener,
	public PooledUIUpdater::SimpleTimer
{
	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override;
	} factory;

	SET_PANEL_NAME("WorkbenchTestPlayer");

	WorkbenchTestPlayer(FloatingTile* parent);;

	void postPostCompile(WorkbenchData::Ptr wb);;

	void play();

	void stop();

	void timerCallback() override;

	void workbenchChanged(WorkbenchData::Ptr newWorkbench) override
	{
		if (wb != nullptr)
			wb->removeListener(this);

		wb = newWorkbench;

		if(wb != nullptr)
			wb->addListener(this);
	}

	void resized() override;

	void paint(Graphics& g) override;

	
	
	HiseAudioThumbnail outputPreview;
	HiseAudioThumbnail inputPreview;

	HiseShapeButton playButton;
	HiseShapeButton stopButton;
	HiseShapeButton midiButton;

	WorkbenchData::Ptr wb;
};
#endif



}
