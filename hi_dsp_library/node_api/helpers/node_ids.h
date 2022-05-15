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
DECLARE_ID(Parameter);
DECLARE_ID(Parameters);
DECLARE_ID(Connections);
DECLARE_ID(Connection);
DECLARE_ID(Properties);
DECLARE_ID(Property);
DECLARE_ID(StepSize);
DECLARE_ID(MidPoint);
DECLARE_ID(Inverted);
DECLARE_ID(Bookmark);
DECLARE_ID(Bookmarks);
DECLARE_ID(MinValue);
DECLARE_ID(MaxValue);
DECLARE_ID(UpperLimit);
DECLARE_ID(SkewFactor);
DECLARE_ID(ShowParameters);
DECLARE_ID(ShowClones);
DECLARE_ID(DisplayedClones);
DECLARE_ID(Bypassed);
DECLARE_ID(SoulPatch);
DECLARE_ID(Debug);
DECLARE_ID(NumParameters);
DECLARE_ID(Value);
DECLARE_ID(ID);
DECLARE_ID(Index);
DECLARE_ID(NodeId);
DECLARE_ID(NumClones);
DECLARE_ID(ParameterId);
DECLARE_ID(Type);
DECLARE_ID(Folded);
DECLARE_ID(FactoryPath);
DECLARE_ID(Frozen);
DECLARE_ID(EmbeddedData);
DECLARE_ID(SwitchTargets);
DECLARE_ID(SwitchTarget);
DECLARE_ID(ModulationTargets);
DECLARE_ID(ModulationTarget);
DECLARE_ID(Automated);
DECLARE_ID(ModulationChain);
DECLARE_ID(SplitSignal);
DECLARE_ID(ValueTarget);
DECLARE_ID(Expression);
DECLARE_ID(Callback);
DECLARE_ID(undefined);
DECLARE_ID(FillMode);
DECLARE_ID(AddToSignal);
DECLARE_ID(UseFreqDomain);
DECLARE_ID(IsVertical);
DECLARE_ID(ResetValue);
DECLARE_ID(UseResetValue);
DECLARE_ID(RoutingMatrix);
DECLARE_ID(SampleIndex);
DECLARE_ID(File);
DECLARE_ID(PublicComponent);
DECLARE_ID(Code);
DECLARE_ID(ComplexData);
DECLARE_ID(Table);
DECLARE_ID(SliderPack);
DECLARE_ID(AudioFile);
DECLARE_ID(CodeLibrary);
DECLARE_ID(ClassId);
DECLARE_ID(AllowSubBlocks);
DECLARE_ID(Mode);
DECLARE_ID(BlockSize);
DECLARE_ID(IsPolyphonic);
DECLARE_ID(UseRingBuffer);
DECLARE_ID(IsProcessingHiseEvent);
DECLARE_ID(IsControlNode);
DECLARE_ID(HasModeTemplateArgument);
DECLARE_ID(ModeNamespaces);
DECLARE_ID(IsOptionalSnexNode);
DECLARE_ID(TemplateArgumentIsPolyphonic);
DECLARE_ID(IsCloneCableNode);
DECLARE_ID(IsRoutingNode);
DECLARE_ID(IsPublicMod);
DECLARE_ID(UseUnnormalisedModulation);
DECLARE_ID(AllowPolyphonic);
DECLARE_ID(AllowCompilation);
DECLARE_ID(UncompileableNode);
DECLARE_ID(CompileChannelAmount);


struct Helpers
{
	static Array<Identifier> getDefaultableIds()
	{
		static const Array<Identifier> dIds =
		{
			Comment,
			NodeColour,
			Folded,
			Expression,
			SkewFactor,
			StepSize,
			Inverted,
			EmbeddedData,
			Automated,
			AllowPolyphonic,
			AllowCompilation,
            CompileChannelAmount
		};

		return dIds;
	}

#define returnIfDefault(id_, v) if(id == id_) return v;

