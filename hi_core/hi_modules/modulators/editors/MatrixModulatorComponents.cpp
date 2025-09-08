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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


namespace hise { using namespace juce;

String MatrixContent::Parent::getDefaultCSS()
{
	static const char* HiseDefaultCSS = R"(
		button
		{
			background: rgba(0, 0, 0, 0.3);
			width: auto;
			padding: 5px 15px;
			font-size: 13px;
			font-weight: bold;
			font-family: Lato;
			color: rgba(255,255,255,0.7);
		}

		div
		{
			margin: 0px;
			flex-wrap: wrap;
			background: rgba(0, 0, 0, 0.2);
			padding: 10px;
			border-radius: 3px;
		}

		.dragger
		{
			background-color: rgba(0, 0, 0, 0.3);
			font-family: 'Lato';
			color: rgba(255,255,255, 0.7);
			width: auto;
			border-radius: 50%;
			padding: 0px 10px;
			cursor: grabbing;
		}

		.dragger:hover { color: white; }
		.dragger:active { transform: scale(98%);}
		button:hover { color: white; }
		#targetid { display: none; }

		.dragger::before
		{
			content: '';	
			background-color: white;
			background-image: var(--dragPath);
			width: 24px;
			margin: 5px;
		}

		tr
		{
			background: rgba(0, 0, 0, 0.1);
			padding: 3px 10px;
			margin: 1px;
			border-radius: 3px;
		}
		)";

	//return File("D:\\test.css").loadFileAsString();

	return String(HiseDefaultCSS);
}

MatrixContent::Parent::Parent()
{
	simple_css::Parser cp(getDefaultCSS());
	auto ok = cp.parse();
	jassert(ok.wasOk());
	css = cp.getCSSValues();
	css.setAnimator(&animator);
}

String MatrixBase::getColumnStyleSheet(const Identifier& id)
{
	if(id == MatrixIds::SourceIndex)
		return "width: 128px;height:24px;";

	if(id == MatrixIds::TargetId)
		return "width: 128px;height:24px;";

	if(id == MatrixIds::Mode)
		return "width: 100px;height:24px;";

	if(id == MatrixIds::Intensity || id == MatrixIds::AuxIntensity)
		return "width: 128px;height:48px;";

	if(id == MatrixIds::Inverted)
		return "width: 60px;height:48px;";

	if(id == MatrixIds::AuxIndex)
		return"width: 128px;height:24px;";

	if(id == Identifier("Plotter"))
		return "width: 100px; height:48px; flex-grow: 1;";

	jassertfalse;
	return {};
}

void MatrixBase::IntensitySlider::mouseDoubleClick(const MouseEvent& event)
{
	if(performModifierAction(event, true, false))
		return;

	Slider::mouseDoubleClick(event);
}

void MatrixBase::IntensitySlider::setActive(bool shouldBeActive, const ValueTree& connectionData, bool updateIntensity)
{
	jassert(!connectionData.isValid() || connectionData.getType() == MatrixIds::Connection);
	auto intensityValue = (double)connectionData[MatrixIds::Intensity];
	FloatSanitizers::sanitizeDoubleNumber(intensityValue);
	auto m = (modulation::TargetMode)(int)connectionData[MatrixIds::Mode];

	if(updateIntensity)
	{
		auto minValue = (m != modulation::TargetMode::Gain) ? -1.0 : 0.0;

		if(!shouldBeActive)
			intensityValue = 0.0;

		setRange(minValue, 1.0);
		setValue(intensityValue, dontSendNotification);
	}
	
	int state = 0;

	bool update = false;

	if(!shouldBeActive)
		state |= (int)simple_css::PseudoClassType::Empty;

	if(lastMode != m)
	{
		// only update the mode when it's not the auxillary slider...
		if(updateIntensity)
			lastMode = m;

		static const StringArray classes({ ".scaled", ".unipolar", ".bipolar" });
		auto c = classes[(int)lastMode];
		simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { ".slider", c });
		update = true;
	}

	if(lastPseudoState != state)
	{
		lastPseudoState = state;
		simple_css::FlexboxComponent::Helpers::writeManualPseudoState(*this, state);
		update = true;
	}

	if(update)
	{
		simple_css::FlexboxComponent::Helpers::invalidateCache(*this);
		repaint();
	}
}

MatrixBase::HeaderBase::HeaderBase():
	FlexboxComponent(simple_css::ElementType::TableHeader)
{
	setTextElementSelector(simple_css::ElementType::TableCell);

	Helpers::setFallbackStyleSheet(*this, "padding: 0px 10px;padding-bottom: 10px;gap: 10px;background:rgba(0,0,0,0.1);border-radius:3px;margin:1px;display:flex;");

	
}

int MatrixContent::HeaderBase::getHeightToUse(int w)
{
	if(w == 0)
		return 0;

	return getAutoHeightForWidth((float)w);
}

void MatrixContent::HeaderBase::setCSS(simple_css::StyleSheet::Collection& css)
{
	if(initialised)
		return;

	if(auto th = css.getForComponent(this))
	{
		auto displayProperty = th->getPropertyValueString({ "display", {}});
		setVisible(displayProperty != "none");
	}

	FlexboxComponent::setCSS(css);
	initialised = true;
}

Component* MatrixContent::HeaderBase::addHeaderItem(const Identifier& id, const String& prettyName)
{
	using namespace simple_css;

	String css_id = "#" + id.toString().toLowerCase();
	auto css_type = Selector(ElementType::TableCell);

	if(id.isValid())
	{
		auto c = addTextElement({ css_id, }, id.toString());

		auto fb = getColumnStyleSheet(id);
		fb << "content: '" << prettyName << "';";
		fb << "height: 24px;";
		fb << "font-weight: bold;";
		fb << "font-family: 'Lato';";
		fb << "font-size: 14px;";
		fb << "color:rgba(255,255,255,0.4);";

		Helpers::appendToElementStyle(*c, fb);

		return c;
	}
	else
	{
		return addTextElement({ css_type.toString() }, prettyName);
	}
}

