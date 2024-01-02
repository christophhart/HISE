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

#pragma once

#define BETTER_TEMPLATE_FORWARDING 1

#define ENABLE_CPP_DEBUG_LOG 0

namespace snex {
namespace cppgen {
using namespace juce;
using namespace scriptnode;

struct ValueTreeIterator: public valuetree::Helpers
{
	
	

	static bool fixCppIllegalCppKeyword(String& s);

	static bool needsModulationWrapper(ValueTree& v);

	static bool getNodePath(Array<int>& path, ValueTree& root, const Identifier& id);

	static bool hasRealParameters(const ValueTree& containerTree);

	static bool hasNodeProperty(const ValueTree& nodeTree, const Identifier& id);

	static var getNodeProperty(const ValueTree& nodeTree, const Identifier& id);

	static bool isComplexDataNode(const ValueTree& nodeTree);

    static bool isRuntimeTargetNode(const ValueTree& nodeTree);
    
	static int getNumDataTypes(const ValueTree& nodeTree, ExternalData::DataType t);

	static int getMaxDataTypeIndex(const ValueTree& rootTree, ExternalData::DataType t);

	static int getDataIndex(const ValueTree& nodeTree, ExternalData::DataType t, int slotIndex);

	static bool isAutomated(const ValueTree& parameterTree);

	static bool isControlNode(const ValueTree& nodeTree);
	
	static String getSnexCode(const ValueTree& nodeTree);

	static NamespacedIdentifier getNodeFactoryPath(const ValueTree& nodeTree);

	static ValueTree getTargetParameterTree(const ValueTree& connectionTree);

    static int getFixRuntimeHash(const ValueTree& nodeTree);
    
	static int calculateChannelCount(const ValueTree& nodeTree, int numCurrentChannels);

	static bool isContainerWithFixedParameters(const ValueTree& nodeTree);

	static bool hasChildNodeWithProperty(const ValueTree& nodeTree, Identifier propId);

	static bool isPolyphonicOrHasPolyphonicTemplate(const ValueTree& v)
	{
		return hasChildNodeWithProperty(v, PropertyIds::IsPolyphonic) || hasChildNodeWithProperty(v, PropertyIds::TemplateArgumentIsPolyphonic);
	}
};



struct Node : public ReferenceCountedObject,
			  public UsingTemplate
{
	using List = ReferenceCountedArray<Node>;
	using Ptr = ReferenceCountedObjectPtr<Node>;

	Node(Base& b, const Identifier& id, const NamespacedIdentifier& templateId) :
		UsingTemplate(b, id, templateId)
	{};

	bool operator==(const Node& other) const
	{
		// We never want the node comparison to return true
		return false;
	}

	bool operator==(const String& expression) const
	{
		auto e = getUsingExpression();
		auto same = e.compare(expression) == 0;
		return same;
	}

	bool isRootNode() const
	{
		if (nodeTree.getParent().getType() == PropertyIds::Network)
			return true;

		return ValueTreeIterator::getIndexInParent(nodeTree) == -1;
	}

	bool isPolyphonicOrHasPolyphonicTemplate() const
	{
		return hasProperty(PropertyIds::IsPolyphonic) || hasProperty(PropertyIds::TemplateArgumentIsPolyphonic);
	}

	bool hasProperty(const Identifier& propId, bool searchRecursive=true) const
	{
		if (searchRecursive)
			return ValueTreeIterator::hasChildNodeWithProperty(nodeTree, propId);
		else
			return CustomNodeProperties::nodeHasProperty(nodeTree, propId);
	}

	bool addOptionalModeTemplate()
	{
		if (readOnly || optionalModeWasAdded)
			return false;

		if (hasProperty(PropertyIds::HasModeTemplateArgument, false))
		{
			auto fId = NamespacedIdentifier(CustomNodeProperties::getModeNamespace(nodeTree));
			auto cId = ValueTreeIterator::getNodeProperty(nodeTree, PropertyIds::Mode).toString().toLowerCase().replaceCharacter(' ', '_');

            ValueTreeIterator::fixCppIllegalCppKeyword(cId);
            
            
			UsingTemplate ud(parent, "unused", fId.getChildId(cId));

			if (hasProperty(PropertyIds::TemplateArgumentIsPolyphonic))
				ud.addTemplateIntegerArgument("NV", true);

			*this << ud;// fId.getChildId(cId).toString();

			optionalModeWasAdded = true;

			return true;
		}

		return false;
	}

	void setReadOnly()
	{
		readOnly = true;
	}

	ValueTree nodeTree;

	bool readOnly = false;
	bool optionalModeWasAdded = false;
};


struct Connection
{
	enum class CableType
	{
		Parameter,
		Modulation,
		CloneCable,
		SwitchTargets,
		SwitchTarget,
		numCableTypes
	};

