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
DECLARE_ID(Name);
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
DECLARE_ID(ShowComments);
DECLARE_ID(ShowClones);
DECLARE_ID(DisplayedClones);
DECLARE_ID(Bypassed);
DECLARE_ID(Debug);
DECLARE_ID(NumParameters);
DECLARE_ID(Value);
DECLARE_ID(DefaultValue);	
DECLARE_ID(ID);
DECLARE_ID(Page);
DECLARE_ID(CurrentPageIndex);
DECLARE_ID(SubGroup);
DECLARE_ID(Index);
DECLARE_ID(NodeId);
DECLARE_ID(NumClones);
DECLARE_ID(LocalId);
DECLARE_ID(ParameterId);
DECLARE_ID(Type);
DECLARE_ID(Folded);
DECLARE_ID(Locked);
DECLARE_ID(FactoryPath);
DECLARE_ID(Frozen);
DECLARE_ID(EmbeddedData);
DECLARE_ID(SwitchTargets);
DECLARE_ID(SwitchTarget);
DECLARE_ID(ModulationTargets);
DECLARE_ID(ModulationTarget);
DECLARE_ID(Automated);
DECLARE_ID(AutomatedExternal);
DECLARE_ID(SmoothingTime);
DECLARE_ID(ModulationChain);
DECLARE_ID(SplitSignal);
DECLARE_ID(ValueTarget);
DECLARE_ID(Expression);
DECLARE_ID(Callback);
DECLARE_ID(undefined);
DECLARE_ID(FillMode);
DECLARE_ID(OutsideSignalPath);
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
DECLARE_ID(Model);
DECLARE_ID(HpfFreq);
DECLARE_ID(BlockSize);
DECLARE_ID(IsPolyphonic);
DECLARE_ID(HasFixedParameters);
DECLARE_ID(UseRingBuffer);
DECLARE_ID(IsProcessingHiseEvent);
DECLARE_ID(IsControlNode);
DECLARE_ID(HasModeTemplateArgument);
DECLARE_ID(ModeNamespaces);
DECLARE_ID(IsOptionalSnexNode);
DECLARE_ID(TemplateArgumentIsPolyphonic);
DECLARE_ID(IsCloneCableNode);
DECLARE_ID(IsRoutingNode);
DECLARE_ID(IsFixRuntimeTarget);
DECLARE_ID(IsDynamicRuntimeTarget);
DECLARE_ID(NeedsModConfig);
DECLARE_ID(IsPublicMod);
DECLARE_ID(UseUnnormalisedModulation);
DECLARE_ID(AllowPolyphonic);
DECLARE_ID(AllowCompilation);
DECLARE_ID(UncompileableNode);
DECLARE_ID(CompileChannelAmount);
DECLARE_ID(HasTail);
DECLARE_ID(SourceId);
DECLARE_ID(SuspendOnSilence);
DECLARE_ID(TextToValueConverter);
DECLARE_ID(ModulationBlockSize);
DECLARE_ID(ExternalModulation);
DECLARE_ID(ModColour);
DECLARE_ID(ReceiveTargets);

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
			HasTail,
			Page,
			CurrentPageIndex,
			SubGroup,
			SuspendOnSilence,
            CompileChannelAmount,
			TextToValueConverter,
			ModulationBlockSize
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
		returnIfDefault(Page, "");
		returnIfDefault(CurrentPageIndex, 0);
		returnIfDefault(SubGroup, "");
		returnIfDefault(SkewFactor, 1.0);
		returnIfDefault(StepSize, 0.0);
		returnIfDefault(Inverted, false);
		returnIfDefault(EmbeddedData, -1);
		returnIfDefault(Automated, false);
		returnIfDefault(AutomatedExternal, false);
		returnIfDefault(HasTail, true);
		returnIfDefault(SuspendOnSilence, false);
		returnIfDefault(AllowCompilation, false);
		returnIfDefault(AllowPolyphonic, false);
        returnIfDefault(CompileChannelAmount, 2);
		returnIfDefault(TextToValueConverter, "Undefined");
		returnIfDefault(ModulationBlockSize, 0);
		returnIfDefault(ExternalModulation, "Disabled");
		returnIfDefault(ModColour, (int)HiseModulationColours::ColourId::ExtraMod);

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

