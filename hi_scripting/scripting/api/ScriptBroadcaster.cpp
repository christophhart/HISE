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

namespace hise { using namespace juce;

namespace ScriptingObjects
{

struct ComponentValueDisplay : public MapItemWithScriptComponentConnection
{
	ComponentValueDisplay(ScriptComponent* c) :
		MapItemWithScriptComponentConnection(c, 170, 52)
	{};

	void timerCallback() override
	{
		if (lastValue.setModValueIfChanged(sc != nullptr ? sc->getValueNormalized() : 0.0))
		{
			alpha.setModValue(1.0);
		}

		if (alpha.setModValueIfChanged(jmax(0.0, alpha.getModValue() - 0.05)))
			repaint();
	}

	ModValue lastValue, alpha;

	void paint(Graphics& g) override
	{
		if (sc == nullptr)
			return;

		auto b = getLocalBounds().reduced(8);

		auto valueArea = b.removeFromRight(50).withSizeKeepingCentre(45, 14).toFloat();

		auto top = b.removeFromTop(b.getHeight() / 2).toFloat();
		auto bottom = b.toFloat();

		g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(sc->get("text"), top, Justification::left);
		g.setFont(GLOBAL_MONOSPACE_FONT());
		g.setColour(Colours::white.withAlpha(0.3f));
		g.drawText(sc->getObjectName().toString(), bottom, Justification::left);

		g.setColour(Colours::white.withAlpha(0.8f * (float)alpha.getModValue() + 0.2f));

		g.drawRoundedRectangle(valueArea, valueArea.getHeight() / 2.0f, 1.0f);
		valueArea = valueArea.reduced(3.0f);

		g.fillRoundedRectangle(valueArea.removeFromLeft(jmax<float>(valueArea.getHeight(), lastValue.getModValue() * valueArea.getWidth())), valueArea.getHeight() / 2.0f);
	}

	static ComponentWithPreferredSize* create(Component* c, const var& v)
	{
		if (auto sc = dynamic_cast<ScriptComponent*>(v.getObject()))
			return new ComponentValueDisplay(sc);

		return nullptr;
	}
};

struct BroadcasterHelpers
{
	static int getNumArgs(const var& defaultValue)
	{
		if (defaultValue.isArray())
			return defaultValue.size();
		else if (auto obj = defaultValue.getDynamicObject())
			return obj->getProperties().size();
		else
			return 1;
	}

	static var getArg(const var& v, int idx)
	{
		if (v.isArray())
			return v[idx];

		jassert(idx == 0);
		return v;
	}

	static var getListOrFirstElement(const var& l)
	{
		if (!l.isArray() || l.size() != 1)
			return l;

		return l[0];
	}

	static var getIdListAsVar(const Array<Identifier>& propertyIds)
	{
		Array<var> list;

		for (const auto& id : propertyIds)
			list.add(id.toString());

		return var(list);
	}

	static Array<Identifier> getIdListFromVar(const var& propertyIds)
	{
		Array<Identifier> idList;

		idList.add(Identifier(getArg(propertyIds, 0).toString()));

		if (propertyIds.isArray())
		{
			for (int i = 1; i < propertyIds.size(); i++)
				idList.add(Identifier(getArg(propertyIds, i).toString()));
		}

		return idList;
	}

	static bool isValidArg(var valueOrList, int index = -1)
	{
		if (valueOrList.isArray() && index != -1)
			valueOrList = valueOrList[index];

		auto v = valueOrList.isVoid();
		auto u = valueOrList.isUndefined();
			
		return !v && !u;
	}

	static Identifier getIllegalProperty(Array<ScriptingApi::Content::ScriptComponent*>& componentList, const Array<Identifier>& propertyIds)
	{
		for (auto sc : componentList)
		{
			for (const auto& id : propertyIds)
			{
				if (sc->getIndexForProperty(id) == -1)
				{
					return id;
				}
			}
		}

		return {};
	}

	static Array<ScriptingApi::Content::ScriptComponent*> getComponentsFromVar(ProcessorWithScriptingContent* p, var componentIds)
	{
		using ScriptComp = ScriptingApi::Content::ScriptComponent;

		Array<ScriptComp*> list;

		auto content = p->getScriptingContent();

		auto getComponentFromSingleVar = [&](const var& v)
		{
			ScriptComp* p = nullptr;

			if (v.isString())
				p = content->getComponentWithName(Identifier(v.toString()));

			else if (v.isObject())
				p = dynamic_cast<ScriptComp*>(v.getObject());

			return p;
		};

		if (componentIds.isArray())
		{
			for (auto& v : *componentIds.getArray())
				list.add(getComponentFromSingleVar(v));
		}
		else
			list.add(getComponentFromSingleVar(componentIds));

		for (int i = 0; i < list.size(); i++)
		{
			if (list[i] == nullptr)
				list.remove(i--);
		}

		return list;
	}

	static void callForEachIfArray(const var& obj, const std::function<bool(const var& d)>& f)
	{
		if (obj.isArray())
		{
			for (const auto& v : *obj.getArray())
			{
				if (!f(v))
					break;
			}
		}
		else
			f(obj);
	}
	
	

};

struct ComponentPropertyMapItem : public hise::MapItemWithScriptComponentConnection
{
	ComponentPropertyMapItem(ScriptComponent* sc, const Array<Identifier>& ids) :
		MapItemWithScriptComponentConnection(sc, 180, 24)
	{
		auto updater = sc->getScriptProcessor()->getMainController_()->getGlobalUIUpdater();
		WeakReference<ScriptComponent> weakRef(sc);

		childLayout = ComponentWithPreferredSize::Layout::ChildrenAreRows;

		for (auto id : ids)
		{
			addChildWithPreferredSize(new LiveUpdateVarBody(updater, id, [weakRef, id]()
				{
					if (weakRef != nullptr)
					{
						return weakRef->getScriptObjectProperty(id);
					}

					return var();
				}));
		}

		marginTop = 24;
		marginLeft = 5;
		marginRight = 5;
		marginBottom = 5;
	}

	int getPreferredWidth() const override { return getMaxWidthOfChildComponents(this); };
	int getPreferredHeight() const override { return getSumOfChildComponentHeight(this); };

	void resized() override
	{
		resizeChildren(this);
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());

		if (sc != nullptr)
			g.drawText(sc->getName().toString(), getLocalBounds().removeFromTop(28).toFloat(), Justification::centred);
	}

	void timerCallback() override
	{

	};