	enum class ConnectionType
	{
		Plain,
		Range,
		RangeWithSkew,
		RangeWithStep,
		Bypass,
		Converted,
		Expression,
		numConnectionTypes
	};

	operator bool() const
	{
		return n != nullptr;
	}

	ConnectionType getType() const
	{
		if (index == scriptnode::bypass::ParameterId)
			return ConnectionType::Bypass;

		if (expressionCode.isNotEmpty())
			return ConnectionType::Expression;

		if (RangeHelpers::isIdentity(targetRange))
			return ConnectionType::Plain;

		if (targetRange.rng.skew != 1.0)
			return ConnectionType::RangeWithSkew;

		if (targetRange.rng.interval > 0.02)
			return ConnectionType::RangeWithStep;

		return ConnectionType::Range;
	}

	bool operator==(const Connection& other) const
	{
		auto sameRange = RangeHelpers::isEqual(targetRange, other.targetRange);
		auto sameExpression = expressionCode.compare(other.expressionCode) == 0;
		auto sameIndex = index == other.index;
		auto sameN = n == other.n;

		return sameRange && sameExpression && sameIndex && sameN;
	}

	Node::Ptr n;
	int index = -1;
	CableType cableType = CableType::numCableTypes;
	scriptnode::InvertableParameterRange targetRange;
	String expressionCode;
};

struct PooledRange : public ReferenceCountedObject
{
	PooledRange(const Symbol& s) :
		id(s)
	{};

	bool operator==(const scriptnode::InvertableParameterRange& r) const
	{
		return RangeHelpers::isEqual(c.targetRange, r);
	}

	bool operator==(const PooledRange& other) const
	{
		return RangeHelpers::isEqual(c.targetRange, other.c.targetRange);
	}

	Symbol id;
	Connection c;
};

struct PooledCableType : public UsingTemplate,
						 public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<PooledCableType>;

	struct TemplateConstants
	{
		bool operator==(const TemplateConstants& other) const
		{
			return numChannels == other.numChannels && isFrame == other.isFrame;
		}

		int numChannels = 0;
		bool isFrame = false;
	};

	PooledCableType(Base& p, const Identifier& id, TemplateConstants& c) :
		UsingTemplate(p, id, NamespacedIdentifier::fromString("cable").getChildId(c.isFrame ? "frame" : "block")),
		data(c)
	{
		*this << c.numChannels;
	}

	bool operator==(const TemplateConstants& other) const
	{
		return data == other;
	}

	bool operator==(const PooledCableType& other) const
	{
		return data == other.data;
	}

	TemplateConstants data;
};

struct PooledExpression: public ReferenceCountedObject
{
	PooledExpression(const Symbol& s) :
		id(s)
	{};

	bool operator==(const String& otherExpression) const
	{
		return expression.compare(otherExpression) == 0;
	}

	bool operator==(const PooledExpression& other) const
	{
		return *this == other.expression;
	}

	Symbol id;
	String expression;
};

struct PooledStackVariable : public StackVariable,
							 public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<PooledStackVariable>;

	PooledStackVariable(Base& p, const ValueTree& nodeTree);;

	bool operator==(const PooledStackVariable& otherExpression) const
	{
		return expression.compare(otherExpression.expression) == 0;
	}

	bool operator==(const String& otherExpression) const
	{
		return expression.compare(otherExpression) == 0;
	}

	ValueTree nodeTree;
};

struct PooledParameter : public UsingTemplate,
						 public ReferenceCountedObject
{
	PooledParameter(Base& b, const Identifier& id, const Identifier& parameterType, const Connection& c_) :
		UsingTemplate(b, id, NamespacedIdentifier("parameter").getChildId(parameterType)),
		c(c_)
	{};

	bool operator==(const Connection& other) const
	{
		return c == other;
	}

	bool operator==(const PooledParameter& o) const
	{
		return c == o.c;
	}

	using Ptr = ReferenceCountedObjectPtr<PooledParameter>;
	using List = ReferenceCountedArray<PooledParameter>;

	Connection c;

#if 0
	PooledParameter(Node::Ptr p_):
		p(p_),
		d(RangeHelpers::getDoubleRange(p->nodeTree)),
		parameterId(p->nodeTree[PropertyIds::ParameterId].toString())
	{}

	bool operator==(const ValueTree& v) const
	{
		auto r1 = RangeHelpers::getDoubleRange(v);
		auto vId = v[PropertyIds::ParameterId].toString();
		return parameterId == Identifier(vId) && RangeHelpers::isEqual(r1, d);
	}

	bool operator==(Node::Ptr other) const
	{
		return p == other;
	}

	bool operator==(const PooledParameter& other) const
	{
		return RangeHelpers::isEqual(d, other.d) && other.parameterId == parameterId;
	}

#endif
};