MatrixBase::SearchBar::ClearButton::ClearButton():
	Button("clear"),
	pathData("230.t0FP.ZBQN++OCIlNbdCQN++OCA.fEQj5Od2P..XQDA..dNjX..XQDs.N.OjNbdCQZ..2CADflPjF.v8PhgDYUPjF.v8P..3ADs.N.OD..d.Q..fmCIF..d.Qp+3cCgDYUPjy++yP.AnID47++LzXsQKmdPD..34PrgElTPzHIH6PrI1dbPTAPG7PrADflPD+F25Pr4IgvPTAPG7ProBZ3PzHIH6Pro7XtPD..34ProBZ3Pz81m3Pr4IgvPj8eQ2PrADflPjG433PrI1dbPj8eQ2PrgElTPz81m3PrQKmdPD..34PiUF")
{
	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { "#clearsearch" });
	simple_css::FlexboxComponent::Helpers::setFallbackStyleSheet(*this, "background-image:var(--icon);background-color:#666;margin:5px;");

	MemoryBlock mb;
	mb.fromBase64Encoding(pathData);
	icon.loadPathFromData(mb.getData(), mb.getSize());

	onClick = [this]()
	{
		auto sb = findParentComponentOfClass<SearchBar>();
		sb->setText("", dontSendNotification);
		sb->updateSearch("");
		this->setVisible(false);
	};
}

void MatrixBase::SearchBar::ClearButton::paintButton(Graphics& g, bool shouldDrawButtonAsHighlighted,
	bool shouldDrawButtonAsDown)
{
	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			ss->setPropertyVariable("icon", pathData);
			simple_css::Renderer r(this, root->stateWatcher);

			auto currentState = simple_css::Renderer::getPseudoClassFromComponent(this);
			root->stateWatcher.checkChanges(this, ss, currentState);

			r.drawBackground(g, getLocalBounds().toFloat(), ss);
			return;
		}
	}

	g.setColour(Colours::white.withAlpha(shouldDrawButtonAsHighlighted ? 0.5f : 0.4f));
	g.fillPath(icon);
}

MatrixBase::IntensitySlider::IntensitySlider(const Identifier& id):
	Slider(id.toString()),
	vtc(ValueToTextConverter::createForMode("NormalizedPercentage"))
{
	setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
	setRange(-1.0, 1.0, 0.0);
	setDoubleClickReturnValue(true, 0.0);
	valueFromTextFunction = vtc;
	textFromValueFunction = vtc;

	this->customPopupFunction = [](const MouseEvent& e)
	{
		
	};
}

MatrixContent::Row::Row(MainController* mc, ValueTree rowData, const String& targetId):
  RowBase(mc),
  data(rowData),
  um(mc->getControlUndoManager()),
  intensitySlider(MatrixIds::Intensity),
  auxIntensitySlider(MatrixIds::AuxIntensity)
{
	jassert(data.getType() == MatrixIds::Connection);

	plotter.setColour(ModPlotter::ColourIds::backgroundColour, Colour(0xFF222222));

	StringArray itemList;
	container = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(getMainController()->getMainSynthChain());

	addComponent(sourceSelector, MatrixIds::SourceIndex);
	addComponent(targetSelector, MatrixIds::TargetId);
	addComponent(modeSelector, MatrixIds::Mode);
	addComponent(invertedButton, MatrixIds::Inverted);
	addComponent(intensitySlider, MatrixIds::Intensity);
	addComponent(auxSelector, MatrixIds::AuxIndex);
	addComponent(auxIntensitySlider, MatrixIds::AuxIntensity);
	Helpers::writeSelectorsToProperties(plotter, { "#plotter", ".modplotter" });
	Helpers::setFallbackStyleSheet(plotter, "flex-grow:1;height:48px;");

	addFlexItem(plotter);

	targetSelector.setEnabled(targetId.isEmpty());

	sourceSelector.setTooltip("The modulation source.");
	targetSelector.setTooltip("The modulation target.");
	modeSelector.setTooltip("How the modulation source is applied to the target.");
	intensitySlider.setTooltip("How much the modulation source is affecting the target.");
	auxSelector.setTooltip("A modulation source that modulates the intensity of this modulation.");
	auxIntensitySlider.setTooltip("How much the aux modulation source is affecting the modulation signal.");

	auxIntensitySlider.setRange(0.0, 1.0, 0.0);
	auxIntensitySlider.valueFromTextFunction = ValueToTextConverter::createForMode("NormalizedPercentage");
	auxIntensitySlider.textFromValueFunction = ValueToTextConverter::createForMode("NormalizedPercentage");
	auxIntensitySlider.updateText();
	
	Helpers::setFallbackStyleSheet(*this,           "width:100%;gap:10px;height:100%;margin:5px; display:flex;align-items:center;");

	valueUpdater.setCallback(data, 
							 MatrixIds::Helpers::getWatchableIds(), 
							 valuetree::AsyncMode::Asynchronously,
							 BIND_MEMBER_FUNCTION_2(Row::updateValue));

	lookAndFeelChanged();
};

void MatrixContent::Row::lookAndFeelChanged()
{
	FlexboxComponent::lookAndFeelChanged();

	if(auto d = dynamic_cast<RingBufferComponentBase::LookAndFeelMethods*>(&getLookAndFeel()))
	{
		plotter.setSpecialLookAndFeel(&getLookAndFeel(), false);
	}
}



void MatrixContent::Row::rebuildItemList(const Identifier& id)
{
	if(id == MatrixIds::Intensity)
		return;

	items[id] = getItemList(id);
}

StringArray MatrixContent::Row::getItemList(const Identifier& id) const
{
	StringArray items;
	items.add("No connection");

	if(id == MatrixIds::Mode)
	{
		return { "Scale", "Unipolar", "Bipolar" };
	}
	else if(id == MatrixIds::SourceIndex || id == MatrixIds::AuxIndex)
	{
		MatrixIds::Helpers::fillModSourceList(getMainController(), items);
	}
	else if(id == MatrixIds::TargetId)
	{
		MatrixIds::Helpers::fillModTargetList(getMainController(), items, MatrixIds::Helpers::TargetType::All);
	}
	else
	{
		jassertfalse;
	}

	return items;
}

int MatrixContent::Row::getIndexInTarget() const
{
	jassert(data.isValid());
	auto parent = data.getParent();

	int targetIndex = 0;

	auto thisTarget = data[MatrixIds::TargetId].toString();

	for(auto c: parent)
	{
		if(c[MatrixIds::TargetId].toString() == thisTarget)
		{
			if(c == data)
				return targetIndex;

			targetIndex++;
		}
	}

	jassertfalse;
	return -1;
}