	static ComponentWithPreferredSize* create(Component* root, const var& v)
	{
		if (auto obj = v.getDynamicObject())
		{
			auto c = obj->getProperty("component");
			auto p = obj->getProperty("properties");

			if (auto sc = dynamic_cast<ScriptComponent*>(c.getObject()))
				return new ComponentPropertyMapItem(sc, BroadcasterHelpers::getIdListFromVar(p));
		}

		return nullptr;
	}
};

struct ScriptBroadcaster::Display: public Component,
													 public ComponentForDebugInformation,
													 public PooledUIUpdater::SimpleTimer,
													 public Label::Listener,
													 public PathFactory
{
	static constexpr int HeaderHeight = 28;
	static constexpr int RowHeight = 28;
	static constexpr int Width = 400;

	Display(ScriptBroadcaster* sb) :
		ComponentForDebugInformation(sb, dynamic_cast<ApiProviderBase::Holder*>(sb->getScriptProcessor())),
		SimpleTimer(sb->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
		resetButton("reset", nullptr, *this),
		breakpointButton("breakpoint", nullptr, *this),
		input()
	{
		setName(getTitle());
		rebuild(*sb);

		resetButton.onClick = [this]()
		{
			if (auto obj = getObject<ScriptBroadcaster>())
			{
				obj->reset();
			}
		};

		breakpointButton.setToggleModeWithColourChange(true);

		breakpointButton.onClick = [this]()
		{
			if (auto obj = getObject<ScriptBroadcaster>())
			{
				obj->triggerBreakpoint = breakpointButton.getToggleState();
			}
		};

		addAndMakeVisible(resetButton);
		addAndMakeVisible(breakpointButton);

        resetButton.setTooltip("Reset to initial value");
        breakpointButton.setTooltip("Set a breakpoint when a message is sent");
        
		input.setColour(TextEditor::ColourIds::textColourId, Colours::black);
		input.setColour(Label::ColourIds::backgroundColourId, Colours::white.withAlpha(0.35f));
		input.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
		input.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
		input.setColour(TextEditor::ColourIds::outlineColourId, Colours::black.withAlpha(0.8f));
		input.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
		input.setFont(GLOBAL_BOLD_FONT());

		input.setEditable(true, true);
		addAndMakeVisible(input);
		input.setFont(GLOBAL_MONOSPACE_FONT());
		input.addListener(this);
	};

	String getTextEditorAsCode() const
	{
		String c = "[";
		c << input.getText() << "]";
		return c;
	}

	

	void labelTextChanged(Label*) override
	{
		auto c = getTextEditorAsCode();

		auto r = Result::ok();

		juce::JavascriptEngine engine;
		auto values = engine.evaluate(c, &r);

		if (r.wasOk())
		{
			if (auto obj = getObject<ScriptBroadcaster>())
			{
				try
				{
					obj->sendAsyncMessage(values);
				}
				catch (String& message)
				{
					r = Result::fail(message);
				}
			}
		}

		if (!r.wasOk())
			PresetHandler::showMessageWindow("Error at evaluating input", r.getErrorMessage(), PresetHandler::IconType::Error);
	}

	void updateTextEditor(ScriptBroadcaster& b)
	{
		if (input.isBeingEdited())
			return;

		auto t = JSON::toString(var(b.lastValues), true);
		input.setText(t.fromFirstOccurrenceOf("[", false, false).upToLastOccurrenceOf("]", false, false), dontSendNotification);
	}

	struct Row : public Component
	{
		Row(TargetBase* i, Display& parent, JavascriptProcessor* jp_) :
			item(i),
			jp(jp_),
			gotoButton("workspace", nullptr, parent),
			powerButton("enable", nullptr, parent)
		{
			if(dynamic_cast<DelayedItem*>(i) != nullptr)
				delayIcon = parent.createPath("delay");

			gotoButton.onClick = [this]()
			{
				if(item != nullptr)
					DebugableObject::Helpers::gotoLocation(nullptr, jp, item->location);
			};

			powerButton.onClick = [this]()
			{
				if(item != nullptr)
					item->enabled = powerButton.getToggleState();
			};

			powerButton.setToggleModeWithColourChange(true);
			powerButton.setToggleStateAndUpdateIcon(i->enabled);

			addAndMakeVisible(gotoButton);
			addAndMakeVisible(powerButton);
		}

		String getText() const
		{
			if (item == nullptr)
				return "Dangling";

			auto o = item->obj;

			if (o.isString())
				return o.toString();

			if (auto d = o.getDynamicObject())
				return JSON::toString(o, true);

			if (auto d = dynamic_cast<DebugableObjectBase*>(o.getObject()))
				return d->getDebugName();

			return {};
		}

		void paint(Graphics& g) override
		{
			bool delayActive = false;

			if (auto d = dynamic_cast<DelayedItem*>(item.get()))
				delayActive = d->delayedFunction != nullptr && d->delayedFunction->isTimerRunning();

			auto b = getLocalBounds().toFloat().reduced(1.0f);

			
			g.setColour(Colours::white.withAlpha(0.1f));
			
			

			g.fillRoundedRectangle(b, 3.0f);
			g.drawRoundedRectangle(b, 3.0f, 1.0f);

			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.setColour(Colours::white.withAlpha(.7f));

			b.removeFromLeft(RowHeight);

			if (!delayIcon.isEmpty())
			{
				scalePath(delayIcon, b.removeFromLeft(RowHeight).reduced(3));
				g.setColour(delayActive ? Colour(SIGNAL_COLOUR) : Colours::white.withAlpha(0.3f));
				g.fillPath(delayIcon);
				b.removeFromLeft(10);
			}
			
			g.setColour(Colours::white.withAlpha(.7f));
			g.drawText(getText(), b.reduced(10.0f, 0.0f), Justification::left);
		}

		void resized() override
		{
			powerButton.setBounds(getLocalBounds().removeFromLeft(RowHeight).reduced(3));
			gotoButton.setBounds(getLocalBounds().removeFromRight(RowHeight).reduced(3));
		}

		JavascriptProcessor* jp;
		HiseShapeButton gotoButton;
		HiseShapeButton powerButton;
		WeakReference<TargetBase> item;
		Path delayIcon;
	};

	void rebuild(ScriptBroadcaster& b)
	{
		rows.clear();

		auto jp = dynamic_cast<JavascriptProcessor*>(b.getScriptProcessor());

		for (auto i : b.items)
		{
			rows.add(new Row(i, *this, jp));
			addAndMakeVisible(rows.getLast());
		}

		setSize(Width, HeaderHeight + RowHeight * rows.size() + 32);
		resized();
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(HeaderHeight);

		resetButton.setBounds(top.removeFromLeft(HeaderHeight).reduced(4));
		breakpointButton.setBounds(top.removeFromLeft(HeaderHeight).reduced(4));

		for (auto r : rows)
			r->setBounds(b.removeFromTop(RowHeight));

		b.removeFromTop(5);

		currentLabel = b.removeFromLeft(95).toFloat();
		b.removeFromLeft(5);

		input.setBounds(b);
	}

	Path createPath(const String& url) const override
	{
		Path p;

		LOAD_PATH_IF_URL("workspace", ColumnIcons::openWorkspaceIcon);
		LOAD_PATH_IF_URL("reset", ColumnIcons::resetIcon);
		LOAD_PATH_IF_URL("breakpoint", ColumnIcons::breakpointIcon);
		LOAD_PATH_IF_URL("enable", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
		LOAD_PATH_IF_URL("delay", ColumnIcons::delayIcon);

		return p;
	}

	void timerCallback() override
	{
		if (auto obj = getObject<ScriptBroadcaster>())
		{
			if ((rows.size() != obj->items.size()) || lastOne == nullptr)
			{
				rebuild(*obj.obj);
			}

			lastOne = obj.obj;

			updateTextEditor(*obj.obj);

			if (obj->lastMessageTime != lastTime)
				alpha = 1.0f;
			else
				alpha *= 0.8f;

			lastTime = obj->lastMessageTime;

			repaint();
		}
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds().removeFromTop(HeaderHeight);

		g.setColour(Colours::white.withAlpha(alpha * 0.3f));
		g.fillRect(b);

		g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Current Value: ", currentLabel, Justification::right);

		if (auto obj = getObject<ScriptBroadcaster>())
		{
			if (!obj->attachedListeners.isEmpty())
				g.drawText("Source: " + obj->attachedListeners.getFirst()->getItemId().toString(), b.toFloat(), Justification::centred);
		}
	}

	OwnedArray<Row> rows;

	WeakReference<ScriptBroadcaster> lastOne;

	Rectangle<float> currentLabel;
	Label input;

	uint32 lastTime = 0;
	float alpha = 0.0f;

	HiseShapeButton resetButton;
	HiseShapeButton breakpointButton;
};

struct ScriptBroadcaster::ComponentPropertyListener::InternalListener
{
	InternalListener(ScriptBroadcaster& parent_, ScriptingApi::Content::ScriptComponent* sc_, const Array<Identifier>& propertyIds) :
		parent(parent_),
		sc(sc_)
	{
		args.add(var(sc_));
		args.add("");
		args.add(0);

		keeper = var(args);

		for (const auto& id : propertyIds)
			idSet.set(id, id.toString());

		listener.setCallback(sc->getPropertyValueTree(), propertyIds, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(InternalListener::update));
	}

	Identifier illegalId;

	void update(const Identifier& id, var newValue)
	{
		if (newValue.isUndefined() || newValue.isVoid())
		{
			newValue = dynamic_cast<ScriptComponent*>(args[0].getObject())->getScriptObjectProperty(id);
		}

		args.set(1, idSet[id]);
		args.set(2, newValue);

		parent.sendMessageInternal(var(args), false);
	}

	NamedValueSet idSet;
	Array<var> args;

	WeakReference<ScriptComponent> sc;

	var keeper;
	ScriptBroadcaster& parent;
	String id;

	valuetree::PropertyListener listener;
};

Array<juce::var> ScriptBroadcaster::ComponentPropertyListener::createChildArray() const
{
	Array<var> list;

	for (auto i : items)
	{
		DynamicObject::Ptr obj = new DynamicObject();
		obj->setProperty("component", var(i->sc.get()));
		obj->setProperty("properties", BroadcasterHelpers::getIdListAsVar(propertyIds));
		list.add(var(obj.get()));
	}

	return list;
}

ScriptBroadcaster::ComponentPropertyListener::ComponentPropertyListener(ScriptBroadcaster* b, var componentIds, const Array<Identifier>& propertyIds_, const var& metadata):
	ListenerBase(metadata),
	propertyIds(propertyIds_)
{
	auto list = BroadcasterHelpers::getComponentsFromVar(b->getScriptProcessor(), componentIds);

	illegalId = BroadcasterHelpers::getIllegalProperty(list, propertyIds_);

	for (auto sc : list)
		items.add(new InternalListener(*b, sc, propertyIds));
}

int ScriptBroadcaster::ComponentPropertyListener::getNumInitialCalls() const
{
	int numProperties = propertyIds.size();
	int i = 0;

	for (auto item : items)
    {
        ignoreUnused(item);
		i += numProperties;
    }

	return i;
}

Array<juce::var> ScriptBroadcaster::ComponentPropertyListener::getInitialArgs(int callIndex) const
{
	int i = 0;

	Array<var> args = { var(), var(), var() };

	for (auto item : items)
	{
		for (auto p : propertyIds)
		{
			if (i++ == callIndex)
			{
				args.set(0, var(item->sc.get()));
				args.set(1, var(p.toString()));
				args.set(2, item->sc->getScriptObjectProperty(p));

				return args;
			}
		}
	}

	return args;
}

void ScriptBroadcaster::ComponentPropertyListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerWithCreate<ComponentPropertyMapItem>();
}



juce::Result ScriptBroadcaster::ComponentPropertyListener::callItem(TargetBase* n)
{
	Array<var> args;
	args.add(0);
	args.add(0);
	args.add(0);
	
	for (auto i : items)
	{
		args.set(0, var(i->sc.get()));

		for (const auto& id : i->idSet)
		{
			auto v = i->sc->getScriptObjectProperty(id.name);

			args.set(1, id.value);
			args.set(2, v);

			auto ok = n->callSync(args);

			if (!ok.wasOk())
				return ok;
		}
	}
	
	return Result::ok();
}


struct ScriptBroadcaster::ComponentVisibilityListener::InternalListener
{
	InternalListener(ScriptBroadcaster& parent_, ScriptComponent* sc_):
		sc(sc_),
		parent(parent_),
		componentTree(sc_->getPropertyValueTree()),
		id("visible")
	{
		auto root = componentTree.getRoot();
		listener.setCallback(root, { id }, valuetree::AsyncMode::Synchronously, BIND_MEMBER_FUNCTION_2(InternalListener::update));
	}

	Array<var> getArgs() const
	{
		auto v = componentTree;

		auto isVisible = true;

		while (isVisible && v.getType() == Identifier("Component"))
		{
			isVisible &= (bool)v.getProperty(id, true);
			v = v.getParent();
		}

		return { componentTree["id"], var(isVisible) };
	}

	void update(const ValueTree& tree, const Identifier&)
	{
		if (tree == componentTree || componentTree.isAChildOf(tree))
		{
			parent.sendAsyncMessage(getArgs());
		}
	}

	const Identifier id;
	ValueTree d;
	
	WeakReference<ScriptComponent> sc;
	ScriptBroadcaster& parent;

	ValueTree componentTree;
	valuetree::RecursivePropertyListener listener;
};

ScriptBroadcaster::ComponentVisibilityListener::ComponentVisibilityListener(ScriptBroadcaster* b, var componentIds, const var& metadata):
	ListenerBase(metadata)
{
	auto list = BroadcasterHelpers::getComponentsFromVar(b->getScriptProcessor(), componentIds);

	for (auto sc : list)
		items.add(new InternalListener(*b, sc));
}

Array<juce::var> ScriptBroadcaster::ComponentVisibilityListener::getInitialArgs(int callIndex) const
{
	if (auto item = items[callIndex])
		return item->getArgs();

	return {};
}


void ScriptBroadcaster::ComponentVisibilityListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerWithCreate<ComponentPropertyMapItem>();
}



juce::Result ScriptBroadcaster::ComponentVisibilityListener::callItem(TargetBase* n)
{
	for (auto item : items)
	{
		auto ok = n->callSync(item->getArgs());

		if (!ok.wasOk())
			return ok;
	}

	return Result::ok();
}

Array<juce::var> ScriptBroadcaster::ComponentVisibilityListener::createChildArray() const
{
	Array<var> list;

	for (auto i : items)
		list.add(var(i->sc));
	
	return list;
}

ScriptBroadcaster::ScriptTarget::ScriptTarget(ScriptBroadcaster* sb, int numArgs, const var& obj_, const var& f, const var& metadata_) :
	TargetBase(obj_, f, metadata_),
	callback(sb->getScriptProcessor(), sb, f, numArgs)
{
	metadata.attachCommentFromCallableObject(f);
	callback.incRefCount();
}





void ScriptBroadcaster::ScriptTarget::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerWithCreate<PrimitiveArrayDisplay>();
}



Array<juce::var> ScriptBroadcaster::ScriptTarget::createChildArray() const
{
	Array<var> list;

	if (!obj.isArray() || isPrimitiveArray(obj))
		list.add(obj);
	else
		list.addArray(*obj.getArray());

	return list;
}

Result ScriptBroadcaster::ScriptTarget::callSync(const Array<var>& args)
{
#if USE_BACKEND
	if (!enabled)
		return Result::ok();
#endif

#if JUCE_DEBUG
	for (const auto& v : args)
	{
		// This should never happen...
		jassert(!v.isUndefined() && !v.isVoid());
	}
#endif

	auto a = var::NativeFunctionArgs(obj, args.getRawDataPointer(), args.size());
	return callback.callSync(a, nullptr);
}

ScriptBroadcaster::DelayedItem::DelayedItem(ScriptBroadcaster* bc, const var& obj_, const var& f_, int milliseconds, const var& metadata) :
	TargetBase(obj_, f_, metadata),
	ms(milliseconds),
	f(f_),
	parent(bc)
{

}

Result ScriptBroadcaster::DelayedItem::callSync(const Array<var>& args)
{
	delayedFunction = new DelayedFunction(parent, f, parent->lastValues, ms, obj);
	return Result::ok();
}

ScriptBroadcaster::OtherBroadcasterTarget::OtherBroadcasterTarget(ScriptBroadcaster* parent_, ScriptBroadcaster* target_, const var& transformFunction, bool async_, const var& metadata):
	TargetBase(var(target_), transformFunction, metadata),
	parent(parent_),
	target(target_),
	argTransformFunction(parent->getScriptProcessor(), parent_, transformFunction, parent->defaultValues.size()),
	async(async_)
{
	argTransformFunction.incRefCount();
	
}

juce::Result ScriptBroadcaster::OtherBroadcasterTarget::callSync(const Array<var>& args)
{
	if (target == nullptr)
		return Result::fail("no broadcaster");

	if (argTransformFunction)
	{
		var rv;

		var::NativeFunctionArgs a(var(parent.get()), args.getRawDataPointer(), args.size());
		auto ok = argTransformFunction.callSync(a, &rv);

		if (!ok.wasOk())
			return ok;

		if (rv.isArray())
		{
			target->sendMessageInternal(rv, async);
			return target->lastResult;
		}
	}
	else
	{
		target->sendMessageInternal(var(args), async);
		return target->lastResult;
	}
    
    return Result::ok();
}