template <typename ItemType> struct PoolBase
{
	template <typename CompareType> ReferenceCountedObjectPtr<ItemType> getExisting(const CompareType& t) const
	{
		for (const auto i : items)
			if (*i == t)
				return i;

		return nullptr;
	}

	ItemType** begin() { return items.begin(); }
	ItemType** end() { return items.end(); }

	ItemType* const* begin() const { return items.begin(); }
	ItemType* const* end() const { return items.end(); }

	ReferenceCountedObjectPtr<ItemType> getLast()
	{
		return items.getLast();
	}

	ReferenceCountedObjectPtr<ItemType> add(ReferenceCountedObjectPtr<ItemType> t)
	{
		for (const auto i : items)
		{
			if (*i == *t)
				return t;
		}

		items.add(t);
		return t;
	}

	void clear()
	{
		items.clear();
	}

private:

	ReferenceCountedArray<ItemType> items;
};

struct HeaderBuilder : public Base
{
	HeaderBuilder(const String& title) :
		Base(Base::OutputType::NoProcessing)
	{
		String headline;
		headline << "/** " << title << "%FILL80";

		lines.add(headline);
		addEmptyLine();
	};

	void flushIfNot()
	{
		if (flushed)
			return;

		addEmptyLine();
		lines.add("*\t%FILL80");
		lines.add("*/");
		lines.add("");

		flushed = false;
	}

	void addEmptyLine() override
	{
		lines.add("*");
	}

	HeaderBuilder& operator <<(const String& t)
	{
		lines.add("*\t" + t);
		return *this;
	}

	String operator()()
	{
		flushIfNot();
		return toString();
	}

	bool flushed = false;
};

struct ValueTreeBuilder: public Base
{
	struct CodeProvider
	{
		virtual ~CodeProvider() {};
		virtual String getCode(const NamespacedIdentifier& nodePath, const Identifier& classId) const = 0;
	};

	struct BuildResult
	{
		BuildResult() :
			r(Result::ok())
		{};

		Result r;
		String code;
		std::shared_ptr<std::set<String>> faustClassIds;
	};
    
    struct ScopedChannelSetter
    {
        ScopedChannelSetter(ValueTreeBuilder& vtb_, int numChannels, bool allowHigherChannelCount):
          vtb(vtb_),
          prevNumChannels(vtb_.numChannelsToCompile)
        {
            jassert(numChannels <= vtb.numChannelsToCompile || allowHigherChannelCount);

#if ENABLE_CPP_DEBUG_LOG
			DBG("Setting channel count to " + String(numChannels));
#endif

            vtb.numChannelsToCompile = numChannels;
        }
        ~ScopedChannelSetter()
        {
#if ENABLE_CPP_DEBUG_LOG
			DBG("Setting channel count back to " + String(prevNumChannels));
#endif

            vtb.numChannelsToCompile = prevNumChannels;
        }
        
        ValueTreeBuilder& vtb;
        int prevNumChannels;
    };

	enum class Format
	{
		JitCompiledInstance,
		CppDynamicLibrary,
		TestCaseFile,
		numFormats
	};

	enum class FormatGlueCode
	{
		PreNamespaceCode,
		WrappedNamespace,
		MainInstanceClass,
		PublicDefinition,
		numFormatGlueCodes
	};

	static int getRootChannelAmount(const ValueTree& v)
	{
		auto c = (int)v.getParent()[PropertyIds::CompileChannelAmount];

		if (c == 0)
			return 2;

		return c;
	}

	ValueTreeBuilder(const ValueTree& data, Format outputFormatToUse) :
		Base(Base::OutputType::AddTabs),
		v(data),
		outputFormat(Format::CppDynamicLibrary),
		r(Result::ok()),
		rootChannelAmount(getRootChannelAmount(v)),
		numChannelsToCompile(rootChannelAmount),
		faustClassIds(new std::set<String>)
	{
		if (numChannelsToCompile == 0)
			numChannelsToCompile = 2;

		// The channel amount will not work otherwise...
		jassert(v.getParent().isValid());
		setHeaderForFormat();
	}

