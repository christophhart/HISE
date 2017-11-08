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

#ifndef ROUNDROBIN_H_INCLUDED
#define ROUNDROBIN_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSynthGroup;

class RoundRobinMidiProcessor: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("RoundRobin", "Round Robin");

	RoundRobinMidiProcessor(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		counter(0)
	{};

	/** Reactivates all groups on shutdown. */
	~RoundRobinMidiProcessor();

	

	float getAttribute(int) const override { return 0.0f; };

	void setInternalAttribute(int, float ) override { };
	
	/** deactivates the child synths in a round robin cycle. */
	void processHiseEvent(HiseEvent &m) override;
	
private:

	int counter;
};


} // namespace hise


#endif