struct ScriptBroadcaster::ModuleParameterListener::ProcessorListener : public SafeChangeListener
{
	ProcessorListener(ScriptBroadcaster* sb_, Processor* p_, const Array<int>& parameterIndexes_) :
		parameterIndexes(parameterIndexes_),
		p(p_),
		sb(sb_)
	{
		for (auto pi : parameterIndexes)
		{
			lastValues.add(p->getAttribute(pi));
			parameterNames.add(p->getIdentifierForParameterIndex(pi).toString());
		}

		args.add(p->getId());
		args.add(0);
		args.add(0.0f);

		p->addChangeListener(this);
	}

	~ProcessorListener()
	{
		if (p != nullptr)
			p->removeChangeListener(this);
	}

	void changeListenerCallback(SafeChangeBroadcaster *b) override
	{
		if (p == nullptr)
			return;

		for (int i = 0; i < parameterIndexes.size(); i++)
		{
			auto newValue = p->getAttribute(parameterIndexes[i]);

			if (lastValues[i] != newValue)
			{
				lastValues.set(i, newValue);
				args.set(1, parameterNames[i]);
				args.set(2, newValue);

				try
				{
					sb->sendAsyncMessage(args);
				}
				catch (String& s)
				{
					debugError(dynamic_cast<Processor*>(sb->getScriptProcessor()), s);
				}

			}
		}
	}

	Array<var> args;

	WeakReference<ScriptBroadcaster> sb;
	WeakReference<Processor> p;
	Array<float> lastValues;
	Array<var> parameterNames;
	const Array<int> parameterIndexes;
};

ScriptBroadcaster::ModuleParameterListener::ModuleParameterListener(ScriptBroadcaster* b, const Array<WeakReference<Processor>>& processors, const Array<int>& parameterIndexes, const var& metadata):
	ListenerBase(metadata)
{
	for (auto& p : processors)
		listeners.add(new ProcessorListener(b, p, parameterIndexes));
}

void ScriptBroadcaster::ModuleParameterListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	struct ModuleConnectionViewer : public Component,
									public ComponentWithPreferredSize
	{
		struct ParameterConnection : public Component,
									 public ComponentWithPreferredSize,
									 PooledUIUpdater::SimpleTimer
		{
			ParameterConnection(Processor* p, int index_):
				SimpleTimer(p->getMainController()->getGlobalUIUpdater()),
				index(index_),
				processor(p)
			{

			}

			void timerCallback() override
			{
				if (processor != nullptr)
				{
					if (value.setModValueIfChanged(processor->getAttribute(index)))
						alpha.setModValue(1.0);
				}

				if (alpha.setModValueIfChanged(jmax(0.0, alpha.getModValue() - 0.05f)))
				{
					repaint();
				}
			}

			String getText() const
			{
				String t;

				if (processor != nullptr)
				{
					t << processor->getId();
					t << ".";
					t << processor->getIdentifierForParameterIndex(index).toString();
					t << ": ";
					t << String(processor->getAttribute(index), 1);
				}

				return t;
			}

			void paint(Graphics& g) override
			{
				auto t = getText();

				auto b = getLocalBounds().toFloat().reduced(4.0f);

				auto a = (float)jlimit(0.0, 1.0, 0.3 + 0.7 * alpha.getModValue());

				g.setColour(Colours::white.withAlpha(a));
				g.setFont(GLOBAL_MONOSPACE_FONT());
				g.drawText(t, b.reduced(5), Justification::left);
			}

			int getPreferredWidth() const override { return GLOBAL_MONOSPACE_FONT().getStringWidth(getText()) + 30; }
			int getPreferredHeight() const override { return 24; };

			const int index;
			WeakReference<Processor> processor;
			ModValue value;
			ModValue alpha;
		};

		ModuleConnectionViewer(Processor* p, const Array<int> parameterIndexes)
		{
			childLayout = ComponentWithPreferredSize::Layout::ChildrenAreRows;

			marginTop = 10;

			for (auto i : parameterIndexes)
			{
				addChildWithPreferredSize(new ParameterConnection(p, i));
			}
		};

		void resized() override { resizeChildren(this); };

		int getPreferredWidth() const override { return getMaxWidthOfChildComponents(this); };

		int getPreferredHeight() const override { return getSumOfChildComponentHeight(this); };

		static ComponentWithPreferredSize* create(Component* r, const var& value)
		{
			auto mc = dynamic_cast<ControlledObject*>(r)->getMainController();
			jassert(mc != nullptr);

			if (auto p = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), value["processorId"].toString()))
			{
				auto list = value["parameterIds"];

				if (list.isArray())
				{
					Array<int> indexes;

					for (auto param : *list.getArray())
					{
						auto idx = p->getParameterIndexForIdentifier(param.toString());

						if (idx != -1)
							indexes.add(idx);
					}

					return new ModuleConnectionViewer(p, indexes);
				}
			}

			return nullptr;
		}
	};

	factory.registerWithCreate<ModuleConnectionViewer>();
}

int ScriptBroadcaster::ModuleParameterListener::getNumInitialCalls() const
{
	int i = 0;

	for (auto l : listeners)
	{
		i += l->parameterIndexes.size();
	}

	return i;
}



Array<juce::var> ScriptBroadcaster::ModuleParameterListener::getInitialArgs(int callIndex) const
{
	int i = 0;

	Array<var> args = { var(), var(), var() };

	for (auto l : listeners)
	{
		args.set(0, l->p->getId());

		for (auto p : l->parameterIndexes)
		{
			if (i++ == callIndex)
			{
				args.set(1, p);
				args.set(2, l->p->getAttribute(p));
				return args;
			}
		}
	}

	jassertfalse;
	return args;
}

Array<juce::var> ScriptBroadcaster::ModuleParameterListener::createChildArray() const
{
	Array<var> list;

	for (auto i : listeners)
	{
		DynamicObject::Ptr p = new DynamicObject();
		
		p->setProperty("processorId", i->p->getId());
		p->setProperty("parameterIds", i->parameterNames);

		list.add(var(p.get()));
	}

	return list;
}

Result ScriptBroadcaster::ModuleParameterListener::callItem(TargetBase* n)
{
	Array<var> args;
	args.add("");
	args.add("");
	args.add(0.0f);

	for (auto p : listeners)
	{
		Processor* processor = p->p.get();

		if (processor == nullptr)
			continue;

		args.set(0, processor->getId());

		for (int i = 0; i < p->parameterNames.size(); i++)
		{
			auto parameterIndex = p->parameterIndexes[i];

			args.set(1, processor->getIdentifierForParameterIndex(parameterIndex).toString());
			args.set(2, processor->getAttribute(parameterIndex));

			auto r = n->callSync(args);

			if (!r.wasOk())
				return r;
		}
	}

	return Result::ok();
}

struct ScriptBroadcaster::ComplexDataListener::Item : public ComplexDataUIUpdaterBase::EventListener
{
	Item(ScriptBroadcaster* sb, ComplexDataUIBase::Ptr data_, bool isDisplay_, String pid, int index_) :
		isDisplay(isDisplay_),
		parent(sb),
		data(data_),
		processorId(pid),
		index(index_)
	{
		data->getUpdater().addEventListener(this);

		args.add(pid);
		args.add(index);
		args.add(0);
		keeper = var(args);
	}

	~Item()
	{
		data->getUpdater().removeEventListener(this);
	}

	void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var newData) override
	{
		if (isDisplay == (t == ComplexDataUIUpdaterBase::EventType::DisplayIndex))
		{
			auto valueToUse = isDisplay ? newData : var(data->toBase64String());
			args.set(2, valueToUse);
			parent->sendAsyncMessage(args);
		}
	}

	Array<var> args;
	var keeper;

	const bool isDisplay;
	WeakReference<ScriptBroadcaster> parent;
	ComplexDataUIBase::Ptr data;
	String processorId;
	int index;
};

juce::Identifier ScriptBroadcaster::ComplexDataListener::getItemId() const
{
	return typeId;
}

ScriptBroadcaster::ComplexDataListener::ComplexDataListener(ScriptBroadcaster* b,
	Array<WeakReference<ExternalDataHolder>> list,
	ExternalData::DataType dataType,
	bool isDisplayListener,
	Array<int> indexList,
	const String& typeString,
	const var& metadata):
	  ListenerBase(metadata),
	  typeId(typeString)
{
	for (auto eh : list)
	{
		for (auto idx : indexList)
		{
			ComplexDataUIBase::Ptr d = eh->getComplexBaseType(dataType, idx);
			items.add(new Item(b, d, isDisplayListener, dynamic_cast<Processor*>(eh.get())->getId(), idx));
		}
	}
}

int ScriptBroadcaster::ComplexDataListener::getNumInitialCalls() const
{
	return items.size();
}

Array<juce::var> ScriptBroadcaster::ComplexDataListener::getInitialArgs(int callIndex) const
{
	Array<var> args;

	if (auto ci = items[callIndex])
	{
		args.add(ci->processorId);
		args.add(ci->index);

		auto valueToUse = ci->isDisplay ? var(ci->data->getUpdater().getLastDisplayValue()) : var(ci->data->toBase64String());
		args.add(valueToUse);
	}

	return args;
}

Array<juce::var> ScriptBroadcaster::ComplexDataListener::createChildArray() const
{
	Array<var> list;

	for (auto i : items)
	{
		DynamicObject* d = new DynamicObject();
		d->setProperty("processorId", i->processorId);
		d->setProperty("type", typeId.toString());
		d->setProperty("index", i->index);

		list.add(var(d));
	}

	return list;
}

Result ScriptBroadcaster::ComplexDataListener::callItem(TargetBase* n)
{
	Array<var> args;
	args.add("");
	args.add(0);
	args.add("");

	for (auto p : items)
	{
		args.setUnchecked(0, p->processorId);
		args.setUnchecked(1, p->index);

		if (p->isDisplay)
			args.setUnchecked(2, p->data->getUpdater().getLastDisplayValue());
		else
			args.setUnchecked(2, p->data->toBase64String());

		auto r = n->callSync(args);

		if (!r.wasOk())
			return r;
	}

	return Result::ok();
}


void ScriptBroadcaster::ComplexDataListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	auto f = [](Component* c, const var& v)
	{
		ComponentWithPreferredSize* newComp = nullptr;

		if (auto obj = v.getDynamicObject())
		{
			auto mc = dynamic_cast<ControlledObject*>(c)->getMainController();
			jassert(mc != nullptr);

			DBG(JSON::toString(v));

			auto p = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), v["processorId"].toString());

			if (auto h = dynamic_cast<ExternalDataHolder*>(p))
			{
				auto idx = (int)v["index"];
				auto typeId = Identifier(v["type"].toString().upToFirstOccurrenceOf(".", false, false));
				auto dataType = ExternalData::getDataTypeForId(typeId);
				auto dataObject = h->getData(dataType, idx);

				jassert(obj != nullptr);

				auto comp = ExternalData::createEditor(dataObject.obj);
				newComp = new PrefferedSizeWrapper<ComplexDataUIBase::EditorBase, 200, 100>(comp);
			}
		}

		return newComp;
	};

	factory.registerFunction(f);
}



struct ScriptBroadcaster::MouseEventListener::InternalMouseListener
{
	InternalMouseListener(ScriptBroadcaster* parent, ScriptComponent* sc_, MouseCallbackComponent::CallbackLevel l):
		sc(sc_)
	{
		sc->attachMouseListener(parent, l);
	}

	WeakReference<ScriptComponent> sc;
};

ScriptBroadcaster::MouseEventListener::MouseEventListener(ScriptBroadcaster* parent, var componentIds, MouseCallbackComponent::CallbackLevel level, const var& metadata):
	ListenerBase(metadata)
{
	for (auto l : BroadcasterHelpers::getComponentsFromVar(parent->getScriptProcessor(), componentIds))
		items.add(new InternalMouseListener(parent, l, level));
}

