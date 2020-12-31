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
			Namespace impl(*this, "impl");
			addComment("Node declarations");
			usingTemplates.add(parseNode(v));
			impl.flush();
		}
		
		UsingTemplate instance(*this, "instance", usingTemplates.getLast()->t.scopedId);
		instance.flush();
	}
	catch (Error& e)
	{
		DBG(e.toString());
	}
	
	DBG(toString());

	jassertfalse;
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

Node::Ptr ValueTreeBuilder::parseNode(const ValueTree& n)
{
	auto typeId = getNodeId(n);

	Node::Ptr newNode = createNode(n, typeId.getIdentifier(), getNodePath(n).toString());
	return parseContainer(newNode);
}

Node::Ptr ValueTreeBuilder::parseContainer(Node::Ptr u)
{
	if (FactoryIds::isContainer(getNodePath(u->nodeTree)))
	{
		for (auto c : u->nodeTree.getChildWithName(PropertyIds::Nodes))
			usingTemplates.add(parseNode(c));

		parseContainerParameters(u);
		parseContainerChildren(u);

		u = parseContainerInitialiser(u);

		u->t.flush();

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

	if(numParameters == 0)
		c->t << "parameter::empty";
	else if (numParameters == 1)
	{
		Node::Ptr up = parseParameter(pTree.getChild(0), Connection::CableType::Parameter);
		up->t.flush();
		c->t << up->t;
	}
	else
	{
		addComment("parameter list for " + c->t.scopedId.toString());

		Node::List parameterList;

		for (auto c : pTree)
			parameterList.add(parseParameter(c, Connection::CableType::Parameter));

		String pId;
		pId << c->t.scopedId.getIdentifier();
		pId << "_plist";

		Node::Ptr l = createNode(pTree, pId, "parameter::list");

		for (auto p : parameterList)
		{
			p->t.flush();
			l->t << p->t;
		}

		l->t.flush();
		c->t << l->t;
	}
}

Node::Ptr ValueTreeBuilder::parseContainerInitialiser(Node::Ptr c)
{
	auto n = createNode(c->nodeTree, c->t.scopedId.getIdentifier(), "wrap::init");

	auto wId = c->t.scopedId.getIdentifier().toString();
	wId << "_inner";

	c->t.scopedId = c->t.scopedId.getParent().getChildId(wId);
	c->t.flush();

	String iId;
	
	iId << n->t.scopedId.getIdentifier();
	iId << "_init";

	{
		addEmptyLine();
		Struct s(*this, iId, {});

		{
			String f;
			f << iId << "(" << c->t.scopedId.toString() << "& obj)";
			*this << f;
			{
				StatementBlock sb(*this);

				int index = 0;

				addComment("child nodes");

				Array<StackVariable> nodeVariables;

				for (auto& childNodeTree : c->nodeTree.getChildWithName(PropertyIds::Nodes))
				{
					auto createNodeVariable = [&](Node::Ptr containerNode, ValueTree& child, bool initialiseParameters)
					{
						auto cId = child[PropertyIds::ID].toString();

						StackVariable sv(*this, cId, TypeInfo(Types::ID::Dynamic, false, true));

						sv << "obj.get<" << ValueTreeIterator::getIndexInParent(child) << ">()";

						if (initialiseParameters)
						{
							int pIndex = 0;
							auto pTree = child.getChildWithName(PropertyIds::Parameters);

							if (pTree.getNumChildren() > 1)
								sv.flush();

							for (auto& p : pTree)
							{
								String v;
								v << sv.toString() << ".setParameter<" << pIndex++ << ">(";
								v << Types::Helpers::getCppValueString(p[PropertyIds::Value], Types::ID::Double);
								v << ");";
								v << "\t// ";
								
								v << getNodePath(child).toString() << "::";
								
								v << p[PropertyIds::ID].toString();
								*this << v;
							}

							if (pTree.getNumChildren() > 0)
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
					addComment("parameters");

					for (auto& p : pTree)
					{
						String def;

						auto pId = p[PropertyIds::ID].toString();
						pId << "_p";

						StackVariable pv(*this, pId, TypeInfo(Types::ID::Dynamic, false, true));

						pv << "obj.getParameter<" << ValueTreeIterator::getIndexInParent(p) << ">()";

						int cIndex = 0;

						auto cTree = p.getChildWithName(PropertyIds::Connections);

						if (cTree.getNumChildren() > 1)
							pv.flush();

						for (auto& c : cTree)
						{
							auto cn = getConnection(c);

							String def;
							def << pv.toString() << ".connect<" << cIndex++ << ">(";

							auto tId = getNodeVariable(cn.n->nodeTree);
							def << tId.toString();
							def << ");";

							*this << def;
						}

						addEmptyLine();
					}
				}
			}
		}
	}

	n->t << c->t;
	n->t << iId;

	return n;
}

Node::Ptr ValueTreeBuilder::parseParameter(const ValueTree& p, Connection::CableType connectionType)
{
	ValueTree cTree;

	auto pId = p[PropertyIds::ID].toString();

	if (connectionType == Connection::CableType::Parameter)
	{
		cTree = p.getChildWithName(PropertyIds::Connections);
		pId << "_param";
	}
	else
	{
		cTree = p.getChildWithName(PropertyIds::ModulationTargets);

		jassert(cTree.isValid());

		pId << "_mod";
	}

	auto numConnections = cTree.getNumChildren();

	if (numConnections == 0)
		return createNode({}, pId, "parameter::empty");

	else if (numConnections == 1)
	{
		if (auto c = getConnection(cTree.getChild(0)))
		{
			auto up = createNode({}, pId, "parameter::plain");
			up->t << c.n->t;
			up->t << c.index;

			return up;
		}
		else
		{
			Error e;
			e.v = cTree.getChild(0);
			e.errorMessage = "Can't find connection";
			throw e;
		}
	}
	else
	{
		auto ch = createNode({}, pId, "parameter::chain");

		Array<Connection> chainList;

		for (auto c : cTree)
		{
			chainList.add(getConnection(c));
		}

		for (auto& c : chainList)
		{
			auto up = createNode({}, {}, "parameter::plain");
			up->t << c.n->t;
			up->t << c.index;
			ch->t << up->t;
		}

		return ch;
	}
}

Connection ValueTreeBuilder::getConnection(const ValueTree& c)
{
	String id = "impl::";
	id << c[PropertyIds::NodeId].toString();
	id << "_type";

	auto pId = c[PropertyIds::ParameterId].toString();
	auto nId = NamespacedIdentifier::fromString(id);

	Connection rc;

	if (c.getType() == PropertyIds::ModulationTarget)
		rc.cableType = Connection::CableType::Modulation;
	else
		rc.cableType = Connection::CableType::Parameter;

	ValueTreeIterator::forEach(v, ValueTreeIterator::Forward, [&](ValueTree& t)
	{
		if (t.getType() == PropertyIds::Node)
		{
			if (getNodeId(t) == nId)
			{
				auto pTree = t.getChildWithName(PropertyIds::Parameters);
				auto c = pTree.getChildWithProperty(PropertyIds::ID, pId);
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

		mod->t.flush();

		auto id = getNodeId(u->nodeTree);

		auto w = createNode(u->nodeTree, id.getIdentifier(), "wrap::mod");

		w->t << mod->t;
		w->t << u->t;
		w->t.flush();
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

NamespacedIdentifier ValueTreeBuilder::getNodePath(const ValueTree& n)
{
	auto s = n[scriptnode::PropertyIds::FactoryPath].toString().replace(".", "::");
	return NamespacedIdentifier::fromString(s);
}



NamespacedIdentifier ValueTreeBuilder::getNodeId(const ValueTree& n)
{
	auto typeId = n[scriptnode::PropertyIds::ID].toString();
	typeId << "_type";
	return getCurrentScope().getChildId(typeId);
}

NamespacedIdentifier ValueTreeBuilder::getNodeVariable(const ValueTree& n)
{
	auto typeId = n[scriptnode::PropertyIds::ID].toString();
	return getCurrentScope().getChildId(typeId);
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