namespace UIValues
{
	static constexpr int HeaderHeight = 24;
	static constexpr int ParameterHeight = 48 + 18 + 20;
	static constexpr int MacroDragHeight = 20;
	static constexpr int NodeWidth = 128;
	static constexpr int NodeHeight = 48;
	static constexpr int NodeMargin = 10;
	static constexpr int ZoomOffset = 60;
	static constexpr int DuplicateSize = 128;
	static constexpr int PinHeight = 24;
	static constexpr int TabHeight = 20;
	static constexpr int GroupHeight = 20;
	static constexpr int ParameterWidth = 100;
}

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
		NamedValueSet unscaledParameterIds;
		NamedValueSet modOutputs;
	};

	template <typename T> static void setPropertyForObject(T& obj, const Identifier& id)
	{
		addNodeIdManually(T::getStaticId(), id);
	}

	static void addModOutput(const Identifier& id, const StringArray& sa)
	{
		Array<var> list;

		for(const auto& s: sa)
			list.add(s);

		CustomNodeProperties d;
		d.data->modOutputs.set(id, var(list));
	}

	static StringArray getModOutputs(const Identifier& id)
	{
		CustomNodeProperties d;
		
		const auto& l = d.data->modOutputs[id];

		StringArray sa;

		if(auto ar = l.getArray())
		{
			for(auto& v: *ar)
				sa.add(v.toString());
		}
		
		return sa;
	}

	static void setInitialised(bool allInitialised)
	{
		CustomNodeProperties d;
		d.data->initialised = allInitialised;
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

	static void clearNodeProperties(const Identifier& nodeId)
	{
#if !HISE_NO_GUI_TOOLS
		CustomNodeProperties d;

		for(auto& l: d.data->properties)
		{
			if(l.value.isArray())
			{
				l.value.getArray()->removeAllInstancesOf(nodeId.toString());
			}
		}
#endif
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

	/** Use this to query whether the given node needs a runtime target. */
	template <typename T> static bool isRuntimeTarget()
	{
		String id = T::WrappedObjectType::getStaticId().toString();

		auto d = nodeHasProperty(id, PropertyIds::IsDynamicRuntimeTarget);
		auto f = nodeHasProperty(id, PropertyIds::IsFixRuntimeTarget);

		return d || f;
	}

	static StringArray getAllNodesWithProperty(const Identifier& propId)
	{
		CustomNodeProperties d;
		StringArray sa;

		if (auto ar = d.data->properties[propId].getArray())
		{
			for (const auto& a : *ar)
				sa.add(a);
		}

		return sa;
	}

	static bool nodeHasProperty(const juce::String& nodeName, const Identifier& propId)
	{
		CustomNodeProperties d;

		if (auto ar = d.data->properties[propId].getArray())
			return ar->contains(var(nodeName));

		return false;
	}

	static void writeAllProperties(String nodeId, DynamicObject::Ptr obj)
	{
		CustomNodeProperties d;

		for(const auto& nv: d.data->properties)
		{
			if(auto l = nv.value.getArray())
			{
				if(l->contains(nodeId))
					obj->setProperty(nv.name, true);
			}
		}

		if (d.data->unscaledParameterIds[nodeId].isArray())
			obj->setProperty(PropertyIds::UseUnnormalisedModulation, d.data->unscaledParameterIds[nodeId].clone());
	}

	static void writeAllProperties(ValueTree& nodeTree, DynamicObject::Ptr obj)
	{
		auto propId = getIdFromValueTree(nodeTree);

		CustomNodeProperties d;

		auto sw = nodeTree.getChildWithName(PropertyIds::SwitchTargets);

		if(sw.isValid())
		{
			auto outputs = getModOutputs(propId);

			if(sw.getNumChildren() == outputs.size())
			{
				for(int i = 0; i < outputs.size(); i++)
				{
					sw.getChild(i).setProperty(PropertyIds::ID, outputs[i], nullptr);
				}
			}
		}

		writeAllProperties(propId, obj);
	}

	static String getIdFromValueTree(const ValueTree& nodeTree)
	{
		auto p = nodeTree[PropertyIds::FactoryPath].toString();
		return p.fromFirstOccurrenceOf(".", false, false);
	}

	/** Use this function whenever you create an unscaled modulation and it will check that
		against the values.
	*/
	static void addUnscaledParameter(const Identifier& id, const String& parameterName)
	{
		CustomNodeProperties d;

		var ar = d.data->unscaledParameterIds[id];

		if (!ar.isArray())
			ar = var(Array<var>());

		ar.insert(ar.size(), parameterName);

		d.data->unscaledParameterIds.set(id, ar);
	}

	static bool isUnscaledParameter(const ValueTree& parameterTree)
	{
		if (!parameterTree.isValid())
			return false;

		jassert(parameterTree.getType() == PropertyIds::Parameter);

		auto nodeTree = parameterTree.getParent().getParent();

		jassert(nodeTree.getType() == PropertyIds::Node);

		auto pId = parameterTree[PropertyIds::ID].toString();
		auto nId = getIdFromValueTree(nodeTree);

		CustomNodeProperties n;
		auto ar = n.data->unscaledParameterIds[nId];
		return ar.indexOf(pId) != -1;
	}
	
	/** This will return the parameter tree that is used to determine the source range of the node.
	
		If you're using an unscaled modulation node that is supposed to forward its incoming value to the target,
		make sure to call addUnscaledParameter() in the createParameter() function.
		
		This information will be used by the code generator as well as the scriptnode UI to figure out
		if the range scale is working correctly (and display the warning icon if there's a range mismatch).
	*/
	static ValueTree getParameterTreeForSourceRange(const ValueTree& nodeTree)
	{
		if (nodeTree.getType() == PropertyIds::Parameter)
		{
			return nodeTree;
		}

		if (nodeTree.getType() == PropertyIds::Node)
		{
			auto id = Identifier(getIdFromValueTree(nodeTree));

			CustomNodeProperties d;
			
			auto l = d.data->unscaledParameterIds[id];

			if (l.isArray())
			{
				for (auto& v : *l.getArray())
				{
					auto pId = v.toString();
					jassertfalse;
				}
			}
		}
        
        jassertfalse;
        return {};
	}

	static bool nodeHasProperty(const ValueTree& nodeTree, const Identifier& propId)
	{
		return nodeHasProperty(getIdFromValueTree(nodeTree), propId);
	}

	SharedResourcePointer<Data> data;
};

}
}

