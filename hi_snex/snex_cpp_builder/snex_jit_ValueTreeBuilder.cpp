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

	static bool isParameter(const NamespacedIdentifier& id)
	{
		return id.getParent().getIdentifier() == parameter;
	}
};

void ValueTreeBuilder::rebuild()
{
	clear();

	usingTemplates.clear();

	try
	{
		Sanitizers::setChannelAmount(v);
		
		{
			Namespace impl(*this, getGlueCode(FormatGlueCode::WrappedNamespace), false);
			addComment("Node & Parameter type declarations", Base::CommentType::FillTo80);
			usingTemplates.add(parseNode(v));
			impl.flushIfNot();

			getGlueCode(FormatGlueCode::PublicDefinition);
		}
	}
	catch (Error& e)
	{
		clear();
		*this << e.toString();
	}
}

String ValueTreeBuilder::getGlueCode(ValueTreeBuilder::FormatGlueCode c)
{
	if(outputFormat == Format::JitCompiledInstance)
	{
		switch (c)
		{
			case FormatGlueCode::WrappedNamespace: return "impl";
			case FormatGlueCode::PublicDefinition:
			{
				UsingTemplate instance(*this, "instance", NamespacedIdentifier::fromString("wrap::node"));
				instance << usingTemplates.getLast()->t;
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
		case FormatGlueCode::WrappedNamespace: return v[PropertyIds::ID].toString() + "_impl";
		case FormatGlueCode::MainInstanceClass: return "instance";
		case FormatGlueCode::PublicDefinition: 
		{
			addComment("Public Definition", Base::CommentType::FillTo80);

			Namespace n(*this, "project", false);

			UsingTemplate instance(*this, v[PropertyIds::ID].toString(), NamespacedIdentifier::fromString("wrap::node"));
			instance << usingTemplates.getLast()->t;
			instance.flushIfNot();
			return {};
		}
		
		}
	}

	return {};
}

Node::Ptr ValueTreeBuilder::parseFixChannel(const ValueTree& n)
{
	auto u = getNode(n, false);

	auto wf = createNode(n, {}, "wrap::fix");

	wf->t << (int)n[PropertyIds::NumChannels];
	wf->t << u->t;

	return wf;
}

Node::Ptr ValueTreeBuilder::getNode(const ValueTree& n, bool allowZeroMatch)
{
	auto id = getNodeId(n);

	if (auto existing = getUsingTemplate(id))
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

			RETURN_IF_NO_THROW(nullptr);
		}
	}
}

Node::Ptr ValueTreeBuilder::getNode(const NamespacedIdentifier& id, bool allowZeroMatch) const
{
	for (auto n : usingTemplates)
	{
		if (n->t.scopedId == id)
			return n;
	}

	if (allowZeroMatch)
		return nullptr;

	Error e;
	e.errorMessage = "Can't find node " + id.toString();
	throw e;

	RETURN_IF_NO_THROW(nullptr);
}

Node::Ptr ValueTreeBuilder::parseSnexNode(Node::Ptr u)
{
	auto code = ValueTreeIterator::getSnexCode(u->nodeTree);

	if (code.isNotEmpty())
	{
		String comment;

		comment << "Custom SNEX class for wrapper " << getNodePath(u->nodeTree).toString();

		

		Struct s(*this, u->nodeTree[PropertyIds::ID].toString(), {}, {});
		addComment(comment, Base::CommentType::FillTo80Light);

		*this << code;

		if (outputFormat == Format::CppDynamicLibrary && code.contains("Math"))
		{
			addIfNotEmptyLine();
			*this << "hmath Math;";
		}

		s.flushIfNot();

		addEmptyLine();

		u->t << s.toExpression();

		

		u->t.flushIfNot();
	}

	return parseContainer(u);
}

Node::Ptr ValueTreeBuilder::parseNode(const ValueTree& n)
{
	if (auto existing = getNode(n, true))
		return existing;

	auto typeId = getNodeId(n);

	Node::Ptr newNode = createNode(n, typeId.getIdentifier(), getNodePath(n).toString());

	return parseSnexNode(newNode);
}