void MatrixContent::Row::updateValue(const Identifier& id, const var& newValue)
{
	if(id == MatrixIds::Intensity)
	{
		intensitySlider.setActive(true, data, true);
	}
	else if (id == MatrixIds::AuxIntensity)
	{
		if(!auxIntensitySlider.down)
			auxIntensitySlider.setValue((double)newValue, dontSendNotification);
	}
	else if (id == MatrixIds::Inverted)
	{
		invertedButton.setToggleState((bool)newValue, dontSendNotification);
	}
	else
	{
		auto cb = comboBoxes[id];
		jassert(cb != nullptr);
		cb->setSelectedItemIndex(valueToComboBoxIndex(id), dontSendNotification);

		if(id == MatrixIds::Mode)
		{
			auto isScale = (int)newValue == 0;
			auto r = isScale ? Range<float>(0.0f, 1.0f) : Range<float>(-1.0f, 1.0f);

			if(isScale)
				plotter.setUseFixRange(r);
			else
				plotter.setUseFixRange(r);

			intensitySlider.setActive(true, data, true);

			updateIntensityConverter();
		}
		else if (id == MatrixIds::AuxIndex)
		{
			auto isEnabled = (int)newValue != -1;

			auxIntensitySlider.setActive(isEnabled, this->data, false);

			auxIntensitySlider.setEnabled(isEnabled);
		}
		else if (id == MatrixIds::SourceIndex)
		{
			auto isEnabled = (int)newValue != -1;
			intensitySlider.setEnabled(isEnabled);
			auxSelector.setEnabled(isEnabled);
			auto plotterIndex = 5;
			setFlexChildVisibility(plotterIndex, false, !isEnabled);
			rebuildLayout();
		}
		else if (id == MatrixIds::TargetId)
		{
			auto newTarget = newValue.toString();
			auto tt = MatrixIds::Helpers::getTargetType(getMainController(), newTarget);

			updateIntensityConverter();

			if(tt == MatrixIds::Helpers::TargetType::Modulators)
			{
				auto mp = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), newTarget);
				if(auto mm = dynamic_cast<MatrixModulator*>(mp))
				{
					
					auto idx = getIndexInTarget();

					if(auto p = mm->getDisplayBuffer(idx))
					{
						plotter.setComplexDataUIBase(p);
					}
				}
			}
			else
			{
				if(auto jmp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getMainController()))
				{
					auto content = jmp->getScriptingContent();

					for(int i = 0; i < content->getNumComponents(); i++)
					{
						using S = ScriptingApi::Content::ScriptSlider;

						if(auto slider = dynamic_cast<S*>(content->getComponent(i)))
						{
							auto p = S::matrixTargetId;
							auto tid = content->getComponent(i)->getScriptObjectProperty(p).toString();

							if(tid == newTarget)
							{
								auto sourceIndex = (int)data[MatrixIds::SourceIndex];

								if(auto p = slider->getMatrixPlotter(sourceIndex))
									plotter.setComplexDataUIBase(p.get());
							}
						}
					}
				}
			}

			
		}
	}
}

void MatrixContent::Row::addComponent(Component& c, const Identifier& id)
{
	c.setName(id.toString());
	c.getProperties().set("id", c.getName().toLowerCase());
	addFlexItem(c);
	Helpers::setFallbackStyleSheet(c, getColumnStyleSheet(id));
	GlobalHiseLookAndFeel::setDefaultColours(c);

	if(auto s = dynamic_cast<Slider*>(&c))
	{
		Helpers::writeSelectorsToProperties(c, { ".slider" });
		s->addListener(this);
	}
	else if (auto tb = dynamic_cast<ToggleButton*>(&c))
	{
		Helpers::writeSelectorsToProperties(c, { "button" });
		tb->addListener(this);
	}
	else if (auto cb = dynamic_cast<ComboBox*>(&c))
	{
		rebuildItemList(id);
		cb->addItemList(items[id], 1);
		cb->addListener(this);
		comboBoxes[id] = cb;
	}
}

void MatrixContent::Row::comboBoxChanged(ComboBox* cb)
{
	auto id = Identifier(cb->getName());
	auto idx = cb->getSelectedItemIndex();
	data.setProperty(id, comboBoxIndexToValue(id, idx), um);
}

void MatrixContent::Row::sliderValueChanged(Slider* slider)
{
	auto id = Identifier(slider->getName());
	data.setProperty(id, slider->getValue(), um);
}

int MatrixContent::Row::valueToComboBoxIndex(const Identifier& id)
{
	auto v = data[id];

	if(id == MatrixIds::TargetId)
	{
		auto tid = v.toString();
		return items[id].indexOf(tid);
	}

	if(id == MatrixIds::Mode)
		return (int)v;

	// unconnected = -1
	return (int)v + 1;
}

var MatrixContent::Row::comboBoxIndexToValue(const Identifier& id, int comboBoxIndex)
{
	auto v = data[id];

	if(id == MatrixIds::Mode)
		return comboBoxIndex;

	if(id == MatrixIds::TargetId)
		return getItemList(id)[comboBoxIndex];

	// unconnected = -1
	return (int)comboBoxIndex - 1;
}

void MatrixContent::Row::resized()
{
	if(useCSS)
	{
		FlexboxComponent::resized();
	}
	else
	{
		auto b = getLocalBounds();
		b.removeFromBottom(Margin);
		sourceSelector.setBounds(b.removeFromLeft(128).reduced(0, 10));
		b.removeFromLeft(Margin);
		targetSelector.setBounds(b.removeFromLeft(128).reduced(0, 10));
		b.removeFromLeft(Margin);
		modeSelector.setBounds(b.removeFromLeft(100).reduced(0, 10));
		b.removeFromLeft(Margin);
		intensitySlider.setBounds(b.removeFromLeft(128).reduced(0, 0));
		b.removeFromLeft(Margin);
		auxSelector.setBounds(b.removeFromLeft(128).reduced(0, 10));
		b.removeFromLeft(Margin);
		plotter.setBounds(b);
	}
}




void MatrixContent::Row::setSliderStyle(Slider::SliderStyle sliderStyle)
{
	intensitySlider.setSliderStyle(sliderStyle);

	if(sliderStyle != Slider::RotaryHorizontalVerticalDrag)
	{
		intensitySlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
		intensitySlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	}

	auxIntensitySlider.setSliderStyle(sliderStyle);

	if(sliderStyle != Slider::RotaryHorizontalVerticalDrag)
	{
		auxIntensitySlider.setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
		auxIntensitySlider.setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
	}
}

void MatrixBase::SearchBar::updateSearch(const String& searchTerm)
{
	auto b = findParentComponentOfClass<MatrixBase>();

	Component::callRecursive<RowBase>(b, [&](RowBase* b)
	{
		auto tid = b->getTargetId().toLowerCase();
		auto shouldBeVisible = searchTerm.isEmpty() || (tid.isNotEmpty() && tid.contains(searchTerm));
		b->updateSearchVisibility(shouldBeVisible, searchTerm);
		return false;
	});

	b->resized();
}

