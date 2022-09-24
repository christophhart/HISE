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
					obj->sendMessage(values, false);
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
		Row(ItemBase* i, Display& parent, JavascriptProcessor* jp_) :
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
		WeakReference<ItemBase> item;
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
			if (obj->attachedListener != nullptr)
				g.drawText("Source: " + obj->attachedListener->getListenerType().toString(), b.toFloat(), Justification::centred);
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

		parent.sendMessage(var(args), false);
	}

	NamedValueSet idSet;
	Array<var> args;

	WeakReference<ScriptComponent> sc;

	var keeper;
	ScriptBroadcaster& parent;
	String id;

	valuetree::PropertyListener listener;
};

ScriptBroadcaster::ComponentPropertyListener::ComponentPropertyListener(ScriptBroadcaster* b, var componentIds, const Array<Identifier>& propertyIds)
{
	for (auto sc : BroadcasterHelpers::getComponentsFromVar(b->getScriptProcessor(), componentIds))
	{
		for (auto& id : propertyIds)
		{
			if (sc->getIndexForProperty(id) == -1)
			{
				illegalId = id;
				return;
			}
		}

		items.add(new InternalListener(*b, sc, propertyIds));
	}
}

juce::Result ScriptBroadcaster::ComponentPropertyListener::callItem(ItemBase* n)
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



ScriptBroadcaster::Item::Item(ProcessorWithScriptingContent* p, int numArgs, const var& obj_, const var& f) :
	ItemBase(obj_, f),
	callback(p, f, numArgs)
{
	callback.incRefCount();
}

Result ScriptBroadcaster::Item::callSync(const Array<var>& args)
{
#if USE_BACKEND
	if (!enabled)
		return Result::ok();
#endif

	auto a = var::NativeFunctionArgs(obj, args.getRawDataPointer(), args.size());
	return callback.callSync(a, nullptr);
}

ScriptBroadcaster::DelayedItem::DelayedItem(ScriptBroadcaster* bc, const var& obj_, const var& f_, int milliseconds) :
	ItemBase(obj_, f_),
	ms(milliseconds),
	f(f_),
	parent(bc)
{

}

Result ScriptBroadcaster::DelayedItem::callSync(const Array<var>& args)
{
	delayedFunction = new DelayedFunction(parent, f, parent->lastValues, ms);
	return Result::ok();
}

ScriptBroadcaster::OtherBroadcasterTarget::OtherBroadcasterTarget(ScriptBroadcaster* parent_, ScriptBroadcaster* target_, const var& transformFunction, bool async_):
	ItemBase(var(target_), transformFunction),
	parent(parent_),
	target(target_),
	argTransformFunction(parent->getScriptProcessor(), transformFunction, parent->defaultValues.size()),
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
			target->sendMessage(rv, async);
			return target->lastResult;
		}
	}
	else
	{
		target->sendMessage(var(args), async);
		return target->lastResult;
	}
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
					sb->sendMessage(args, false);
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

ScriptBroadcaster::ModuleParameterListener::ModuleParameterListener(ScriptBroadcaster* b, const Array<WeakReference<Processor>>& processors, const Array<int>& parameterIndexes)
{
	for (auto& p : processors)
		listeners.add(new ProcessorListener(b, p, parameterIndexes));
}

Result ScriptBroadcaster::ModuleParameterListener::callItem(ItemBase* n)
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
			parent->sendMessage(args, false);
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

juce::Identifier ScriptBroadcaster::ComplexDataListener::getListenerType() const
{
	return typeId;
}

ScriptBroadcaster::ComplexDataListener::ComplexDataListener(ScriptBroadcaster* b,
	Array<WeakReference<ExternalDataHolder>> list,
	ExternalData::DataType dataType,
	bool isDisplayListener,
	Array<int> indexList,
	const String& typeString):
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

Result ScriptBroadcaster::ComplexDataListener::callItem(ItemBase* n)
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

