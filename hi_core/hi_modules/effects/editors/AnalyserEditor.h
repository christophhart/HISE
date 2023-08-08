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

class AnalyserEditor  : public ProcessorEditorBody
{
public:
    //==============================================================================
    AnalyserEditor (ProcessorEditor *p);
    ~AnalyserEditor();

    //==============================================================================
    
	void updateGui() override;

	int getBodyHeight() const override { return analyser != nullptr ? 250 : 40; };

    void paint (Graphics& g) override;
    void resized() override;

private:

	int getIdForCurrentType() const
	{
		if (analyser == nullptr) return 1;
		if (dynamic_cast<Goniometer*>(analyser.get())) return 2;
		if (dynamic_cast<Oscilloscope*>(analyser.get())) return 3;
		if (dynamic_cast<FFTDisplay*>(analyser.get())) return 4;

		jassertfalse;
		return -1;
	}

	int h;

    //==============================================================================
    
	ScopedPointer<HiComboBox> typeSelector;
	ScopedPointer<HiComboBox> bufferSize;
	ScopedPointer<AudioAnalyserComponent> analyser;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AnalyserEditor)
};


}