int MatrixBase::SearchBar::getHeightToUse()
{
	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			auto labelHeight = ss->getLocalBoundsFromText("test").getHeight();
			auto shouldBeVisible = ss->getPropertyValue({ "display", 0}).toString() != "none";
			ss->setPropertyVariable("icon", searchIcon);
			return shouldBeVisible ? labelHeight : 0;
		}
	}

	return 32;
}

void MatrixBase::SearchBar::editorShown(Label* label, TextEditor& te)
{
	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(te, { ".search" });
	simple_css::FlexboxComponent::Helpers::invalidateCache(te);
	te.addListener(this);

	clearButton.setVisible(false);

	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(&te))
		{
			ss->setPropertyVariable("icon", searchIcon);
			ss->setupComponent(root, &te, 0);
		}
	}

	te.setSelectAllWhenFocused(true);
}

void MatrixBase::RowBase::setIntensityConverter(MatrixModulator* mm)
{
	auto cd = mm->getIntensityTextConstructData();
	
	customConverters.clear();

	for(int i = 0; i < getNumIntensitySliders(); i++)
	{
		auto si = getIntensitySlider(i);
		cd.connectedSlider = si;
		cd.tm = getTargetModeForIntensitySlider(i);
		customConverters.add(new MatrixIds::Helpers::IntensityTextConverter(cd));
		customConverters.getLast()->showInactive = true;
		auto vtc = ValueToTextConverter::createForCustomClass(customConverters.getLast());
		si->valueFromTextFunction = vtc;
		si->textFromValueFunction = vtc;
		si->updateText();
		si->repaint();
	}
}

void MatrixBase::RowBase::setIntensityConverter(JavascriptMidiProcessor* jmp, int componentIndex)
{
	using S = ScriptingApi::Content::ScriptSlider;

	customConverters.clear();

	if(auto s = dynamic_cast<S*>(jmp->getScriptingContent()->getComponent(componentIndex)))
	{
		for(int i = 0; i < getNumIntensitySliders(); i++)
		{
			auto si = getIntensitySlider(i);
			auto sourceIndex = getSourceIndexForIntensitySlider(i);
			auto cd = s->createIntensityConverter(sourceIndex);

			cd.connectedSlider = si;
			customConverters.add(new MatrixIds::Helpers::IntensityTextConverter(cd));
			customConverters.getLast()->showInactive = true;
			auto vtc = ValueToTextConverter::createForCustomClass(customConverters.getLast());
			si->valueFromTextFunction = vtc;
			si->textFromValueFunction = vtc;
			si->updateText();
			si->repaint();
		}
	}
}

void MatrixBase::RowBase::updateIntensityConverter()
{
	auto tt = MatrixIds::Helpers::getTargetType(getMainController(), getTargetId());

	if(tt == MatrixIds::Helpers::TargetType::Modulators)
	{
		auto mp = ProcessorHelpers::getFirstProcessorWithName(getMainController()->getMainSynthChain(), getTargetId());

		if(auto mm = dynamic_cast<MatrixModulator*>(mp))
			setIntensityConverter(mm);
	}
	else
	{
		if(auto jmp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getMainController()))
		{
			auto content = jmp->getScriptingContent();

			for(int i = 0; i < content->getNumComponents(); i++)
			{
				using S = ScriptingApi::Content::ScriptSlider;

				if(auto slider = dynamic_cast<S*>(content->getComponent(i)))
				{
					auto p = S::matrixTargetId;
					auto tid = content->getComponent(i)->getScriptObjectProperty(p).toString();

					if(tid == getTargetId())
						setIntensityConverter(jmp, i);
				}
			}
		}
	}
}

int MatrixBase::RowBase::getHeightToUse(int fullWidth)
{
	if(useCSS)
	{
		return getAutoHeightForWidth((float)fullWidth);
	}
	else
	{
		return getDefaultRowHeight() + Margin;
	}
}


MatrixContent::Controller::ModulationDragger::ModulationDragger(const String& name):
	p64("228.t0FYgMEQn4KjCwFMYIDQn4KjCwFMYIDQfI+oCwF..BCQ..DgCwFMYIDQvrQPCwFMYIDQPL3aCwFYgMEQPL3aCwFYgMEQXI1JCwFaGeDQXI1JCwFYelEQPS.xBwFl3sFQXI1JCwFnd+EQXI1JCwFnd+EQPL3aCwFylCGQPL3aCwFylCGQvrQPCwF.fEHQ..DgCwFylCGQfI+oCwFylCGQn4KjCwFnd+EQn4KjCwFnd+EQXwrrCwFl3sFQXwrrCwFYelEQH6m0CwFaGeDQXwrrCwFYgMEQXwrrCwFYgMEQn4KjCMVY")
{
	setName(name);
	setRepaintsOnMouseActivity(true);
	setTooltip("Drag on a knob / slider to modulate it with " + name);
	auto id = String("#") + getName().toLowerCase().replace(" ", "_");
	simple_css::FlexboxComponent::Helpers::setCustomType(*this, simple_css::Selector(simple_css::ElementType::Panel));
	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { ".dragger", id });
	simple_css::FlexboxComponent::Helpers::setFallbackStyleSheet(*this, "width: auto; height: 24px;--dragPath:" + p64 + ";");
	setMouseCursor(MouseCursor::PointingHandCursor);
}

void MatrixContent::Controller::ModulationDragger::mouseDown(const MouseEvent& e)
{
	auto sourceIndex = findParentComponentOfClass<Controller>()->draggers.indexOf(this);

	MatrixIds::Helpers::startDragging(this, sourceIndex);

	auto chain = findParentComponentOfClass<Controller>()->getMainController()->getMainSynthChain();

	if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
	{
		gc->sendDragMessage(sourceIndex, "", GlobalModulatorContainer::DragAction::DragStart);

		if(gc->matrixProperties.selectableSources)
			gc->currentMatrixSourceBroadcaster.sendMessage(sendNotificationSync, sourceIndex);
	}
		
}

void MatrixContent::Controller::ModulationDragger::mouseUp(const MouseEvent& event)
{
	MatrixIds::Helpers::stopDragging(this);
}