ScriptBroadcaster::MouseEventListener::MouseEventListener(ScriptBroadcaster* parent, var componentIds, MouseCallbackComponent::CallbackLevel level)
{
	for (auto l : BroadcasterHelpers::getComponentsFromVar(parent->getScriptProcessor(), componentIds))
		l->attachMouseListener(parent, level);
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

ScriptBroadcaster::ComponentValueListener::ComponentValueListener(ScriptBroadcaster* parent, var componentIds)
{
	for (auto l : BroadcasterHelpers::getComponentsFromVar(parent->getScriptProcessor(), componentIds))
	{
		items.add(new InternalListener(parent, l));
	}
}

juce::Result ScriptBroadcaster::ComponentValueListener::callItem(ItemBase* n)
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

struct ScriptBroadcaster::RadioGroupListener::InternalListener
{
	InternalListener(ScriptBroadcaster* b, ScriptComponent* sc) :
		radioButton(sc)
	{
		radioButton->valueListener = b;
	}

	WeakReference<ScriptComponent> radioButton;
};

ScriptBroadcaster::RadioGroupListener::RadioGroupListener(ScriptBroadcaster* b, int radioGroupIndex):
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

	if (currentIndex == -1)
		currentIndex = b->defaultValues[0];

#if 0
	// force initial update
	lastValues.set(0, -1);

	sendMessage(currentIndex, true);
#endif
}

void ScriptBroadcaster::RadioGroupListener::setButtonValueFromIndex(int newIndex)
{
	if (currentIndex != newIndex)
	{
		for (int i = 0; i < items.size(); i++)
		{
			items[i]->radioButton->setValue(newIndex == i);
		}
	}
}

juce::Result ScriptBroadcaster::RadioGroupListener::callItem(ItemBase* n)
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

	if (currentIndex != 0)
	{
		Array<var> args;
		args.add(currentIndex);
		auto ok = n->callSync(args);

		if (!ok.wasOk())
			return ok;
	}

	return Result::ok();
}

struct ScriptBroadcaster::Wrapper
{
	API_METHOD_WRAPPER_2(ScriptBroadcaster, addListener);
	API_METHOD_WRAPPER_3(ScriptBroadcaster, addDelayedListener);
	API_METHOD_WRAPPER_1(ScriptBroadcaster, removeListener);
	API_VOID_METHOD_WRAPPER_0(ScriptBroadcaster, reset);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, sendMessage);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, sendMessageWithDelay);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, attachToComponentProperties);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, attachToComponentMouseEvents);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, attachToComponentValue);
	API_VOID_METHOD_WRAPPER_2(ScriptBroadcaster, attachToModuleParameter);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, attachToRadioGroup);
    API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, attachToComplexData);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, attachToOtherBroadcaster);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, callWithDelay);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setReplaceThisReference);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setEnableQueue);
    API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setRealtimeMode);
	API_VOID_METHOD_WRAPPER_3(ScriptBroadcaster, setBypassed);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, setId);
	API_VOID_METHOD_WRAPPER_1(ScriptBroadcaster, resendLastMessage);
};

ScriptBroadcaster::ScriptBroadcaster(ProcessorWithScriptingContent* p, const var& defaultValue):
	ConstScriptingObject(p, 0),
	lastResult(Result::ok())
{
	ADD_API_METHOD_2(addListener);
	ADD_API_METHOD_3(addDelayedListener);
	ADD_API_METHOD_1(removeListener);
	ADD_API_METHOD_0(reset);
	ADD_API_METHOD_2(sendMessage);
	ADD_API_METHOD_2(attachToComponentProperties);
	ADD_API_METHOD_2(attachToComponentMouseEvents);
	ADD_API_METHOD_1(attachToComponentValue);
	ADD_API_METHOD_2(attachToModuleParameter);
	ADD_API_METHOD_1(attachToRadioGroup);
    ADD_API_METHOD_3(attachToComplexData);
	ADD_API_METHOD_3(attachToOtherBroadcaster);
	ADD_API_METHOD_3(callWithDelay);
	ADD_API_METHOD_1(setReplaceThisReference);
	ADD_API_METHOD_1(setEnableQueue);
    ADD_API_METHOD_1(setRealtimeMode);
	ADD_API_METHOD_1(resendLastMessage);
	ADD_API_METHOD_3(setBypassed);
	ADD_API_METHOD_1(setId);
    
	if (auto obj = defaultValue.getDynamicObject())
	{
		for (const auto& p : obj->getProperties())
		{
			defaultValues.add(p.value);
			argumentIds.add(p.name);
		}
	}
	else if (defaultValue.isArray())
		defaultValues.addArray(*defaultValue.getArray());
	else
		defaultValues.add(defaultValue);

	lastValues.addArray(defaultValues);

	for(auto p: argumentIds)
	{

	}


	Array<var> k;
	k.add(lastValues);
	k.add(defaultValues);
	keepers = var(k);
}

