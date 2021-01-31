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
DECLARE_ID(NumTables);
DECLARE_ID(NumSliderPacks);
DECLARE_ID(NumAudioFiles);
DECLARE_ID(LowerLimit);
DECLARE_ID(UpperLimit);
DECLARE_ID(SkewFactor);
DECLARE_ID(ShowParameters);
DECLARE_ID(Bypassed);
DECLARE_ID(SoulPatch);
DECLARE_ID(DynamicBypass);
DECLARE_ID(NumParameters);
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
DECLARE_ID(SwitchTargets);
DECLARE_ID(SwitchTarget);
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
DECLARE_ID(Mode);
DECLARE_ID(DataIndex);

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



}
