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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;
using namespace snex::Types;

struct NodeBase;

class ParameterHolder
{
public:

	virtual ~ParameterHolder() {};

	virtual void createParameters(ParameterDataList& data) {};

};

struct polyphonic_base
{
	virtual ~polyphonic_base() {};

	polyphonic_base(const Identifier& id, bool addProcessEventFlag=true)
	{
		cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsPolyphonic);

		if(addProcessEventFlag)
			cppgen::CustomNodeProperties::addNodeIdManually(id, PropertyIds::IsProcessingHiseEvent);
	}
};

class HiseDspBase: public ParameterHolder
{
public:

	HiseDspBase() {};

	virtual ~HiseDspBase() {};

	bool isPolyphonic() const { return false; }

	virtual void initialise(NodeBase* n)
	{
		ignoreUnused(n);
	}
};

template <class T> class SingleWrapper : public HiseDspBase
{
public:

    inline void initialise(NodeBase* n) override
	{
		obj.initialise(n);
	}

	void handleHiseEvent(HiseEvent& e)
	{
		obj.handleHiseEvent(e);
	}

	T obj;
};


}