MatrixContent::Controller::Controller(MainController* mc, Component& p):
  FlexboxComponent(simple_css::Selector(simple_css::ElementType::Panel)),
  ControlledObject(mc),
  addButton("Add"),
  removeButton("Remove"),
  clearButton("Clear")
{
	FlexboxComponent::addFlexItem(addButton);
	FlexboxComponent::addFlexItem(removeButton);
	FlexboxComponent::addFlexItem(clearButton);

	addButton.addListener(this);
	removeButton.addListener(this);
	clearButton.addListener(this);

	initLaf(&p);

	addButton.setEnabled(false);
	removeButton.setEnabled(false);
	clearButton.setEnabled(false);

	addButton.setTooltip("Add a new modulation connection");
	removeButton.setTooltip("Remove the last modulation connection");
	clearButton.setTooltip("Clear all modulation connections");

	Helpers::writeSelectorsToProperties(addButton, { "#add", ".control-button"});
	Helpers::writeSelectorsToProperties(removeButton, { "#remove", ".control-button"} );
	Helpers::writeSelectorsToProperties(clearButton, { "#clear", ".control-button"} );
	Helpers::writeSelectorsToProperties(*this, { "#controller"} );

	Helpers::setFallbackStyleSheet(addButton,    "width: 128px;height:24px;");
	Helpers::setFallbackStyleSheet(removeButton, "width: 128px;height:24px;");
	Helpers::setFallbackStyleSheet(clearButton, "width: 128px;height:24px;");
	Helpers::setFallbackStyleSheet(*this,        "width:100%;gap:10px;height:100%;margin:5px;display:flex;flex-wrap: wrap;align-items:center;background:rgba(0,0,0,0.1);border-radius:3px;");

	auto chain = getMainController()->getMainSynthChain();

	if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
	{
		auto modChain = gc->getChildProcessor(ModulatorSynth::InternalChains::GainModulation);

		for(int i = 0; i < modChain->getNumChildProcessors(); i++)
		{
			auto sourceId = modChain->getChildProcessor(i)->getId();
			auto nd = new ModulationDragger(sourceId);
			draggers.add(nd);
			addFlexItem(*nd);
		}

		gc->currentMatrixSourceBroadcaster.addListener(*this, [](Controller& c, int index)
		{
			int idx = 0;

			for(auto& d: c.draggers)
			{
				d->currentlySelected = index == idx++;
				d->repaint();
			};
		});
	}
}

MatrixContent::Controller::~Controller()
{
	auto chain = getMainController()->getMainSynthChain();

	if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(chain))
	{
		gc->currentMatrixSourceBroadcaster.removeListener(*this);
	}
}

int MatrixContent::Controller::getHeightToUse(int w)
{
	if(w == lastWidth)
		return lastHeight;

	lastHeight = (int)getAutoHeightForWidth((float)w);
	lastWidth = w;
	return lastHeight;
}

void MatrixContent::Controller::setValueTree(const ValueTree& data_, const String& targetId_, UndoManager* um_)
{
	data = data_;
	targetId = targetId_;
	um = um_;

	addButton.setEnabled(data.isValid());
	removeButton.setEnabled(data.isValid());
	clearButton.setEnabled(data.isValid());
}

void MatrixContent::Controller::paint(Graphics& g)
{
	String message;

	if(!data.isValid())
	{
		message = "No global modulator container found!";
	}

	using namespace simple_css;

	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			Renderer r(this, root->stateWatcher);
			root->stateWatcher.checkChanges(this, ss, r.getPseudoClassState());

			auto b = getLocalBounds().toFloat();
			r.drawBackground(g, b, ss);

			if(!b.isEmpty())
				r.renderText(g, b, message, ss);
		}
	}
}

void MatrixContent::Controller::resized()
{
	initLaf(this);
	FlexboxComponent::resized();
}

void MatrixContent::Controller::buttonClicked(Button* b)
{
	if(!data.isValid())
		return;

	if(b == &addButton)
		MatrixIds::Helpers::addConnection(data, getMainController(), targetId);
	if(b == &removeButton)
		MatrixIds::Helpers::removeLastConnection(data, um, targetId);
	if(b == &clearButton)
		MatrixIds::Helpers::removeAllConnections(data, um, targetId);
}

void MatrixContent::Controller::initLaf(Component* p)
{
	if(laf == nullptr)
	{
		if(p == nullptr)
			p = this;

		if(auto c = simple_css::CSSRootComponent::find(*p))
		{
			laf = new simple_css::StyleSheetLookAndFeel(*c);

			addButton.setLookAndFeel(laf);
			removeButton.setLookAndFeel(laf);
			clearButton.setLookAndFeel(laf);
			setCSS(c->css);
			rebuildLayout();
		}
	}
}

MatrixContent::MatrixContent(MainController* mc, const String& targetId_, bool useViewport_):
	MatrixBase(mc),
	targetId(targetId_),
	data(MatrixIds::Helpers::getMatrixDataFromGlobalContainer(mc))
{
	addAndMakeVisible(searchBar);
	setUseViewport(useViewport_);

	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { ".matrix" });

	if(data.isValid())
		rowListener.setCallback(data, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(MatrixContent::onRowChange));

	addAndMakeVisible(header);
}

void MatrixContent::onRowChange(ValueTree v, bool wasAdded)
{
	if(MatrixIds::Helpers::matchesTarget(v, targetId))
	{
		if(wasAdded)
		{
			Component* parent = useViewport ? &rowContent : this;

			auto r = new Row(getMainController(), v, targetId);
			r->setSliderStyle(sliderStyle);

			parent->addAndMakeVisible(r);

			r->setLookAndFeel(&getLookAndFeel());

			if(auto root = simple_css::CSSRootComponent::find(*this))
				r->setCSS(root->css);

			rows.add(r);
		}
		else
		{
			for(auto r: rows)
			{
				if(r->data == v)
				{
					rows.removeObject(r);
					break;
				}
			}
		}

		if(useViewport)
			resized();
		else
			setSize(getWidth(), getHeightToUse(getHeight()));

		if(resizeFunction)
			resizeFunction();
	}
}

int MatrixContent::getHeightToUse(int parentHeight) const
{
	if(useViewport)
		return parentHeight;

	int h = 0;

	if(getWidth() == 0)
		return h;

	if(header.isVisible())
		h += const_cast<Header*>(&header)->getHeightToUse(getWidth());

	for(auto r: rows)
		h += r->getHeightToUse(getWidth());

	return h;
}

