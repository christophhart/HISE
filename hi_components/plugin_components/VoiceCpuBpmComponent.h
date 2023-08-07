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

#ifndef VOICECPUBPMCOMPONENT_H_INCLUDED
#define VOICECPUBPMCOMPONENT_H_INCLUDED

namespace hise { using namespace juce;

class VuMeter;

/** A component that shows the current statistics (voice count, CPU usage etc.). 
	@ingroup hise_ui
*/
class VoiceCpuBpmComponent : public Component,
	public ControlledObject,
	public Timer,
	public ButtonListener,
	public MainController::SampleManager::PreloadListener,
	public DebugLogger::Listener
{
public:

	VoiceCpuBpmComponent(MainController *mc_);

	~VoiceCpuBpmComponent();

	// ================================================================================================================

	void buttonClicked(Button *b) override;
	void timerCallback() override;
	void resized() override;

	

	void paint(Graphics& g) override;
	void paintOverChildren(Graphics& g) override;

	void setMainControllers(Array<WeakReference<MainController>> &newMainControllers);

	void preloadStateChanged(bool isPreloading) override;

	void recordStateChanged(bool state) override;

	void mouseDown(const MouseEvent& e) override;

private:

	struct InternalSleepListener;

	ScopedPointer<InternalSleepListener> il;

	bool isRecording = false;

	bool preloadActive = false;

	// ================================================================================================================

	Array<WeakReference<MainController>> mainControllers;

	ScopedPointer<ShapeButton> panicButton;
	ScopedPointer<ShapeButton> midiButton;

	ScopedPointer<Label> voiceLabel;
	ScopedPointer<VuMeter> cpuSlider;
	ScopedPointer<Label> bpmLabel;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoiceCpuBpmComponent)
};

} // namespace hise

#endif  // VOICECPUBPMCOMPONENT_H_INCLUDED