#if HISE_INCLUDE_SCRIPTNODE_DATABASE
namespace scriptnode {
using namespace juce;
using namespace hise;

struct NodeDatabase
{
	static constexpr char ItemMarker = 89;
	static constexpr char IDMarker = 90;
	static constexpr char DescriptionMarker = 95;
	static constexpr char TreeMarker = 92;
	static constexpr char PropertyMarker = 91;
	static constexpr char ScreenshotMarker = 93;

	struct Item
	{
		Item(const ValueTree& v) :
			nodeTree(v.createCopy()),
			properties(new DynamicObject())
		{

		}

		Item() :
			nodeTree(ValueTree()),
			properties(nullptr)
		{};

		operator bool() const { return nodeTree.isValid(); }

		Item(InputStream& is)
		{
			auto ok = is.readByte() == DescriptionMarker;

			if(ok)
				description = is.readString();

			ok = is.readByte() == PropertyMarker;

			if (ok)
			{
				auto obj = var::readFromStream(is);
				properties = obj.getDynamicObject();
			}

			ok = is.readByte() == TreeMarker;

			if (ok)
			{
				nodeTree = ValueTree::readFromStream(is);
			}

		}

		void writeToStream(OutputStream& os) const
		{
			os.writeByte(DescriptionMarker);
			os.writeString(description);
			os.writeByte(PropertyMarker);

			if(properties != nullptr)
			{
				var obj(properties.get());
				obj.writeToStream(os);
			}
			else
			{
				var obj(new DynamicObject());
				obj.writeToStream(os);
			}

			os.writeByte(TreeMarker);
			nodeTree.writeToStream(os);
		}

		ValueTree nodeTree;
		DynamicObject::Ptr properties;
		String description;
	};

	NodeDatabase()
	{
	}

	void writeToStream(OutputStream& os) const
	{
		for (const auto& i : data->items)
		{
			auto id = i.first;
			os.writeByte(IDMarker);
			os.writeString(id);
			os.writeByte(ItemMarker);
			i.second.writeToStream(os);
		}
	}

