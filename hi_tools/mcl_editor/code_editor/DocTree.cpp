/** ============================================================================
 *
 * MCL Text Editor JUCE module 
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */

namespace mcl
{
using namespace juce;

void DocTreeBuilder::Item::clearChildren()
{
	children.clear();
}

// insert stuff

bool DocTreeBuilder::Item::forEach(const std::function<bool(Item*)>& f)
{
	if (f(this))
		return true;

	for (auto c : *this)
	{
		if (c->forEach(f))
			return true;
	}

	return false;
}

juce::ValueTree DocTreeBuilder::Item::toValueTree() const
{
	ValueTree v("Item");
	v.setProperty("ID", name, nullptr);
	v.setProperty("Line", lineNumber, nullptr);

	for (auto c : children)
		v.addChild(c->toValueTree(), -1, nullptr);

	return v;
}

juce::String DocTreeBuilder::Item::getPath() const
{
	StringArray p;

	p.add(name);

	Item* c = parent;

	while (c != nullptr)
	{
		p.add(c->name);
		c = c->parent;
	}

	String path;

	for (int i = p.size() - 1; i >= 0; i--)
	{
		path << p[i];

		if (i != 0)
			path << "::";
	}

	return path;
}

mcl::DocTreeBuilder::Item* DocTreeBuilder::Item::getParent() const
{
	return parent.get();
}

void DocTreeBuilder::Item::addChild(Item* n)
{
	children.add(n);
	n->parent = this;
}

void DocTreeBuilder::codeChanged(bool, int, int)
{
	currentRoot = createItems();

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->treeWasRebuilt(currentRoot);
	}
}

}