Array<juce::var> ScriptBroadcaster::MouseEventListener::createChildArray() const
{
	Array<var> list;

	for (auto i : items)
		list.add(var(i->sc.get()));
	
	return list;
}

struct ScriptBroadcaster::ContextMenuListener::InternalMenuListener
{
	InternalMenuListener(ScriptBroadcaster* parent, ScriptComponent* l, var stateFunction, const StringArray& itemList):
		stateCallback(parent->getScriptProcessor(), parent, stateFunction, 2)
	{
		stateCallback.incRefCount();
		stateCallback.setThisObject(l);

		l->attachMouseListener(parent, 
			MouseCallbackComponent::CallbackLevel::PopupMenuOnly, 
			BIND_MEMBER_FUNCTION_1(InternalMenuListener::itemIsTicked),
			BIND_MEMBER_FUNCTION_1(InternalMenuListener::itemIsEnabled),
			BIND_MEMBER_FUNCTION_1(InternalMenuListener::getDynamicItemText),
			itemList);
	};

	var itemIsEnabled(int index)
	{
		var args[2] = { var("enabled"), var(index) };
		var rv(false);

		if (stateCallback)
			auto ok = stateCallback.callSync(args, 2, &rv);

		return rv;
	}

	var itemIsTicked(int index)
	{
		var args[2] = { var("active"), var(index) };
		var rv(false);

		if(stateCallback)
			auto ok = stateCallback.callSync(args, 2, &rv);

		return rv;
	}

	var getDynamicItemText(int index)
	{
		var args[2] = { var("text"), var(index) };
		var rv("");

		if (stateCallback)
			auto ok = stateCallback.callSync(args, 2, &rv);

		return rv;
	}

	WeakCallbackHolder stateCallback;
};

ScriptBroadcaster::ContextMenuListener::ContextMenuListener(ScriptBroadcaster* parent, 
															var componentIds, 
															var stateFunction, 
															const StringArray& itemList, 
															const var& metadata):
	ListenerBase(metadata)
{
	for (auto l : BroadcasterHelpers::getComponentsFromVar(parent->getScriptProcessor(), componentIds))
		items.add(new InternalMenuListener(parent, l, stateFunction, itemList));
}


struct ScriptBroadcaster::ComponentValueListener::InternalListener
{
	InternalListener(ScriptBroadcaster* parent, ScriptComponent* sc_) :
		sc(sc_)
	{
		sc->attachValueListener(parent);
	};

	~InternalListener()
	{

	}

	WeakReference<ScriptComponent> sc;
};

ScriptBroadcaster::ComponentValueListener::ComponentValueListener(ScriptBroadcaster* parent, var componentIds, const var& metadata):
	ListenerBase(metadata)
{
	for (auto l : BroadcasterHelpers::getComponentsFromVar(parent->getScriptProcessor(), componentIds))
	{
		items.add(new InternalListener(parent, l));
	}
}



void ScriptBroadcaster::ComponentValueListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerFunction(ComponentValueDisplay::create);
}

Array<juce::var> ScriptBroadcaster::ComponentValueListener::getInitialArgs(int callIndex) const
{
	Array<var> args;

	if (auto it = items[callIndex])
	{
		args.add(var(it->sc.get()));
		args.add(it->sc->getValue());
	}
	else
	{
		jassertfalse;
	}

	return args;
}

juce::Result ScriptBroadcaster::ComponentValueListener::callItem(TargetBase* n)
{
	Array<var> args;
	args.add(0);
	args.add(0);

	for (auto i : items)
	{
		args.set(0, var(i->sc.get()));
		args.set(1, var(i->sc->getValue()));

		auto ok = n->callSync(args);
		if (!ok.wasOk())
			return ok;
	}

	return Result::ok();
}

Array<juce::var> ScriptBroadcaster::ComponentValueListener::createChildArray() const
{
	Array<var> list;

	for (auto i : items)
		list.add(i->sc.get());

	return list;
}

struct ScriptBroadcaster::RadioGroupListener::InternalListener
{
	InternalListener(ScriptBroadcaster* b, ScriptComponent* sc) :
		radioButton(sc)
	{
		radioButton->valueListener = b;
	}

	WeakReference<ScriptComponent> radioButton;
};

ScriptBroadcaster::RadioGroupListener::RadioGroupListener(ScriptBroadcaster* b, int radioGroupIndex, const var& metadata):
	ListenerBase(metadata),
	radioGroup(radioGroupIndex)
{
	auto content = b->getScriptProcessor()->getScriptingContent();

	static const Identifier radioGroup("radioGroup");

	if (radioGroupIndex == 0)
		b->reportScriptError("illegal radio group index " + String(radioGroupIndex));

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		ScriptComponent* sc = content->getComponent(i);

		if ((int)sc->getPropertyValueTree()["radioGroup"] == radioGroupIndex)
		{
			if (sc->getValue())
				currentIndex = items.size();

			items.add(new InternalListener(b, sc));
		}
	}

	if (items.isEmpty())
	{
		String e;
		e << "No buttons with radio group ";
		e << String(radioGroupIndex);
		e << " found";
		b->reportScriptError(e);
	}

	if (currentIndex == -1 && BroadcasterHelpers::isValidArg(b->defaultValues[0]))
		currentIndex = b->defaultValues[0];

#if 0
	// force initial update
	lastValues.set(0, -1);

	sendMessage(currentIndex, true);
#endif
}

Array<juce::var> ScriptBroadcaster::RadioGroupListener::createChildArray() const
{
	Array<var> list;

	for (auto i : items)
		list.add(var(i->radioButton.get()));

	return list;
}



void ScriptBroadcaster::RadioGroupListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	struct RadioButtonItem : public MapItemWithScriptComponentConnection
	{
		RadioButtonItem(ScriptComponent* b) :
			MapItemWithScriptComponentConnection(b, 100, 28)
		{
			text = sc->get("text").toString();
		}

		static ComponentWithPreferredSize* create(Component* r, const var& v)
		{
			if (auto sc = dynamic_cast<ScriptComponent*>(v.getObject()))
				return new RadioButtonItem(sc);

			return nullptr;
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(on ? 0.8f : 0.4f));
			g.setFont(GLOBAL_BOLD_FONT());

			auto b = getLocalBounds().toFloat();

			auto circle = b.removeFromLeft(b.getHeight()).withSizeKeepingCentre(8, 8);

			g.drawEllipse(circle, 1.0f);

			if (on)
				g.fillEllipse(circle.reduced(2.0f));

			g.drawText(text, b, Justification::left);
		}

		void timerCallback() override
		{
			if (sc != nullptr)
			{
				auto shouldBeOn = (bool)sc->getValue();

				if (on != shouldBeOn)
				{
					on = shouldBeOn;
					repaint();
				};
			}
		}

		String text;
		bool on = false;
	};

	factory.registerFunction(RadioButtonItem::create);
}

void ScriptBroadcaster::RadioGroupListener::setButtonValueFromIndex(int newIndex)
{
	if (currentIndex != newIndex)
	{
		currentIndex = newIndex;

		for (int i = 0; i < items.size(); i++)
		{
			items[i]->radioButton->setValue(newIndex == i);
		}
	}
}

juce::Result ScriptBroadcaster::RadioGroupListener::callItem(TargetBase* n)
{
	int index = 0;
	currentIndex = -1;

	for (auto i : items)
	{
		if (i->radioButton->getValue())
		{
			currentIndex = index;
			break;
		}

		index++;
	}

	if (currentIndex != -1)
	{
		Array<var> args;
		args.add(currentIndex);
		auto ok = n->callSync(args);

		if (!ok.wasOk())
			return ok;
	}

	return Result::ok();
}




Array<juce::var> ScriptBroadcaster::OtherBroadcasterListener::createChildArray() const
{
	Array<var> list;

	for(auto s: sources)
		list.add(var(s.get()));

	return list;
}

juce::Result ScriptBroadcaster::OtherBroadcasterListener::callItem(TargetBase* n)
{
	return Result::ok();
}

ScriptBroadcaster::DebugableObjectListener::DebugableObjectListener(ScriptBroadcaster* parent_, const var& metadata, DebugableObjectBase* obj_, const Identifier& callbackId_) :
	ListenerBase(metadata),
	parent(parent_),
	obj(obj_),
	callbackId(callbackId_)
{

}

juce::Result ScriptBroadcaster::DebugableObjectListener::callItem(TargetBase* n)
{
	for (const auto&v : parent->lastValues)
	{
		if (v.isUndefined())
			return Result::ok();
	}

	return n->callSync(parent->lastValues);
}



Array<juce::var> ScriptBroadcaster::DebugableObjectListener::createChildArray() const
{
	Array<var> list;

	if (obj != nullptr)
		list.add(var(dynamic_cast<ReferenceCountedObject*>(obj.get())));

	return list;
}

void ScriptBroadcaster::DebugableObjectListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	struct DebugableObjectItem : public Component,
								 public ComponentWithPreferredSize,
							     public PathFactory
	{
		DebugableObjectItem(Processor* p, DebugableObjectBase* obj):
			gotoButton("goto", nullptr, *this)
		{
			addAndMakeVisible(gotoButton);

			if (auto ptr = DebugableObject::Helpers::getDebugInformation(dynamic_cast<JavascriptProcessor*>(p)->getProviderBase(), obj))
			{
				displayText = ptr->getTextForName();
				location = ptr->getLocation();

				auto loc = location;

				gotoButton.onClick = [p, loc]()
				{
					DebugableObject::Helpers::gotoLocation(nullptr, dynamic_cast<JavascriptProcessor*>(p), loc);
				};
			}

			f = GLOBAL_MONOSPACE_FONT();
			w = f.getStringWidth(displayText) + 24 + 30;
		}

		Path createPath(const String& url) const override
		{
			Path p;
			p.loadPathFromData(ColumnIcons::openWorkspaceIcon, sizeof(ColumnIcons::openWorkspaceIcon));
			return p;
		}

		int getPreferredWidth() const override { return w; };
		int getPreferredHeight() const override { return 28; };

		void resized() override
		{
			gotoButton.setBounds(getLocalBounds().removeFromLeft(getHeight()).reduced(6));
		}

		void paint(Graphics& g) override
		{
			g.setFont(f);
			g.setColour(Colours::white.withAlpha(0.5f));

			auto b = getLocalBounds().toFloat();
			b.removeFromLeft(28.0f);

			g.drawText(displayText, b, Justification::centredLeft);
		}

		HiseShapeButton gotoButton;

		static ComponentWithPreferredSize* create(Component* r, const var& obj)
		{
			if (auto o = obj.getObject())
			{
				Processor* p = nullptr;

				if (auto so = dynamic_cast<ScriptingObject*>(o))
					p = dynamic_cast<Processor*>(so->getScriptProcessor());
				else
					return nullptr;

				if (auto dbo = dynamic_cast<DebugableObjectBase*>(o))
					return new DebugableObjectItem(p, dbo);
			}
            
            return nullptr;
		}

		WeakReference<DebugableObjectBase> obj;
		String displayText;
		Font f;
		int w;

		DebugableObject::Location location;
	};

	factory.registerFunction(DebugableObjectItem::create);
}

struct ScriptBroadcaster::ScriptCallListener::ScriptCallItem: public ReferenceCountedObject
{
	void bang()
	{
		lastBangTime = Time::getMillisecondCounter();
	}
    
    bool isLastBang = false;
	uint32 lastBangTime = 0;
	Processor* p;
	Identifier id;
	DebugableObject::Location location;
};

ScriptBroadcaster::ScriptCallListener::ScriptCallListener(ScriptBroadcaster* b, const Identifier& id, DebugableObjectBase::Location location):
	ListenerBase(var())
{
    metadata.c = Colour(MIDI_PROCESSOR_COLOUR);
	auto ni = new ScriptCallItem();
	ni->p = dynamic_cast<Processor*>(b->getScriptProcessor());
	ni->id = id;
	ni->location = location;
	items.add(ni);
}