Node::Ptr ValueTreeBuilder::parseContainer(Node::Ptr u)
{
	if (FactoryIds::isContainer(getNodePath(u->nodeTree)))
	{
		for (auto c : u->nodeTree.getChildWithName(PropertyIds::Nodes))
			usingTemplates.add(parseNode(c));

		parseContainerParameters(u);
		parseContainerChildren(u);

		auto needsInitialisation = [](Node::Ptr u)
		{
			auto hasParameters = u->nodeTree.getChildWithName(PropertyIds::Parameters).getNumChildren() != 0;

			bool hasNodesWithParameters = false;

			for (auto c : u->nodeTree.getChildWithName(PropertyIds::Nodes))
			{
				hasNodesWithParameters |= c.getChildWithName(PropertyIds::Parameters).getNumChildren() != 0;
			}

			return hasParameters || hasNodesWithParameters;
		};

		if (needsInitialisation(u))
		{
			u = parseContainerInitialiser(u);
		}

		u->t.flushIfNot();

		return u;
	}

	return parseMod(u);
}

void ValueTreeBuilder::parseContainerChildren(Node::Ptr container)
{
	Node::List children;

	ValueTreeIterator::forEach(container->nodeTree.getChildWithName(PropertyIds::Nodes), ValueTreeIterator::OnlyChildren, [&](ValueTree& c)
	{
		auto parentPath = getNodePath(ValueTreeIterator::findParentWithType(c, PropertyIds::Node));

		auto useFix = c.getParent().indexOf(c) == 0 || parentPath.getIdentifier() == Identifier("multi");

		if (useFix)
			children.add(parseFixChannel(c));
		else
			children.add(getNode(c, false));

		return false;
	});

	for (auto& c : children)
		container->t << c->t;
}

void ValueTreeBuilder::parseContainerParameters(Node::Ptr c)
{
	addEmptyLine();

	auto pTree = c->nodeTree.getChildWithName(PropertyIds::Parameters);
	auto numParameters = pTree.getNumChildren();

	Namespace n(*this, c->t.scopedId.getIdentifier().toString() + "_parameters", !ValueTreeIterator::hasRealParameters(c->nodeTree));

	if(numParameters == 0)
		c->t << "parameter::empty";
	else if (numParameters == 1)
	{
		auto up = parseParameter(pTree.getChild(0), Connection::CableType::Parameter);
		n.flushIfNot();
		c->t << up->t;
	}
	else
	{
		addComment("Parameter list for " + c->t.scopedId.toString(), Base::CommentType::FillTo80Light);

		PooledParameter::List parameterList;

		for (auto c : pTree)
			parameterList.add(parseParameter(c, Connection::CableType::Parameter));

		String pId;
		pId << c->t.scopedId.getIdentifier();
		pId << "_plist";

		Node::Ptr l = createNode(pTree, pId, "parameter::list");

		for (auto p : parameterList)
		{
			p->t.flushIfNot();
			l->t << p->t;
		}

		l->t.flushIfNot();

		n.flushIfNot();

		c->t << l->t;
	}
}

