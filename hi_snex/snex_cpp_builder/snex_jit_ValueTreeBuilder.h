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

namespace snex {
namespace cppgen {
using namespace juce;
using namespace scriptnode;

struct ValueTreeIterator
{
	using Func = std::function<bool(ValueTree& v)>;

	enum IterationType
	{
		Forward,
		Backwards,
		ChildrenFirst,
		ChildrenFirstBackwards,
		OnlyChildren,
		OnlyChildrenBackwards
	};

	static ValueTree findParentWithType(const ValueTree& v, const Identifier& id)
	{
		auto p = v.getParent();

		if (!p.isValid())
			return {};

		if (p.getType() == id)
			return p;

		return findParentWithType(p, id);
	}

	static bool isLast(const ValueTree& v)
	{
		return (v.getParent().getNumChildren() - 1) == getIndexInParent(v);
	}

	static int getIndexInParent(const ValueTree& v)
	{
		return v.getParent().indexOf(v);
	}

	static bool isBetween(IterationType l, IterationType u, IterationType v);
	static bool isBackwards(IterationType t);
	static bool isRecursive(IterationType t);
	static bool forEach(ValueTree v, IterationType type, const Func& f);

	static bool forEachParent(ValueTree& v, const Func& f);

	

	static bool getNodePath(Array<int>& path, ValueTree& root, const Identifier& id);

	static bool hasRealParameters(const ValueTree& containerTree);

	static bool hasNodeProperty(const ValueTree& nodeTree, const Identifier& id);

	static var getNodeProperty(const ValueTree& nodeTree, const Identifier& id);

	static bool isComplexDataNode(const ValueTree& nodeTree);

	static bool isAutomated(const ValueTree& parameterTree);

	static String getSnexCode(const ValueTree& nodeTree);
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

	ValueTree nodeTree;
};


struct Connection
{
	enum class CableType
	{
		Parameter,
		Modulation,
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

		if (targetRange.skew != 1.0)
			return ConnectionType::RangeWithSkew;

		if (targetRange.interval > 0.02)
			return ConnectionType::RangeWithStep;

		return ConnectionType::Range;
	}

	bool operator==(const Connection& other) const
	{
		auto sameRange = RangeHelpers::isEqual(targetRange, other.targetRange);
		auto sameExpression = expressionCode.compare(other.expressionCode) == 0;
		auto sameIndex = index == other.index;

		return sameRange && sameExpression && sameIndex;
	}

	Node::Ptr n;
	int index = -1;
	CableType cableType = CableType::numCableTypes;
	NormalisableRange<double> targetRange;
	String expressionCode;
};

struct PooledRange : public ReferenceCountedObject
{
	PooledRange(const Symbol& s) :
		id(s)
	{};

	bool operator==(const NormalisableRange<double>& r) const
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

	ItemType** begin() const
	{
		return items.begin();
	}

	ItemType** end() const
	{
		return items.end();
	}

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
	struct BuildResult
	{
		BuildResult() :
			r(Result::ok())
		{};

		Result r;
		String code;
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
		WrappedNamespace,
		MainInstanceClass,
		PublicDefinition,
		numFormatGlueCodes
	};

	ValueTreeBuilder(const ValueTree& data, Format outputFormatToUse) :
		Base(Base::OutputType::AddTabs),
		v(data),
		outputFormat(outputFormatToUse),
		r(Result::ok())
	{
		setHeaderForFormat();
	}

	BuildResult createCppCode()
	{
		rebuild();

		BuildResult br;
		br.r = r;

		if (r.wasOk())
			br.code = getCurrentCode();
		else
			br.code = wrongNodeCode;

		return br;
	}

private:

	void setHeaderForFormat();

	String getCurrentCode() const { return toString(); }

	struct Error
	{
		Error()
		{
			int x = 5;
		}

