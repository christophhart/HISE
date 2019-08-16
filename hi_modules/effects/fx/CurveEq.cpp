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

namespace hise { using namespace juce;

float CurveEq::getAttribute(int index) const
{
	if(index == -1) return 0.0f;

	const int filterIndex = index / BandParameter::numBandParameters;
	const BandParameter parameter = (BandParameter)(index % numBandParameters);

	if (auto filter = filterBands[filterIndex])
	{
		switch (parameter)
		{
		case BandParameter::Gain:	 return Decibels::gainToDecibels((float)filter->getGain());
		case BandParameter::Freq:	 return (float)filter->getFrequency();
		case BandParameter::Q:		 return (float)filter->getQ();
		case BandParameter::Type:	 return (float)filter->getType();
		case BandParameter::Enabled: return filter->isEnabled() ? 1.0f : 0.0f;
		case numBandParameters:
		default:                     return 0.0f;
		}
	}
	else
	{
		jassertfalse;
		return 0;
	}
	

	

	
}

void CurveEq::setInternalAttribute(int index, float newValue)
{
	if (index == -1) return;

	const int filterIndex = index / BandParameter::numBandParameters;
	const BandParameter parameter = (BandParameter)(index % numBandParameters);

	StereoFilter *filter = filterBands[filterIndex];

	jassert(filter != nullptr);

	if (filter != nullptr)
	{
		switch (parameter)
		{
		case BandParameter::Gain:	filter->setGain(Decibels::decibelsToGain(newValue)); break;
		case BandParameter::Freq:	filter->setFrequency(newValue); break;
		case BandParameter::Q:		filter->setQ(newValue); break;
		case BandParameter::Type:	filter->setType((int)newValue); break;
		case BandParameter::Enabled:filter->setEnabled(newValue >= 0.5f); break;
		case numBandParameters:
		default:                    break;
		}
	}
	else
	{
		debugError(this, "Invalid attribute index: " + String(index));
	}
}

ProcessorEditorBody *CurveEq::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new CurveEqEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

} // namespace hise