Array<juce::var> ScriptBroadcaster::ScriptCallListener::createChildArray() const
{
	Array<var> list;

	for (auto l : items)
		list.add(var(l));

	return list;
}

juce::Result ScriptBroadcaster::ScriptCallListener::callItem(TargetBase* n)
{
	return Result::ok();
}

void ScriptBroadcaster::ScriptCallListener::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	struct CallItem : public Component,
					  public PooledUIUpdater::SimpleTimer,
					  public ComponentWithPreferredSize,
					  public PathFactory
	{
		CallItem(ScriptCallListener::ScriptCallItem* item_):
			SimpleTimer(item_->p->getMainController()->getGlobalUIUpdater()),
			item(item_),
			gotoButton("goto", nullptr, *this)
		{
			addAndMakeVisible(gotoButton);

            font = GLOBAL_MONOSPACE_FONT();
            w = font.getStringWidth(item->id.toString()) + 50;
            
			gotoButton.onClick = [this]()
			{
				DebugableObject::Helpers::gotoLocation(nullptr, dynamic_cast<JavascriptProcessor*>(item->p), item->location);
			};
		}

		void resized() override
		{
			gotoButton.setBounds(getLocalBounds().removeFromLeft(getHeight()).reduced(5));
		}

		void timerCallback() override
		{
			auto thisBang = item->lastBangTime;

            auto thisLast = item->isLastBang;
            
            if(thisLast != isLast)
            {
                isLast = thisLast;
                repaint();
            }
            
			if (thisBang != lastBang)
			{
				lastBang = thisBang;
				alpha.setModValue(1.0);
			}

			if (alpha.setModValueIfChanged(jmax(0.0, alpha.getModValue() - 0.05)))
				repaint();
		}

		ModValue alpha;

		uint32 lastBang = 0;
        bool isLast = false;

		HiseShapeButton gotoButton;

		int getPreferredWidth() const override { return w; };
		int getPreferredHeight() const override { return 24; };

		Path createPath(const String& url) const override
		{
			Path p;
			p.loadPathFromData(ColumnIcons::openWorkspaceIcon, sizeof(ColumnIcons::openWorkspaceIcon));
			return p;
		}

		void paint(Graphics& g) override
		{
			g.setFont(font);
			g.setColour(Colours::white.withAlpha((float)alpha.getModValue() * 0.8f + 0.2f));
			auto b = getLocalBounds().toFloat();
			b.removeFromLeft(6.0f);
            
            auto r = b.removeFromLeft(getHeight()).reduced(8.0f);
            
            g.drawEllipse(r, 1.0f);
            
            if(isLast)
                g.fillEllipse(r.reduced(2.0f));
            
			g.drawText(item->id.toString(), b, Justification::centredLeft);
		}

		static ComponentWithPreferredSize* create(Component*, const var& obj)
		{
			if (auto o = dynamic_cast<ScriptCallItem*>(obj.getObject()))
				return new CallItem(o);

			return nullptr;
		}
        
        int w;
        Font font;
		ReferenceCountedObjectPtr<ScriptCallItem> item;
	};

	factory.registerWithCreate<CallItem>();
}

ScriptBroadcaster::ComponentPropertyItem::ComponentPropertyItem(ScriptBroadcaster* sb,
																const var& obj, 
																const Array<Identifier>& properties_, 
																const var& f,
															    const var& metadata):
	TargetBase(obj, f, metadata),
	properties(properties_)
{
	auto numArgs = sb->defaultValues.size();

	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		optionalCallback = new WeakCallbackHolder(sb->getScriptProcessor(), sb, f, numArgs + 1);
		optionalCallback->setHighPriority();
		optionalCallback->incRefCount();
	}
	else
	{
		if (numArgs != 3)
			sb->reportScriptError("A Component property target must be added to a broadcaster with three arguments (component, property, value)");
	}
}

void ScriptBroadcaster::ComponentPropertyItem::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerWithCreate<ComponentPropertyMapItem>();
}

Array<var> ScriptBroadcaster::ComponentPropertyItem::createChildArray() const
{
	Array<var> list;

	BroadcasterHelpers::callForEachIfArray(obj, [&](const var& v)
	{
		DynamicObject::Ptr o = new DynamicObject();
		o->setProperty("component", v);
		o->setProperty("properties", BroadcasterHelpers::getIdListAsVar(properties));
		list.add(var(o.get()));
		return true;
	});

	return list;
}

juce::Result ScriptBroadcaster::ComponentPropertyItem::callSync(const Array<var>& args)
{
	auto r = Result::ok();
	
	if (!enabled)
		return r;
	
	if (optionalCallback == nullptr)
	{
		auto component = args[0];
		
		auto value = args[2];

		BroadcasterHelpers::callForEachIfArray(obj, [&](const var& v)
		{
			// Skip setting the same property
			if (v == component)
				return true;

			if (auto sc = dynamic_cast<ScriptComponent*>(v.getObject()))
			{
				for (const auto& prop : properties)
				{
					if (!sc->hasProperty(prop))
					{
						r = Result::fail("illegal property " + prop.toString());
						break;
					}

					sc->setScriptObjectPropertyWithChangeMessage(prop, value);
				}
			}

			return r.wasOk();
		});
	}
	else
	{
		Array<var> optionalArgs;
		optionalArgs.add(-1);
		optionalArgs.addArray(args);
		
		BroadcasterHelpers::callForEachIfArray(obj, [&](const var& v)
		{
			optionalArgs.set(0, obj.indexOf(v));
			var::NativeFunctionArgs a(obj, optionalArgs.getRawDataPointer(), optionalArgs.size());

			var rv;

			r = optionalCallback->callSync(a, &rv);

			if (rv.isUndefined() || rv.isVoid())
				r = Result::fail("You need to return a value");

			if (r.wasOk())
			{
				if (auto sc = dynamic_cast<ScriptComponent*>(v.getObject()))
				{
					for (auto& prop : properties)
					{
						if (!sc->hasProperty(prop))
						{
							r = Result::fail("illegal property " + properties[0].toString());
							break;
						}

						sc->setScriptObjectPropertyWithChangeMessage(prop, rv);
					}
				}
			}

			return r.wasOk();
		});
	}

	return r;
}

ScriptBroadcaster::ComponentRefreshItem::ComponentRefreshItem(ScriptBroadcaster* sb, const var& obj, const String refreshMode_, const var& metadata):
	TargetBase(obj, {}, metadata),
	refreshModeString(refreshMode_)
{
	if (refreshMode_ == "repaint")
		refreshMode = RefreshType::repaint;
	else if (refreshMode_ == "changed")
		refreshMode = RefreshType::changed;
	else if (refreshMode_ == "updateValueFromProcessorConnection")
		refreshMode = RefreshType::updateValueFromProcessorConnection;
	else if (refreshMode_ == "loseFocus")
		refreshMode = RefreshType::loseFocus;
	else if (refreshMode_ == "resetValueToDefault")
		refreshMode = RefreshType::resetValueToDefault;

	for (int i = 0; i < obj.size(); i++)
		timeSlots.add(new RefCountedTime());
}

Array<juce::var> ScriptBroadcaster::ComponentRefreshItem::createChildArray() const
{
	Array<var> objWithTime;

	for (int i = 0; i < obj.size(); i++)
	{
		Array<var> item;
		item.add(obj[i]);
		item.add(var(timeSlots[i].get()));
		item.add(var(refreshModeString));

		objWithTime.add(var(item));
	}

	return objWithTime;
}

void ScriptBroadcaster::ComponentRefreshItem::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	struct RefreshBlinkComponent : public MapItemWithScriptComponentConnection
	{
		RefreshBlinkComponent(ScriptComponent* sc, const var& t, const String& mode_) :
			MapItemWithScriptComponentConnection(sc, GLOBAL_MONOSPACE_FONT().getStringWidth(mode_) + 50, 32),
			mode(mode_),
			time(dynamic_cast<RefCountedTime*>(t.getObject()))
		{
			jassert(time != nullptr);
		};

		void timerCallback() override
		{
			auto changed = lastTime != time->lastTime;

			if (changed)
			{
				lastTime = time->lastTime;
				alpha.setModValue(1.0);
			}
			
			if (alpha.setModValueIfChanged(jmax(0.0, alpha.getModValue() - 0.05)))
				repaint();
		}

		static ComponentWithPreferredSize* create(Component* c, const var& v)
		{
			if (auto sc = dynamic_cast<ScriptComponent*>(v[0].getObject()))
				return new RefreshBlinkComponent(sc, v[1], v[2].toString());

			return nullptr;
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat().reduced(5.0f);

			auto circle = b.removeFromLeft(b.getHeight()).reduced(5.0f);

			g.setColour(Colours::white.withAlpha(0.8f));

			g.drawEllipse(circle, 1.0f);

			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.drawText(mode, b, Justification::centredLeft);

			g.setColour(Colours::white.withAlpha((float)alpha.getModValue()));

			g.fillEllipse(circle.reduced(3.0f));
		}

		const String mode;
		ModValue alpha;
		RefCountedTime::Ptr time;
		uint32 lastTime;
	};

	factory.registerFunction(RefreshBlinkComponent::create);
}

juce::Result ScriptBroadcaster::ComponentRefreshItem::callSync(const Array<var>& )
{
	jassert(obj.isArray());

	for (int i = 0; i < obj.size(); i++)
	{
		auto sc = dynamic_cast<ScriptComponent*>(obj[i].getObject());

		timeSlots[i]->lastTime = Time::getMillisecondCounter();

		jassert(sc != nullptr);

		if (refreshMode == RefreshType::changed)
			sc->changed();

		if (refreshMode == RefreshType::repaint)
			sc->sendRepaintMessage();

		if (refreshMode == RefreshType::updateValueFromProcessorConnection)
			sc->updateValueFromProcessorConnection();

		if (refreshMode == RefreshType::loseFocus)
			sc->loseFocus();

		if (refreshMode == RefreshType::resetValueToDefault)
			sc->resetValueToDefault();
	}

	return Result::ok();
}

ScriptBroadcaster::ComponentValueItem::ComponentValueItem(ScriptBroadcaster* sb, const var& obj, const var& f, const var& metadata):
	TargetBase(obj, f, metadata)
{
	
	auto numArgs = sb->defaultValues.size();

	if (HiseJavascriptEngine::isJavascriptFunction(f))
	{
		optionalCallback = new WeakCallbackHolder(sb->getScriptProcessor(), sb, f, numArgs + 1);
		optionalCallback->setHighPriority();
		optionalCallback->incRefCount();
	}
}

void ScriptBroadcaster::ComponentValueItem::registerSpecialBodyItems(ComponentWithPreferredSize::BodyFactory& factory)
{
	factory.registerFunction(ComponentValueDisplay::create);
}


Array<juce::var> ScriptBroadcaster::ComponentValueItem::createChildArray() const
{
	Array<var> list;

	BroadcasterHelpers::callForEachIfArray(obj, [&](const var& cv)
	{
		list.add(cv);
		return true;
	});

	return list;
}

juce::Result ScriptBroadcaster::ComponentValueItem::callSync(const Array<var>& args)
{
	auto ok = Result::ok();

	if (optionalCallback == nullptr)
	{
		auto v = args.getLast();

		BroadcasterHelpers::callForEachIfArray(obj, [&](const var& cv)
		{
			if (auto sc = dynamic_cast<ScriptComponent*>(cv.getObject()))
				sc->setValue(v);

			return true;
		});
	}
	else
	{
		var oArgs[6];

		for (int i = 0; i < args.size(); i++)
			oArgs[i + 1] = args[i];

		BroadcasterHelpers::callForEachIfArray(obj, [&](const var& cv)
		{
			oArgs[0] = obj.indexOf(cv);

			var::NativeFunctionArgs args_(obj, oArgs, args.size() + 1);
			var rv;

			ok = optionalCallback->callSync(args_, &rv);

			if (rv.isUndefined() || rv.isVoid())
				ok = Result::fail("You need to return a value");

			if (!ok.wasOk())
				return false;

			if (auto sc = dynamic_cast<ScriptComponent*>(cv.getObject()))
				sc->setValue(rv);

			return true;
		});
	}

	return ok;
}