Component* ScriptBroadcaster::createPopupComponent(const MouseEvent& e, Component* parent)
{
	return new Display(this);
}

Result ScriptBroadcaster::call(HiseJavascriptEngine* engine, const var::NativeFunctionArgs& args, var* returnValue)
{
	if (auto rb = dynamic_cast<RadioGroupListener*>(attachedListener.get()))
	{
		if ((bool)args.arguments[1])
		{
			auto clickedButton = args.arguments[0];

			int idx = 0;

			for (auto i : rb->items)
			{
				if (i->radioButton == clickedButton.getObject())
				{
					sendMessage(idx, false);
					break;
				}

				idx++;
			}
		}

		return lastResult;
	}
	

	if (args.numArguments == defaultValues.size())
	{
		Array<var> argArray;

		for (int i = 0; i < args.numArguments; i++)
			argArray.add(args.arguments[i]);

		try
		{
			bool shouldBeSync = attachedListener == nullptr;

			sendMessage(var(argArray), shouldBeSync);
			return lastResult;
		}
		catch (String& s)
		{
			return Result::fail(s);
		}
	}

	return Result::fail("argument amount mismatch. Expected: " + String(defaultValues.size()));
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

bool ScriptBroadcaster::addListener(var object, var function)
{
    if(isRealtimeSafe())
    {
        if(auto c = dynamic_cast<WeakCallbackHolder::CallableObject*>(function.getObject()))
        {
            if(!c->isRealtimeSafe())
                reportScriptError("You need to use inline functions in order to ensure realtime safe execution");
        }
    }
    
	ScopedPointer<Item> ni = new Item(getScriptProcessor(), defaultValues.size(), object, function);

	if (items.contains(ni.get()))
	{
		reportScriptError("this object is already registered to the listener");
		return false;
	}

	if (attachedListener != nullptr)
	{
		// If it's attached to a listener, we'll update it with the current values.
		auto r = attachedListener->callItem(ni);

		if (!r.wasOk())
			reportScriptError(r.getErrorMessage());
	}
	else
	{
		auto callListener = true;

		for (const auto& v : lastValues)
			callListener &= !v.isUndefined();

		if (callListener)
		{
			auto r = ni->callSync(lastValues);

			if (!r.wasOk())
				reportScriptError(r.getErrorMessage());
		}
	}
	
	items.add(ni.release());

	return true;
}

bool ScriptBroadcaster::addDelayedListener(int delayInMilliSeconds, var obj, var function)
{
	if (delayInMilliSeconds == 0)
		return addListener(obj, function);

	ScopedPointer<DelayedItem> ni = new DelayedItem(this, obj, function, delayInMilliSeconds);

	if (items.contains(ni.get()))
	{
		reportScriptError("this object is already registered to the listener");
		return false;
	}

	items.add(ni.release());
	return true;
}

bool ScriptBroadcaster::removeListener(var objectToRemove)
{
	for (auto i : items)
	{
		if (i->obj == objectToRemove)
		{
			items.removeObject(i);
			return true;
		}
	}

	return false;
}

void ScriptBroadcaster::sendMessage(var args, bool isSync)
{
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
            lastValues.set(i, getArg(args, i));
        }
        
        lastResult = sendInternal(lastValues);
        
        if (!lastResult.wasOk())
            reportScriptError(lastResult.getErrorMessage());
        
        return;
    }
    
	Array<var> newValues;

	for (int i = 0; i < defaultValues.size(); i++)
	{
		auto v = getArg(args, i);
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

void ScriptBroadcaster::sendMessageWithDelay(var args, int delayInMilliseconds)
{
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
	ScopedValueSetter<bool> svs(forceSend, true);

	sendMessage(var(lastValues), isSync);
}





void ScriptBroadcaster::setId(String newName)
{
	this->currentExpression = newName;
}

void ScriptBroadcaster::attachToComponentProperties(var componentIds, var propertyIds)
{
	throwIfAlreadyConnected();

	if (defaultValues.size() != 3)
	{
		reportScriptError("If you want to attach a broadcaster to property events, it needs three parameters (component, propertyId, value)");
	}

	Array<Identifier> idList;

	idList.add(Identifier(getArg(propertyIds, 0).toString()));

	if (propertyIds.isArray())
	{
		for(int i = 1; i < propertyIds.size(); i++)
			idList.add(Identifier(getArg(propertyIds, i).toString()));
	}

	attachedListener = new ComponentPropertyListener(this, componentIds, idList);

#if 0
	eventSources.clear();

    eventSources.add(new ScriptComponentPropertyEvent(*this, componentIds, idList));
#endif

	auto illegalId = dynamic_cast<ComponentPropertyListener*>(attachedListener.get())->illegalId;

    if (illegalId.isValid())
        reportScriptError("Illegal property id: " + illegalId.toString());
}

void ScriptBroadcaster::attachToComponentValue(var componentIds)
{
	throwIfAlreadyConnected();

	if (defaultValues.size() != 2)
	{
		reportScriptError("If you want to attach a broadcaster to value events, it needs two parameters (component, value)");
	}

	attachedListener = new ComponentValueListener(this, componentIds);
}



void ScriptBroadcaster::attachToComponentMouseEvents(var componentIds, var callbackLevel)
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

	attachedListener = new MouseEventListener(this, componentIds, cLevel);
}

