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

namespace scriptnode {
using namespace juce;
using namespace hise;

template <class DynamicProcessorType> class DynamicsNodeBase : public HiseDspBase
{
public:

	static Identifier getStaticId();

	SET_HISE_NODE_EXTRA_HEIGHT(30);
	GET_SELF_AS_OBJECT(DynamicsNodeBase<DynamicProcessorType>);
	SET_HISE_NODE_IS_MODULATION_SOURCE(true);

	DynamicsNodeBase();

	void createParameters(Array<ParameterData>& data);
	Component* createExtraComponent(PooledUIUpdater* updater) override;
	bool handleModulation(double& max) noexcept;;
	void prepare(PrepareSpecs ps);
	void reset() noexcept;
	void process(ProcessData& d);
	void processSingle(float* data, int numChannels);

	DynamicProcessorType obj;
};

class EnvelopeFollowerNode : public HiseDspBase
{
public:

	SET_HISE_NODE_ID("envelope_follower");
	SET_HISE_NODE_EXTRA_HEIGHT(0);
	GET_SELF_AS_OBJECT(EnvelopeFollowerNode);
	SET_HISE_NODE_IS_MODULATION_SOURCE(false);

	EnvelopeFollowerNode();

	bool handleModulation(double&) noexcept { return false; };
	void prepare(PrepareSpecs ps) override;
	void reset() noexcept;
	void process(ProcessData& d);
	void processSingle(float* data, int numChannels);
	void createParameters(Array<ParameterData>& data);

	EnvelopeFollower::AttackRelease envelope;
};

namespace dynamics
{

DEFINE_EXTERN_MONO_TEMPLATE(gate, DynamicsNodeBase<chunkware_simple::SimpleGate>);
DEFINE_EXTERN_MONO_TEMPLATE(comp, DynamicsNodeBase<chunkware_simple::SimpleComp>);
DEFINE_EXTERN_MONO_TEMPLATE(limiter, DynamicsNodeBase<chunkware_simple::SimpleLimit>);

using envelope_follower = EnvelopeFollowerNode;
}


}