struct ScriptBroadcaster::Wrapper
{
	API_METHOD_WRAPPER_3(ScriptBroadcaster, addListener);
	API_METHOD_WRAPPER_4(ScriptBroadcaster, addDelayedListener);
	API_METHOD_WRAPPER_4(ScriptBroadcaster, addComponentPropertyListener);
	API_METHOD_WRAPPER_3(ScriptBroadcaster, addComponentValueListener);
	API_METHOD_WRAPPER_3(ScriptBroadcaster, addComponentRefreshListener);
	API_METHOD_WRAPPER_1(ScriptBroadcaster, removeListener);
	API_METHOD_WRAPPER_1(ScriptBroadcaster, removeSource);
	API_VOID_METHOD_WRAPPER_0(ScriptBroadcaster, removeAllListeners);
	API_VOID_METHOD_WRAPPER_0(ScriptBroadcaster, removeAllSources);
	API_VOID_METHOD_WRAPPER_0(ScriptBroadcaster, reset);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, sendMessage);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, sendSyncMessage);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, sendAsyncMessage);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, sendMessageWithDelay);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, attachToComponentProperties);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, attachToComponentVisibility);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, attachToComponentMouseEvents);
	API_VOID_METHOD_WRAPPER_4(ScriptBroadcaster, attachToContextMenu);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, attachToComponentValue);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, attachToModuleParameter);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, attachToRadioGroup);
    API_VOID_METHOD_WRAPPER_4(ScriptBroadcaster, attachToComplexData);
	API_VOID_METHOD_WRAPPER_4(ScriptBroadcaster, attachToOtherBroadcaster);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, callWithDelay);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setReplaceThisReference);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setEnableQueue);
    API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setRealtimeMode);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, setBypassed);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setForceSynchronousExecution);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setSendMessageForUndefinedArgs);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, resendLastMessage);
	API_METHOD_WRAPPER_0(ScriptBroadcaster, isBypassed);
};

ScriptBroadcaster::ScriptBroadcaster(ProcessorWithScriptingContent* p, const var& md):
	ConstScriptingObject(p, 0),
	lastResult(Result::ok())
{
	dynamic_cast<JavascriptProcessor*>(p)->registerCallableObject(this);

	ADD_API_METHOD_3(addListener);
	ADD_API_METHOD_4(addDelayedListener);
	ADD_API_METHOD_4(addComponentPropertyListener);
	ADD_API_METHOD_3(addComponentValueListener);
	ADD_API_METHOD_3(addComponentRefreshListener);
	ADD_API_METHOD_1(removeListener);
	ADD_API_METHOD_1(removeSource);
	ADD_API_METHOD_0(removeAllListeners);
	ADD_API_METHOD_0(removeAllSources);
	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(sendMessage);
	ADD_API_METHOD_1(sendAsyncMessage);
	ADD_API_METHOD_1(sendSyncMessage);
	ADD_API_METHOD_3(attachToComponentProperties);
	ADD_API_METHOD_3(attachToComponentMouseEvents);
	ADD_API_METHOD_2(attachToComponentValue);
	ADD_API_METHOD_2(attachToComponentVisibility);
	ADD_API_METHOD_3(attachToModuleParameter);
	ADD_API_METHOD_2(attachToRadioGroup);
    ADD_API_METHOD_4(attachToComplexData);
	ADD_API_METHOD_4(attachToContextMenu);
	ADD_API_METHOD_4(attachToOtherBroadcaster);
	ADD_API_METHOD_3(callWithDelay);
	ADD_API_METHOD_1(setReplaceThisReference);
	ADD_API_METHOD_1(setEnableQueue);
    ADD_API_METHOD_1(setRealtimeMode);
	ADD_API_METHOD_1(resendLastMessage);
	ADD_API_METHOD_3(setBypassed);
	ADD_API_METHOD_0(isBypassed);
	ADD_API_METHOD_1(setSendMessageForUndefinedArgs);
	ADD_API_METHOD_1(setForceSynchronousExecution);

	if (auto obj = md.getDynamicObject())
	{
		if (obj->hasProperty("id") && obj->hasProperty("args"))
		{
			metadata = Metadata(md, true);

			auto args = md["args"];
            
            obj = args.getDynamicObject();
            
            if(args.isArray())
            {
                for(const auto& v: *args.getArray())
                {
                    defaultValues.add(var());
                    argumentIds.add(Identifier(v.toString()));
                }
            }
		}

        if(obj != nullptr)
        {
            for (const auto& p : obj->getProperties())
            {
                defaultValues.add(p.value);
                argumentIds.add(p.name);
            }
        }
	}
	else if (md.isArray())
    {
        for(const auto& v: *md.getArray())
        {
            defaultValues.add(var());
            argumentIds.add(Identifier(v.toString()));
        }
    }
	else
		defaultValues.add(md);

	lastValues.addArray(defaultValues);

	Array<var> k;
	k.add(lastValues);
	k.add(defaultValues);
	keepers = var(k);

	setWantsCurrentLocation(true);
}

ScriptBroadcaster::~ScriptBroadcaster()
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(getScriptProcessor()))
	{
		jp->deregisterCallableObject(this);
	}
}

Component* ScriptBroadcaster::createPopupComponent(const MouseEvent& e, Component* parent)
{
	return new Display(this);
}

Result ScriptBroadcaster::call(HiseJavascriptEngine* engine, const var::NativeFunctionArgs& args, var* returnValue)
{
	for (auto attachedListener : attachedListeners)
	{
		if (auto rb = dynamic_cast<RadioGroupListener*>(attachedListener))
		{
			if ((bool)args.arguments[1])
			{
				auto clickedButton = args.arguments[0];

				int idx = 0;

				for (auto i : rb->items)
				{
					if (i->radioButton == clickedButton.getObject())
					{
						sendAsyncMessage(idx);
						break;
					}

					idx++;
				}
			}

			return lastResult;
		}
	}

	if (args.numArguments == defaultValues.size())
	{
		Array<var> argArray;

		for (int i = 0; i < args.numArguments; i++)
			argArray.add(args.arguments[i]);

		try
		{
			bool shouldBeSync = attachedListeners.isEmpty();

			sendMessageInternal(var(argArray), shouldBeSync);
			return lastResult;
		}
		catch (String& s)
		{
			return Result::fail(s);
		}
	}

	String e;

	e << metadata.id.toString() << " - " << "argument amount mismatch for connected callback. Expected: " << String(args.numArguments);

	return Result::fail(e);
}

hise::DebugInformationBase* ScriptBroadcaster::getChildElement(int index)
{
	String id = "%PARENT%.";
	

	if (isPositiveAndBelow(index, argumentIds.size()))
		id << argumentIds[index];
	else
		id << "arg" << String(index);

	WeakReference<ScriptBroadcaster> safeThis(this);

	return new LambdaValueInformation([index, safeThis]()
	{
		var x;
			
		if (safeThis != nullptr)
		{
			SimpleReadWriteLock::ScopedReadLock sl(safeThis->lastValueLock);
			x = safeThis->lastValues[index];
		}

		return x;
			
	}, Identifier(id), {}, (DebugInformation::Type)getTypeNumber(), getLocation());
}

bool ScriptBroadcaster::addListener(var object, var metadata, var function)
{
    if(isRealtimeSafe())
    {
        if(auto c = dynamic_cast<WeakCallbackHolder::CallableObject*>(function.getObject()))
        {
            if(!c->isRealtimeSafe())
                reportScriptError("You need to use inline functions in order to ensure realtime safe execution");
        }
    }
    
	ScopedPointer<ScriptTarget> ni = new ScriptTarget(this, defaultValues.size(), object, function, metadata);

	if (items.contains(ni.get()))
	{
		reportScriptError("this object is already registered to the listener");
		return false;
	}

	initItem(ni.get());
	
    ItemBase::PrioritySorter sorter;
    
	items.addSorted(sorter, ni.release());

	return true;
}

bool ScriptBroadcaster::addDelayedListener(int delayInMilliSeconds, var obj, var metadata, var function)
{
	if (delayInMilliSeconds == 0)
		return addListener(obj, metadata, function);

	ScopedPointer<DelayedItem> ni = new DelayedItem(this, obj, function, delayInMilliSeconds, metadata);

	if (items.contains(ni.get()))
	{
		reportScriptError("this object is already registered to the listener");
		return false;
	}

    ItemBase::PrioritySorter sorter;
    
	items.addSorted(sorter, ni.release());
	return true;
}

bool ScriptBroadcaster::addComponentPropertyListener(var object, var propertyList, var metadata, var optionalFunction)
{
	auto list = BroadcasterHelpers::getComponentsFromVar(getScriptProcessor(), object);
	auto idList = BroadcasterHelpers::getIdListFromVar(propertyList);

	auto illegalId = BroadcasterHelpers::getIllegalProperty(list, idList);
	
	if (illegalId.isValid())
		reportScriptError("illegal property: " + illegalId.toString());

	Array<var> l;

	for (auto sc : list)
		l.add(var(sc));

	ScopedPointer<TargetBase> ni = new ComponentPropertyItem(this, BroadcasterHelpers::getListOrFirstElement(var(l)), idList, optionalFunction, metadata);

	initItem(ni);

    ItemBase::PrioritySorter sorter;
    
	items.addSorted(sorter, ni.release());

	

	return true;
}


bool ScriptBroadcaster::addComponentValueListener(var object, var metadata, var optionalFunction)
{
	auto list = BroadcasterHelpers::getComponentsFromVar(getScriptProcessor(), object);

	Array<var> l;

	for (auto sc : list)
		l.add(var(sc));

	ScopedPointer<TargetBase> ni = new ComponentValueItem(this, BroadcasterHelpers::getListOrFirstElement(var(l)), optionalFunction, metadata);

	initItem(ni);

    ItemBase::PrioritySorter sorter;
    
	items.addSorted(sorter, ni.release());


	return true;
}

bool ScriptBroadcaster::addComponentRefreshListener(var componentIds, String refreshType, var metadata)
{
	auto list = BroadcasterHelpers::getComponentsFromVar(getScriptProcessor(), componentIds);

	if (list.isEmpty())
		reportScriptError("Can't find components for the given componentId object");

	Array<var> l;

	for (auto sc : list)
		l.add(var(sc));

	auto newObject = new ComponentRefreshItem(this, var(l), refreshType, metadata);

	ScopedPointer<TargetBase> ni = newObject;

	if (newObject->refreshMode == ComponentRefreshItem::RefreshType::numRefreshTypes)
		reportScriptError("Unknown refresh mode: " + refreshType);

	initItem(ni);

	ItemBase::PrioritySorter sorter;
	items.addSorted(sorter, ni.release());

	return true;
}

bool ScriptBroadcaster::removeListener(var objectToRemove)
{
	for (auto i : items)
	{
		if (i->metadata == objectToRemove)
		{
			items.removeObject(i);
			return true;
		}
	}

	return false;
}

bool ScriptBroadcaster::removeSource(var metadata)
{
	for (auto a : attachedListeners)
	{
		if (a->metadata == metadata)
		{
			attachedListeners.removeObject(a);
			return true;
		}
	}

	return false;
}

void ScriptBroadcaster::removeAllListeners()
{
	items.clear();
}

void ScriptBroadcaster::removeAllSources()
{
	attachedListeners.clear();
}

void ScriptBroadcaster::sendMessage(var args, bool isSync)
{
	debugError(dynamic_cast<Processor*>(getScriptProcessor()), "sendMessage is deprecated (because it's second parameter is hard to guess). Use either sendAsyncMessage or sendSyncMessage instead");
	sendMessageInternal(args, isSync);
}