Node::Ptr ValueTreeBuilder::parseContainerInitialiser(Node::Ptr c)
{
	if (c->t.getLength() > 60)
	{
		c->t.scopedId = c->t.scopedId.getParent().getChildId(c->t.scopedId.getIdentifier().toString() + "_");
		c->t.flushIfNot();
		addEmptyLine();
	}
		
	String iId;

	iId << getNodeId(c->nodeTree).getIdentifier();

	{
		
		if (c->isRootNode())
		{
			iId = getGlueCode(FormatGlueCode::MainInstanceClass);
			addComment("Root node initialiser class", Base::CommentType::FillTo80);
		}
			

		Struct s(*this, iId, { &c->t }, {});

		if (c->isRootNode())
		{
			Struct m(*this, "metadata", {}, {});

			Macro(*this, "SNEX_METADATA_ID", { iId });
			Macro(*this, "SNEX_METADATA_NUM_CHANNELS", {c->nodeTree[PropertyIds::NumChannels].toString()});
			Macro(*this, "SNEX_METADATA_PARAMETERS", { "2", "\"Funky\"", "\"Why\"" });

#if 0
			String s;
			zstd::ZDefaultCompressor comp;
			MemoryBlock mb;
			comp.compress(v, mb);
			s = mb.toBase64Encoding();
			Macro(*this, "SNEX_METADATA_SNIPPET", { s });
#endif

			m.flushIfNot();
			addEmptyLine();
		}

		{
			String f;
			f << iId << "()";
			*this << f;
			{
				StatementBlock sb(*this);

				int index = 0;

				addComment("Child nodes", Base::CommentType::FillTo80Light);

				Array<StackVariable> nodeVariables;

				for (auto& childNodeTree : c->nodeTree.getChildWithName(PropertyIds::Nodes))
				{
					auto createNodeVariable = [&](Node::Ptr containerNode, ValueTree& child, bool initialiseParameters)
					{
						auto cId = child[PropertyIds::ID].toString();

						StackVariable sv(*this, cId, TypeInfo(Types::ID::Dynamic, false, true));

						sv << "get<" << ValueTreeIterator::getIndexInParent(child) << ">()";

						if (initialiseParameters)
						{
							int pIndex = 0;
							auto pTree = child.getChildWithName(PropertyIds::Parameters);

							if (pTree.getNumChildren() > 1)
							{
								sv.flushIfNot();
								addComment(getNodePath(child).toString(), CommentType::AlignOnSameLine);
							}
								
							for (auto& p : pTree)
							{
								if (ValueTreeIterator::isAutomated(p))
								{
									addEmptyLine();
									String comment;
									comment << p[PropertyIds::ID].toString() << " (automated)";
									addComment(comment, Base::CommentType::AlignOnSameLine);
									continue;
								}
									

								String v;
								v << sv.toExpression() << ".setParameter<" << pIndex++ << ">(";
								v << Types::Helpers::getCppValueString(p[PropertyIds::Value], Types::ID::Double);
								v << ");";
								*this << v;

								String comment;

								if (pTree.getNumChildren() == 1)
									comment << getNodePath(child).toString() << "::";

								comment << p[PropertyIds::ID].toString();

								addComment(comment, Base::CommentType::AlignOnSameLine);
							}

							addEmptyLine();
						}

						return sv;
					};

					nodeVariables.add(createNodeVariable(c, childNodeTree, true));
				}

				index = 0;

				auto pTree = c->nodeTree.getChildWithName(PropertyIds::Parameters);

				if (pTree.getNumChildren() > 0)
				{
					addComment("Parameters", Base::CommentType::FillTo80Light);

					for (auto& p : pTree)
					{
						String def;

						auto pId = p[PropertyIds::ID].toString();
						pId << "_p";

						StackVariable pv(*this, pId, TypeInfo(Types::ID::Dynamic, false, true));

						pv << "getParameter<" << ValueTreeIterator::getIndexInParent(p) << ">()";

						auto cTree = p.getChildWithName(PropertyIds::Connections);

						if (cTree.getNumChildren() > 1)
							pv.flushIfNot();

						for (auto& c_ : cTree)
						{
							auto targetId = Identifier(c_[PropertyIds::NodeId].toString());

							String def;
							def << pv.toExpression() << ".connect<" << ValueTreeIterator::getIndexInParent(c_) << ">(";

							bool found = false;

							for (auto& nv : nodeVariables)
							{
								if (nv.scopedId.getIdentifier() == targetId)
								{
									nv.flushIfNot();
									def << nv.toExpression();
									
									found = true;
									break;
								}
							}

							if (!found)
							{
								auto e = getChildNodeAsStackVariable(c->nodeTree, targetId, "_pt");
								
								e.flushIfNot();
								def << e.toExpression();
							}

							def << ");";

							*this << def;
						}

						if(!ValueTreeIterator::isLast(p))
							addEmptyLine();
					}
				}

				if (c->isRootNode())
				{
					auto modList = getModulationNodes(c);

					if (modList.size() > 0)
					{
						addEmptyLine();
						addComment("Modulation connections", Base::CommentType::FillTo80Light);

						for (auto m : modList)
						{
							Array<int> path;

							auto sourceId = m->nodeTree[PropertyIds::ID].toString();

							auto ms = getChildNodeAsStackVariable(c->nodeTree, sourceId, "_ms");

							auto mtargetTree = m->nodeTree.getChildWithName(PropertyIds::ModulationTargets);

							ms.flushIfNot();
							addComment(sourceId, Base::CommentType::AlignOnSameLine);

							for (auto t: mtargetTree)
							{
								auto cn = getConnection(t);

								String def;

								def << ms.toExpression() << ".getParameter().connect<" << ValueTreeIterator::getIndexInParent(t) << ">(";

								auto targetId = cn.n->nodeTree[PropertyIds::ID].toString();

								auto mt = getChildNodeAsStackVariable(c->nodeTree, targetId, "_mt");

								if (mt.getLength() > 20)
								{
									mt.flushIfNot();
									addComment(targetId, Base::CommentType::AlignOnSameLine);
								}
								

								def << mt.toExpression();

								def << ");";

								*this << def;
								
								String comment;
								comment << sourceId << " -> " << targetId;

								addComment(comment, CommentType::AlignOnSameLine);
							}

						}
					}

					
				}
			}
		}

		s.flushIfNot();
	}

	auto n = createNode(c->nodeTree, iId, {});

	return n;
}

