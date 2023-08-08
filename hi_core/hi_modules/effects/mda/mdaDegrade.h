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

#ifndef __mdaDegrade_H
#define __mdaDegrade_H

namespace hise { using namespace juce;

class mdaDegrade : public MdaEffect
{
public:

	

	mdaDegrade();
	~mdaDegrade();

	String getEffectName() const override { return "Degrade";};
	int getNumParameters() const override { return 6; };

	void process(float **, float **, int ) override {};
	void processReplacing(float **inputs, float **outputs, int sampleFrames) override;

	void setParameter(int index, float value) override;
	float getParameter(int index)  const override;
	void getParameterLabel(int index, char *label)  const override;
	void getParameterDisplay(int index, char *text) const override;
	void getParameterName(int index, char *text) const override;
	float filterFreq(float hz);
	void suspend();

protected:
	float fParam1;
	float fParam2;
	float fParam3;
	float fParam4;
	float fParam5;
	float fParam6;
	float fi2, fo2, clp, lin, lin2, g1, g2, g3, mode;
	float buf0, buf1, buf2, buf3, buf4, buf5, buf6, buf7, buf8, buf9;

	float buf0R, buf1R, buf2R, buf3R, buf4R, buf5R, buf6R, buf7R, buf8R, buf9R;

	int tn, tcount;

};

} // namespace hise

#endif