void MatrixContent::resized()
{
	auto b = getLocalBounds();

	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			auto ma = ss->getArea(b.toFloat(), { simple_css::PropertyKey("margin", 0)});
			auto pd = ss->getArea(ma, { simple_css::PropertyKey("padding", 0)});
			b = pd.toNearestInt();
		}

		header.setCSS(root->css);
	}

	auto w = useViewport ? b.getWidth() - rowViewport.getScrollBarThickness() : b.getWidth();

	positionSearchBar(b);

	if(header.isVisible())
	{
		auto hb = b.removeFromTop(header.getHeightToUse(w));
		header.setBounds(hb.withWidth(w));
	}

	if(useViewport)
	{
		rowViewport.setBounds(b);
		Rectangle<int> contentBounds(0, 0, w, 100000);

		int h = 0;
		for(auto r: rows)
		{
			if(r->isVisible())
			{
				auto thisHeight = r->getHeightToUse(w);
				r->setBounds(contentBounds.removeFromTop(thisHeight));
				h += thisHeight;
			}
		}

		rowContent.setSize(w, h);
	}
	else
	{
		for(auto r: rows)
		{
			auto h = r->getHeightToUse(b.getWidth());
			r->setBounds(b.removeFromTop(h));
		}
	}

	repaint();
}

void MatrixContent::paint(Graphics& g)
{
	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			simple_css::Renderer r(this, root->stateWatcher);

			int state = 0;

			if(rows.isEmpty())
				state |= (int)simple_css::PseudoClassType::Empty;

			r.setPseudoClassState(state, true);
			r.drawBackground(g, getLocalBounds().toFloat(), ss);
			r.renderText(g, getLocalBounds().toFloat(), {}, ss);
		}
	}
}

void MatrixContent::setSliderStyle(Slider::SliderStyle newSliderStyle)
{
	if(sliderStyle != newSliderStyle)
	{
		MatrixBase::setSliderStyle(newSliderStyle);

		for(auto r: rows)
			r->setSliderStyle(sliderStyle);
	}
}

void MatrixContent::setUseViewport(bool shouldUseViewport)
{
	if(shouldUseViewport != useViewport)
	{
		useViewport = shouldUseViewport;
		if(useViewport)
		{
			rowViewport.setViewedComponent(&rowContent, false);
			addAndMakeVisible(rowViewport);
			rowViewport.setScrollBarThickness(12);
			sf.addScrollBarToAnimate(rowViewport.getVerticalScrollBar());
		}
	}
}

SliderMatrix::SliderMatrix(MainController* mc, const String& targetFilter):
	MatrixBase(mc),
	gc(ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(mc->getMainSynthChain()))
{

	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*this, { ".matrix" });

	MatrixIds::Helpers::fillModSourceList(getMainController(), sources);

	if(targetFilter.isEmpty())
		MatrixIds::Helpers::fillModTargetList(getMainController(), targets, MatrixIds::Helpers::TargetType::All);
	else
		targets.add(targetFilter);

	addAndMakeVisible(searchBar);
	addAndMakeVisible(header = new Header(sources));

	addAndMakeVisible(viewport);
	viewport.setViewedComponent(&content, false);
	
	sf.addScrollBarToAnimate(viewport.getVerticalScrollBar());

	for(auto t: targets)
	{
		auto r = new Row(*this, sources, t);
		rows.add(r);
		content.addAndMakeVisible(r);
	}

	propertyListener.setCallback(gc->getMatrixModulatorData(), 
      { MatrixIds::Intensity, MatrixIds::Mode },
      valuetree::AsyncMode::Asynchronously,
      BIND_MEMBER_FUNCTION_2(SliderMatrix::onPropertyChange));

	connectionListener.setCallback(gc->getMatrixModulatorData(),
		valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(SliderMatrix::onConnectionChange));
}

SliderMatrix::Header::Header(const StringArray& sources):
	HeaderBase()
{
	auto c = addTextElement({ ".targetLabel"}, "Targets");
	Helpers::appendToElementStyle(*c, "width:80px;flex-grow:0;");

	for(auto id: sources)
	{
		auto c = addHeaderItem(Identifier(), id);
		Helpers::appendToElementStyle(*c, "flex-grow:1;");
	}
}

SliderMatrix::Row::Row(SliderMatrix& parent_, const StringArray& sources, const String& targetId_):
	RowBase(parent_.getMainController()),
	parent(parent_),
	targetId(targetId_)
{
	

	int idx = 0;

	Helpers::setFallbackStyleSheet(*this, "display:flex;");

	setTextElementSelector(simple_css::ElementType::Label);
	auto tl = addTextElement({ ".targetLabel"}, targetId);
	tl->getProperties().remove("style");
	Helpers::appendToElementStyle(*tl, "width:80px;flex-grow:0;");

	for(auto s: sources)
	{
		auto si = new IntensitySlider(Identifier(s));

		si->setCustomPopupFunction(BIND_MEMBER_FUNCTION_1(Row::handlePopup));
		si->setDoubleClickReturnValue(true, 0.0);
		Helpers::writeSelectorsToProperties(*si, { ".slider" });
		Helpers::setFallbackStyleSheet(*si, "height:30px;flex-grow:1;");

		addFlexItem(*si);
		si->addListener(this);
		intensitySliders.add(si);

		auto con = getConnectionForSlider(idx, false);
		auto shouldBeActive = con.isValid() && (double)con[MatrixIds::Intensity] != 0.0;
		si->setActive(shouldBeActive, con, true);

		idx++;
	}

	updateIntensityConverter();
}

ValueTree SliderMatrix::Row::getConnectionForSlider(int sourceIndex, bool addIfNotExisting) const
{
	auto md = parent.gc->getMatrixModulatorData();

	for(auto c: md)
	{
		if(MatrixIds::Helpers::matchesTarget(c, targetId) && (int)c[MatrixIds::SourceIndex] == sourceIndex)
		{
			return c;
		}
	}

	if(addIfNotExisting)
	{
		return MatrixIds::Helpers::addConnection(md, parent.getMainController(), targetId, sourceIndex);
	}

	return ValueTree();
}

void SliderMatrix::Row::sliderValueChanged(Slider* slider)
{
	auto sourceIndex = intensitySliders.indexOf(dynamic_cast<IntensitySlider*>(slider));

	auto um = parent.getMainController()->getControlUndoManager();

	auto v = getConnectionForSlider(sourceIndex, true);
	v.setProperty(MatrixIds::Intensity, var(slider->getValue()), um);

	
}

void SliderMatrix::Row::sliderDragEnded(Slider* slider)
{
	if(slider->getDoubleClickReturnValue() == slider->getValue())
	{
		auto gm = parent.gc->getMatrixModulatorData();
		auto um = parent.getMainController()->getControlUndoManager();
		auto sourceIndex = intensitySliders.indexOf(dynamic_cast<IntensitySlider*>(slider));
		MatrixIds::Helpers::removeConnection(gm, um, targetId, sourceIndex);
	}
}