void ScriptBroadcaster::attachToModuleParameter(var moduleIds, var parameterIds)
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

	attachedListener = new ModuleParameterListener(this, processors, parameterIndexes);

	enableQueue = true;
}

void ScriptBroadcaster::attachToRadioGroup(int radioGroupIndex)
{
	if (attachedListener != nullptr)
		reportScriptError("This callback is already registered to " + attachedListener->getListenerType().toString());

	attachedListener = new RadioGroupListener(this, radioGroupIndex);
}

void ScriptBroadcaster::attachToOtherBroadcaster(var otherBroadcaster, var argTransformFunction, bool async)
{
	Array<ScriptBroadcaster*> sources;

	if (otherBroadcaster.isArray())
	{
		for (const auto& v : *otherBroadcaster.getArray())
		{
			if (auto bc = dynamic_cast<ScriptBroadcaster*>(v.getObject()))
			{
				sources.add(bc);
			}
			else
			{
				reportScriptError("not a broadcaster");
			}
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
}

void ScriptBroadcaster::attachToComplexData(String dataTypeAndEvent, var moduleIds, var indexList)
{
	if (attachedListener != nullptr)
		reportScriptError("This callback is already registered to " + attachedListener->getListenerType().toString());

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

    dataTypeAndEvent;
    
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
                reportScriptError("illegal index: " + idx);
            }
        }
    }
    
    attachedListener = new ComplexDataListener(this, processors, type, isDisplay, indexListArray, dataTypeAndEvent);
    
    enableQueue = processors.size() > 1 || indexListArray.size() > 1;
}

void ScriptBroadcaster::callWithDelay(int delayInMilliseconds, var argArray, var function)
{
	if (currentDelayedFunction != nullptr)
		currentDelayedFunction->stopTimer();

	ScopedPointer<DelayedFunction> newFunction;

	if (HiseJavascriptEngine::isJavascriptFunction(function) && argArray.isArray())
		newFunction = new DelayedFunction(this, function, *argArray.getArray(), delayInMilliseconds);
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

bool ScriptBroadcaster::assign(const Identifier& id, const var& newValue)
{
	auto idx = argumentIds.indexOf(id);

	if (idx == -1)
	{
		reportScriptError("This broadcaster doesn't have a " + id.toString() + " property");
		return false;
	}
	
	handleDebugStuff();

	if (lastValues[idx] != newValue)
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

void ScriptBroadcaster::handleDebugStuff()
{
#if USE_BACKEND
	if (bypassed)
		return;

	lastMessageTime = Time::getMillisecondCounter();

	if (triggerBreakpoint)
	{
		reportScriptError("There you go...");
	}
#endif
}

juce::var ScriptBroadcaster::getArg(const var& v, int idx)
{
	if (v.isArray())
		return v[idx];

	jassert(idx == 0);
	return v;
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
            
            if(!r.wasOk())
                return r;
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
                return r;
            }
        }

		if (auto rb = dynamic_cast<RadioGroupListener*>(attachedListener.get()))
		{
			int idx = (int)lastValues[0];

			rb->setButtonValueFromIndex(idx);
		}
    }
    
	return Result::ok();
}

