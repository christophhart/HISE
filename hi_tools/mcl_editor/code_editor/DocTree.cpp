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

DocTreeBuilder::Item** DocTreeBuilder::Item::begin() const
{ return const_cast<Item**>(children.begin()); }

DocTreeBuilder::Item** DocTreeBuilder::Item::end() const
{ return const_cast<Item**>(children.end()); }

DocTreeBuilder::Listener::~Listener()
{}

DocTreeBuilder::DocTreeBuilder(TextDocument& t):
	CoallescatedCodeDocumentListener(t.getCodeDocument()),
	doc(t)
{}

DocTreeBuilder::~DocTreeBuilder()
{}

void DocTreeBuilder::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void DocTreeBuilder::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

CodeDocument::Iterator DocTreeBuilder::createIterator() const
{
	return CodeDocument::Iterator(doc.getCodeDocument());
}

DocTreeView::DocTreeViewItem::DocTreeViewItem(DocTreeBuilder::Ptr item_):
	item(item_)
{}

bool DocTreeView::DocTreeViewItem::mightContainSubItems()
{
	return item->begin() != item->end();
}

String DocTreeView::DocTreeViewItem::getUniqueName() const
{
	return item->getPath();
}

void DocTreeView::DocTreeViewItem::itemOpennessChanged(bool isNowOpen)
{
	if (isNowOpen)
	{
		for (auto c : *item)
			addSubItem(new DocTreeViewItem(c));
	}
	else
		clearSubItems();
}

void DocTreeView::DocTreeViewItem::paintItem(Graphics& g, int width, int height)
{
	Font f(Font::getDefaultMonospacedFontName(), 16.0f, Font::plain);
	g.setFont(f);
	g.setColour(Colours::white.withAlpha(0.7f));
	g.drawText(item->name, 0.0f, 0.0f, (float)width, (float)height, Justification::centredLeft);
}

void DocTreeView::treeWasRebuilt(DocTreeBuilder::Ptr newRoot)
{
	tree.setRootItem(nullptr);

	rootItem = new DocTreeViewItem(newRoot);
	tree.setRootItem(rootItem);
	tree.setDefaultOpenness(true);
	tree.setRootItemVisible(false);
	resized();
}

DocTreeView::DocTreeView(TextDocument& doc)
{
	addAndMakeVisible(tree);
}

void DocTreeView::setBuilder(DocTreeBuilder* ownedBuilder)
{
	builder = ownedBuilder;
	builder->addListener(this);
}

void DocTreeView::resized()
{
	tree.setBounds(getLocalBounds());
}

DocTreeView::~DocTreeView()
{
	tree.setRootItem(nullptr);
	rootItem = nullptr;

	if (builder != nullptr)
	{
		builder->removeListener(this);
		builder = nullptr;
	}
}

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