void ScriptBroadcaster::sendMessageInternal(var args, bool isSync)
{
	if (forceSync)
		isSync = true;

#if USE_BACKEND
    if(isSync && getScriptProcessor()->getMainController_()->getKillStateHandler().getCurrentThread() ==
       MainController::KillStateHandler::TargetThread::AudioThread)
    {
        if(!isRealtimeSafe())
            reportScriptError("You need to enable realtime safe execution if you want to call synchronously on the audio thread");
    }
#endif
    
	handleDebugStuff();

	if ((args.isArray() && args.size() != defaultValues.size()) || (!args.isArray() && defaultValues.size() != 1))
	{
		String e;
		e << "argument amount mismatch. Expected: " << String(defaultValues.size());
		reportScriptError(e);
	}

	bool somethingChanged = false;

    if(isSync && isRealtimeSafe())
    {
        for(int i = 0; i < lastValues.size(); i++)
        {
            lastValues.set(i, BroadcasterHelpers::getArg(args, i));
        }
        
        lastResult = sendInternal(lastValues);
        
        if (!lastResult.wasOk())
            reportScriptError(lastResult.getErrorMessage());
        
        return;
    }
    
	Array<var> newValues;

	for (int i = 0; i < defaultValues.size(); i++)
	{
		auto v = BroadcasterHelpers::getArg(args, i);
		somethingChanged |= lastValues[i] != v;
		newValues.add(v);
	}

	if (somethingChanged || enableQueue || forceSend)
	{
		{
			SimpleReadWriteLock::ScopedWriteLock sl(lastValueLock);
			lastValues.swapWith(newValues);
		}

		if (bypassed)
			return;

		if (isSync)
		{
			lastResult = sendInternal(lastValues);

			if (!lastResult.wasOk())
				reportScriptError(lastResult.getErrorMessage());
		}
		else
		{
			if (!asyncPending.load() || enableQueue)
			{
				WeakReference<ScriptBroadcaster> safeThis(this);

				auto& pool = getScriptProcessor()->getMainController_()->getJavascriptThreadPool();

				Array<var> queuedValue;

				if (enableQueue)
				{
					for (auto& lv : lastValues)
						queuedValue.add(lv);
				}

				auto f = [safeThis, queuedValue](JavascriptProcessor* jp)
				{
					if (safeThis == nullptr)
						return Result::fail("dangling listener");

					auto& arrayToUse = safeThis->enableQueue ? queuedValue : safeThis->lastValues;

					auto r = safeThis->sendInternal(arrayToUse);

					safeThis->asyncPending.store(false);
					return r;
				};

				// If the queue is enabled, we want all to go through
				if(!enableQueue)
					asyncPending.store(true);

				pool.addJob(JavascriptThreadPool::Task::HiPriorityCallbackExecution,
					dynamic_cast<JavascriptProcessor*>(getScriptProcessor()),
					f);
			}
		}
	}
}

void ScriptBroadcaster::sendSyncMessage(var args)
{
	sendMessageInternal(args, true);
}

void ScriptBroadcaster::sendAsyncMessage(var args)
{
	sendMessageInternal(args, false);
}

void ScriptBroadcaster::sendMessageWithDelay(var args, int delayInMilliseconds)
{
	if (forceSync)
	{
		sendMessage(args, true);
		return;
	}
	
#if USE_BACKEND
	if (triggerBreakpoint)
	{
		reportScriptError("There you go...");
	}
#endif

	pendingData = args;
	startTimer(delayInMilliseconds);
}

void ScriptBroadcaster::reset()
{
	auto ok = sendInternal(defaultValues);

	if (!ok.wasOk())
		reportScriptError(ok.getErrorMessage());
}

void ScriptBroadcaster::resendLastMessage(bool isSync)
{
	if (forceSync)
		isSync = true;

	ScopedValueSetter<bool> svs(forceSend, true);

	sendMessage(var(lastValues), isSync);
}

void ScriptBroadcaster::setForceSynchronousExecution(bool shouldExecuteSynchronously)
{
	forceSync = shouldExecuteSynchronously;
}

void ScriptBroadcaster::setSendMessageForUndefinedArgs(bool shouldSendWhenUndefined)
{
	sendWhenUndefined = shouldSendWhenUndefined;
}

void ScriptBroadcaster::attachToComponentProperties(var componentIds, var propertyIds, var optionalMetadata)
{
	throwIfAlreadyConnected();

	if (defaultValues.size() != 3)
	{
		reportScriptError("If you want to attach a broadcaster to property events, it needs three parameters (component, propertyId, value)");
	}

	auto idList = BroadcasterHelpers::getIdListFromVar(propertyIds);

	attachedListeners.add(new ComponentPropertyListener(this, componentIds, idList, optionalMetadata));


	auto illegalId = dynamic_cast<ComponentPropertyListener*>(attachedListeners.getLast())->illegalId;

	if (illegalId.isValid())
	{
		String e;
		e << "Illegal property id: " + illegalId.toString();
		errorBroadcaster.sendMessage(sendNotificationAsync, attachedListeners.getLast(), e);
		reportScriptError(e);
	}
        
}

void ScriptBroadcaster::attachToComponentValue(var componentIds, var optionalMetadata)
{
	throwIfAlreadyConnected();

	attachedListeners.add(new ComponentValueListener(this, componentIds, optionalMetadata));

	if (defaultValues.size() != 2)
	{
		String e = "If you want to attach a broadcaster to value events, it needs two parameters (component, value)";
		errorBroadcaster.sendMessage(sendNotificationAsync, attachedListeners.getLast(), e);
		reportScriptError(e);
	}

	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}



void ScriptBroadcaster::attachToComponentVisibility(var componentIds, var optionalMetadata)
{
	throwIfAlreadyConnected();

	attachedListeners.add(new ComponentVisibilityListener(this, componentIds, optionalMetadata));

	if (defaultValues.size() != 2)
	{
		String e = "If you want to attach a broadcaster to visibility events, it needs two parameters (id, isVisible)";
		errorBroadcaster.sendMessage(sendNotificationAsync, attachedListeners.getLast(), e);
		reportScriptError(e);
	}

	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}

void ScriptBroadcaster::attachToComponentMouseEvents(var componentIds, var callbackLevel, var optionalMetadata)
{
	throwIfAlreadyConnected();

	if (defaultValues.size() != 2)
	{
		reportScriptError("If you want to attach a broadcaster to mouse events, it needs two parameters (component, event)");
	}

	auto cl = callbackLevel.toString();
	auto clValue = MouseCallbackComponent::getCallbackLevels(false).indexOf(cl);

	if (clValue == -1)
		reportScriptError("illegal callback level: " + cl);

	auto cLevel = (MouseCallbackComponent::CallbackLevel)clValue;

	attachedListeners.add(new MouseEventListener(this, componentIds, cLevel, optionalMetadata));
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}

void ScriptBroadcaster::attachToContextMenu(var componentIds, var stateFunction, var itemList, var optionalMetadata)
{
	throwIfAlreadyConnected();

	if (defaultValues.size() != 2)
		reportScriptError("If you want to attach a broadcaster to context menu events, it needs to parameters (component, menuItemIndex)");

	StringArray sa;

	if (itemList.isString())
		sa.add(itemList.toString());
	else if (itemList.isArray())
	{
		for (const auto& v : *itemList.getArray())
			sa.add(v.toString());
	}

	enableQueue = true;

	attachedListeners.add(new ContextMenuListener(this, componentIds, stateFunction, sa, optionalMetadata));
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}

void ScriptBroadcaster::attachToModuleParameter(var moduleIds, var parameterIds, var optionalMetadata)
{
	throwIfAlreadyConnected();

	if (defaultValues.size() != 3)
	{
		reportScriptError("If you want to attach a broadcaster to mouse events, it needs three parameters (processorId, parameterId, value)");
	}

	auto synthChain = getScriptProcessor()->getMainController_()->getMainSynthChain();

	Array<WeakReference<Processor>> processors;
	
	if (moduleIds.isArray())
	{
		for (const auto& pId : *moduleIds.getArray())
		{
			auto p = ProcessorHelpers::getFirstProcessorWithName(synthChain, pId.toString());

			if (!processors.isEmpty() && p->getType() != processors.getFirst()->getType())
				reportScriptError("the modules must have the same type");

			processors.add(p);
		}
	}
	else
		processors.add(ProcessorHelpers::getFirstProcessorWithName(synthChain, moduleIds.toString()));

	
	Array<int> parameterIndexes;

	if (parameterIds.isArray())
	{
		for (const auto& pId : *parameterIds.getArray())
		{
			auto idx = processors.getFirst()->getParameterIndexForIdentifier(pId.toString());

			if (idx == -1)
				reportScriptError("unknown parameter ID: " + pId.toString());

			parameterIndexes.add(idx);
		}
	}
	else
	{
		auto idx = processors.getFirst()->getParameterIndexForIdentifier(parameterIds.toString());

		if (idx == -1)
			reportScriptError("unknown parameter ID: " + parameterIds.toString());

		parameterIndexes.add(idx);
	}

	attachedListeners.add(new ModuleParameterListener(this, processors, parameterIndexes, optionalMetadata));
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());

	enableQueue = true;
}

void ScriptBroadcaster::attachToRadioGroup(int radioGroupIndex, var optionalMetadata)
{
	throwIfAlreadyConnected();

	attachedListeners.add(new RadioGroupListener(this, radioGroupIndex, optionalMetadata));
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}

void ScriptBroadcaster::attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async, var optionalMetadata)
{
	throwIfAlreadyConnected();

	Array<WeakReference<ScriptBroadcaster>> sources;

	if (otherBroadcaster.isArray())
	{
		for (const auto& v : *otherBroadcaster.getArray())
		{
			if (auto bc = dynamic_cast<ScriptBroadcaster*>(v.getObject()))
				sources.add(bc);
			else
				reportScriptError("not a broadcaster");
		}
	}
	else if (auto bc = dynamic_cast<ScriptBroadcaster*>(otherBroadcaster.getObject()))
	{
		sources.add(bc);
	}
	else
	{
		reportScriptError("not a broadcaster");
	}

	for (auto bc : sources)
		bc->addBroadcasterAsListener(this, argTransformFunction, async);

	attachedListeners.add(new OtherBroadcasterListener(sources, optionalMetadata));
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}

void ScriptBroadcaster::attachToComplexData(String dataTypeAndEvent, var moduleIds, var indexList, var optionalMetadata)
{
	throwIfAlreadyConnected();

    const String dataType = dataTypeAndEvent.upToFirstOccurrenceOf(".", false, false);
    const String eventType = dataTypeAndEvent.fromFirstOccurrenceOf(".", false, false);
    
    if(dataType.isEmpty() || eventType.isEmpty())
    {
        reportScriptError("dataTypeAndEvent must be formatted like `AudioFile.Content`");
    }
    
    ExternalData::DataType type;
    
    ExternalData::forEachType([&type, dataType](ExternalData::DataType t)
    {
        if(ExternalData::getDataTypeName(t, false) == dataType)
            type = t;
    });
    
    bool isDisplay = eventType == "Display";
    
    if (defaultValues.size() != 3)
    {
        reportScriptError("If you want to attach a broadcaster to complex data events, it needs three parameters (processorId, index, value)");
    }

    auto synthChain = getScriptProcessor()->getMainController_()->getMainSynthChain();

    Array<WeakReference<ExternalDataHolder>> processors;
    
    if (moduleIds.isArray())
    {
        for (const auto& pId : *moduleIds.getArray())
        {
            auto p = ProcessorHelpers::getFirstProcessorWithName(synthChain, pId.toString());

            if (auto asHolder = dynamic_cast<ExternalDataHolder*>(p))
            {
                processors.add(asHolder);
            }
            else
                reportScriptError(pId.toString() + " is not a complex data module");
        }
    }
    else
    {
        auto p = ProcessorHelpers::getFirstProcessorWithName(synthChain, moduleIds.toString());

        if (auto asHolder = dynamic_cast<ExternalDataHolder*>(p))
        {
            processors.add(asHolder);
        }
        else
            reportScriptError(moduleIds.toString() + " is not a complex data module");
    }
        

    
    Array<int> indexListArray;

    if (indexList.isArray())
    {
        for (const auto& idx : *indexList.getArray())
        {
            indexListArray.add((int)idx);
        }
    }
    else
    {
        indexListArray.add((int)indexList);
    }

    for(auto idx: indexListArray)
    {
        for(auto h: processors)
        {
            if(!isPositiveAndBelow(idx, h->getNumDataObjects(type)))
            {
                reportScriptError("illegal index: " + String(idx));
            }
        }
    }
    
    attachedListeners.add(new ComplexDataListener(this, processors, type, isDisplay, indexListArray, dataTypeAndEvent, optionalMetadata));
    
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());

    enableQueue = processors.size() > 1 || indexListArray.size() > 1;
}

