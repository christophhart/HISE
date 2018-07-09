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


void EffectProcessor::finaliseModChains()
{
	modChains.finalise();

	for (auto& mb : modChains)
		mb.getChain()->setParentProcessor(this);

	finalised = true;
}

void EffectProcessor::checkTailing(AudioSampleBuffer &b, int startSample, int numSamples)
{
	// Call this only on effects that produce a tail!
	jassert(hasTail());

	const float maxInL = FloatVectorOperations::findMaximum(tailCheck.getReadPointer(0, startSample), numSamples);
	const float maxInR = FloatVectorOperations::findMaximum(tailCheck.getReadPointer(1, startSample), numSamples);

	const float maxL = FloatVectorOperations::findMaximum(b.getReadPointer(0, startSample), numSamples);
	const float maxR = FloatVectorOperations::findMaximum(b.getReadPointer(1, startSample), numSamples);

	const float in = maxInL + maxInR;
	const float out = maxL + maxR;
		
	isTailing = (in == 0.0f && out >= 0.01f);
}

juce::Path VoiceEffectProcessor::getSpecialSymbol() const
{
	Path path;

	static const unsigned char pathData[] = { 110,109,0,0,2,67,92,174,213,67,98,0,0,2,67,211,15,215,67,217,133,255,66,92,46,216,67,0,0,250,66,92,46,216,67,98,39,122,244,66,92,46,216,67,0,0,240,66,211,15,215,67,0,0,240,66,92,174,213,67,98,0,0,240,66,230,76,212,67,39,122,244,66,92,46,211,67,0,0,250,
	66,92,46,211,67,98,217,133,255,66,92,46,211,67,0,0,2,67,230,76,212,67,0,0,2,67,92,174,213,67,99,109,0,0,230,66,92,174,213,67,98,0,0,230,66,211,15,215,67,217,133,225,66,92,46,216,67,0,0,220,66,92,46,216,67,98,39,122,214,66,92,46,216,67,0,0,210,66,211,
	15,215,67,0,0,210,66,92,174,213,67,98,0,0,210,66,230,76,212,67,39,122,214,66,92,46,211,67,0,0,220,66,92,46,211,67,98,217,133,225,66,92,46,211,67,0,0,230,66,230,76,212,67,0,0,230,66,92,174,213,67,99,109,0,0,200,66,92,174,213,67,98,0,0,200,66,211,15,215,
	67,217,133,195,66,92,46,216,67,0,0,190,66,92,46,216,67,98,39,122,184,66,92,46,216,67,0,0,180,66,211,15,215,67,0,0,180,66,92,174,213,67,98,0,0,180,66,230,76,212,67,39,122,184,66,92,46,211,67,0,0,190,66,92,46,211,67,98,217,133,195,66,92,46,211,67,0,0,200,
	66,230,76,212,67,0,0,200,66,92,174,213,67,99,109,0,0,2,67,92,46,206,67,98,0,0,2,67,211,143,207,67,217,133,255,66,92,174,208,67,0,0,250,66,92,174,208,67,98,39,122,244,66,92,174,208,67,0,0,240,66,211,143,207,67,0,0,240,66,92,46,206,67,98,0,0,240,66,230,
	204,204,67,39,122,244,66,92,174,203,67,0,0,250,66,92,174,203,67,98,217,133,255,66,92,174,203,67,0,0,2,67,230,204,204,67,0,0,2,67,92,46,206,67,99,109,0,0,230,66,92,46,206,67,98,0,0,230,66,211,143,207,67,217,133,225,66,92,174,208,67,0,0,220,66,92,174,208,
	67,98,39,122,214,66,92,174,208,67,0,0,210,66,211,143,207,67,0,0,210,66,92,46,206,67,98,0,0,210,66,230,204,204,67,39,122,214,66,92,174,203,67,0,0,220,66,92,174,203,67,98,217,133,225,66,92,174,203,67,0,0,230,66,230,204,204,67,0,0,230,66,92,46,206,67,99,
	109,0,0,200,66,92,46,206,67,98,0,0,200,66,211,143,207,67,217,133,195,66,92,174,208,67,0,0,190,66,92,174,208,67,98,39,122,184,66,92,174,208,67,0,0,180,66,211,143,207,67,0,0,180,66,92,46,206,67,98,0,0,180,66,230,204,204,67,39,122,184,66,92,174,203,67,0,
	0,190,66,92,174,203,67,98,217,133,195,66,92,174,203,67,0,0,200,66,230,204,204,67,0,0,200,66,92,46,206,67,99,109,0,0,2,67,92,174,198,67,98,0,0,2,67,211,15,200,67,217,133,255,66,92,46,201,67,0,0,250,66,92,46,201,67,98,39,122,244,66,92,46,201,67,0,0,240,
	66,211,15,200,67,0,0,240,66,92,174,198,67,98,0,0,240,66,230,76,197,67,39,122,244,66,92,46,196,67,0,0,250,66,92,46,196,67,98,217,133,255,66,92,46,196,67,0,0,2,67,230,76,197,67,0,0,2,67,92,174,198,67,99,109,0,0,230,66,92,174,198,67,98,0,0,230,66,211,15,
	200,67,217,133,225,66,92,46,201,67,0,0,220,66,92,46,201,67,98,39,122,214,66,92,46,201,67,0,0,210,66,211,15,200,67,0,0,210,66,92,174,198,67,98,0,0,210,66,230,76,197,67,39,122,214,66,92,46,196,67,0,0,220,66,92,46,196,67,98,217,133,225,66,92,46,196,67,0,
	0,230,66,230,76,197,67,0,0,230,66,92,174,198,67,99,109,0,0,200,66,92,174,198,67,98,0,0,200,66,211,15,200,67,217,133,195,66,92,46,201,67,0,0,190,66,92,46,201,67,98,39,122,184,66,92,46,201,67,0,0,180,66,211,15,200,67,0,0,180,66,92,174,198,67,98,0,0,180,
	66,230,76,197,67,39,122,184,66,92,46,196,67,0,0,190,66,92,46,196,67,98,217,133,195,66,92,46,196,67,0,0,200,66,230,76,197,67,0,0,200,66,92,174,198,67,99,101,0,0 };

	path.loadPathFromData(pathData, sizeof(pathData));

	return path;
}

} // namespace hise