PooledParameter::Ptr ValueTreeBuilder::parseParameter(const ValueTree& p, Connection::CableType connectionType)
{
	ValueTree cTree;

	auto pId = p[PropertyIds::ID].toString();

	if (connectionType == Connection::CableType::Parameter)
	{
		cTree = p.getChildWithName(PropertyIds::Connections);
	}
	else
	{
		cTree = p.getChildWithName(PropertyIds::ModulationTargets);

		jassert(cTree.isValid());

		pId << "_mod";
	}

	auto numConnections = cTree.getNumChildren();

	if (numConnections == 0)
		return createParameterFromConnection({}, "empty", -1);

	else if (numConnections == 1)
	{
		return createParameterFromConnection(getConnection(cTree.getChild(0)), pId, -1);
	}
	else
	{
		auto ch = makeParameter(pId, "parameter::chain", {});

		Array<Connection> chainList;

		for (auto c : cTree)
		{
			chainList.add(getConnection(c));
		}

		auto inputRange = RangeHelpers::getDoubleRange(p);

		if(RangeHelpers::isIdentity(inputRange))
			ch->t << "ranges::Identity";
		else
		{
			String fWhat;

			fWhat << pId << "_InputRange";



		}

		int cIndex = 0;
		for (auto& c : chainList)
		{
			auto up = createParameterFromConnection(c, pId, cIndex++);
			ch->t << up->t;
		}

		addIfNotEmptyLine();
		ch->t.flushIfNot();
		addEmptyLine();

		return addParameterAndReturn(ch);
	}
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

	rc.targetRange = RangeHelpers::getDoubleRange(c);
	
	if (c.getType() == PropertyIds::ModulationTarget)
		rc.cableType = Connection::CableType::Modulation;
	else
		rc.cableType = Connection::CableType::Parameter;

	if (pId == PropertyIds::Bypassed.toString())
		rc.index = scriptnode::bypass::ParameterId;

	rc.expressionCode = c[PropertyIds::Expression].toString();

	ValueTreeIterator::forEach(v, ValueTreeIterator::Forward, [&](ValueTree& t)
	{
		if (t.getType() == PropertyIds::Node)
		{
			if (getNodeId(t) == nId)
			{
				auto pTree = t.getChildWithName(PropertyIds::Parameters);
				auto c = pTree.getChildWithProperty(PropertyIds::ID, pId);

				if(rc.getType() != Connection::ConnectionType::Bypass)
					rc.index = pTree.indexOf(c);

				

				if (getNode(t, true) == nullptr)
					usingTemplates.add(parseNode(t));

				return true;
			}
		}

		return false;
	});

	for (auto n : usingTemplates)
	{
		if (n->t.scopedId == nId)
		{
			rc.n = n;
			return rc;
		}
	}

	return {};
}

Node::Ptr ValueTreeBuilder::parseMod(Node::Ptr u)
{
	auto modTree = u->nodeTree.getChildWithName(PropertyIds::ModulationTargets);

	if (modTree.isValid() && modTree.getNumChildren() > 0)
	{
		auto mod = parseParameter(u->nodeTree, Connection::CableType::Modulation);

		mod->t.flushIfNot();

		auto id = getNodeId(u->nodeTree);

		auto w = createNode(u->nodeTree, id.getIdentifier(), "wrap::mod");

		w->t << mod->t;
		w->t << u->t;
		w->t.flushIfNot();
		return w;
	}

	return u;
}