	void addItem(const String& path, Item&& item)
	{
		data->items[path] = std::move(item);
	}

	StringArray getFactoryIds(bool getSignalNodes)
	{
		if(auto pd = data->getProjectData())
		{
			StringArray combined;

			combined.addArray(getSignalNodes ? data->signalFactoryIds : data->cableFactoryIds);
			combined.addArray(getSignalNodes ? pd->signalFactoryIds : pd->cableFactoryIds);
			return combined;
		}
		else
		{
			return getSignalNodes ? data->signalFactoryIds : data->cableFactoryIds;
		}
	}

	StringArray getNodeIds(bool getSignalNodes)
	{
		if (auto pd = data->getProjectData())
		{
			StringArray combined;

			combined.addArray(getSignalNodes ? data->signalNodeIds : data->cableNodeIds);
			combined.addArray(getSignalNodes ? pd->signalNodeIds : pd->cableNodeIds);
			return combined;
		}
		else
		{
			return getSignalNodes ? data->signalNodeIds : data->cableNodeIds;
		}
	}

	std::map<String, String> getDescriptions()
	{
		if(auto pd = data->getProjectData())
		{
			auto m1 = data->descriptions;
			auto m2 = pd->descriptions;

			std::map<String, String> combined;
			combined.merge(m1);
			combined.merge(m2);
			return combined;
		}
		else
			return data->descriptions;
	}

	ValueTree getValueTree(const String& factoryPath) const
	{
		if(data->items.find(factoryPath) != data->items.end())
			return data->items.at(factoryPath).nodeTree.createCopy();

		if (auto pd = data->getProjectData())
		{
			if (pd != nullptr && pd->items.find(factoryPath) != pd->items.end())
				return pd->items.at(factoryPath).nodeTree.createCopy();
		}

		return ValueTree();
	}

	DynamicObject::Ptr getProperties(const String& path)
	{
		if (data->items.find(path) != data->items.end())
			return data->items.at(path).properties;

		if(auto pd = data->getProjectData())
		{
			if (pd->items.find(path) != pd->items.end())
				return pd->items.at(path).properties;
		}
		
		return nullptr;
	}

	void processValueTrees(const std::function<void(ValueTree)>& f)
	{
		for(auto& d: data->items)
		{
			f(d.second.nodeTree);
		}

		if(auto pd = data->getProjectData())
		{
			for(auto& d: pd->items)
				f(d.second.nodeTree);
		}
	}

	void clear()
	{
		data->clear();
	}

	size_t getNumBytesUncompressed() const { return data->numBytesUncompressed; }

	void setProjectDataFolder(const File& projectDataFolder)
	{
		jassert(projectDataFolder.isDirectory());
		jassert(projectDataFolder.getFileName() == "ThirdParty");

		data->projectData = new ProjectData(projectDataFolder);
	}

	void writeProjectData(std::map<String, Item>& projectItems, const File& targetFile)
	{
		ProjectData::writeProjectData(projectItems, targetFile);
	}

private:

	struct Common
	{
		virtual ~Common() = default;

		void initNodeIdLists()
		{
			signalNodeIds.ensureStorageAllocated(items.size());
			cableNodeIds.ensureStorageAllocated(items.size());

			for (const auto& i : items)
			{
				if (i.second.properties->getProperty(PropertyIds::OutsideSignalPath))
				{
					cableNodeIds.add(i.first);
					cableFactoryIds.addIfNotAlreadyThere(i.first.upToFirstOccurrenceOf(".", false, false));
				}

				else
				{
					signalNodeIds.add(i.first);
					signalFactoryIds.addIfNotAlreadyThere(i.first.upToFirstOccurrenceOf(".", false, false));
				}

				descriptions[i.first] = i.second.description;
			}

			idsInitialised = true;
		}

		virtual void clear()
		{
			items.clear();

			signalFactoryIds.clear();
			signalNodeIds.clear();

			cableFactoryIds.clear();
			cableNodeIds.clear();

			descriptions.clear();

			idsInitialised = false;
		}

		std::map<String, Item> items;

		bool idsInitialised = false;
		StringArray signalNodeIds, cableNodeIds, signalFactoryIds, cableFactoryIds;

		std::map<String, String> descriptions;
	};