void ScriptBroadcaster::callWithDelay(int delayInMilliseconds, var argArray, var function)
{
	if (currentDelayedFunction != nullptr)
		currentDelayedFunction->stopTimer();

	ScopedPointer<DelayedFunction> newFunction;

	if (HiseJavascriptEngine::isJavascriptFunction(function) && argArray.isArray())
		newFunction = new DelayedFunction(this, function, *argArray.getArray(), delayInMilliseconds, var());
	else if (!argArray.isArray())
		reportScriptError("argArray must be an array");
	
	ScopedLock sl(delayFunctionLock);
	std::swap(newFunction, currentDelayedFunction);
}

void ScriptBroadcaster::setReplaceThisReference(bool shouldReplaceThisReference)
{
	replaceThisReference = shouldReplaceThisReference;
}

void ScriptBroadcaster::setEnableQueue(bool shouldUseQueue)
{
	enableQueue = shouldUseQueue;
}

void ScriptBroadcaster::setBypassed(bool shouldBeBypassed, bool sendMessageIfEnabled, bool async)
{
	if (shouldBeBypassed != bypassed)
	{
		bypassed = shouldBeBypassed;

		if (!bypassed && sendMessageIfEnabled)
			resendLastMessage(async);
	}
}

bool ScriptBroadcaster::isBypassed() const
{
	return bypassed;
}

bool ScriptBroadcaster::assign(const Identifier& id, const var& newValue)
{
	auto idx = argumentIds.indexOf(id);

	if (idx == -1)
	{
		reportScriptError("This broadcaster doesn't have a " + id.toString() + " property");
		return false;
	}
	
	handleDebugStuff();

	if (lastValues[idx] != newValue || enableQueue)
	{
		lastValues.set(idx, newValue);

		lastResult = sendInternal(lastValues);

		if (!lastResult.wasOk())
			reportScriptError(lastResult.getErrorMessage());
	}

	return true;
}

juce::var ScriptBroadcaster::getDotProperty(const Identifier& id) const
{
	auto idx = argumentIds.indexOf(id);

	if (idx == -1)
		reportScriptError("This broadcaster doesn't have a " + id.toString() + " property");

	if(isPositiveAndBelow(idx, lastValues.size()))
		return lastValues[idx];

	return var();
}

void ScriptBroadcaster::addAsSource(DebugableObjectBase* b, const Identifier& callbackId)
{
	throwIfAlreadyConnected();

	attachedListeners.add(new DebugableObjectListener(this, {}, b, callbackId));

	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
}

bool ScriptBroadcaster::addLocationForFunctionCall(const Identifier& id, const DebugableObjectBase::Location& location)
{
	if (!argumentIds.contains(id) && id.toString() != "sendMessage")
		return false;

	for (auto attachedListener : attachedListeners)
	{
		if (auto existing = dynamic_cast<ScriptCallListener*>(attachedListener))
		{
			for (auto t : existing->items)
			{
				if (t->location == location)
					return false;
			}

			auto ni = new ScriptCallListener::ScriptCallItem();
			ni->id = id;
			ni->location = location;
			ni->p = dynamic_cast<Processor*>(getScriptProcessor());

			existing->items.add(ni);
			return true;
		}
	}

	
	
	throwIfAlreadyConnected();

	attachedListeners.add(new ScriptCallListener(this, id, location));
	checkMetadataAndCallWithInitValues(attachedListeners.getLast());
	return true;
}

bool ScriptBroadcaster::isPrimitiveArray(const var& obj)
{
	if (obj.isArray())
	{
		bool isPrimitive = true;

		for (auto& v : *obj.getArray())
		{
			isPrimitive &= (!v.isObject() && !v.isArray());

			if (!isPrimitive)
				break;
		}

		return isPrimitive;
	}

	return false;
}



void ScriptBroadcaster::handleDebugStuff()
{
#if USE_BACKEND
	if (bypassed)
		return;

	for (auto attachedListener : attachedListeners)
	{
		if (auto sl = dynamic_cast<ScriptCallListener*>(attachedListener))
		{
			ScriptCallListener::ScriptCallItem* bangItem = nullptr;

			for (auto i : sl->items)
			{
				if (i->location == getCurrentLocationInFunctionCall())
				{
					bangItem = i;
					i->bang();
				}
			}

			for (auto i : sl->items)
				i->isLastBang = i == bangItem;
		}
	}
    
	lastMessageTime = Time::getMillisecondCounter();

	if (triggerBreakpoint)
	{
		reportScriptError("There you go...");
	}
#endif
}

Result ScriptBroadcaster::sendInternal(const Array<var>& args)
{
	{
		SimpleReadWriteLock::ScopedReadLock v(lastValueLock);

		for (int i = 0; i < defaultValues.size(); i++)
		{
			auto v = args[i];
			if (v.isUndefined() || v.isVoid())
				return Result::ok();
		}
	}
	
    if(realtimeSafe)
    {
        for (auto i : items)
        {
            auto r = i->callSync(args);
            
			if (!r.wasOk())
			{
				sendErrorMessage(i, r.getErrorMessage(), false);
				return r;
			}
        }
    }
    else
    {
        for (auto i : items)
        {
            Array<var> thisValues;
            thisValues.ensureStorageAllocated(lastValues.size());

            {
                SimpleReadWriteLock::ScopedReadLock v(lastValueLock);
                thisValues.addArray(args);
            }

            auto r = i->callSync(thisValues);
            if (!r.wasOk())
            {
				sendErrorMessage(i, r.getErrorMessage(), false);
                return r;
            }
        }

		for (auto attachedListener : attachedListeners)
		{
			if (auto rb = dynamic_cast<RadioGroupListener*>(attachedListener))
			{
				int idx = (int)lastValues[0];

				rb->setButtonValueFromIndex(idx);
			}
		}
    }
    
	return Result::ok();
}

void ScriptBroadcaster::sendErrorMessage(ItemBase* i, const String& message, bool throwError/*=true*/)
{
	if (throwError)
		reportScriptError(message);

	if (i != nullptr)
		errorBroadcaster.sendMessage(sendNotificationAsync, i, message);
}

void ScriptBroadcaster::initItem(TargetBase* ni)
{
	checkMetadataAndCallWithInitValues(ni);

	if (!attachedListeners.isEmpty())
	{
		for (auto attachedListener : attachedListeners)
		{
			// If it's attached to a listener, we'll update it with the current values.
			auto r = attachedListener->callItem(ni);

			if (!r.wasOk())
				sendErrorMessage(ni, r.getErrorMessage());
		}
	}
	else
	{
		auto callListener = true;

		for (const auto& v : lastValues)
			callListener &= (!v.isUndefined() && !v.isVoid());

		if (callListener || sendWhenUndefined)
		{
			auto r = ni->callSync(lastValues);

			if (!r.wasOk())
				sendErrorMessage(ni, r.getErrorMessage());
		}
	}
}

void ScriptBroadcaster::checkMetadataAndCallWithInitValues(ItemBase* i)
{
	if (!i->metadata.r.wasOk())
		sendErrorMessage(i, i->metadata.r.getErrorMessage(), true);

	if (auto l = dynamic_cast<ListenerBase*>(i))
	{
		if (!items.isEmpty())
		{
			int numInitArgs = l->getNumInitialCalls();

			for (int j = 0; j < numInitArgs; j++)
			{
				auto args = l->getInitialArgs(j);

				for (auto target : items)
					target->callSync(args);
			}
		}
	}
}

void ScriptBroadcaster::throwIfAlreadyConnected()
{
#if 0
	if (attachedListener != nullptr)
	{
		sendErrorMessage(attachedListener, "This callback is already registered to " + attachedListener->getItemId().toString());
	}
#endif
}

void ScriptBroadcaster::timerCallback()
{
	sendMessage(pendingData, false);
	stopTimer();
}

void ScriptBroadcaster::setRealtimeMode(bool enableRealTimeMode)
{
	realtimeSafe = enableRealTimeMode;
}

void ScriptBroadcaster::addBroadcasterAsListener(ScriptBroadcaster* targetBroadcaster, const var& transformFunction, bool async)
{
	items.add(new OtherBroadcasterTarget(this, targetBroadcaster, transformFunction, async, targetBroadcaster->metadata.toJSON()));
}













ScriptBroadcaster::Metadata::Metadata(const var& obj, bool mustBeValid) :
	r(Result::ok())
{
	if (obj.isString())
	{
		c = Colours::grey;

		if (obj.toString().isEmpty())
		{
			if (mustBeValid)
				r = Result::fail("metadata string must not be empty");
		}
		else
			id = Identifier(obj.toString());

		return;
	}

	if (mustBeValid)
	{
		if (obj.getDynamicObject() == nullptr)
			r = Result::fail("metadata must be a JSON object with `id`, [`commment` and `colour`]");
		else if (obj["id"].toString().isEmpty())
			r = Result::fail("metadata must have at least a id property");
	}

	priority = (int)obj["priority"];

	comment = obj["comment"];

	auto tags_ = obj["tags"];

	if (tags_.isArray())
	{
		for (auto& t : *tags_.getArray())
			tags.add(Identifier(t.toString()));
	}

	auto idString = obj["id"].toString();

	if (idString.isNotEmpty())
		id = Identifier(idString);

	hash = idString.hashCode64();

	if (!obj.hasProperty("colour"))
	{
		c = Colours::lightgrey;
	}
	else if ((int)obj["colour"] == -1)
	{
		c = Colour((uint32)hash).withBrightness(0.7f).withSaturation(0.6f);
	}
	else
		c = scriptnode::PropertyHelpers::getColourFromVar(obj["colour"]);
}

ScriptBroadcaster::Metadata::Metadata() :
	r(Result::ok()),
	hash(0)
{

}

void ScriptBroadcaster::Metadata::attachCommentFromCallableObject(const var& callableObject, bool useDebugInformation)
{
	if (comment.isNotEmpty())
		return;

	if (auto obj = dynamic_cast<WeakCallbackHolder::CallableObject*>(callableObject.getObject()))
	{
		comment = obj->getComment();

		if (comment.isEmpty() && useDebugInformation)
		{
			if (auto cso = dynamic_cast<ScriptingObject*>(callableObject.getObject()))
			{
				auto jp = dynamic_cast<JavascriptProcessor*>(cso->getScriptProcessor());
				
				if (auto info = DebugableObject::Helpers::getDebugInformation(jp->getScriptEngine(), callableObject))
					comment = info->getDescription().getText();
			}
		}
	}
}

juce::var ScriptBroadcaster::Metadata::toJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();
	obj->setProperty("id", id.toString());
	obj->setProperty("comment", var(comment));
	obj->setProperty("colour", (int64)c.getARGB());

	Array<var> tags_;

	for (auto& t : tags)
		tags_.add(t.toString());

	obj->setProperty("tags", var(tags_));

	return var(obj.get());
}

}

} 
