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

/** A component that displays the content of a buffer loaded into a AudioSampleProcessor. */
class HiseAudioSampleBufferComponent : public AudioSampleBufferComponentBase
{
public:

	HiseAudioSampleBufferComponent(SafeChangeBroadcaster* p);;

	bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
	void updatePlaybackPosition() override;
	File getDefaultDirectory() const override;
	void itemDropped(const SourceDetails& dragSourceDetails) override;
	void loadFile(const File& f) override;
	void newBufferLoaded() override;
	void updateProcessorConnection() override;
	void poolItemWasDropped(const PoolReference& /*ref*/);
	void mouseDoubleClick(const MouseEvent&) override;
};

using AudioSampleBufferComponent = HiseAudioSampleBufferComponent;


/** A component that displays the waveform of a sample.
*
*	It uses a thumbnail data to display the waveform of the selected ModulatorSamplerSound and has some SampleArea
*	objects that allow changing of its sample ranges (playback range, loop range etc.) @see SampleArea.
*
*	It uses a timer to display the current playbar.
*/
class SamplerSoundWaveform : public AudioDisplayComponent,
	public Timer
{
public:

	/** Creates a new SamplerSoundWaveform.
	*
	*	@param ownerSampler the ModulatorSampler that the SamplerSoundWaveform should use.
	*/
	SamplerSoundWaveform(const ModulatorSampler *ownerSampler);

	~SamplerSoundWaveform();


	/** used to display the playing positions / sample start position. */
	void timerCallback() override;

	/** draws a vertical ruler at the position where the sample was recently started. */
	void drawSampleStartBar(Graphics &g);

	/** enables the range (makes it possible to drag the edges). */
	void toggleRangeEnabled(AreaTypes type);

	/** Call this whenever the sample ranges change.
	*
	*	If you only want to refresh the sample area (while dragging), use refreshSampleAreaBounds() instead.
	*/
	void updateRanges(SampleArea *areaToSkip = nullptr) override;

	void updateRange(AreaTypes area, bool refreshBounds);

	double getSampleRate() const override;

	void paint(Graphics &g) override;

	void resized() override;

	/** Sets the currently displayed sound.
	*
	*	It listens for the global sound selection and displays the last selected sound if the selection changes.
	*/
	void setSoundToDisplay(const ModulatorSamplerSound *s, int multiMicIndex = 0);

	const ModulatorSamplerSound *getCurrentSound() const { return currentSound.get(); }


	float getNormalizedPeak() override;

private:

	const ModulatorSampler *sampler;
	ReferenceCountedObjectPtr<ModulatorSamplerSound> currentSound;

	int numSamplesInCurrentSample;


	double sampleStartPosition;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerSoundWaveform)
};


}