	static var getDefaultValue(const Identifier& id)
	{
		returnIfDefault(Comment, "");
		returnIfDefault(NodeColour, 0x000000);
		returnIfDefault(Folded, false);
		returnIfDefault(Expression, "");
		returnIfDefault(SkewFactor, 1.0);
		returnIfDefault(StepSize, 0.0);
		returnIfDefault(Inverted, false);
		returnIfDefault(EmbeddedData, -1);
		returnIfDefault(Automated, false);
		returnIfDefault(AllowCompilation, false);
		returnIfDefault(AllowPolyphonic, false);
        returnIfDefault(CompileChannelAmount, 2);

        return {};
	}

#undef returnIfDefault

	static var getWithDefault(const ValueTree& v, const Identifier& id)
	{
		if (v.hasProperty(id))
			return v[id];

		return getDefaultValue(id);
	}

	static void setToDefault(ValueTree& v, const Identifier& id, UndoManager* um = nullptr)
	{
		v.setProperty(id, getDefaultValue(id), um);
	}
};

enum EditType
{
	Toggle,
	Text,
	Slider,
	Choice,
	Hidden
};

}



#undef DECLARE_ID

}

namespace snex
{
namespace cppgen
{

using namespace juce;
using namespace scriptnode;

struct CustomNodeProperties
{
	struct Data
	{
		bool initialised = false;
		NamedValueSet properties;
	};

	template <typename T> static void setPropertyForObject(T& obj, const Identifier& id)
	{
		addNodeIdManually(T::getStaticId(), id);
	}

	

	static void setInitialised(bool allInitialised)
	{
		CustomNodeProperties d;
		d.data->initialised = true;
	}

	static bool isInitialised()
	{
		CustomNodeProperties d;
		return d.data->initialised;
	}

	static void setModeNamespace(const Identifier& nodeId, const String& modeNamespace)
	{
#if !HISE_NO_GUI_TOOLS
		CustomNodeProperties d;

		auto l = d.data->properties[PropertyIds::ModeNamespaces];

		if (l.isVoid())
		{
			l = var(new DynamicObject());
			d.data->properties.set(PropertyIds::ModeNamespaces, l);
		}

		l.getDynamicObject()->setProperty(nodeId, modeNamespace);
#endif
	}

	static String getModeNamespace(const Identifier& nodeId)
	{
		CustomNodeProperties d;
		return d.data->properties[PropertyIds::ModeNamespaces].getProperty(nodeId, "");
	}

	static String getModeNamespace(const ValueTree& nodeTree)
	{
		return getModeNamespace(Identifier(getIdFromValueTree(nodeTree)));
	}

	static void addNodeIdManually(const Identifier& nodeId, const Identifier& propId)
	{
#if !HISE_NO_GUI_TOOLS
		CustomNodeProperties d;
		auto l = d.data->properties[propId];

		if (l.isVoid())
		{
			d.data->properties.set(propId, Array<var>());
			l = d.data->properties[propId];
		}

		if (auto ar = l.getArray())
			ar->addIfNotAlreadyThere(nodeId.toString());
		else
			jassertfalse;
#endif
	}

	static bool nodeHasProperty(const juce::String& nodeName, const Identifier& propId)
	{
		CustomNodeProperties d;

		if (auto ar = d.data->properties[propId].getArray())
			return ar->contains(var(nodeName));

		return false;
	}

	static String getIdFromValueTree(const ValueTree& nodeTree)
	{
		auto p = nodeTree[PropertyIds::FactoryPath].toString();
		return p.fromFirstOccurrenceOf(".", false, false);
	}

	static bool nodeHasProperty(const ValueTree& nodeTree, const Identifier& propId)
	{
		return nodeHasProperty(getIdFromValueTree(nodeTree), propId);
	}

	SharedResourcePointer<Data> data;
};

}
}


