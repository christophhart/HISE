/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace snex {
namespace cppgen {
using namespace juce;

namespace FactoryIds
{
#define DECLARE_ID(x) static const Identifier x(#x);

	DECLARE_ID(container);
	DECLARE_ID(core);
	DECLARE_ID(wrap);
	DECLARE_ID(parameter);

#undef  DECLARE_ID

	static bool isContainer(const NamespacedIdentifier& id)
	{
		return id.getParent().getIdentifier() == container;
	}

	static bool isClone(const NamespacedIdentifier& id)
	{
		return id.toString() == "container::clone";
	}

	static bool isMulti(const NamespacedIdentifier& id)
	{
		return id.toString() == "container::multi";
	}
};

struct CloneHelpers
{
    static int indexOfRecursive(const ValueTree& root, const ValueTree& vToSearch)
    {
        auto thisIndex = root.indexOf(vToSearch);
        
        if(thisIndex != -1)
            return thisIndex;
        
        int index = 0;
        for(const auto& c: root)
        {
            auto idx = indexOfRecursive(c, vToSearch);
            
            if(idx != -1)
                return index;
            
            index++;
        }
        
        return -1;
    }
    
    static bool isCloneContainer(const ValueTree& v)
    {
        auto s = v[scriptnode::PropertyIds::FactoryPath].toString();
        return s == "container.clone";
    }
    
	static bool isCloneCable(const ValueTree& v)
	{
		return cppgen::CustomNodeProperties::nodeHasProperty(v, PropertyIds::IsCloneCableNode);
	}

    static int getCloneIndex(const ValueTree& v)
    {
        ValueTree t = v;
        
        if(isCloneContainer(t))
            return -1;
        
        while(t.isValid() && !isCloneContainer(t))
        {
            t = t.getParent();
        }
        
        if(!t.isValid())
            return -1;
        
        auto idx = indexOfRecursive(t.getChildWithName(PropertyIds::Nodes), v);
        
        jassert(idx != -1);
        return idx;
    }
};

void ValueTreeBuilder::checkUnflushed(Node::Ptr n)
{
    if(n->isFlushed())
    {
        Error e;
        e.v = n->nodeTree;
        e.errorMessage << "Trying to wrap node that was already flushed";
        throw e;
    }
}

Result ValueTreeBuilder::cleanValueTreeIds(ValueTree& vToClean)
{
    Array<Identifier> existingIds;
    
    auto r = Result::ok();
    auto rptr = &r;
    
	ValueTreeIterator::forEach(vToClean, [&existingIds, rptr](ValueTree& c)
	{
		static const Array<Identifier> idsToClean = { PropertyIds::ID, PropertyIds::NodeId, PropertyIds::ParameterId };

        if(c.getType() == PropertyIds::Node)
        {
            auto thisId = Identifier(c[PropertyIds::ID].toString());
            
            if(existingIds.contains(thisId))
            {
                *rptr = Result::fail("duplicate ID: " + thisId.toString());
                return true;
            }
            
            existingIds.add(thisId);
        }
        
		for (const auto& id : idsToClean)
		{
			if (c.hasProperty(id))
			{
				auto possibleVariableName = c[id].toString();
				auto betterVariableName = cppgen::StringHelpers::makeValidCppName(possibleVariableName);

				if (possibleVariableName.compare(betterVariableName) != 0)
				{
					c.setProperty(id, betterVariableName, nullptr);
				}
			}
		}

		return false;
	}, ValueTreeIterator::IterationType::ChildrenFirst);
    
    return r;
}

void ValueTreeBuilder::setHeaderForFormat()
{
	switch (outputFormat)
	{
	case Format::TestCaseFile:
	{
		JitFileTestCase::HeaderBuilder hb(v);
		setHeader(hb);
		break;
	}
	case Format::JitCompiledInstance:
	{
#if 0
		HeaderBuilder hb("My Funky Node");

		String l1, l2, l3;

		l1 << "Author:" << AlignMarker << "Funkymaster";
		l2 << "Creation Date:" << AlignMarker << Time::getCurrentTime().toString(true, false, false);
		l3 << "Project:" << AlignMarker << "ProjectName";

		hb << l1 << l2 << l3;

		hb.addEmptyLine();
		hb << "This code is autogenerated and will be overwritten the next time";
		hb << "you change the scriptnode patch";
#endif

		setHeader([](){ return "/* Autogenerated code */"; });
		break;
	}
	case Format::CppDynamicLibrary:
	{

	}
	}

}

bool ValueTreeBuilder::addNumVoicesTemplate(Node::Ptr n)
{
#if !BETTER_TEMPLATE_FORWARDING
	
#endif

	return false;
}

void ValueTreeBuilder::rebuild()
{
	clear();

	try
	{
		//Sanitizers::setChannelAmount(v);
		
		if (ValueTreeIterator::hasChildNodeWithProperty(v, PropertyIds::IsPolyphonic))
		{
			auto polyId = v[PropertyIds::ID].toString();
			CustomNodeProperties::addNodeIdManually(Identifier(polyId), PropertyIds::IsPolyphonic);
		}

		{
			*this << getGlueCode(FormatGlueCode::PreNamespaceCode);

			Namespace impl(*this, getGlueCode(FormatGlueCode::WrappedNamespace), false);
			addComment("Node & Parameter type declarations", Base::CommentType::FillTo80);
			pooledTypeDefinitions.add(parseNode(v));
			impl.flushIfNot();

			getGlueCode(FormatGlueCode::PublicDefinition);
		}
	}
	catch (Error& e)
	{
		r = Result::fail(e.errorMessage);
		wrongNodeCode = e.toString();
	}
}

String ValueTreeBuilder::getGlueCode(ValueTreeBuilder::FormatGlueCode c)
{
	if (outputFormat == Format::TestCaseFile)
	{
		switch (c)
		{
		case FormatGlueCode::WrappedNamespace: return "impl";
		case FormatGlueCode::PublicDefinition:
		{
			UsingTemplate instance(*this, "processor", NamespacedIdentifier::fromString("wrap::node"));

			instance << *pooledTypeDefinitions.getLast();
			instance.flushIfNot();
			return {};
		}
		case FormatGlueCode::MainInstanceClass: return getNodeId(v).getIdentifier().toString();
		}
	}

	if(outputFormat == Format::JitCompiledInstance)
	{
		switch (c)
		{
			case FormatGlueCode::WrappedNamespace: return "impl";
			case FormatGlueCode::PublicDefinition:
			{
				UsingTemplate instance(*this, v[PropertyIds::ID].toString(), NamespacedIdentifier::fromString("wrap::node"));

				instance << *pooledTypeDefinitions.getLast();

				if (!instance.templateArguments.isEmpty())
					addComment("polyphonic template declaration", Base::CommentType::RawWithNewLine);

				instance.flushIfNot();
				return {};
			}
			case FormatGlueCode::MainInstanceClass: return getNodeId(v).getIdentifier().toString();
		}
	}

	if (outputFormat == Format::CppDynamicLibrary)
	{
		switch (c)
		{
		case FormatGlueCode::PreNamespaceCode: 
		{
			*this << "#pragma once";
			addEmptyLine();


			ValueTreeIterator::forEach(v, [this](ValueTree& c)
            {
                if(c.getType() == PropertyIds::Node)
                {
                    auto id = ValueTreeIterator::getNodeFactoryPath(c);
                    if(id.getParent().getIdentifier() == Identifier("project"))
                    {
                        auto f = File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory);
                        
                        Include(*this, f, f.getChildFile(id.getIdentifier().toString()).withFileExtension(".h"));
                    }
                }
                
                return false;
            });
            
            addComment("These will improve the readability of the connection definition", Base::CommentType::RawWithNewLine);
            
            *this << "#define getT(Idx) template get<Idx>()";
            *this << "#define connectT(Idx, target) template connect<Idx>(target)";
            *this << "#define getParameterT(Idx) template getParameter<Idx>()";
            *this << "#define setParameterT(Idx, value) template setParameter<Idx>(value)";
            *this << "#define setParameterWT(Idx, value) template setWrapParameter<Idx>(value)";
            
			UsingNamespace(*this, NamespacedIdentifier("scriptnode"));
			UsingNamespace(*this, NamespacedIdentifier("snex"));
			UsingNamespace(*this, NamespacedIdentifier("snex").getChildId("Types"));
			
			return {};
		}
		case FormatGlueCode::WrappedNamespace: return v[PropertyIds::ID].toString() + "_impl";
		case FormatGlueCode::MainInstanceClass: return "instance";
		case FormatGlueCode::PublicDefinition: 
		{
            *this << "#undef getT";
            *this << "#undef connectT";
            *this << "#undef setParameterT";
            *this << "#undef setParameterWT";
            *this << "#undef getParameterT";
            
			addComment("Public Definition", Base::CommentType::FillTo80);

			Namespace n(*this, "project", false);

			UsingTemplate instance(*this, v[PropertyIds::ID].toString(), NamespacedIdentifier::fromString("wrap::node"));
			instance << *pooledTypeDefinitions.getLast();

			if (!instance.templateArguments.isEmpty())
				addComment("polyphonic template declaration", Base::CommentType::RawWithNewLine);

			instance.flushIfNot();
			return {};
		}
		
		}
	}

	return {};
}

void ValueTreeBuilder::addNodeComment(Node::Ptr n)
{
	auto nodeComment = n->nodeTree[PropertyIds::Comment].toString();

	if (nodeComment.isNotEmpty())
	{
		nodeComment = nodeComment.upToFirstOccurrenceOf("\n", false, false);
		addComment(nodeComment.trim(), Base::CommentType::FillTo80Light);
		n->flushIfNot();
	}
}

Node::Ptr ValueTreeBuilder::parseFixChannel(const ValueTree& n, int numChannelsToUse)
{
	auto u = getNode(n, false);

	auto id = n[PropertyIds::ID].toString();

#if ENABLE_CPP_DEBUG_LOG
	DBG("Parse with fix channel: " + id + " Channels: " + String(numChannelsToUse));
#endif

	auto wf = createNode(n, {}, "wrap::fix");

	*wf << numChannelsToUse;

	//*wf << (int)n[PropertyIds::NumChannels];
	*wf << *u;

	return wf;
}

Node::Ptr ValueTreeBuilder::getNode(const ValueTree& n, bool allowZeroMatch)
{
	auto id = getNodeId(n);

	if (auto existing = getTypeDefinition(id))
	{
		return existing;
	}
	else
	{
		if (allowZeroMatch)
			return nullptr;
		else
		{
			Error e;
			e.v = n;
			e.errorMessage = "Can't find node";
			throw e;
		}
	}
}

Node::Ptr ValueTreeBuilder::getNode(const NamespacedIdentifier& id, bool allowZeroMatch) const
{
	for (auto n : pooledTypeDefinitions)
	{
		if (n->scopedId == id)
			return n;
	}

	if (allowZeroMatch)
		return nullptr;

	Error e;
	e.errorMessage = "Can't find node " + id.toString();
	throw e;
}



Node::Ptr ValueTreeBuilder::parseNode(const ValueTree& n)
{
	if (auto existing = getNode(n, true))
		return existing;

	auto typeId = getNodeId(n);

	Node::Ptr newNode = createNode(n, typeId.getIdentifier(), getNodePath(n).toString());

	if (newNode->hasProperty(PropertyIds::UncompileableNode, false))
	{
		Error e;
		e.errorMessage << "You can't compile a Network with a " << getNodePath(n).toString() << " node.";
		throw e;
	}

	if (newNode->hasProperty(PropertyIds::IsPolyphonic, false))
	{
		newNode->addTemplateIntegerArgument("NV", true);
	}

	auto isBypassed = (bool)newNode->nodeTree[PropertyIds::Bypassed];
	auto isContainer = FactoryIds::isContainer(getNodePath(n));

	if (isBypassed && !isContainer)
	{
		newNode = wrapNode(newNode, NamespacedIdentifier::fromString("wrap::no_process"));
	}

	return parseFaustNode(newNode);
}

Node::Ptr ValueTreeBuilder::parseFaustNode(Node::Ptr u)
{
	if (u->nodeTree[PropertyIds::FactoryPath].toString() == "core.faust")
	{
		auto nodeProperties = u->nodeTree.getChildWithName(PropertyIds::Properties);
		auto faustClass = nodeProperties.getChildWithProperty(PropertyIds::ID, PropertyIds::ClassId.toString())[PropertyIds::Value].toString();
		auto faustPath = "project::" + faustClass;
		u = createNode(u->nodeTree, getNodeId(u->nodeTree).getIdentifier(), faustPath);
		// add Template argument "NV" (polyphony)
		u->addTemplateIntegerArgument("NV", true);

		faustClassIds->insert(faustClass);
		DBG("Exporting faust scriptnode, class: " + faustClass);
	}
		
	return parseRoutingNode(u);
}

Node::Ptr ValueTreeBuilder::parseRoutingNode(Node::Ptr u)
{
    jassert(u->nodeTree.isValid());
	auto np = getNodePath(u->nodeTree);

	if (np.getIdentifier() == Identifier("matrix"))
	{
		auto mid = getNodeId(u->nodeTree).getIdentifier().toString();

		mid << "_matrix";

		auto b64 = ValueTreeIterator::getNodeProperty(u->nodeTree, PropertyIds::EmbeddedData).toString();

		MemoryBlock mb;
		mb.fromBase64Encoding(b64);

		auto v = ValueTree::readFromGZIPData(mb.getData(), mb.getSize());

		Array<int> channelIndexes;
		Array<int> sendChannelIndexes;

		bool hasSendConnections = false;

		if (v.isValid())
		{
			for (int i = 0; i < numChannelsToCompile; i++)
			{
				Identifier c("Channel" + String(i));
				Identifier s("Send" + String(i));

				channelIndexes.add(v.getProperty(c, i));
				auto si = (int)v.getProperty(s, -1);
				hasSendConnections |= (si != -1);

				sendChannelIndexes.add(si);
			}
		}

		String bc = "routing::static_matrix<";
		bc << String(numChannelsToCompile) << ", " << mid << ", " << (hasSendConnections ? "true" : "false") << ">";

		Array<NamespacedIdentifier> bc2 = { NamespacedIdentifier::fromString(bc) };

		Struct mClass(*this, Identifier(mid), bc2, {}, false);

		String l1, l2;

		l1 << "static constexpr int channels[" << String(numChannelsToCompile) << "] = ";
		*this << l1;
		{
			StatementBlock s(*this, true);
			String initValues;

			for (int i = 0; i < numChannelsToCompile; i++)
			{
				initValues << String(channelIndexes[i]) << ", ";
			}

			*this << initValues.upToLastOccurrenceOf(", ", false, false);
		}

		if (hasSendConnections)
		{
			l2 << "static constexpr int sendChannels[" << String(numChannelsToCompile) << "] = ";
			*this << l2;
			{
				StatementBlock s(*this, true);
				String initValues;

				for (int i = 0; i < numChannelsToCompile; i++)
				{
					initValues << String(sendChannelIndexes[i]) << ", ";
				}

				*this << initValues.upToLastOccurrenceOf(", ", false, false);
			}
		}

		mClass.flushIfNot();

		*u << mClass;
	}
	else if (u->hasProperty(PropertyIds::IsRoutingNode, false))
	{
		PooledCableType::TemplateConstants c;

		c.numChannels = numChannelsToCompile;
		c.isFrame = ValueTreeIterator::forEachParent(u->nodeTree, [&](ValueTree& v)
		{
			if (v.getType() == PropertyIds::Node)
			{
				if (v[scriptnode::PropertyIds::FactoryPath].toString().contains("frame"))
					return true;
			}

			return false;
		});


		PooledCableType::Ptr nc;

		if (auto existing = pooledCables.getExisting(c))
		{
			nc = existing;
		}
		else
		{
			String s;

			if (c.numChannels == 1)
				s << "mono";
			else if (c.numChannels == 2)
				s << "stereo";
			else
				s << "multi" << c.numChannels;

			if (c.isFrame)
				s << "_frame";

			s << "_cable";

			nc = new PooledCableType(*this, s, c);
			
			nc->flushIfNot();

			pooledCables.add(nc);
		}

		*u << nc->toExpression();
	}

	return parseOptionalSnexNode(u);
}



Node::Ptr ValueTreeBuilder::parseOptionalSnexNode(Node::Ptr u)
{
    jassert(u->nodeTree.isValid());
    
	auto p = getNodePath(u->nodeTree).toString();

	if (CustomNodeProperties::nodeHasProperty(u->nodeTree, PropertyIds::IsOptionalSnexNode))
	{
		String type = ValueTreeIterator::getNodeProperty(u->nodeTree, PropertyIds::Mode).toString().toLowerCase();

		

		if (type == "custom")
			return parseSnexNode(u);
		else
		{
			addNumVoicesTemplate(u);
			u->addOptionalModeTemplate();
			return parseMod(u);
		}
	}

	if (getNodePath(u->nodeTree).getIdentifier().toString().endsWith("expr"))
	{
		return parseExpressionNode(u);
	}

	if (p.contains("snex"))
		return parseSnexNode(u);

	if (p.endsWith("midi"))
	{
		

#if 0
		Node::Ptr wn = new Node(*this, u->scopedId.id, NamespacedIdentifier(p));

		wn->nodeTree = u->nodeTree;
		String subName = "midi_logic::" + type;

		*wn << subName;

		return parseMod(wn);
#endif
	}

	return parseContainer(u);
}


Node::Ptr ValueTreeBuilder::parseExpressionNode(Node::Ptr u)
{
    jassert(u->nodeTree.isValid());
    
	bool isMathNode = getNodePath(u->nodeTree).getParent() == NamespacedIdentifier("math");
	ExpressionNodeBuilder nb(*this, u, isMathNode);

	return parseMod(nb.parse());
}

Node::Ptr ValueTreeBuilder::parseSnexNode(Node::Ptr u)
{
	SnexNodeBuilder s(*this, u);

	return parseMod(s.parse());
}



Node::Ptr ValueTreeBuilder::parseContainer(Node::Ptr u)
{
    jassert(u->nodeTree.isValid());
    
	if (FactoryIds::isClone(getNodePath(u->nodeTree)))
	{
		return parseCloneContainer(u);
	}

	if (FactoryIds::isContainer(getNodePath(u->nodeTree)))
	{
		auto numToUse = ValueTreeIterator::calculateChannelCount(u->nodeTree, numChannelsToCompile);

		auto realPath = u->nodeTree[PropertyIds::FactoryPath].toString().fromFirstOccurrenceOf("container.", false, false);

        auto isSidechain = realPath.startsWith("sidechain");
        
        if(isSidechain)
            numToUse *= 2;
        
        ScopedChannelSetter sns(*this, numToUse, isSidechain);
        
		for (auto c : u->nodeTree.getChildWithName(PropertyIds::Nodes))
        {
            auto childNode = parseNode(c);
            
            jassert(childNode->nodeTree.isValid());
			pooledTypeDefinitions.add(childNode);
        }

		parseContainerParameters(u);
		parseContainerChildren(u);

		auto needsInitialisation = u->isRootNode();
		
		if (needsInitialisation)
			return parseRootContainer(u);

		auto useSpecialWrapper = !(bool)u->nodeTree[PropertyIds::Bypassed];

		if (realPath.startsWith("modchain"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::control_rate"));
		}
		if (useSpecialWrapper && realPath.startsWith("frame"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::frame"), numChannelsToCompile);
		}
		if (realPath.startsWith("soft_bypass"))
		{
			auto smoothingTime = (int)ValueTreeIterator::getNodeProperty(u->nodeTree, PropertyIds::SmoothingTime);

			if (smoothingTime == 0)
				smoothingTime = 20;

			u = wrapNode(u, NamespacedIdentifier::fromString("bypass::smoothed"), smoothingTime);
		}
		if (useSpecialWrapper && realPath.startsWith("midi"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::event"));
		}
		if (realPath.startsWith("offline"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::offline"));
		}
        if (isSidechain)
        {
            u = wrapNode(u, NamespacedIdentifier::fromString("wrap::sidechain"));
        }
		if (realPath.startsWith("no_midi"))
		{
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::no_midi"));
		}
		if (useSpecialWrapper && realPath.startsWith("fix"))
		{
			auto bs = realPath.fromFirstOccurrenceOf("fix", false, false).getIntValue();
            
            if(bs == 0)
            {
                // fetch the block size from the property
                bs = ValueTreeIterator::getNodeProperty(u->nodeTree, PropertyIds::BlockSize);
            }
            
            if(bs == 0 || !isPowerOfTwo(bs))
            {
                Error e;
                e.errorMessage << "Illegal block size for fix_block container: " << String(bs);
                throw e;
            }
            
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::fix_block"), bs);
		}
		if (useSpecialWrapper && realPath.startsWith("oversample"))
		{
			auto os = realPath.fromFirstOccurrenceOf("oversample", false, false).getIntValue();
			u = wrapNode(u, NamespacedIdentifier::fromString("wrap::oversample"), os);
		}

        jassert(u->nodeTree.isValid());
		jassert(!u->isFlushed());

		addNodeComment(u);

#if 0
		if (u->hasProperty(PropertyIds::IsPolyphonic) ||
			u->hasProperty(PropertyIds::TemplateArgumentIsPolyphonic, true))
		{
			u->addTemplateIntegerArgument("NV", true);
		}
#endif

		u->flushIfNot();

	}

	return parseMod(u);
}

Node::Ptr ValueTreeBuilder::parseCloneContainer(Node::Ptr u)
{
	

	

    auto ct = u->nodeTree;
    auto pTree = ct.getChildWithName(PropertyIds::Parameters);
    auto nTree = ct.getChildWithName(PropertyIds::Nodes);
    
	auto hasNumCloneAutomation = (bool)pTree.getChild(0)[PropertyIds::Automated];

	auto cloneRange = RangeHelpers::getDoubleRange(pTree.getChildWithProperty(PropertyIds::ID, "NumClones"));

	// Check all `control.clone_cable` nodes if they are connected to a child node of this clone container
	// and verify that the NumClones parameter has the same range as the NumClones parameter
	// of this clone container
	ValueTreeIterator::forEach(v, [cloneRange, nTree](ValueTree& tree)
	{
		if (tree[PropertyIds::FactoryPath].toString() == "control.clone_cable")
		{
			for (auto m : tree.getChildWithName(PropertyIds::ModulationTargets))
			{
				auto targetNodeId = m[PropertyIds::NodeId];

				auto targetIsChild = ValueTreeIterator::forEach(nTree, [targetNodeId](ValueTree& tree)
				{
					if (tree.getType() == PropertyIds::Node)
						return tree[PropertyIds::ID] == targetNodeId;

					return false;
				});

				if (targetIsChild)
				{
					auto cableP = tree.getChildWithName(PropertyIds::Parameters).getChildWithProperty(PropertyIds::ID, "NumClones");

					auto cableRange = RangeHelpers::getDoubleRange(cableP);

					if (!RangeHelpers::isEqual(cableRange, cloneRange))
					{
						Error e;
						e.errorMessage = "Clone node sanity check failed: range mismatch between clone container and clone cable";
						e.errorMessage << "`" << RangeHelpers::toDisplayString(cableRange) << "` vs. `" << RangeHelpers::toDisplayString(cloneRange) << "`";
						e.v = nTree;
						throw e;
					}
				}
			}
		}

		return false;
	});

	if (hasNumCloneAutomation)
	{
		for (auto p : v.getChildWithName(PropertyIds::Parameters))
		{
			auto cTree = p.getChildWithName(PropertyIds::Connections);

			auto c = cTree.getChildWithProperty(PropertyIds::NodeId, ct[PropertyIds::ID]);

			if (c.isValid())
			{
				auto parameterRange = RangeHelpers::getDoubleRange(p);

				

				if (!RangeHelpers::isEqual(parameterRange, cloneRange))
				{
					Error e;
					e.errorMessage = "Clone node sanity check failed: range mismatch at automated parameter";
					e.errorMessage << "`" << RangeHelpers::toDisplayString(parameterRange) << "` vs. `" << RangeHelpers::toDisplayString(cloneRange) << "`";
					e.v = u->nodeTree;
					throw e;
				}

				if (cTree.indexOf(c) != 0)
				{
					Error e;
					e.errorMessage = "Clone node sanity check failed: NumClones must be first target in root parameter";
					e.v = u->nodeTree;
					throw e;
				}
			}
		}

		
	}

	auto processType = (CloneProcessType)(int)pTree.getChild(1)[PropertyIds::Value];

    int numClones = hasNumCloneAutomation ?
                    nTree.getNumChildren() :
                    (int)pTree.getChild(0)[PropertyIds::Value];
    
	String cloneClassId;

    if(!hasNumCloneAutomation)
        cloneClassId << "fix_";
    
    static const StringArray names = { "chain", "split", "copy"};
    
    cloneClassId << "clone";
    cloneClassId << names[(int)processType];
    
    u->setTemplateId(NamespacedIdentifier("wrap").getChildId(cloneClassId));
    
	auto firstChild = nTree.getChild(0);

	auto firstNode = parseNode(firstChild);

    pooledTypeDefinitions.add(firstNode);
    
	if (!FactoryIds::isContainer(getNodePath(firstChild)))
	{
        firstNode = parseFixChannel(firstNode->nodeTree, numChannelsToCompile);
        
		UsingTemplate innerChain(*this, "internal", NamespacedIdentifier::fromString("container::chain"));

		innerChain << "parameter::empty";
		innerChain << *firstNode;

		*u << innerChain;
	}
	else
		*u << *firstNode;

    *u << numClones;
    
    u->flushIfNot();
    
	return u;
}

void ValueTreeBuilder::emitRangeDefinition(const Identifier& rangeId, InvertableParameterRange r)
{
	StringArray args;

	args.add(rangeId.toString());

	args.add(Helpers::getCppValueString(r.rng.start, Types::ID::Double));
	args.add(Helpers::getCppValueString(r.rng.end, Types::ID::Double));

	String macroName = "DECLARE_PARAMETER_RANGE";

	if (r.rng.skew != 1.0)
	{
		macroName << "_SKEW";
		args.add(Helpers::getCppValueString(r.rng.skew, Types::ID::Double));
	}
	else if (r.rng.interval > 0.001)
	{
		macroName << "_STEP";
		args.add(Helpers::getCppValueString(r.rng.interval, Types::ID::Double));
	}
	if (r.inv)
	{
		macroName << "_INV";
	}

	for (auto& a : args)
		a = StringHelpers::withToken(IntendMarker, a);

	Macro(*this, macroName, args);
}

void ValueTreeBuilder::parseContainerChildren(Node::Ptr container)
{
	Node::List children;

    auto nodeTree = container->nodeTree.getChildWithName(PropertyIds::Nodes);
    
	bool isMulti = FactoryIds::isMulti(getNodePath(container->nodeTree));

	ValueTreeIterator::forEach(nodeTree, [&](ValueTree& c)
	{
		auto parentPath = getNodePath(ValueTreeIterator::findParentWithType(c, PropertyIds::Node));

		auto numToUse = -1;

		if (c.getParent().indexOf(c) == 0 || isMulti)
            numToUse = numChannelsToCompile;
		
		if (numToUse != -1)
			children.add(parseFixChannel(c, numToUse));
		else
			children.add(getNode(c, false));

		return false;
	}, ValueTreeIterator::IterationType::OnlyChildren);

	if (children.isEmpty())
	{
		UsingTemplate u(*this, "empty", NamespacedIdentifier::fromString("core::empty"));

		auto wf = createNode(container->nodeTree, {}, "wrap::fix");

		*wf << numChannelsToCompile;
		*wf << u;

		*container << *wf;
	}
		
	for (auto& c : children)
	{
		*container << *c;
	}
}

void ValueTreeBuilder::parseContainerParameters(Node::Ptr c)
{
	addEmptyLine();

	auto pTree = c->nodeTree.getChildWithName(PropertyIds::Parameters);
	auto numParameters = pTree.getNumChildren();

	Namespace n(*this, c->scopedId.getIdentifier().toString() + "_parameters", !ValueTreeIterator::hasRealParameters(c->nodeTree));

	if(numParameters == 0)
		*c << "parameter::empty";
	else if (numParameters == 1)
	{
		auto up = parseParameter(pTree.getChild(0), Connection::CableType::Parameter);
		n.flushIfNot();
		*c << *up;
	}
	else
	{
		addComment("Parameter list for " + c->scopedId.toString(), Base::CommentType::FillTo80Light);

		PooledParameter::List parameterList;

		for (auto c : pTree)
			parameterList.add(parseParameter(c, Connection::CableType::Parameter));

		String pId;
		pId << c->scopedId.getIdentifier();
		pId << "_plist";

		Node::Ptr l = createNode(pTree, pId, "parameter::list");

		for (auto p : parameterList)
		{
			p->flushIfNot();
			*l << *p;
		}

		l->flushIfNot();
		n.flushIfNot();
		*c << *l;
	}
}



Node::Ptr ValueTreeBuilder::parseRootContainer(Node::Ptr container)
{
	jassert(container->isRootNode());
	RootContainerBuilder rb(*this, container);

	return rb.parse();
}


Node::Ptr ValueTreeBuilder::parseComplexDataNode(Node::Ptr u, bool flushNode)
{
    jassert(u->nodeTree.isValid());
    
	if (ValueTreeIterator::isComplexDataNode(u->nodeTree))
	{
		ComplexDataBuilder c(*this, u);
		c.setFlushNode(flushNode);
		return c.parse();
	}

	return u;
}

PooledParameter::Ptr ValueTreeBuilder::parseParameter(const ValueTree& p, Connection::CableType connectionType)
{
	ValueTree cTree;

	auto pId = p[PropertyIds::ID].toString();

	if (connectionType == Connection::CableType::CloneCable)
	{
		cTree = p.getChildWithName(PropertyIds::ModulationTargets);

		if (cTree.getNumChildren() > 1)
		{
			auto chainId = pId;
			chainId << "_cc";

			int pIndex = 0;

			auto dc = makeParameter(chainId, "clonechain", {});

			for (auto c : cTree)
			{
				auto ip = createParameterFromConnection(getConnection(c), pId, pIndex++, {});
				auto cp = makeParameter(pId, "cloned", {});
				*cp << *ip;
				*dc << *cp;
			}

			return dc;
		}
		else
		{
			auto rawParameter = parseParameter(p, Connection::CableType::Modulation);
			pId << "_cable_mod";
			auto cp = makeParameter(pId, "cloned", {});
			*cp << *rawParameter;
			return cp;
		}
	}
	if (connectionType == Connection::CableType::Parameter)
	{
		cTree = p.getChildWithName(PropertyIds::Connections);
	}
	else if (connectionType == Connection::CableType::SwitchTarget)
	{
		cTree = p.getChildWithName(PropertyIds::Connections);

		pId = p.getParent().getParent()[PropertyIds::ID].toString();
		pId << "_c" << p.getParent().indexOf(p);
	}
	else if (connectionType == Connection::CableType::SwitchTargets)
	{
		cTree = p.getChildWithName(PropertyIds::SwitchTargets);

		pId << "_multimod";

		PooledParameter::List parameterList;

		for (auto c : cTree)
			parameterList.add(parseParameter(c, Connection::CableType::SwitchTarget));

		auto l = makeParameter(pId, "list", {});

		for (auto p : parameterList)
		{
			//p->flushIfNot();
			*l << *p;
		}

		l->flushIfNot();
		addEmptyLine();
		return addParameterAndReturn(l);
	}
	else
	{
		cTree = p.getChildWithName(PropertyIds::ModulationTargets);
		jassert(cTree.isValid());
		pId << "_mod";
	}

	auto numConnections = cTree.getNumChildren();

	if (numConnections == 0)
		return createParameterFromConnection({}, pId, -1, p);

	auto inputRange = RangeHelpers::getDoubleRange(p);

	auto mustCreateChain = !RangeHelpers::isIdentity(inputRange) || numConnections > 1;

	

	if (numConnections == 1)
	{
		auto pTree = ValueTreeIterator::getTargetParameterTree(cTree.getChild(0));
		auto targetRange = RangeHelpers::getDoubleRange(pTree);

		auto isSameRange = RangeHelpers::equalsWithError(inputRange, targetRange, 0.001);
		auto isUnscaledMod = cppgen::CustomNodeProperties::isUnscaledParameter(pTree);

		if(isSameRange || isUnscaledMod)
			mustCreateChain = false;
	}

	if (!mustCreateChain)
	{
		return createParameterFromConnection(getConnection(cTree.getChild(0)), pId, -1, p);
	}
	else
	{
		auto ch = makeParameter(pId, "chain", {});

		Array<Connection> chainList;

		bool unEqualRange = false;

		auto useUnnormalisedModulation = CustomNodeProperties::nodeHasProperty(p, PropertyIds::UseUnnormalisedModulation);

		for (auto c : cTree)
		{
            auto targetTree = ValueTreeIterator::getTargetParameterTree(c);
            
			auto targetRange = RangeHelpers::getDoubleRange(targetTree);
			auto isSameRange = RangeHelpers::equalsWithError(inputRange, targetRange, 0.001);
			auto isUnscaled = cppgen::CustomNodeProperties::isUnscaledParameter(targetTree);

			// Only check the range if the target parameter is scaled
			// and the source is using a scaled input range
			if(!useUnnormalisedModulation && !isUnscaled)
				unEqualRange |= !isSameRange;

			chainList.add(getConnection(c));
		}

		if(RangeHelpers::isIdentity(inputRange) || !unEqualRange)
			*ch << "ranges::Identity";
		else
		{
			String fWhat;
			fWhat << pId << "_InputRange";

			emitRangeDefinition(fWhat, inputRange);

			*ch << fWhat;
		}

		bool isPoly = false;

		int cIndex = 0;

		for (auto& c : chainList)
		{
			ScopedValueSetter<bool> svs(allowPlainParameterChild, !unEqualRange);

			if (c.n == nullptr)
			{
				jassertfalse;
				continue;
			}

			if (c.n->isPolyphonicOrHasPolyphonicTemplate())
				isPoly = true;

			auto up = createParameterFromConnection(c, pId, cIndex++, p);

			//auto up = createParameterFromConnection(c, pId, cIndex++, unEqualRange ? ValueTree() : p);
			*ch << *up;
		}

#if !BETTER_TEMPLATE_FORWARDING
		if (isPoly)
			ch->addTemplateIntegerArgument("NV");
#endif

		addIfNotEmptyLine();

		ch->flushIfNot();
		addEmptyLine();

		return addParameterAndReturn(ch);
	}
}

Node::Ptr ValueTreeBuilder::wrapNode(Node::Ptr u, const NamespacedIdentifier& wrapId, int firstIntParam)
{
	// The id is already used by the wrapped node...
    checkUnflushed(u);

	Node::Ptr wn = new Node(*this, u->scopedId.id, wrapId);

    wn->nodeTree = u->nodeTree;
    
	if (u->getLength() > 30)
	{
		StringHelpers::addSuffix(u->scopedId, "_");
		u->flushIfNot();
		addEmptyLine();
	}

	if(firstIntParam != -1)
		*wn << firstIntParam;
	*wn << *u;

	return wn;
}

Connection ValueTreeBuilder::getConnection(const ValueTree& c)
{
	jassert(c.getType() == PropertyIds::Connection || c.getType() == PropertyIds::ModulationTarget);

	String id = getGlueCode(FormatGlueCode::WrappedNamespace);
	
	id << "::" << c[PropertyIds::NodeId].toString();
	id << "_t";

	auto pId = c[PropertyIds::ParameterId].toString();
	auto nId = NamespacedIdentifier::fromString(id);

	Connection rc;

	auto targetTree = ValueTreeIterator::getTargetParameterTree(c);

	rc.targetRange = RangeHelpers::getDoubleRange(targetTree);
	
	if (cppgen::CustomNodeProperties::isUnscaledParameter(targetTree))
		rc.targetRange = {};

	if (c.getParent().getType() == PropertyIds::ModulationTargets)
		rc.cableType = Connection::CableType::Modulation;
	else
		rc.cableType = Connection::CableType::Parameter;

	if (pId == PropertyIds::Bypassed.toString())
		rc.index = scriptnode::bypass::ParameterId;

	rc.expressionCode = c[PropertyIds::Expression].toString();

	ValueTreeIterator::forEach(v, [&](ValueTree& t)
	{
		if (t.getType() == PropertyIds::Node)
		{
			if (CloneHelpers::getCloneIndex(t) > 0)
				return false;

			if (getNodeId(t) == nId)
			{
				auto pTree = t.getChildWithName(PropertyIds::Parameters);
				auto c = pTree.getChildWithProperty(PropertyIds::ID, pId);

				if(rc.getType() != Connection::ConnectionType::Bypass)
					rc.index = pTree.indexOf(c);

				if (getNode(t, true) == nullptr)
				{
#if ENABLE_CPP_DEBUG_LOG
					auto id = t[PropertyIds::ID].toString();

					DBG("Node " + id + " does not exist yet. Precursoring...");

#endif

					// We need to calculate the channel amount from scratch
					// because we're not in the right container
					auto root = ValueTreeIterator::getRoot(t);
					auto numChannelsToUse = rootChannelAmount;

					ValueTreeIterator::forEach(root, [&](ValueTree& c)
					{
						if (FactoryIds::isContainer(getNodePath(c)) && ValueTreeIterator::isParent(t, c))
							numChannelsToUse = ValueTreeIterator::calculateChannelCount(c, numChannelsToUse);

						if (c == t)
							return true;

						return false;
					});

					// We allow setting a higher channel count here because the node is not necessarily
					// inside the current channel container
					ScopedChannelSetter svs(*this, numChannelsToUse, true);
					pooledTypeDefinitions.add(parseNode(t));
				}

				return true;
			}
		}

		return false;
	});

	for (auto n : pooledTypeDefinitions)
	{
		if (n->scopedId == nId)
		{
			rc.n = n;
			return rc;
		}
	}

	return {};
}

Node::Ptr ValueTreeBuilder::parseMod(Node::Ptr u)
{
    jassert(u->nodeTree.isValid());
    
	auto modTree = u->nodeTree.getChildWithName(PropertyIds::ModulationTargets);

	auto multiModTree = u->nodeTree.getChildWithName(PropertyIds::SwitchTargets);

	auto hasSingleModTree = modTree.isValid() && modTree.getNumChildren() > 0;
	auto hasMultiModTree = multiModTree.isValid() && multiModTree.getNumChildren() > 0;

	if (hasSingleModTree || hasMultiModTree)
	{
		addEmptyLine();

		Connection::CableType c;

		if (hasSingleModTree)
		{
			c = CloneHelpers::isCloneCable(u->nodeTree) ? 
				  Connection::CableType::CloneCable :
				  Connection::CableType::Modulation;
		}
		else
			c = Connection::CableType::SwitchTargets;

		auto mod = parseParameter(u->nodeTree, c);

		mod->flushIfLong();

		auto id = getNodeId(u->nodeTree);

		

		if (!ValueTreeIterator::needsModulationWrapper(u->nodeTree))
		{
			*u << *mod;

			u->addOptionalModeTemplate();

			u = parseComplexDataNode(u);
		}
		else
		{
			u->addOptionalModeTemplate();
			auto inner = parseComplexDataNode(u, false);

			jassert(!inner->isFlushed());
			
			u = createNode(u->nodeTree, id.getIdentifier(), "wrap::mod");
			*u << *mod;
			*u << *inner;
		}

		addNodeComment(u);

		
		

		addNumVoicesTemplate(u);

		u->flushIfNot();

		return u;
	}
	else if (ValueTreeIterator::isControlNode(u->nodeTree))
	{
		*u << "parameter::empty";
		jassertfalse;
	}
	else
	{
		// Could be an unused node with a template parameter (like smoothed_parameter)...
		u->addOptionalModeTemplate();
	}

	return parseComplexDataNode(u);
}



Node::Ptr ValueTreeBuilder::getTypeDefinition(const NamespacedIdentifier& id)
{
	for (auto& c : pooledTypeDefinitions)
	{
		if (c->scopedId == id)
			return c;
	}

	return nullptr;
}

NamespacedIdentifier ValueTreeBuilder::getNodePath(const ValueTree& n)
{
	return ValueTreeIterator::getNodeFactoryPath(n);
	
}



snex::cppgen::PooledParameter::Ptr ValueTreeBuilder::createParameterFromConnection(const Connection& c, const Identifier& pName, int parameterIndexInChain, const ValueTree& pTree)
{
	String p = pName.toString();

	if (!c)
	{
		auto up = makeParameter(p, "empty", {});
		return addParameterAndReturn(up);
	}

	if (auto existing = pooledParameters.getExisting(c))
		return existing;

	if (parameterIndexInChain != -1)
		p << "_" << parameterIndexInChain;


	if (!c || c.index == -1)
	{
		Error e;
		e.errorMessage << "Can't find connection for " << pName.toString() << "\n> Make sure to save the network after you've added Faust or SNEX nodes so that all parameters are saved into the network XML.";
		throw e;
	}

	auto parseRange = [&](const InvertableParameterRange& r, const String& fWhat)
	{
		if (auto existing = pooledRanges.getExisting(r))
		{
			return existing;
		}
		else
		{
			auto r = new PooledRange({ *this, fWhat });
			r->c = c;

			emitRangeDefinition(fWhat, r->c.targetRange);

			addEmptyLine();
			return pooledRanges.add(r);
		}
	};

	auto t = c.getType();

	switch (t)
	{
	case Connection::ConnectionType::Bypass:
	{
		auto up = makeParameter(p, "bypass", c);
		*up << *c.n;

		if (!RangeHelpers::isIdentity(c.targetRange))
		{
			String fWhat;

			fWhat << up->scopedId.getIdentifier();
			fWhat << "Range";

			auto r = parseRange(c.targetRange, fWhat);

			*up << r->id.toExpression();
		}
		
		up->flushIfNot();
		addEmptyLine();

		return addParameterAndReturn(up);
	}
	case Connection::ConnectionType::Expression:
	{
		// parameter::expression<T, P, Expression>;

		auto up = makeParameter(p, "expression", c);
		*up << *c.n;
		*up << c.index;

		String fWhat;
		fWhat << up->scopedId.getIdentifier();
		fWhat << "Expression";

		if (auto existing = pooledExpressions.getExisting(c.expressionCode))
		{
			*up << existing->id.toExpression();
		}
		else
		{
			auto r = new PooledExpression({ *this, fWhat });

			r->expression = c.expressionCode;

			//DECLARE_PARAMETER_EXPRESSION(fWhat, x)
			Macro(*this, "DECLARE_PARAMETER_EXPRESSION", { StringHelpers::withToken(IntendMarker, fWhat), StringHelpers::withToken(IntendMarker, c.expressionCode) });
			addEmptyLine();
			*up << r->id.toExpression();

			pooledExpressions.add(r);
		}

		up->flushIfNot();
		addEmptyLine();

		return addParameterAndReturn(up);
	}
	case Connection::ConnectionType::RangeWithSkew:
	case Connection::ConnectionType::RangeWithStep:
	case Connection::ConnectionType::Range:
	{
		auto tr = c.targetRange;
		auto sr = RangeHelpers::getDoubleRange(pTree);

		auto sourceNodeTree = ValueTreeIterator::findParentWithType(pTree, PropertyIds::Node);

		auto n = pTree[PropertyIds::ID].toString();

		auto useNormalisedMod = CustomNodeProperties::nodeHasProperty(pTree, PropertyIds::UseUnnormalisedModulation);

		if (allowPlainParameterChild && (RangeHelpers::isEqual(tr, sr) || useNormalisedMod) && !tr.inv)
		{
			// skip both ranges and just pass on the parameter;
			auto up = makeParameter(p, "plain", c);
			*up << *c.n;
			*up << c.index;

			return addParameterAndReturn(up);
		}
		else
		{
			auto up = makeParameter(p, c.targetRange.inv ? "from0To1_inv" : "from0To1", c);
			*up << *c.n;
			*up << c.index;

			String fWhat;

			fWhat << up->scopedId.getIdentifier();
			fWhat << "Range";

			auto r = parseRange(c.targetRange, fWhat);

			*up << r->id.toExpression();

			up->flushIfNot();
			addEmptyLine();

			return addParameterAndReturn(up);
		}
	}
	default:
	{
		auto up = makeParameter(p, c.targetRange.inv ? "inverted" : "plain", c);
		*up << *c.n;
		*up << c.index;

		return addParameterAndReturn(up);
	}
	}
}

NamespacedIdentifier ValueTreeBuilder::getNodeId(const ValueTree& n)
{
	auto typeId = n[scriptnode::PropertyIds::ID].toString();
	typeId << "_t";

	return NamespacedIdentifier(getGlueCode(FormatGlueCode::WrappedNamespace)).getChildId(typeId);
}

NamespacedIdentifier ValueTreeBuilder::getNodeVariable(const ValueTree& n)
{
	auto typeId = n[scriptnode::PropertyIds::ID].toString();
	return getCurrentScope().getChildId(typeId);
}








bool ValueTreeIterator::needsModulationWrapper(ValueTree& v)
{
	if (v.getChildWithName(PropertyIds::ModulationTargets).getNumChildren() == 0)
		return false;

	return !isControlNode(v);
}

bool ValueTreeIterator::getNodePath(Array<int>& path, ValueTree& root, const Identifier& id)
{
	if (root[PropertyIds::ID].toString() == id.toString())
	{
		return true;
	}

	auto n = root.getChildWithName(PropertyIds::Nodes);

	for (auto c : n)
	{
        if (getNodePath(path, c, id))
		{
            // If the node is cloned, we have to omit the clone index as
            // path item. However if the clone is not a container, we can't
            // omit it (because we have introduced an artificial container
            // at parseCloneContainer), therefore, we have to insert the index
            auto isClone = CloneHelpers::isCloneContainer(n.getParent());
            
            auto isContainer = FactoryIds::isContainer(getNodeFactoryPath(c));
            
            if(!isClone || !isContainer)
                path.insert(0, getIndexInParent(c));
            
			return true;
		}
	}

	return false;
}

bool ValueTreeIterator::hasRealParameters(const ValueTree& containerTree)
{
	auto pTree = containerTree.getChildWithName(PropertyIds::Parameters);

	for (auto p : pTree)
	{
		auto c = p.getChildWithName(PropertyIds::Connections);

		if (c.getNumChildren() != 0)
			return true;
	}

	return false;
}

bool ValueTreeIterator::hasNodeProperty(const ValueTree& nodeTree, const Identifier& id)
{
	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	return propTree.getChildWithProperty(PropertyIds::ID, id.toString()).isValid();
}

juce::var ValueTreeIterator::getNodeProperty(const ValueTree& nodeTree, const Identifier& id)
{
	if (!hasNodeProperty(nodeTree, id))
		return {};

	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	return propTree.getChildWithProperty(PropertyIds::ID, id.toString())[PropertyIds::Value];
}

bool ValueTreeIterator::isComplexDataNode(const ValueTree& nodeTree)
{
	auto cTree = nodeTree.getChildWithName(PropertyIds::ComplexData);

	if (cTree.isValid())
	{
		int numData = 0;

		numData += getNumDataTypes(nodeTree, ExternalData::DataType::Table);
		numData += getNumDataTypes(nodeTree, ExternalData::DataType::SliderPack);
		numData += getNumDataTypes(nodeTree, ExternalData::DataType::AudioFile);
		
		// Only count the external slots for filters
		numData += getMaxDataTypeIndex(nodeTree, ExternalData::DataType::FilterCoefficients);

		// Count the embedded display buffers (they will be handled by the ComplexDataParser later).
		numData += getNumDataTypes(nodeTree, ExternalData::DataType::DisplayBuffer);

		return numData != 0;
	}

	return false;
}

int ValueTreeIterator::getNumDataTypes(const ValueTree& nodeTree, ExternalData::DataType t)
{
	auto cTree = nodeTree.getChildWithName(PropertyIds::ComplexData);
	return cTree.getChildWithName(ExternalData::getDataTypeName(t, true)).getNumChildren();
}

int ValueTreeIterator::getMaxDataTypeIndex(const ValueTree& rootTree, ExternalData::DataType t)
{
	int max = -1;

	forEach(rootTree, [t, &max](ValueTree& v)
	{
		if (v.getType() == PropertyIds::Node)
		{
			auto numSlots = getNumDataTypes(v, t);

			for (int i = 0; i < numSlots; i++)
			{
				max = jmax(max, getDataIndex(v, t, i));
			};
		}

		return false;
	});

	return max+1;
}

int ValueTreeIterator::getDataIndex(const ValueTree& nodeTree, ExternalData::DataType t, int slotIndex)
{
	auto cTree = nodeTree.getChildWithName(PropertyIds::ComplexData);
	auto dTree = cTree.getChildWithName(ExternalData::getDataTypeName(t, true));
	auto sTree = dTree.getChild(slotIndex);
	auto idx = (int)sTree[PropertyIds::Index];
	return idx;
}


bool ValueTreeIterator::isAutomated(const ValueTree& parameterTree)
{
	auto root = parameterTree.getRoot();

	auto pId = Identifier(parameterTree[PropertyIds::ID].toString());
	auto nodeId = Identifier(findParentWithType(parameterTree, PropertyIds::Node)[PropertyIds::ID].toString());

	return forEach(root, [pId, nodeId](ValueTree& v)
	{
		if (v.getType() == PropertyIds::Connection || v.getType() == PropertyIds::ModulationTarget)
		{
			auto thisP = Identifier(v[PropertyIds::ParameterId].toString());
			auto thisNode = Identifier(v[PropertyIds::NodeId].toString());

			if (thisP == pId && thisNode == nodeId)
				return true;
		}

		return false;
	});
}

bool ValueTreeIterator::isControlNode(const ValueTree& nodeTree)
{
	return CustomNodeProperties::nodeHasProperty(nodeTree, PropertyIds::IsControlNode);
}

String ValueTreeIterator::getSnexCode(const ValueTree& nodeTree)
{
	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	auto c = propTree.getChildWithProperty(PropertyIds::ID, PropertyIds::ClassId.toString());
	return c[PropertyIds::Value].toString();
}

snex::NamespacedIdentifier ValueTreeIterator::getNodeFactoryPath(const ValueTree& n)
{
	auto s = n[scriptnode::PropertyIds::FactoryPath].toString().replace(".", "::");

	auto fId = NamespacedIdentifier::fromString(s);

	if (FactoryIds::isContainer(fId))
	{
		auto isSplit = fId.id.toString() == "split";
		auto isMulti = fId.id.toString() == "multi";
		auto isClone = fId.id.toString() == "clone";

		if (!isSplit && !isMulti && !isClone)
		{
			return NamespacedIdentifier::fromString("container::chain");
		}
	}

	return NamespacedIdentifier::fromString(s);
}

juce::ValueTree ValueTreeIterator::getTargetParameterTree(const ValueTree& connectionTree)
{
	jassert(connectionTree.getType() == PropertyIds::Connection ||
		    connectionTree.getType() == PropertyIds::ModulationTarget);

	auto nodeId = connectionTree[PropertyIds::NodeId].toString();
	auto pId = connectionTree[PropertyIds::ParameterId].toString();

	jassert(!nodeId.isEmpty());
	jassert(!pId.isEmpty());

	ValueTree ptr;

	forEach(getRoot(connectionTree), [&](ValueTree& v)
	{
		if (v.getType() == PropertyIds::Parameter)
		{
			if (v[PropertyIds::ID].toString() == pId)
			{
				auto nodeTree = v.getParent().getParent();
				jassert(nodeTree.getType() == PropertyIds::Node);

				if (nodeTree[PropertyIds::ID].toString() == nodeId)
				{
					ptr = v;
					return true;
				}
			}
		}

		return false;
	});

	return ptr;
}

int ValueTreeIterator::calculateChannelCount(const ValueTree& nodeTree, int numCurrentChannels)
{
	auto realPath = nodeTree[PropertyIds::FactoryPath].toString().fromFirstOccurrenceOf("container.", false, false);

	if (realPath.startsWith("multi"))
	{
		int numChildren = nodeTree.getChildWithName(PropertyIds::Nodes).getNumChildren();
		numCurrentChannels /= jmax(1, numChildren);
	}

	if (realPath.startsWith("modchain"))
		numCurrentChannels = 1;

	return numCurrentChannels;
}

bool ValueTreeIterator::hasChildNodeWithProperty(const ValueTree& nodeTree, Identifier propId)
{
	return forEach(nodeTree, [propId](ValueTree& v)
	{
		if (v.getType() != PropertyIds::Node)
			return false;

		return CustomNodeProperties::nodeHasProperty(v, propId);
	}, IterationType::ChildrenFirst);
}

snex::cppgen::Node::Ptr ValueTreeBuilder::RootContainerBuilder::parse()
{
	parent.addNumVoicesTemplate(root);

    root->scopedId = root->scopedId.getParent().getChildId(root->scopedId.getIdentifier().toString() + "_");
    root->flushIfNot();
    parent.addEmptyLine();


	nodeClassId = Identifier(parent.getGlueCode(FormatGlueCode::MainInstanceClass));
	parent.addComment("Root node initialiser class", Base::CommentType::FillTo80);

	auto base = UsingTemplate(parent, "unused", root->scopedId);

	auto hasPolyData = root->isPolyphonicOrHasPolyphonicTemplate() || root->templateArguments[0].argumentId.toString() == "NV";

	if (hasPolyData)
		base.addTemplateIntegerArgument("NV", true);

	Array<DefinitionBase*> baseClasses = { &base };

	auto routing_public_mod = UsingTemplate(parent, "unused", NamespacedIdentifier("routing::public_mod_target"));

	auto hasPublicMod = root->hasProperty(PropertyIds::IsPublicMod, true);

	if (hasPublicMod)
		baseClasses.add(&routing_public_mod);

	Struct s(parent, nodeClassId, baseClasses, root->templateArguments);

	addMetadata();

	{
		String f;
		f << nodeClassId << "()";
		parent << f;

		{
			StatementBlock sb(parent);

			parent.addComment("Node References", Base::CommentType::FillTo80Light);
			
			createStackVariablesForChildNodes();

			addParameterConnections();
			addModulationConnections();
			addSendConnections();

			if (hasPublicMod)
			{
				parent.addEmptyLine();
				parent.addComment("Public Mod Connection", Base::CommentType::FillTo80Light);

				for (auto& sv : stackVariables)
				{
					if (CustomNodeProperties::nodeHasProperty(sv->nodeTree, PropertyIds::IsPublicMod))
					{
						String def;
						def << sv->toExpression() << ".connect(*this);";
						parent << def;
					}
				}

				parent.addEmptyLine();
			}

			addDefaultParameters();
		}

		if (hasPolyData)
		{
			parent.addEmptyLine();
			parent << "static constexpr bool isPolyphonic() { return NV > 1; };";
		}

		if (root->hasProperty(PropertyIds::IsProcessingHiseEvent))
		{
			parent.addEmptyLine();
			parent << "static constexpr bool isProcessingHiseEvent() { return true; };";
		}

		{
			auto hasTail = parent.v.getParent().getProperty(PropertyIds::HasTail, true);
			parent.addEmptyLine();

			String def;
			def << "static constexpr bool hasTail() { return " << (hasTail ? "true" : "false") << "; };";
			parent << def;
		}

		{
			auto isSuspendedOnSilence = parent.v.getParent().getProperty(PropertyIds::SuspendOnSilence, false);
			parent.addEmptyLine();

			String def;
			def << "static constexpr bool isSuspendedOnSilence() { return " << (isSuspendedOnSilence ? "true" : "false") << "; };";
			parent << def;
		}

		if (hasComplexTypes())
		{

			parent.addEmptyLine();

			parent << "void setExternalData(const ExternalData& b, int index)";

			stackVariables.clear();

			StatementBlock sb(parent);

			parent.addComment("External Data Connections", Base::CommentType::FillTo80Light);

			ValueTreeIterator::forEach(root->nodeTree.getChildWithName(PropertyIds::Nodes), [&](ValueTree& childNode)
			{
				if (childNode.getType() == PropertyIds::Node)
				{
					if (CloneHelpers::getCloneIndex(childNode) > 0)
						return false;

					if (ValueTreeIterator::isComplexDataNode(childNode))
					{
						auto sv = getChildNodeAsStackVariable(childNode);
						

						String def;

						def << sv->toExpression() << ".setExternalData(b, index);";

						parent << def;

						auto type = parent.getNode(childNode, false);
						parent.addComment(type->toExpression(), Base::CommentType::AlignOnSameLine);
					}
				}

				return false;
			});

		}
	}

	

	s.flushIfNot();

	auto n = parent.createNode(root->nodeTree, nodeClassId, s.scopedId.toString());

	if (hasPolyData)
		n->addTemplateIntegerArgument("NV", true);

	return n;
}



void ValueTreeBuilder::RootContainerBuilder::addDefaultParameters()
{
	parent.addComment("Default Values", Base::CommentType::FillTo80Light);

	for (auto sv : stackVariables)
	{
		auto child = sv->nodeTree;

		auto pTree = child.getChildWithName(PropertyIds::Parameters);

		for (auto p : pTree)
		{
			String comment;

			auto np = NamespacedIdentifier(child[PropertyIds::ID].toString());

			if (ValueTreeIterator::isAutomated(p))
			{
				parent << ";";
				comment << "" << np.getChildId(p[PropertyIds::ID].toString()).toString();
				comment << " is automated";
				parent.addComment(comment, Base::CommentType::AlignOnSameLine);
				continue;
			}

			if (CustomNodeProperties::nodeHasProperty(sv->nodeTree, PropertyIds::IsCloneCableNode))
			{
				// We don't want the NumClones property to be set as default value
				// (if it's connected properly, it will automatically resize and this
				// call would reset it otherwise.
				if (p[PropertyIds::ID] == PropertyIds::NumClones.toString())
				{
					parent << ";";
					comment << "" << np.getChildId(p[PropertyIds::ID].toString()).toString();
					comment << " is deactivated";
					parent.addComment(comment, Base::CommentType::AlignOnSameLine);
					continue;
				}
			}

			String v;
            
            v << sv->toExpression();
            v << ".setParameterT(";
            v << pTree.indexOf(p) << ", ";
			v << Types::Helpers::getCppValueString(p[PropertyIds::Value], Types::ID::Double);
			v << ");";
			parent << v;

			if(!FactoryIds::isContainer(np))
				np = parent.getNodePath(child);

			comment << np.getChildId(p[PropertyIds::ID].toString()).toString();

			parent.addComment(comment, Base::CommentType::AlignOnSameLine);
		}

		parent.addEmptyLine();
	}

	for (auto p : root->nodeTree.getChildWithName(PropertyIds::Parameters))
	{
		String l;
		l << "this->setParameterT(" << ValueTreeIterator::getIndexInParent(p) << ", ";
		l << Helpers::getCppValueString(p[PropertyIds::Value], Types::ID::Double) << ");";
		parent << l;
	}

	if (outputFormat == Format::CppDynamicLibrary && hasComplexTypes())
	{
		parent << "this->setExternalData({}, -1);";
	}
}

PooledStackVariable::Ptr ValueTreeBuilder::RootContainerBuilder::getChildNodeAsStackVariable(const ValueTree& v)
{
	Array<int> path;

	auto id = v[PropertyIds::ID].toString();

	ValueTreeIterator::getNodePath(path, root->nodeTree, id);

	String tid = id;

	PooledStackVariable::Ptr e = new PooledStackVariable(parent, v);
	
	int index = 0;

	*e << "this->";

	for (auto i : path)
	{
		*e << "getT(" << i << ")";

		if (++index != path.size())
			*e << JitTokens::dot;
	}

	if(path.size() > 7)
		e->addBreaksBeforeDots(path.size() / 2);

	if (auto existing = stackVariables.getExisting(*e))
		return existing;

	return e;
}

snex::cppgen::PooledStackVariable::Ptr ValueTreeBuilder::RootContainerBuilder::getChildNodeAsStackVariable(const String& nodeId)
{
	for (auto n : stackVariables)
	{
		if (n->nodeTree[PropertyIds::ID].toString() == nodeId)
			return n;
	}

	return nullptr;
}




void ValueTreeBuilder::RootContainerBuilder::addMetadata()
{
	Struct m(parent, "metadata", {}, {});

	ExternalData::forEachType([&](ExternalData::DataType t)
	{
		String def;
		def << "static const int " << ExternalData::getNumIdentifier(t) << " = ";
		def << ValueTreeIterator::getMaxDataTypeIndex(root->nodeTree, t) << ";";
		parent << def;
	});

	parent.addEmptyLine();

	Macro(parent, "SNEX_METADATA_ID", { root->nodeTree[PropertyIds::ID].toString() });
	Macro(parent, "SNEX_METADATA_NUM_CHANNELS", { String(parent.numChannelsToCompile) });

	auto pCopy = root->nodeTree.getChildWithName(PropertyIds::Parameters).createCopy();

	scriptnode::parameter::encoder encoder(pCopy);

	cppgen::EncodedParameterMacro(parent, encoder);
	m.flushIfNot();
	parent.addEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::addParameterConnections()
{
	auto pList = getContainersWithParameter();

	if (!pList.isEmpty())
	{
		parent.addComment("Parameter Connections", Base::CommentType::FillTo80Light);

		for (auto containerWithParameter : pList)
		{
			for (auto p : containerWithParameter->nodeTree.getChildWithName(PropertyIds::Parameters))
			{
				String def;

				PooledStackVariable::Ptr c = getChildNodeAsStackVariable(containerWithParameter->nodeTree);
				
				auto pId = p[PropertyIds::ID].toString();
				pId << "_p";

				StackVariable pv(parent, pId, TypeInfo(Types::ID::Dynamic, false, true));

				if (containerWithParameter->isRootNode())
				{
					pv << "this->";
				}
				else
				{
					pv << *c;
					pv << ".";
				}

				pv << "getParameterT(" << ValueTreeIterator::getIndexInParent(p) << ")";

				auto cTree = p.getChildWithName(PropertyIds::Connections);

				if (cTree.getNumChildren() > 1)
					pv.flushIfNot();

				for (auto c_ : cTree)
				{
					auto targetId = Identifier(c_[PropertyIds::NodeId].toString());

					String def;
					
					auto cIndex = ValueTreeIterator::getIndexInParent(c_);

					def << pv.toExpression() << ".connectT(" << cIndex << ", ";

					bool found = false;

					for (auto nv : stackVariables)
					{
						if (nv->scopedId.getIdentifier() == targetId)
						{
							def << nv->toExpression();

							found = true;
							break;
						}
					}

					if (!found)
					{
						jassertfalse;

						auto e = getChildNodeAsStackVariable(c_);
						def << e->toExpression();
					}

					def << ");";

					parent << def;

					String comment;

					comment << p[PropertyIds::ID].toString() << " -> " << targetId << "::" << c_[PropertyIds::ParameterId].toString();

					parent.addComment(comment, Base::CommentType::AlignOnSameLine);
				}

				if (!ValueTreeIterator::isLast(p))
					parent.addEmptyLine();
			}

			
		}
	}

	parent.addIfNotEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::addModulationConnections()
{
	auto modList = getModulationNodes();

	if (modList.size() > 0)
	{
		parent.addEmptyLine();
		parent.addComment("Modulation Connections", Base::CommentType::FillTo80Light);

		for (auto m : modList)
		{
			Array<int> path;

			auto mtargetTree = m->nodeTree.getChildWithName(PropertyIds::ModulationTargets);
			
			for (auto t : mtargetTree)
				addModConnection(t, m);

			auto stargetTree = m->nodeTree.getChildWithName(PropertyIds::SwitchTargets);

			if (stargetTree.getNumChildren() > 0)
			{
				auto mId = getChildNodeAsStackVariable(m->nodeTree)->toExpression();

				StackVariable sv(parent, mId + "_p", TypeInfo(Types::ID::Dynamic, false, true));

				sv << mId << ".getWrappedObject().getParameter()";

				sv.flushIfNot();

				int pIndex = 0;

				for (auto st : stargetTree)
				{
					for (auto c : st.getChildWithName(PropertyIds::Connections))
					{
						addModConnection(c, m, pIndex);
					}

					pIndex++;
				}
			}

			
				
		}
	}

	parent.addEmptyLine();
}

void ValueTreeBuilder::RootContainerBuilder::addModConnection(ValueTree& t, Node::Ptr m, int pIndex)
{
	auto sourceId = m->nodeTree[PropertyIds::ID].toString();
	auto ms = getChildNodeAsStackVariable(m->nodeTree);

	auto cn = parent.getConnection(t);

	if (cn.n == nullptr || !cn.n->nodeTree.isValid())
	{
		Error e;
		if (cn.n == nullptr)
			e.errorMessage = "No node";
		else
			e.errorMessage = "No ValueTree for node " + cn.n->toString();

		throw e;
	}

	String def;

	auto needsModWrapper = ValueTreeIterator::needsModulationWrapper(m->nodeTree);

	def << ms->toExpression();

	if (pIndex != -1)
	{
		def << "_p.getParameterT";
		
		def << "(" << pIndex << ")";
	}
	else
	{
		if (!needsModWrapper)
			def << ".getWrappedObject()";

		def << ".getParameter()";
	}

	def << ".connectT(" << ValueTreeIterator::getIndexInParent(t) << ", ";
	auto targetId = cn.n->nodeTree[PropertyIds::ID].toString();
	auto mt = getChildNodeAsStackVariable(cn.n->nodeTree);

	if (mt->getLength() > 20)
	{
		mt->flushIfNot();
		//parent.addComment(targetId, Base::CommentType::AlignOnSameLine);
	}

	def << mt->toExpression();
	def << ");";

	parent << def;

	String comment;
	comment << sourceId << " -> " << targetId << "::" << t[PropertyIds::ParameterId].toString();

	parent.addComment(comment, CommentType::AlignOnSameLine);
}

void ValueTreeBuilder::RootContainerBuilder::addSendConnections()
{
	auto sendList = getSendNodes();

	if (sendList.isEmpty())
		return;

	parent.addComment("Send Connections", Base::CommentType::FillTo80Light);

	for (auto s : sendList)
	{
		auto propTree = s->nodeTree.getChildWithName(PropertyIds::Properties);
		auto c = propTree.getChildWithProperty(PropertyIds::ID, PropertyIds::Connection.toString());
		auto ids = c[PropertyIds::Value].toString();

		auto source = getChildNodeAsStackVariable(s->nodeTree);

		for (auto& id : StringArray::fromTokens(ids, ";", ""))
		{
			auto target = getChildNodeAsStackVariable(id);

			if (target == nullptr)
			{
				Error e;
				e.v = s->nodeTree;
				e.errorMessage << id << " not found";
				throw e;
			}

			String def;

			def << source->toExpression() << ".connect(" << target->toExpression() << ");";
			parent << def;
		}

	}

	parent.addEmptyLine();
}



void ValueTreeBuilder::RootContainerBuilder::createStackVariablesForChildNodes()
{
	ValueTreeIterator::forEach(root->nodeTree.getChildWithName(PropertyIds::Nodes), [&](ValueTree& childNode)
	{
		if (childNode.getType() == PropertyIds::Node)
		{
            auto cloneIndex = CloneHelpers::getCloneIndex(childNode);
            
            if(cloneIndex > 0)
                return false;
            
			auto sv = getChildNodeAsStackVariable(childNode);
			
			sv->flushIfNot();

			auto type = parent.getNode(childNode, false);
			parent.addComment(type->toExpression(), Base::CommentType::AlignOnSameLine);
			
			stackVariables.add(sv);
		}

		return false;
	});

	parent.addIfNotEmptyLine();
}

int ValueTreeBuilder::RootContainerBuilder::getNumParametersToInitialise(ValueTree& child)
{
	int numParameters = 0;

	for (auto p : child.getChildWithName(PropertyIds::Parameters))
	{
		if (!ValueTreeIterator::isAutomated(p))
			numParameters++;
	}

	return numParameters;
}

Node::List ValueTreeBuilder::RootContainerBuilder::getSendNodes()
{
	Node::List l;

	for (auto n : parent.pooledTypeDefinitions)
	{
		if (getNodePath(n->nodeTree).toString() == "routing::send")
		{
			l.add(n);
		}
	}

	return l;
}

snex::cppgen::Node::List ValueTreeBuilder::RootContainerBuilder::getModulationNodes()
{
	Node::List l;

	for (auto u : parent.pooledTypeDefinitions)
	{
		if (u->nodeTree.getChildWithName(PropertyIds::ModulationTargets).getNumChildren() > 0)
			l.addIfNotAlreadyThere(u);

		if (u->nodeTree.getChildWithName(PropertyIds::SwitchTargets).getNumChildren() > 0)
			l.addIfNotAlreadyThere(u);
	}

	return l;
}

snex::cppgen::Node::List ValueTreeBuilder::RootContainerBuilder::getContainersWithParameter()
{
	Node::List l;

	for (auto u : parent.pooledTypeDefinitions)
	{
		if (FactoryIds::isContainer(parent.getNodePath(u->nodeTree)))
		{
			if (ValueTreeIterator::hasRealParameters(u->nodeTree))
				l.add(u);
		}
	}

	if (ValueTreeIterator::hasRealParameters(root->nodeTree))
		l.addIfNotAlreadyThere(root);

	return l;
}

bool ValueTreeBuilder::RootContainerBuilder::hasComplexTypes() const
{
	return ValueTreeIterator::forEach(root->nodeTree, [](ValueTree& v)
	{
		if(ValueTreeIterator::isComplexDataNode(v))
			return true;

		return false;
	});
}

PooledStackVariable::PooledStackVariable(Base& p, const ValueTree& nodeTree_) :
	StackVariable(p, nodeTree_[PropertyIds::ID].toString(), TypeInfo(Types::ID::Dynamic, false, CloneHelpers::getCloneIndex(nodeTree_) == -1)),
	nodeTree(nodeTree_)
{

}

ValueTreeBuilder::ComplexDataBuilder::ComplexDataBuilder(ValueTreeBuilder& parent_, Node::Ptr nodeToWrap):
	n(nodeToWrap),
	parent(parent_)
{

}

Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parse()
{
	jassert(ValueTreeIterator::isComplexDataNode(n->nodeTree));

	auto numTables = ValueTreeIterator::getNumDataTypes(n->nodeTree, ExternalData::DataType::Table);
	auto numSliderPacks = ValueTreeIterator::getNumDataTypes(n->nodeTree, ExternalData::DataType::SliderPack);
	auto numAudioFiles = ValueTreeIterator::getNumDataTypes(n->nodeTree, ExternalData::DataType::AudioFile);
	auto numFilters = ValueTreeIterator::getMaxDataTypeIndex(n->nodeTree, ExternalData::DataType::FilterCoefficients);
	auto numDisplayBuffers = ValueTreeIterator::getNumDataTypes(n->nodeTree, ExternalData::DataType::DisplayBuffer);

	bool isSingleData = (numTables + numSliderPacks + numAudioFiles + numFilters + numDisplayBuffers) == 1;

	if (isSingleData)
	{
		auto t = numTables == 1 ?	   ExternalData::DataType::Table :
				 numSliderPacks == 1 ? ExternalData::DataType::SliderPack :
				 numAudioFiles == 1 ?  ExternalData::DataType::AudioFile :
				 numFilters == 1	?  ExternalData::DataType::FilterCoefficients:
									   ExternalData::DataType::DisplayBuffer;

		auto idx = ValueTreeIterator::getDataIndex(n->nodeTree, t, 0);

		

		if (idx == -1)
		{
			return parseEmbeddedDataNode(t);
		}
		else
		{
			return parseExternalDataNode(t, idx);
		}
	}
	else
	{
		return parseMatrixDataNode();
	}
}

Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parseSingleDisplayBufferNode(bool enableBuffer)
{
	if (!CustomNodeProperties::nodeHasProperty(n->nodeTree, PropertyIds::UseRingBuffer))
		return n;

	*n << (enableBuffer ? "true" : "false");

	return n;
}



Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parseEmbeddedDataNode(ExternalData::DataType t)
{
	if (t == ExternalData::DataType::DisplayBuffer)
	{
		auto n = parseSingleDisplayBufferNode(false);

		Node::Ptr wn = new Node(parent, n->scopedId.id, NamespacedIdentifier("wrap::no_data"));
		wn->nodeTree = n->nodeTree;
		*wn << *n;

		if(flushNodeBeforeReturning)
			wn->flushIfNot();

		return wn;
	}

	Node::Ptr wn = new Node(parent, n->scopedId.id, NamespacedIdentifier("wrap::data"));
	wn->nodeTree = n->nodeTree;
	NamespacedIdentifier edId = NamespacedIdentifier::fromString("data::embedded");
	edId = edId.getChildId(ExternalData::getDataTypeName(t).toLowerCase());
	UsingTemplate ed(parent, "unused", edId);
	auto sId = n->scopedId;
	StringHelpers::addSuffix(sId, "_data");

	if (t == ExternalData::DataType::AudioFile)
	{
		auto cTree = n->nodeTree.getChildWithName(PropertyIds::ComplexData);
		auto dTree = cTree.getChildWithName(ExternalData::getDataTypeName(t, true));
		auto sTree = dTree.getChild(0);
		auto base64 = sTree[PropertyIds::EmbeddedData].toString();

		if (auto ref = parent.loadAudioFile(base64))
		{
			Array<NamespacedIdentifier> baseClasses;
			baseClasses.add(NamespacedIdentifier::fromString("data::embedded::multichannel_data"));

			Struct s(parent, sId.getIdentifier(), baseClasses, {}, true);

			String l1, l2, l3;

			auto embedDirectly = false;// ref->buffer.getNumSamples() < 8000;

			l1 << "int    getNumSamples()     const override { return " << ref->buffer.getNumSamples() << "; }";
			l2 << "double getSamplerate()     const override { return " << Helpers::getCppValueString(ref->sampleRate) << "; }";
			l3 << "int    getNumChannels()    const override { return " << ref->buffer.getNumChannels() << "; }";

			parent.addComment("Metadata functions for embedded data", Base::CommentType::FillTo80Light);
			parent << l1;
			parent << l2;
			parent << l3;



			parent << "const float* getChannelData(int index) override";
			
			{
				StatementBlock sb(parent, true);

				for (int i = 0; i < ref->buffer.getNumChannels(); i++)
				{
					String l;

					String variableName;

					if (embedDirectly)
						variableName << "data" << i << ".begin()";
					else
						variableName << "audiodata::" << sId.toString().replace("::", "_") << i;

					l << "if(index == " << i << ") { return reinterpret_cast<const float*>(" << variableName << "); }";
					parent << l;
				}

				parent << "jassertfalse;    return nullptr;";
			}
			

			if (embedDirectly)
			{
				parent.addEmptyLine();
				parent.addComment("Zero-padded 64bit encoded data:", Base::CommentType::FillTo80Light);

				for (int i = 0; i < ref->buffer.getNumChannels(); i++)
				{
					parent.addEmptyLine();

					if (embedDirectly)
						IntegerArray<uint32, float>(parent, "data" + String(i), ref->buffer.getReadPointer(0), ref->buffer.getNumSamples());
				}
			}
			else
				parent.addExternalSample(sId.toString().replace("::", "_"), ref);

			s.flushIfNot();

			ed << s.toExpression();
		}
		else
		{
			Error e;
			e.v = n->nodeTree;
			e.errorMessage << "Error at embedding audio file: " << base64 << " not found";
			throw e;
		}
	}
	else
	{
		Struct s(parent, sId.getIdentifier(), {}, {});
		FloatArray(parent, "data", getEmbeddedData(n->nodeTree, t, 0));
		s.flushIfNot();

		ed << s.toExpression();
	}

	parent.addEmptyLine();

	*wn << *n;
	*wn << ed;

	parent.addNumVoicesTemplate(wn);

	if(flushNodeBeforeReturning)
		wn->flushIfNot();

	return wn;
}

Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parseExternalDataNode(ExternalData::DataType t, int slotIndex)
{
	// wrap::data<NodeType, data::external::table<idx>>;

	if (t == ExternalData::DataType::DisplayBuffer)
		n = parseSingleDisplayBufferNode(true);

    parent.checkUnflushed(n);
    
    
    
	Node::Ptr wn = new Node(parent, n->scopedId.id, NamespacedIdentifier("wrap::data"));
    
	wn->nodeTree = n->nodeTree;
	NamespacedIdentifier edId = NamespacedIdentifier::fromString("data::external");
	edId = edId.getChildId(ExternalData::getDataTypeName(t).toLowerCase());
	UsingTemplate ed(parent, "unused", edId);
	ed << String(slotIndex);
	*wn << *n;
	*wn << ed;

	parent.addNumVoicesTemplate(wn);

	if(flushNodeBeforeReturning)
		wn->flushIfNot();

	return wn;
}

bool needsMatrix(const ValueTree& nodeTree)
{
	int numThisTime = -1;
	ExternalData::DataType dt = ExternalData::DataType::numDataTypes;
	bool multipleDataTypes = false;

	// Check if multiple data types are defined
	ExternalData::forEachType([&](ExternalData::DataType t)
	{
		int num = ValueTreeIterator::getNumDataTypes(nodeTree, t);

		if (num == 0)
			return;

		if (numThisTime == -1)
		{
			numThisTime = num;
			dt = t;
		}
			
		else if ( numThisTime != num)
			multipleDataTypes = true;
	});

	if (multipleDataTypes)
		return true;

	for (int i = 0; i < numThisTime; i++)
	{
		auto slotIndex = ValueTreeIterator::getDataIndex(nodeTree, dt, i);

		if (i != slotIndex)
			return true;
	}

	return false;
}

Node::Ptr ValueTreeBuilder::ComplexDataBuilder::parseMatrixDataNode()
{
	if (ValueTreeIterator::getMaxDataTypeIndex(n->nodeTree, ExternalData::DataType::DisplayBuffer) > 0)
		n = parseSingleDisplayBufferNode(true);

	if (!needsMatrix(n->nodeTree))
		return n;

    parent.checkUnflushed(n);
    
	Node::Ptr wn = new Node(parent, n->scopedId.id, NamespacedIdentifier("wrap::data"));
	wn->nodeTree = n->nodeTree;
	NamespacedIdentifier edId = NamespacedIdentifier::fromString("data::matrix");
	
	UsingTemplate ed(parent, "unused", edId);

	int numMax = 0;

	ExternalData::forEachType([&](ExternalData::DataType t)
	{
		numMax = jmax(numMax, ValueTreeIterator::getNumDataTypes(n->nodeTree, t));
	});


	auto sId = n->scopedId;
	StringHelpers::addSuffix(sId, "_matrix");

	Struct s(parent, sId.getIdentifier(), {}, {});

	ExternalData::forEachType([&](ExternalData::DataType t)
	{
		String l;
		auto num = ValueTreeIterator::getNumDataTypes(n->nodeTree, t);
		l << "static const int " << ExternalData::getNumIdentifier(t) << " = " << num << ";";
		parent << l;
	});

	String l5;
	l5 << "const int matrix[3][" << numMax << "] =";

	parent.addEmptyLine();
	parent.addComment("Index mapping matrix", Base::CommentType::FillTo80Light);
	parent << l5;

	int embeddedCounter = 0;

	{
		StatementBlock m(parent, true);
		String r[5], c[5];

		auto getCell = [&](ExternalData::DataType t, int column)
		{
			auto numSlots = ValueTreeIterator::getNumDataTypes(n->nodeTree, t);

			if (column >= numSlots)
				return -1;

			auto slotIndex = ValueTreeIterator::getDataIndex(n->nodeTree, t, column);

			if (slotIndex == -1)
				return 1000 + embeddedCounter++;

			return slotIndex;
		};

		auto formatRow = [](String& s)
		{
			s = s.upToLastOccurrenceOf(", ", false, false);
			String f;
			f << "{ " << s << " }";
			s = f;
		};

		ExternalData::forEachType([&](ExternalData::DataType t)
		{
			for (int i = 0; i < numMax; i++)
			{
				auto cellIdx = getCell(t, i);


				r[(int)t] << String(cellIdx) << ", ";

				if (cellIdx >= 0)
				{
					String x;
					if (cellIdx >= 1000)
						x << "e[" << String(cellIdx - 1000) << "]";
					else
						x << cellIdx;

					c[(int)t] << " | " << String(i) << "->" << x;
				}
			}

			formatRow(r[(int)t]);
		});

		parent << r[0] + ", "; parent.addComment(c[0], Base::CommentType::AlignOnSameLine);
		parent << r[1] + ", "; parent.addComment(c[1], Base::CommentType::AlignOnSameLine);
		parent << r[2]; parent.addComment(c[2], Base::CommentType::AlignOnSameLine);
	}

	if (embeddedCounter > 0)
	{
		parent.addEmptyLine();
		parent << "private:";
		parent.addEmptyLine();

		int newCounter = 0;

		StringArray ids;

		ExternalData::forEachType([&](ExternalData::DataType t)
		{
			for (int i = 0; i < ValueTreeIterator::getNumDataTypes(n->nodeTree, t); i++)
			{
				auto b = getEmbeddedData(n->nodeTree, t, i);

				if (!b.isEmpty())
				{
					String newId;
					newId << "d" << newCounter++;
					ids.add(newId);
					FloatArray(parent, newId, b);
					parent.addEmptyLine();
				}
			}

		});

		parent << "public:";
		parent.addEmptyLine();

		String def;

		def << "const span<dyn<float>, " << newCounter << "> embeddedData = { ";
		
		for (auto id : ids)
			def << id << ", ";

		def = def.upToLastOccurrenceOf(", ", false, false);

		def << " };";

		parent << def;
	}

	s.flushIfNot();

	ed << s.toExpression();

	parent.addEmptyLine();

	*wn << *n;
	*wn << ed;

	parent.addNumVoicesTemplate(wn);

	if(flushNodeBeforeReturning)
		wn->flushIfNot();

	return wn;
}

Array<float> ValueTreeBuilder::ComplexDataBuilder::getEmbeddedData(const ValueTree& nodeTree, ExternalData::DataType t, int slotIndex)
{
	if (ValueTreeIterator::getDataIndex(nodeTree, t, slotIndex) != -1)
		return {};

	jassert(t != ExternalData::DataType::FilterCoefficients);

	auto cTree = nodeTree.getChildWithName(PropertyIds::ComplexData);
	auto dTree = cTree.getChildWithName(ExternalData::getDataTypeName(t, true));
	auto sTree = dTree.getChild(slotIndex);
	auto base64 = sTree[PropertyIds::EmbeddedData].toString();

	Array<float> data;

	if (t == ExternalData::DataType::Table)
	{
		SampleLookupTable s;
		s.fromBase64String(base64);
		data.addArray(s.getReadPointer(), SAMPLE_LOOKUP_TABLE_SIZE);
	}
	if (t == ExternalData::DataType::SliderPack)
	{
		SliderPackData d;
		d.fromBase64String(base64);
		data.addArray(d.getCachedData(), d.getNumSliders());
	}

	return data;
}



Node::Ptr ValueTreeBuilder::SnexNodeBuilder::parse()
{
	p = getNodePath(n->nodeTree);
	classId = ValueTreeIterator::getSnexCode(n->nodeTree);

	if (classId.isEmpty())
	{
		Error e;
		e.v = n->nodeTree;
		e.errorMessage = "No class specified";
		throw e;
	}

	if (!parent.definedSnexClasses.contains(classId))
	{
		// You have to set a code provider
		jassert(parent.codeProvider != nullptr);
		code = parent.codeProvider->getCode(getNodePath(n->nodeTree), classId);

		parent << code;
		parent.addEmptyLine();
		parent.definedSnexClasses.add(classId);
	}

	if (needsWrapper(p))
		return parseWrappedSnexNode();
	else
		return parseUnwrappedSnexNode();
}

Node::Ptr ValueTreeBuilder::SnexNodeBuilder::parseWrappedSnexNode()
{
	Node::Ptr wn = new Node(parent, n->scopedId.id, p);
	wn->nodeTree = n->nodeTree;
		
	if (CustomNodeProperties::nodeHasProperty(wn->nodeTree, PropertyIds::IsPolyphonic))
		wn->addTemplateIntegerArgument("NV", true);

	UsingTemplate ud(parent, "unused", NamespacedIdentifier(classId));

	if (wn->hasProperty(PropertyIds::TemplateArgumentIsPolyphonic))
		ud.addTemplateIntegerArgument("NV", true);

	*wn << ud;

	// Avoid appending other template types...
	wn->setReadOnly();

	return wn;
}

Node::Ptr ValueTreeBuilder::SnexNodeBuilder::parseUnwrappedSnexNode()
{
	Node::Ptr wn = new Node(parent, n->scopedId.id, NamespacedIdentifier(classId));
	wn->nodeTree = n->nodeTree;

	wn->addTemplateIntegerArgument("NV", true);

	return wn;
}

Node::Ptr ValueTreeBuilder::ExpressionNodeBuilder::parse()
{
	auto code = ValueTreeIterator::getNodeProperty(n->nodeTree, PropertyIds::Code).toString();
	auto id = n->nodeTree[PropertyIds::ID].toString();

	Namespace ns(parent, "custom", false);
	
	Struct s(parent, id, {}, {});

	auto sig = mathNode ? "static float op(float input, float value)" :
						   "static double op(double input)";

	parent << sig;

	{
		StatementBlock sb(parent);
		String l;
		l << "return " << code << ";";
		parent << l;
	}

	s.flushIfNot();
	ns.flushIfNot();

	*n << s.toExpression();

	return n;
}

}
}
