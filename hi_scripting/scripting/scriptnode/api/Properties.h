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

#define DECLARE_ID(x) static const Identifier x(#x);

namespace NamespaceIds
{

DECLARE_ID(wr);
DECLARE_ID(one);
DECLARE_ID(multi);
DECLARE_ID(bypass);

}




namespace PropertyIds
{
DECLARE_ID(Coallescated);
DECLARE_ID(Network);
DECLARE_ID(Node);
DECLARE_ID(Nodes);
DECLARE_ID(NodeColour);
DECLARE_ID(Comment);
DECLARE_ID(CommentWidth);
DECLARE_ID(Parameter);
DECLARE_ID(Parameters);
DECLARE_ID(Connections);
DECLARE_ID(Connection);
DECLARE_ID(Properties);
DECLARE_ID(Property);
DECLARE_ID(Public);
DECLARE_ID(Converter);
DECLARE_ID(StepSize);
DECLARE_ID(MidPoint);
DECLARE_ID(MinValue);
DECLARE_ID(MaxValue);
DECLARE_ID(Inverted);
DECLARE_ID(EmbeddedData);
DECLARE_ID(LowerLimit);
DECLARE_ID(UpperLimit);
DECLARE_ID(SkewFactor);
DECLARE_ID(ShowParameters);
DECLARE_ID(Bypassed);
DECLARE_ID(DynamicBypass);
DECLARE_ID(BypassRampTimeMs);
DECLARE_ID(Value);
DECLARE_ID(ID);
DECLARE_ID(Index);
DECLARE_ID(NodeId);
DECLARE_ID(ParameterId);
DECLARE_ID(Type);
DECLARE_ID(Folded);
DECLARE_ID(FactoryPath);
DECLARE_ID(FreezedPath);
DECLARE_ID(FreezedId);
DECLARE_ID(NumChannels);
DECLARE_ID(LockNumChannels);
DECLARE_ID(ModulationTargets);
DECLARE_ID(ModulationTarget);
DECLARE_ID(ModulationChain);
DECLARE_ID(ValueTarget);
DECLARE_ID(OpType);
DECLARE_ID(Expression);
DECLARE_ID(Callback);
DECLARE_ID(undefined);
DECLARE_ID(FillMode);
DECLARE_ID(AddToSignal);
DECLARE_ID(UseMidi);
DECLARE_ID(UseFreqDomain);
DECLARE_ID(ResetValue);
DECLARE_ID(UseResetValue);
DECLARE_ID(RoutingMatrix);
DECLARE_ID(SampleIndex);
DECLARE_ID(File);
DECLARE_ID(PublicComponent);
DECLARE_ID(Code);
DECLARE_ID(AllowSubBlocks);
DECLARE_ID(Enabled);

enum EditType
{
	Toggle,
	Text,
	Slider,
	Choice,
	Hidden
};

}

namespace ConverterIds
{
DECLARE_ID(Identity);
DECLARE_ID(Decibel2Gain);
DECLARE_ID(Gain2Decibel);
DECLARE_ID(SubtractFromOne);
DECLARE_ID(WetAmount);
DECLARE_ID(DryAmount);
}

namespace OperatorIds
{
DECLARE_ID(SetValue);
DECLARE_ID(Multiply);
DECLARE_ID(Add);
}

#undef DECLARE_ID

struct PropertyHelpers
{
	static Colour getColour(ValueTree data)
	{
		while (data.getParent().isValid())
		{
			if (data.hasProperty(PropertyIds::NodeColour))
			{
				auto c = getColourFromVar(data[PropertyIds::NodeColour]);

				if (!c.isTransparent())
					return c;
			}

			data = data.getParent();
		}

		return Colour();
	}

	static Colour getColourFromVar(const var& value)
	{
		int64 colourValue = 0;

		if (value.isInt64() || value.isInt())
			colourValue = (int64)value;
		else if (value.isString())
		{
			auto string = value.toString();

			if (string.startsWith("0x"))
				colourValue = string.getHexValue64();
			else
				colourValue = string.getLargeIntValue();
		}

		return Colour((uint32)colourValue);
	};

	static PropertyComponent* createPropertyComponent(ProcessorWithScriptingContent* p, ValueTree& d, const Identifier& id, UndoManager* um);
};



#define ALLOCA_FLOAT_ARRAY(size) (float*)alloca(size * sizeof(float)); 
#define CLEAR_FLOAT_ARRAY(v, size) memset(v, 0, sizeof(float)*size);

#define CREATE_EXTRA_COMPONENT(className) Component* createExtraComponent(PooledUIUpdater* updater) \
										  { return new className(updater); };

#define SET_HISE_POLY_NODE_ID(id) SET_HISE_NODE_ID(id); bool isPolyphonic() const override { return NumVoices > 1; };

#define SET_HISE_NODE_ID(id) static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER(id); };
#define SET_HISE_NODE_EXTRA_HEIGHT(x) int getExtraHeight() const final override { return x; };
#define SET_HISE_NODE_EXTRA_WIDTH(x) int getExtraWidth() const final override { return x; };
#define SET_HISE_NODE_IS_MODULATION_SOURCE(x) static constexpr bool isModulationSource = x;
#define SET_HISE_EXTRA_COMPONENT(height, className) SET_HISE_NODE_EXTRA_HEIGHT(height); \
												    CREATE_EXTRA_COMPONENT(className);

#define GET_OBJECT_FROM_CONTAINER(index) &obj.getObject().get<index>()
#define GET_SELF_AS_OBJECT(className) className& getObject() { return *this;} const className& getObject() const { return *this; }

#define HISE_EMPTY_RESET void reset() {}
#define HISE_EMPTY_PREPARE void prepare(PrepareSpecs) {}
#define HISE_EMPTY_PROCESS void process(ProcessData&) {}
#define HISE_EMPTY_PROCESS_SINGLE void processSingle(float*, int) {}
#define HISE_EMPTY_CREATE_PARAM void createParameters(Array<ParameterData>&){}
#define HISE_EMPTY_MOD bool handleModulation(double& ) { return false; }

#define DEFINE_EXTERN_MONO_TEMPLATE(monoName, classWithTemplate) extern template class classWithTemplate; using monoName = classWithTemplate;

#define DEFINE_EXTERN_NODE_TEMPLATE(monoName, polyName, className) extern template class className<1>; \
using monoName = className<1>; \
extern template class className<NUM_POLYPHONIC_VOICES>; \
using polyName = className<NUM_POLYPHONIC_VOICES>; 

#define DEFINE_EXTERN_MONO_TEMPIMPL(classWithTemplate) template class classWithTemplate;

#define DEFINE_EXTERN_NODE_TEMPIMPL(className) template class className<1>; template class className<NUM_POLYPHONIC_VOICES>;

namespace UIValues
{
static constexpr int HeaderHeight = 24;
static constexpr int ParameterHeight = 48 + 18 + 10;
static constexpr int NodeWidth = 128;
static constexpr int NodeHeight = 48;
static constexpr int NodeMargin = 10;
static constexpr int PinHeight = 24;
}

}