		String toString()
		{
			String s;

			if (ScopedPointer<XmlElement> xml = v.createXml())
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

		void addSendConnections();

		void createStackVariablesForChildNodes();

		int getNumParametersToInitialise(ValueTree& child);

		Node::List getSendNodes();

		Node::List getModulationNodes();

		Node::List getContainersWithParameter();

		bool hasComplexTypes() const;

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

		static ExternalData::DataType getType(const ValueTree& v);

	private:

		Node::Ptr parseDataClass();

		

		Array<float> getEmbeddedData() const;
		

		ValueTreeBuilder& parent;
		Node::Ptr n;
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


	struct Sanitizers
	{
		static bool setChannelAmount(ValueTree& v);
	};

	Node::Ptr parseFixChannel(const ValueTree& n);

	Node::Ptr getNode(const ValueTree& n, bool allowZeroMatch);

	Node::Ptr getNode(const NamespacedIdentifier& id, bool allowZeroMatch) const;

	Node::Ptr parseRoutingNode(Node::Ptr u);

	Node::Ptr parseComplexDataNode(Node::Ptr u);

	Node::Ptr parseSnexNode(Node::Ptr u);

	Node::Ptr parseNode(const ValueTree& n);
	
	Node::Ptr parseContainer(Node::Ptr u);

	

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
		return pooledParameters.add(p);
	}

	PooledParameter::Ptr createParameterFromConnection(const Connection& c, const Identifier& pName, int parameterIndexInChain)
	{
		if (auto existing = pooledParameters.getExisting(c))
			return existing;

		String p = pName.toString();

		if (parameterIndexInChain != -1)
			p << "_" << parameterIndexInChain;

		if (!c)
		{
			auto up = makeParameter(p, "empty", {});
			return addParameterAndReturn(up);
		}

		

		

		if (!c)
		{
			Error e;
			e.errorMessage = "Can't find connection for " + pName.toString();
			throw e;
		}

		auto t = c.getType();

		switch (t)
		{
		case Connection::ConnectionType::Bypass:
		{
			auto up = makeParameter(p, "bypass", c);
			*up << *c.n;

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
			auto up = makeParameter(p, "from0To1", c);
			*up << *c.n;
			*up << c.index;

			String fWhat;

			fWhat << up->scopedId.getIdentifier();
			fWhat << "Range";

			auto parseRange = [&](const NormalisableRange<double>& r)
			{
				if (auto existing = pooledRanges.getExisting(r))
				{
					return existing;
				}
				else
				{
					auto r = new PooledRange({ *this, fWhat });
					r->c = c;

					StringArray args;

					args.add(fWhat);

					args.add(Helpers::getCppValueString(c.targetRange.start, Types::ID::Double));
					args.add(Helpers::getCppValueString(c.targetRange.end, Types::ID::Double));

					String macroName = "DECLARE_PARAMETER_RANGE";

					if (t == Connection::ConnectionType::RangeWithSkew)
					{
						macroName << "_SKEW";
						args.add(Helpers::getCppValueString(c.targetRange.skew, Types::ID::Double));
					}
					else if (t == Connection::ConnectionType::RangeWithStep)
					{
						macroName << "_STEP";
						args.add(Helpers::getCppValueString(c.targetRange.interval, Types::ID::Double));
					}

					for (auto& a : args)
						a = StringHelpers::withToken(IntendMarker, a);

					Macro(*this, macroName, args);
					addEmptyLine();
					return pooledRanges.add(r);
				}
			};

			auto r = parseRange(c.targetRange);

			*up << r->id.toExpression();

			up->flushIfNot();
			addEmptyLine();

			return addParameterAndReturn(up);
		}
		default:
		{
			auto up = makeParameter(p, "plain", c);
			*up << *c.n;
			*up << c.index;
			
			return addParameterAndReturn(up);
		}
		}
	}

	PoolBase<Node> pooledTypeDefinitions;
	PoolBase<PooledRange> pooledRanges;
	PoolBase<PooledParameter> pooledParameters;
	PoolBase<PooledExpression> pooledExpressions;
	PoolBase<PooledCableType> pooledCables;

	NamespacedIdentifier getNodeId(const ValueTree& n);
	NamespacedIdentifier getNodeVariable(const ValueTree& n);

	ValueTree v;
};





}
}
