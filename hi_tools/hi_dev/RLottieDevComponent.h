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

#pragma once

namespace hise {
using namespace juce;

/** A testbench for RLottie animations. It includes a preview, a code editor and a bunch of navigation functions. */
class RLottieDevComponent : public Component,
	public Timer
{
public:
	RLottieDevComponent(RLottieManager::Ptr manager);
	~RLottieDevComponent();

	void timerCallback() override;

	void paint(Graphics&) override;
	void resized() override;

private:

    hise::HiPropertyPanelLookAndFeel alaf;
    
	RLottieManager::Ptr manager;
	RLottieComponent animationComponent;

	CodeDocument doc;
	CodeEditorComponent editor;
	TextButton loadButton;
	TextButton compileButton;
	Slider frameSlider;
	TextButton autoPlayButton;
	TextButton exportButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RLottieDevComponent)
};


}