Node::Ptr ValueTreeBuilder::getUsingTemplate(const NamespacedIdentifier& id)
{
	for (auto& c : usingTemplates)
	{
		if (c->t.scopedId == id)
			return c;
	}

	return nullptr;
}



Node::List ValueTreeBuilder::getModulationNodes(Node::Ptr c)
{
	// You need to make the modulation connections on the top level...
	jassert(c->isRootNode());

	Node::List l;

	for (auto u : usingTemplates)
	{
		if (u->nodeTree.getChildWithName(PropertyIds::ModulationTargets).getNumChildren() > 0)
		{
			l.add(u);
		}
	}

	return l;
}

NamespacedIdentifier ValueTreeBuilder::getNodePath(const ValueTree& n)
{
	auto s = n[scriptnode::PropertyIds::FactoryPath].toString().replace(".", "::");
	return NamespacedIdentifier::fromString(s);
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

snex::cppgen::StackVariable ValueTreeBuilder::getChildNodeAsStackVariable(ValueTree& root, const Identifier& id, const String& suffix)
{
	Array<int> path;

	ValueTreeIterator::getNodePath(path, root, id);

	String tid = id.toString();
	tid << suffix;

	StackVariable e(*this, tid, TypeInfo(Types::ID::Dynamic, false, true));

	int index = 0;

	for (auto i : path)
	{
		e << "get<" << i << ">()";

		if (++index != path.size())
			e << JitTokens::dot;
	}

	return e;
}

bool ValueTreeIterator::isBetween(IterationType l, IterationType u, IterationType v)
{
	return v >= l && v <= u;
}

bool ValueTreeIterator::isBackwards(IterationType t)
{
	return t == Backwards || t == ChildrenFirstBackwards || t == OnlyChildrenBackwards;
}

bool ValueTreeIterator::isRecursive(IterationType t)
{
	return !isBetween(OnlyChildren, OnlyChildrenBackwards, t);
}

bool ValueTreeIterator::forEach(ValueTree& v, IterationType type, const std::function<bool(ValueTree& v)>& f)
{
	if (isBetween(Forward, Backwards, type))
	{
		if (f(v))
			return true;
	}

	if (isBackwards(type))
	{
		for (int i = v.getNumChildren() - 1; i >= 0; i--)
		{
			if (isRecursive(type))
			{
				if (forEach(v.getChild(i), type, f))
					return true;
			}
			else
			{
				if (f(v.getChild(i)))
					return true;
			}
		}
	}
	else
	{
		for (auto c : v)
		{
			if (isRecursive(type))
			{
				if (forEach(c, type, f))
					return true;
			}
			else
			{
				if (f(c))
					return true;
			}
		}
	}

	if (isBetween(ChildrenFirst, ChildrenFirstBackwards, type))
	{
		if (f(v))
			return true;
	}

	return false;
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

bool ValueTreeIterator::isAutomated(const ValueTree& parameterTree)
{
	auto root = parameterTree.getRoot();

	auto pId = Identifier(parameterTree[PropertyIds::ID].toString());
	auto nodeId = Identifier(findParentWithType(parameterTree, PropertyIds::Node)[PropertyIds::ID].toString());

	return forEach(root, Forward, [pId, nodeId](ValueTree& v)
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

String ValueTreeIterator::getSnexCode(const ValueTree& nodeTree)
{
	auto propTree = nodeTree.getChildWithName(PropertyIds::Properties);
	auto c = propTree.getChildWithProperty(PropertyIds::ID, PropertyIds::Code.toString());
	return c[PropertyIds::Value].toString();
}

bool ValueTreeBuilder::Sanitizers::setChannelAmount(ValueTree& v)
{
	int numChannels = v[PropertyIds::NumChannels];

	auto n = v.getChildWithName(PropertyIds::Nodes);

	if (n.isValid())
	{
		for (auto c : n)
		{
			if (numChannels < (int)c[PropertyIds::NumChannels])
			{
				c.setProperty(PropertyIds::NumChannels, numChannels, nullptr);
			}

			setChannelAmount(c);
		}
	}

	return false;
}

}



}
