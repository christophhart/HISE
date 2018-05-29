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

#ifndef MPE_MODULATOR_EDITORS_H_INCLUDED
#define MPE_MODULATOR_EDITORS_H_INCLUDED

namespace hise {
using namespace juce;


class MPEModulatorEditor : public ProcessorEditorBody
{
public:
	MPEModulatorEditor(ProcessorEditor* parent);

	void updateGui() override
	{
		typeSelector->updateValue();
		smoothingTime->updateValue();
		defaultValue->updateValue();
	}

	int getBodyHeight() const override
	{
		return 300;
	}

	void resized() override;

	void paint(Graphics& g) override;

private:

	ScopedPointer<TableEditor> tableEditor;
	ScopedPointer<HiComboBox> typeSelector;
	ScopedPointer<HiSlider> smoothingTime;
	ScopedPointer<HiSlider> defaultValue;
	ScopedPointer<MPEKeyboard> mpePanel;
};


}

#endif