	struct ProjectData: public Common
	{
		ProjectData(const File& projectDataFolder)
		{
			auto projectDataFile = projectDataFolder.getChildFile("project_database.dat");

			MemoryBlock compressed;
			
			if(projectDataFile.loadFileAsData(compressed))
			{
				MemoryBlock mb;
				zstd::ZDefaultCompressor comp;
				comp.expand(compressed, mb);

				MemoryInputStream mis(mb, false);

				while(!mis.isExhausted())
				{
					auto ok = mis.readByte() == IDMarker;

					if(!ok)
					{
						jassertfalse;
						break;
					}

					auto id = mis.readString();

					ok = mis.readByte() == ItemMarker;

					if(!ok)
					{
						jassertfalse;
						break;
					}
					
					auto ni = Item(mis);

					if(ni)
						items[id] = std::move(ni);
				}
			}

			initNodeIdLists();
		}

		static ValueTree processProjectValueTree(const String& id, const ValueTree& v)
		{
			jassert(v.getType() == PropertyIds::Network);

			auto isCompiler = v[PropertyIds::AllowCompilation];
			auto numChannels = v.getProperty(PropertyIds::CompileChannelAmount, 2);
			
			auto root = v.getChildWithName(PropertyIds::Node).createCopy();

			root.setProperty(PropertyIds::CompileChannelAmount, numChannels, nullptr);

			if(isCompiler)
			{
				root.setProperty(PropertyIds::FactoryPath, id, nullptr);
				
				auto nodes = root.getChildWithName(PropertyIds::Nodes);
				root.removeChild(nodes, nullptr);

				auto parameters = root.getChildWithName(PropertyIds::Parameters);

				valuetree::Helpers::forEach(parameters, [](ValueTree& con)
				{
					if(con.getType() == PropertyIds::Connections)
						con.removeAllChildren(nullptr);

					return false;
				});
			}
			else
			{
				root.setProperty(PropertyIds::Locked, true, nullptr);
			}
			
			return root;
		}

		static void writeProjectData(std::map<String, Item>& projectItems, const File& targetFolder)
		{
			jassert(targetFolder.isDirectory());
			auto targetFile = targetFolder.getChildFile("project_database.dat");

			MemoryOutputStream mos;

			for (auto& pi : projectItems)
			{
				mos.writeByte(IDMarker);

				String pid = "project.";
				pid << pi.first;

				mos.writeString(pid);
				mos.writeByte(ItemMarker);
				pi.second.nodeTree = processProjectValueTree(pid, pi.second.nodeTree);
				pi.second.writeToStream(mos);
			}

			mos.flush();

			MemoryBlock compressed;

			zstd::ZDefaultCompressor comp;

			comp.compress(mos.getMemoryBlock(), compressed);

			targetFile.deleteFile();
			targetFile.create();
			FileOutputStream pd(targetFile);

			MemoryInputStream mis(compressed, false);
			pd.writeFromInputStream(mis, compressed.getSize());
			pd.flush();
		}
	};

	struct Data: public Common
	{
		Data()
		{
			MemoryBlock mb(ScriptnodeDataBase::scriptnode_database_dat, ScriptnodeDataBase::scriptnode_database_datSize);
			MemoryBlock uncompressed;

			zstd::ZDefaultCompressor comp;
			comp.expand(mb, uncompressed);

			numBytesUncompressed = uncompressed.getSize();

			MemoryInputStream is(uncompressed, false);

			// we need to create at least one dynamic object before we can read vars from a stream...
			DynamicObject::Ptr empty = new DynamicObject();

			while (!is.isExhausted())
			{
				auto ok = is.readByte() == IDMarker;

				if (!ok)
				{
					jassertfalse;
					break;
				}

				auto path = is.readString();

				ok = is.readByte() == ItemMarker;

				if (!ok)
				{
					jassertfalse;
					break;
				}

				Item i(is);

				if (i)
					items[path] = std::move(i);
			}

			initNodeIdLists();
		}

		void clear() override
		{
			Common::clear();
			numBytesUncompressed = 0;
			projectData = nullptr;
		}

		ProjectData* getProjectData() { return projectData.get(); }
		const ProjectData* getProjectData() const { return projectData.get(); }

		size_t numBytesUncompressed = 0;

		ScopedPointer<ProjectData> projectData;
	};

	SharedResourcePointer<Data> data;

	

};



}
#endif


