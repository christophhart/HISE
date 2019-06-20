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

	Identifier getProcessorTypeId() const override;
	virtual Component* createComponentForNetwork(DspNetwork* parent) = 0;
	Component* createContentComponent(int index) override;
	void fillModuleList(StringArray& moduleList) override;
	virtual bool hasSubIndex() const { return true; }
	void fillIndexList(StringArray& sa);
};

struct DspNetworkGraphPanel : public NetworkPanel
{
	DspNetworkGraphPanel(FloatingTile* parent);

	SET_PANEL_NAME("DspNetworkGraph");

	Component* createComponentForNetwork(DspNetwork* p) override;
};


class NodePropertyPanel : public NetworkPanel
{
public:

	NodePropertyPanel(FloatingTile* parent);

	SET_PANEL_NAME("NodePropertyPanel");

	Component* createComponentForNetwork(DspNetwork* p) override;
};



}