	BuildResult createCppCode()
	{
		rebuild();

		BuildResult br;
		br.r = r;


		if (r.wasOk())
		{
			br.code = getCurrentCode();
		}
		else
			br.code = wrongNodeCode;
		br.faustClassIds = faustClassIds;

		return br;
	}

	void setCodeProvider(CodeProvider* p)
	{
		codeProvider = p;
	}

	static Result cleanValueTreeIds(ValueTree& vToClean);

	void addAudioFileProvider(hise::MultiChannelAudioBuffer::DataProvider* p)
	{
		audioFileProviders.add(p);
	}

	hise::MultiChannelAudioBuffer::SampleReference::Ptr loadAudioFile(const String& ref)
	{
		for (auto sr : audioFileProviders)
		{
			auto p = sr->loadFile(ref);

			if (p != nullptr && p->r.wasOk())
				return p;
		}

		return nullptr;
	}

	struct ExternalSample
	{
		String className;
		hise::MultiChannelAudioBuffer::SampleReference::Ptr data;
	};

	void addExternalSample(const String& id, hise::MultiChannelAudioBuffer::SampleReference::Ptr s)
	{
		externalReferences.add({ id, s });
	}

	using SampleList = Array<ExternalSample>;

	SampleList getExternalSampleList() const { return externalReferences; };

private:

    void checkUnflushed(Node::Ptr n);
    
	SampleList externalReferences;

	ReferenceCountedArray<hise::MultiChannelAudioBuffer::DataProvider> audioFileProviders;

	ScopedPointer<CodeProvider> codeProvider;

	std::shared_ptr<std::set<String>> faustClassIds;

	void setHeaderForFormat();

	String getCurrentCode() const { return toString(); }


	struct Error
	{
		String toString()
		{
			String s;

			if (auto xml = v.createXml())
			{
				s << errorMessage;
				s << "\nValueTree: \n";
				s << xml->createDocument("", false);
			}

			return s;
		}
		ValueTree v;
		String errorMessage;
	};

	bool addNumVoicesTemplate(Node::Ptr n);


	void clear() override
	{
		Base::clear();

		r = Result::ok();
		pooledTypeDefinitions.clear();
		pooledRanges.clear();
		pooledParameters.clear();
		pooledExpressions.clear();
	}

	void rebuild();

	Result r;
	String wrongNodeCode;

	struct RootContainerBuilder
	{
		RootContainerBuilder(ValueTreeBuilder& parent_, Node::Ptr rootContainer) :
			parent(parent_),
			root(rootContainer),
			outputFormat(parent.outputFormat)
		{};

		Node::Ptr parse();

		void addDefaultParameters();
		PooledStackVariable::Ptr getChildNodeAsStackVariable(const ValueTree& v);

		PooledStackVariable::Ptr getChildNodeAsStackVariable(const String& nodeId);

		void addMetadata();

		void addParameterConnections();

		void addModulationConnections();

		void addModConnection(ValueTree& t, Node::Ptr modSource, int index=-1);

		void addSendConnections();

		void createStackVariablesForChildNodes();

		int getNumParametersToInitialise(ValueTree& child);

		Node::List getSendNodes();

		Node::List getModulationNodes();

		Node::List getContainersWithParameter();

		bool hasComplexTypes() const;
        
        bool hasRuntimeTargets() const;

		ValueTreeBuilder& parent;
		Format outputFormat;
		Node::Ptr root;

		Identifier nodeClassId;

		PoolBase<PooledStackVariable> stackVariables;
	};

	struct ComplexDataBuilder
	{
		ComplexDataBuilder(ValueTreeBuilder& parent, Node::Ptr nodeToWrap);

		Node::Ptr parse();

		void setFlushNode(bool shouldFlushNode)
		{
			flushNodeBeforeReturning = shouldFlushNode;
		}
		
		static Array<float> getEmbeddedData(const ValueTree& nodeTree, ExternalData::DataType t, int slotIndex);

	private:

		bool flushNodeBeforeReturning = true;

		Node::Ptr parseEmbeddedDataNode(ExternalData::DataType t);
		Node::Ptr parseExternalDataNode(ExternalData::DataType t, int slotIndex);
		Node::Ptr parseMatrixDataNode();
		Node::Ptr parseSingleDisplayBufferNode(bool enableBuffer);

		ValueTreeBuilder& parent;
		Node::Ptr n;
	};