void SliderMatrix::Row::handlePopup(const MouseEvent& e)
{
	auto sourceIndex = intensitySliders.indexOf(dynamic_cast<IntensitySlider*>(e.eventComponent));

	if(sourceIndex == -1)
		return;

	auto c = getConnectionForSlider(sourceIndex, false);

	if(c.isValid())
	{
		auto um = parent.getMainController()->getControlUndoManager();

		ScopedPointer<LookAndFeel> laf;
		PopupMenu m;

		m.setLookAndFeel(&getLookAndFeel());

		auto inverted = (bool)c[MatrixIds::Inverted];
		auto mode = (int)c[MatrixIds::Mode];

		m.addItem(1, "Invert", true, inverted);
		m.addSeparator();
		m.addItem(2, "Scale", true, mode == 0);
		m.addItem(3, "Unipolar", true, mode == 1);
		m.addItem(4, "Bipolar", true, mode == 2);

		if(auto r = m.show())
		{
			if(r == 1)
				c.setProperty(MatrixIds::Inverted, !inverted, um);
			else
				c.setProperty(MatrixIds::Mode, r-2, um);
		}
	}
}

void SliderMatrix::Row::resized()
{
	auto b = getLocalBounds();

	if(intensitySliders.isEmpty() || b.isEmpty())
		return;

	if(useCSS)
	{
		FlexboxComponent::resized();
	}
	else
	{
		auto w = b.getWidth() / intensitySliders.size();

		for(auto s: intensitySliders)
			s->setBounds(b.removeFromLeft(w));
	}
}

void SliderMatrix::Row::updateSearchVisibility(bool shouldBeVisible, const String& searchTerm)
{
	Array<IntensitySlider*> matches;

	if(searchTerm.isNotEmpty())
	{
		for(auto si: intensitySliders)
		{
			if(si->getName().toLowerCase().contains(searchTerm))
				matches.add(si);
		}
	}

	if(matches.isEmpty())
	{
		for(int i = 0; i < intensitySliders.size(); i++)
			setFlexChildVisibility(i, false, false);

		RowBase::updateSearchVisibility(shouldBeVisible, searchTerm);
		return;
	}

	int idx = 1;

	for(auto si: intensitySliders)
	{
		auto show =  matches.contains(si);
		setFlexChildVisibility(idx++, false, !show);
	}

	rebuildLayout();
}

modulation::TargetMode SliderMatrix::Row::getTargetModeForIntensitySlider(int index) const
{
	auto x = getConnectionForSlider(index, false);

	if(x.isValid() && x.hasProperty(MatrixIds::Mode))
		return (scriptnode::modulation::TargetMode)(int)x[MatrixIds::Mode];
	else if (isPositiveAndBelow(index, intensitySliders.size()))
	{
		return intensitySliders[index]->lastMode;
		jassertfalse;
	}


	return modulation::TargetMode::Gain;
}

void SliderMatrix::paint(Graphics& g)
{
	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			simple_css::Renderer r(this, root->stateWatcher);
			r.drawBackground(g, getLocalBounds().toFloat(), ss);

			if(content.getBounds().isEmpty())
				r.renderText(g, getLocalBounds().toFloat(), "No connections", ss);
		}
	}
}

void SliderMatrix::resized()
{
	auto b = getLocalBounds();

	if(auto root = simple_css::CSSRootComponent::find(*this))
	{
		if(auto ss = root->css.getForComponent(this))
		{
			auto ma = ss->getArea(b.toFloat(), { simple_css::PropertyKey("margin", 0)});
			auto pd = ss->getArea(ma, { simple_css::PropertyKey("padding", 0)});
			b = pd.toNearestInt();
		}

		

		header->setCSS(root->css);

		for(auto r: rows)
			r->setCSS(root->css);
	}

	positionSearchBar(b);

	if(header->isVisible())
		header->setBounds(b.removeFromTop(header->getHeightToUse(b.getWidth())));

	viewport.setBounds(b);

	int h = 0;

	auto cb = Rectangle<int>(0, 0, viewport.getWidth() - viewport.getScrollBarThickness(), 100000);

	auto wasEmpty = content.getHeight() == 0;

	for(auto r: rows)
	{
		auto th = r->getHeightToUse(cb.getWidth());

		if(!r->isVisible())
			continue;

		r->setBounds(cb.removeFromTop(th));
		h += th;
	}

	

	content.setSize(cb.getWidth(), h);

	auto isEmpty = content.getHeight() == 0;

	if(isEmpty != wasEmpty)
	{
		int state = 0;

		if(isEmpty)
			state |= (int)simple_css::PseudoClassType::Empty;

		simple_css::FlexboxComponent::Helpers::writeManualPseudoState(*this, state);
		simple_css::FlexboxComponent::Helpers::invalidateCache(*this);
		repaint();
	}
}

void SliderMatrix::setSliderStyle(Slider::SliderStyle newSliderStyle)
{
	if(sliderStyle != newSliderStyle)
	{
		MatrixBase::setSliderStyle(newSliderStyle);

		for(auto r: rows)
		{
			for(auto s: r->intensitySliders)
			{
				s->setSliderStyle(sliderStyle);

				if(sliderStyle != Slider::RotaryHorizontalVerticalDrag)
				{
					s->setColour(Slider::textBoxOutlineColourId, Colours::transparentBlack);
					s->setTextBoxStyle(Slider::NoTextBox, false, 0, 0);
				}
			}
		}
	}
}

void SliderMatrix::onPropertyChange(const ValueTree& v, const Identifier& id)
{
	auto sourceIndex = v[MatrixIds::SourceIndex];
	auto tid = v[MatrixIds::TargetId].toString();

	for(auto r: rows)
	{
		if(MatrixIds::Helpers::matchesTarget(v, r->targetId))
		{
			if(isPositiveAndBelow(sourceIndex, r->intensitySliders.size()))
			{
				auto sl = r->intensitySliders[sourceIndex];

				if(id == MatrixIds::Intensity || id == MatrixIds::Mode)
				{
					sl->setActive(true, v, true);
				}
			}

			if(id == MatrixIds::Mode)
				r->updateIntensityConverter();
		}
	}
}