void ScriptBroadcaster::throwIfAlreadyConnected()
{
	if (attachedListener != nullptr)
		reportScriptError("This callback is already registered to " + attachedListener->getListenerType().toString());
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
	items.add(new OtherBroadcasterTarget(this, targetBroadcaster, transformFunction, async));
}

ScriptBroadcaster::Panel::Panel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{

}

juce::Identifier ScriptBroadcaster::Panel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

#if 0
struct ScriptBroadcasterMap : public Component,
							  public ControlledObject,
							  public GlobalScriptCompileListener,
							  public AsyncUpdater
{
	using BroadcasterList = Array<WeakReference<ScriptBroadcaster>>;

	struct Row : public Component
	{
		struct EntryBase : public Component
		{
			using List = Array<WeakReference<EntryBase>>;

			virtual int getPreferredHeight() const = 0;

			virtual ~EntryBase() {};

			List inputPins, outputPins;

			JUCE_DECLARE_WEAK_REFERENCEABLE(EntryBase);
		};

		struct BroadcasterEntry : public EntryBase
		{
			BroadcasterEntry(ScriptBroadcaster* sb) :
				b(sb)
			{};

			int getPreferredHeight() const override { return 80; }

			void paint(Graphics& g) override
			{
				g.fillAll(Colours::blue);
			}

			WeakReference<ScriptBroadcaster> b;
		};

		struct ListenerEntry : public EntryBase
		{
			ScriptBroadcaster::ListenerBase
		};

		struct Column : public Component
		{
			int getPreferredHeight() const
			{
				int y = 0;

				for (auto e : entries)
				{
					y += e->getPreferredHeight();
				}

				return y;
			}

			void resized() override
			{
				auto b = getLocalBounds();

				for (auto e : entries)
				{
					e->setBounds(b.removeFromTop(e->getPreferredHeight()));
				}
			}

			OwnedArray<EntryBase> entries;
		};

		int getPreferredHeight() const
		{
			int h = 0;

			for (auto c : columns)
				h = jmax(h, c->getPreferredHeight());

			return h;
		}

		Row(ScriptBroadcaster* b)
		{
			ScopedPointer<Column> sources;
			ScopedPointer<Column> object;
			ScopedPointer<Column> targets;

			

			if (sources->getPreferredHeight() > 0)
				columns.add(sources.release());

			if (object->getPreferredHeight() > 0)
				columns.add(object.release());

			if (targets->getPreferredHeight() > 0)
				columns.add(targets.release());
		}

		OwnedArray<Column> columns;
	};

	void handleAsyncUpdate() override { rebuild(); }

	ScriptBroadcasterMap(JavascriptProcessor* p_):
		ControlledObject(dynamic_cast<Processor*>(p_)->getMainController()),
		p(p_)
	{
		rebuild();

		getMainController()->addScriptListener(this);
	}

	~ScriptBroadcasterMap()
	{
		getMainController()->removeScriptListener(this);
	}

	void rebuild()
	{
		auto broadcasters = createBroadcasterList();

		rows.clear();

		for (auto b : broadcasters)
		{

		}
	}

	static void forEachDebugInformation(DebugInformationBase::Ptr di, const std::function<void(DebugInformationBase::Ptr)>& f)
	{
		f(di);

		for (int i = 0; i < di->getNumChildElements(); i++)
			f(di->getChildElement(i));
	}

	BroadcasterList createBroadcasterList()
	{
		BroadcasterList list;

		if (auto ah = dynamic_cast<ApiProviderBase*>(p.get()))
		{
			auto check = [&](DebugInformation::Ptr di)
			{
				if (auto sb = dynamic_cast<ScriptBroadcaster*>(di->getObject()))
					list.add(sb);
			};

			for (int i = 0; i < ah->getNumDebugObjects(); i++)
				forEachDebugInformation(ah->getDebugInformation(i), check);
		}

		return list;
	}

	void scriptWasCompiled(JavascriptProcessor *processor) override
	{
		if (p == processor)
		{
			triggerAsyncUpdate();
		}
	}

	WeakReference<JavascriptProcessor> p;

	OwnedArray<Row> rows;
};
#endif

Component* ScriptBroadcaster::Panel::createContentComponent(int)
{
	//if (auto jp = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()))
		//return new ScriptBroadcasterMap(jp);

	return nullptr;
}





}

} 
