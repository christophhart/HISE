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

class HardcodedNode;




class ParameterHolder
{
public:

	using ParameterData = ParameterDataImpl;

	virtual ~ParameterHolder() {};

	virtual void createParameters(Array<ParameterData>& data) {};

};

class HiseDspBase: public ParameterHolder
{
public:

	HiseDspBase() {};

	virtual ~HiseDspBase() {};

	virtual void initialise(NodeBase* n)
	{
		ignoreUnused(n);
	}

	virtual void prepare(PrepareSpecs ) {};

	virtual void handleHiseEvent(HiseEvent& e)
	{
		ignoreUnused(e);
	};

	virtual bool isPolyphonic() const { return false; }

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseDspBase);

	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseDspBase);
};

template <class T> class SingleWrapper : public HiseDspBase
{
public:

	inline void initialise(NodeBase* n) override
	{
		obj.initialise(n);
	}

	void handleHiseEvent(HiseEvent& e) final override
	{
		obj.handleHiseEvent(e);
	}

	bool isPolyphonic() const override
	{
		return obj.getWrappedObject().isPolyphonic();
	}

protected:

	T obj;
};


}