	struct SnexNodeBuilder
	{
		SnexNodeBuilder(ValueTreeBuilder& parent_, Node::Ptr nodeToWrap) :
			parent(parent_),
			n(nodeToWrap)
		{};

		Node::Ptr parse();

	private:

		static bool needsWrapper(const NamespacedIdentifier& p)
		{
			return p != NamespacedIdentifier::fromString("core::snex_node");
		}

		Node::Ptr parseWrappedSnexNode();

		Node::Ptr parseUnwrappedSnexNode();

		String classId;
		String code;
		NamespacedIdentifier p;

		ValueTreeBuilder& parent;
		Node::Ptr n;
	};

	struct ExpressionNodeBuilder
	{
		ExpressionNodeBuilder(ValueTreeBuilder& parent_, Node::Ptr u, bool isMathNode):
			parent(parent_),
			mathNode(isMathNode),
			n(u)
		{

		}

		Node::Ptr parse();

		ValueTreeBuilder& parent;
		Node::Ptr n;
		const bool mathNode;
	};

	Format outputFormat =  Format::TestCaseFile;

	String getGlueCode(FormatGlueCode c);

	PooledParameter::Ptr makeParameter(const Identifier& id, const String path, const Connection& c)
	{
		jassert(!path.contains("parameter"));
		auto n = new PooledParameter(*this, id, path, c);
		return n;
	}

	Node::Ptr createNode(const ValueTree& v, const Identifier& id, const String& path)
	{
		auto n = new Node(*this, id, NamespacedIdentifier::fromString(path));
		n->nodeTree = v;

		return n;
	}

	void addNodeComment(Node::Ptr n);

	Node::Ptr parseFixChannel(const ValueTree& n, int numChannelsToUse);

	Node::Ptr getNode(const ValueTree& n, bool allowZeroMatch);

	Node::Ptr getNode(const NamespacedIdentifier& id, bool allowZeroMatch) const;

    Node::Ptr parseRuntimeTargetNode(Node::Ptr u);
    
	Node::Ptr parseFaustNode(Node::Ptr u);

	Node::Ptr parseRoutingNode(Node::Ptr u);

	Node::Ptr parseComplexDataNode(Node::Ptr u, bool flushNode=true);

	Node::Ptr parseOptionalSnexNode(Node::Ptr u);

	Node::Ptr parseExpressionNode(Node::Ptr u);

	Node::Ptr parseSnexNode(Node::Ptr u);

	Node::Ptr parseNode(const ValueTree& n);
	
	Node::Ptr parseContainer(Node::Ptr u);

	Node::Ptr parseCloneContainer(Node::Ptr u);

	void emitRangeDefinition(const Identifier& rangeId, InvertableParameterRange r);

	void parseContainerChildren(Node::Ptr container);
	void parseContainerParameters(Node::Ptr container);
	Node::Ptr parseRootContainer(Node::Ptr container);

	Node::Ptr parseMod(Node::Ptr u);

	PooledParameter::Ptr parseParameter(const ValueTree& p, Connection::CableType connectionType);

	Node::Ptr wrapNode(Node::Ptr, const NamespacedIdentifier& wrapId, int firstIntParam=-1);

	Connection getConnection(const ValueTree& c);

	Node::Ptr getTypeDefinition(const NamespacedIdentifier& id);

	static NamespacedIdentifier getNodePath(const ValueTree& n);

	PooledParameter::Ptr addParameterAndReturn(PooledParameter::Ptr p)
	{
#if !BETTER_TEMPLATE_FORWARDING
		if (p->c.n != nullptr && p->c.n->isPolyphonicOrHasPolyphonicTemplate())
		{
			p->addTemplateIntegerArgument("NV");
		}
#endif

		return pooledParameters.add(p);
	}

	PooledParameter::Ptr createParameterFromConnection(const Connection& c, const Identifier& pName, int parameterIndexInChain, const ValueTree& pTree);

	PoolBase<Node> pooledTypeDefinitions;
	PoolBase<PooledRange> pooledRanges;
	PoolBase<PooledParameter> pooledParameters;
	PoolBase<PooledExpression> pooledExpressions;
	PoolBase<PooledCableType> pooledCables;

	StringArray definedSnexClasses;

	NamespacedIdentifier getNodeId(const ValueTree& n);
	NamespacedIdentifier getNodeVariable(const ValueTree& n);

	ValueTree v;
	const int rootChannelAmount;
    int numChannelsToCompile;
	bool allowPlainParameterChild = true;
};





}
}
