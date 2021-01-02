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
	static bool forEach(ValueTree& v, IterationType type, const Func& f);

	static bool getNodePath(Array<int>& path, ValueTree& root, const Identifier& id);

	static bool hasRealParameters(const ValueTree& containerTree);

	static bool isAutomated(const ValueTree& parameterTree);

	static String getSnexCode(const ValueTree& nodeTree);
};



struct Node : public ReferenceCountedObject
{
	using List = ReferenceCountedArray<Node>;
	using Ptr = ReferenceCountedObjectPtr<Node>;

	Node(UsingTemplate&& t_) :
		t(t_)
	{};

	bool isRootNode() const
	{
		return ValueTreeIterator::getIndexInParent(nodeTree) == -1;
	}

	ValueTree nodeTree;

	UsingTemplate t;
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

		if (targetRange.skew == 1.0)
			return ConnectionType::Range;

		return ConnectionType::RangeWithSkew;
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

struct PooledParameter : public Node
{
	PooledParameter(UsingTemplate&& t, const Connection& c_) :
		Node(std::move(t)),
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

private:

	ReferenceCountedArray<ItemType> items;
};

struct ValueTreeBuilder: public ValueTree::Listener,
						 public Timer,
						 public Base
{
	enum class Format
	{
		JitCompiledInstance,
		CppDynamicLibrary,
		numFormats
	};

	enum class FormatGlueCode
	{
		WrappedNamespace,
		MainInstanceClass,
		PublicDefinition,
		numFormatGlueCodes
	};

	ValueTreeBuilder(const ValueTree& data) :
		Base(Base::OutputType::AddTabs),
		v(data)
	{
		v.addListener(this);
		rebuild();
	}

	void valueTreePropertyChanged(ValueTree&, const Identifier&) override { startTimer(2000); }
	void valueTreeChildAdded(ValueTree& , ValueTree& ) override { startTimer(2000); }
	void valueTreeChildRemoved(ValueTree& , ValueTree& , int ) override { startTimer(2000); }
	void valueTreeChildOrderChanged(ValueTree& , int , int )  override { startTimer(2000); }
	void valueTreeParentChanged(ValueTree&)  override { startTimer(2000); }

	void timerCallback() override
	{
		rebuild();
		stopTimer();
	}

	void rebuild();

	~ValueTreeBuilder()
	{
		v.removeListener(this);
	}

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

private:

	Format outputFormat =  Format::CppDynamicLibrary;

	String getGlueCode(FormatGlueCode c);

	PooledParameter::Ptr makeParameter(const Identifier& id, const String path, const Connection& c)
	{
		auto n = new PooledParameter(UsingTemplate(*this, id, NamespacedIdentifier::fromString(path)), c);
		return n;
	}

	Node::Ptr createNode(const ValueTree& v, const Identifier& id, const String& path)
	{
		auto n = new Node(UsingTemplate(*this, id, NamespacedIdentifier::fromString(path)));
		n->nodeTree = v;
		return n;
	}


	struct Sanitizers
	{
		static bool setChannelAmount(ValueTree& v);
	};

	Node::Ptr parseFixChannel(const ValueTree& n);

	Node::Ptr getNode(const ValueTree& n, bool allowZeroMatch);

	Node::Ptr getNode(const NamespacedIdentifier& id, bool allowZeroMatch) const;

	Node::Ptr parseSnexNode(Node::Ptr u);

	Node::Ptr parseNode(const ValueTree& n);
	
	Node::Ptr parseContainer(Node::Ptr u);
	void parseContainerChildren(Node::Ptr container);
	void parseContainerParameters(Node::Ptr container);
	Node::Ptr parseContainerInitialiser(Node::Ptr container);

	Node::Ptr parseMod(Node::Ptr u);

	PooledParameter::Ptr parseParameter(const ValueTree& p, Connection::CableType connectionType);

	Connection getConnection(const ValueTree& c);

	Node::Ptr getUsingTemplate(const NamespacedIdentifier& id);

	Node::List usingTemplates;

	

	Node::List getModulationNodes(Node::Ptr c);

	static NamespacedIdentifier getNodePath(const ValueTree& n);

	StackVariable getChildNodeAsStackVariable(ValueTree& root, const Identifier& id, const String& suffix);;

	PooledParameter::Ptr addParameterAndReturn(PooledParameter::Ptr p)
	{
		return pooledParameters.add(p);
	}

	PooledParameter::Ptr createParameterFromConnection(const Connection& c, const Identifier& pName, int parameterIndexInChain)
	{
		if (auto existing = pooledParameters.getExisting(c))
			return existing;

		if (pName.toString() == "empty")
		{
			auto up = makeParameter({}, "parameter::empty", {});
			return addParameterAndReturn(up);
		}

		String p = pName.toString();

		if(parameterIndexInChain != -1)
			p << "_" << parameterIndexInChain;

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
			auto up = makeParameter(p, "parameter::bypass", c);
			up->t << c.n->t;

			return addParameterAndReturn(up);
		}
		case Connection::ConnectionType::Expression:
		{
			// parameter::expression<T, P, Expression>;

			auto up = makeParameter(p, "parameter::expression", c);
			up->t << c.n->t;
			up->t << c.index;

			String fWhat;
			fWhat << up->t.scopedId.getIdentifier();
			fWhat << "Expression";

			if (auto existing = pooledExpressions.getExisting(c.expressionCode))
			{
				up->t << existing->id.toExpression();
			}
			else
			{
				auto r = new PooledExpression({ *this, fWhat });

				r->expression = c.expressionCode;

				//DECLARE_PARAMETER_EXPRESSION(fWhat, x)
				Macro(*this, "DECLARE_PARAMETER_EXPRESSION", { StringHelpers::withToken(IntendMarker, fWhat), StringHelpers::withToken(IntendMarker, c.expressionCode) });
				addEmptyLine();
				up->t << r->id.toExpression();

				pooledExpressions.add(r);
			}

			up->t.flushIfNot();
			addEmptyLine();

			return addParameterAndReturn(up);
		}
		case Connection::ConnectionType::RangeWithSkew:
		case Connection::ConnectionType::Range:
		{
			auto up = makeParameter(p, "parameter::from0To1", c);
			up->t << c.n->t;
			up->t << c.index;

			String fWhat;

			fWhat << up->t.scopedId.getIdentifier();
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

					for (auto& a : args)
						a = StringHelpers::withToken(IntendMarker, a);

					Macro(*this, macroName, args);
					addEmptyLine();
					return pooledRanges.add(r);
				}
			};

			auto r = parseRange(c.targetRange);

			up->t << r->id.toExpression();

			up->t.flushIfNot();
			addEmptyLine();

			return addParameterAndReturn(up);
		}
		default:
		{
			auto up = makeParameter(p, "parameter::plain", c);
			up->t << c.n->t;
			up->t << c.index;
			
			return addParameterAndReturn(up);
		}
		}
	}

	PoolBase<PooledRange> pooledRanges;
	PoolBase<PooledParameter> pooledParameters;
	PoolBase<PooledExpression> pooledExpressions;

	NamespacedIdentifier getNodeId(const ValueTree& n);
	NamespacedIdentifier getNodeVariable(const ValueTree& n);

	ValueTree v;
};





}
}
