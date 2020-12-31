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

	static int getIndexInParent(const ValueTree& v)
	{
		return v.getParent().indexOf(v);
	}

	static bool isBetween(IterationType l, IterationType u, IterationType v);
	static bool isBackwards(IterationType t);
	static bool isRecursive(IterationType t);
	static bool forEach(ValueTree& v, IterationType type, const Func& f);
};



struct Node : public ReferenceCountedObject
{
	using List = ReferenceCountedArray<Node>;
	using Ptr = ReferenceCountedObjectPtr<Node>;

	Node(UsingTemplate&& t_) :
		t(t_)
	{};

	ValueTree nodeTree;
	UsingTemplate t;
};


struct Connection
{
	enum class CableType
	{
		Parameter,
		Modulation,
		Bypass,
		numCableTypes
	};

	operator bool() const
	{
		return n != nullptr;
	}

	Node::Ptr n;
	int index = -1;
	CableType cableType;
};



struct ValueTreeBuilder: public ValueTree::Listener,
						 public Timer,
						 public Base
{
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
		String toString()
		{
			ScopedPointer<XmlElement> xml = v.createXml();
			String s;
			
			s << errorMessage;
			s << "\nValueTree: \n";
			s << xml->createDocument("", false);
			
			return s;
		}
		ValueTree v;
		String errorMessage;
	};

private:

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

	Node::Ptr parseNode(const ValueTree& n);
	
	Node::Ptr parseContainer(Node::Ptr u);
	void parseContainerChildren(Node::Ptr container);
	void parseContainerParameters(Node::Ptr container);
	Node::Ptr parseContainerInitialiser(Node::Ptr container);

	Node::Ptr parseMod(Node::Ptr u);

	Node::Ptr parseParameter(const ValueTree& p, Connection::CableType connectionType);

	Connection getConnection(const ValueTree& c);

	Node::Ptr getUsingTemplate(const NamespacedIdentifier& id);

	Node::List usingTemplates;

	static NamespacedIdentifier getNodePath(const ValueTree& n);

	NamespacedIdentifier getNodeId(const ValueTree& n);
	NamespacedIdentifier getNodeVariable(const ValueTree& n);

	ValueTree v;
};





}
}