void SliderMatrix::onConnectionChange(const ValueTree& v, bool wasAdded)
{
	auto sourceIndex = (int)v[MatrixIds::SourceIndex];
	auto targetId = v[MatrixIds::TargetId].toString();

	if(!wasAdded && (sourceIndex == -1 || targetId.isEmpty()))
	{
		auto md = gc->getMatrixModulatorData();

		for(auto r: rows)
		{
			for(int i = 0; i < r->intensitySliders.size(); i++)
			{
				auto isConnected = MatrixIds::Helpers::getConnection(md, i, targetId).isValid();

				if(!isConnected)
				{
					r->intensitySliders[i]->setActive(false, v, true);
				}
			}
		}

		return;
	}


	for(auto r: rows)
	{
		if(MatrixIds::Helpers::matchesTarget(v, r->targetId))
		{
			if(isPositiveAndBelow(sourceIndex, r->intensitySliders.size()))
			{
				r->getIntensitySlider(sourceIndex)->setActive(wasAdded, v, true);
			}

			break;
		}
	}
}

ModulationMatrixBasePanel::ModulationMatrixBasePanel(FloatingTile* parent):
	PanelWithProcessorConnection(parent)
{}

Identifier ModulationMatrixBasePanel::getProcessorTypeId() const
{
	return Identifier("MatrixProcessors");
}

void ModulationMatrixBasePanel::fillModuleList(StringArray& moduleList)
{
	auto mc = getMainController()->getMainSynthChain();
	moduleList.addArray(ProcessorHelpers::getAllIdsForType<GlobalModulatorContainer>(mc));
	moduleList.addArray(ProcessorHelpers::getAllIdsForType<MatrixModulator>(mc));
}

Component* ModulationMatrixBasePanel::createContentComponent(int index)
{
	if(getConnectedProcessor() != nullptr)
	{
		auto targetId = getConnectedProcessor()->getId();

		if(dynamic_cast<GlobalModulatorContainer*>(getConnectedProcessor()) != nullptr)
			targetId = "";

		return createMatrixComponent(targetId);
	}

	return nullptr;
}

ModulationMatrixPanel::ModulationMatrixPanel(FloatingTile* parent):
	ModulationMatrixBasePanel(parent)
{}

var ModulationMatrixPanel::toDynamicObject() const
{
	auto obj = ModulationMatrixBasePanel::toDynamicObject();

	if(auto p = getContent<MatrixBase>())
	{
		obj.getDynamicObject()->setProperty("MatrixStyle", p->getMatrixType().toString());

		auto s = p->sliderStyle;

		switch(s)
		{
		case Slider::SliderStyle::RotaryHorizontalVerticalDrag:
			obj.getDynamicObject()->setProperty("SliderStyle", "Knob");
			break;
		case Slider::SliderStyle::LinearHorizontal:
			obj.getDynamicObject()->setProperty("SliderStyle", "Horizontal");
			break;
		case Slider::SliderStyle::LinearBarVertical:
			obj.getDynamicObject()->setProperty("SliderStyle", "Vertical");
			break;
		}
	}
	else
	{

		obj.getDynamicObject()->setProperty("SliderStyle", "Knob");
	}

	return obj;
}

void ModulationMatrixPanel::fromDynamicObject(const var& object)
{
	auto tid = object["MatrixStyle"].toString();

	if(tid.isNotEmpty())
		typeId = Identifier(tid);
	else
		typeId = Identifier("TableMatrix");

	targetId = object["TargetFilter"].toString(); 

	ModulationMatrixBasePanel::fromDynamicObject(object);

	StringArray sa;
	sa.add("Knob");
	sa.add("Horizontal");
	sa.add("Vertical");

	auto idx = sa.indexOf(object["SliderStyle"].toString());

	if(auto p = getContent<MatrixBase>())
	{
		switch(idx)
		{
		case 0:
			p->setSliderStyle(Slider::SliderStyle::RotaryHorizontalVerticalDrag);
			break;
		case 1:
			p->setSliderStyle(Slider::SliderStyle::LinearBar);
			break;
		case 2:
			p->setSliderStyle(Slider::SliderStyle::LinearBarVertical);
			break;
		}
	}
	
}

int ModulationMatrixPanel::getNumDefaultableProperties() const
{
	return numSpecialPanelIds;
}

Identifier ModulationMatrixPanel::getDefaultablePropertyId(int index) const
{
	if(index < SpecialPanelIds::SliderType)
		return ModulationMatrixBasePanel::getDefaultablePropertyId(index);

	switch(index)
	{
	case SpecialPanelIds::MatrixType: return Identifier("MatrixType");
	case SpecialPanelIds::SliderType: return Identifier("SliderType");
	case SpecialPanelIds::TargetFilter: return Identifier("TargetFilter");
	}

	jassertfalse;
	return Identifier();
}

var ModulationMatrixPanel::getDefaultProperty(int index) const
{
	if(index < SpecialPanelIds::SliderType)
		return ModulationMatrixBasePanel::getDefaultProperty(index);
	switch(index)
	{
	case SpecialPanelIds::MatrixType: return var("TableMatrix");
	case SpecialPanelIds::SliderType: return var("Knob");
	case SpecialPanelIds::TargetFilter: return var("");
	}

	return var();
}

Component* ModulationMatrixPanel::createMatrixComponent(const String& processorId)
{
	auto targetIdToUse = processorId;

	if(!targetId.isEmpty())
		targetIdToUse = targetId;

	if(typeId == Identifier("SliderMatrix"))
		return new SliderMatrix(getMainController(), targetId);
	else
		return new MatrixContent(getMainController(), targetId, true);	
}

ModulationMatrixControlPanel::ModulationMatrixControlPanel(FloatingTile* parent):
	ModulationMatrixBasePanel(parent)
{}

Component* ModulationMatrixControlPanel::createMatrixComponent(const String& targetId)
{
	if(getParentShell()->isOnInterface())
	{
		auto controller = new MatrixContent::Controller(getMainController(), *getParentShell());

		if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(getMainController()->getMainSynthChain()))
		{
			controller->setValueTree(gc->getMatrixModulatorData(),
								    targetId, 
									getMainController()->getControlUndoManager());
		}

		return controller;	
	}
	else
	{
		struct FloatingTileParent: public Component,
								   public MatrixContent::Parent
		{
			FloatingTileParent(MainController* mc, const String& targetId):
			  controller(mc, *this)
			{
				addAndMakeVisible(controller);
				controller.setCSS(css);

				if(auto gc = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(mc->getMainSynthChain()))
				{
					controller.setValueTree(gc->getMatrixModulatorData(),
										    targetId, 
											mc->getControlUndoManager());

					// this is not working yet, check the CSS root component stuff...
					jassertfalse;
				}
			}

			void resized() override
			{
				controller.setBounds(getLocalBounds());
			}

			MatrixContent::Controller controller;
		};

		return new FloatingTileParent(getMainController(), targetId);
	}
}

} // namespace hise
