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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode {
using namespace juce;
using namespace hise;
using namespace snex;
using namespace snex::Types;


namespace core
{

struct OscDisplay : public ScriptnodeExtraComponent<OscillatorDisplayProvider>
{
	OscDisplay(OscillatorDisplayProvider* n, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent(n, updater)
	{
		p = f.createPath("sine");
		this->setSize(0, 50);
	};

	void paint(Graphics& g) override
	{
		auto h = this->getHeight();
		auto b = this->getLocalBounds().withSizeKeepingCentre(h * 2, h).toFloat();
		p.scaleToFit(b.getX(), b.getY(), b.getWidth(), b.getHeight(), true);
		GlobalHiseLookAndFeel::fillPathHiStyle(g, p, h * 2, h, false);
	}

	static Component* createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		return new OscDisplay(reinterpret_cast<ObjectType*>(obj), updater);
	}

	void timerCallback() override
	{
		if (getObject() == nullptr)
			return;

		auto thisMode = (int)getObject()->currentMode;

		if (currentMode != thisMode)
		{
			currentMode = thisMode;

			auto pId = MarkdownLink::Helpers::getSanitizedFilename(this->getObject()->modes[currentMode]);
			p = f.createPath(pId);
			this->repaint();
		}
	}

	int currentMode = 0;
	WaveformComponent::WaveformFactory f;
	Path p;
};



} // namespace core

}
