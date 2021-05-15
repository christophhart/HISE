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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;

namespace RangeIcons
{
	static const unsigned char pinIcon[] = { 110,109,172,28,90,62,0,0,0,0,108,8,172,128,64,0,0,0,0,108,8,172,128,64,139,108,103,63,108,168,198,83,64,139,108,103,63,108,168,198,83,64,246,40,68,64,98,160,26,119,64,186,73,92,64,51,51,135,64,4,86,130,64,238,124,135,64,125,63,153,64,108,172,28,34,64,
125,63,153,64,108,238,124,7,64,131,192,6,65,108,94,186,217,63,125,63,153,64,108,0,0,0,0,125,63,153,64,98,111,18,3,60,4,86,130,64,219,249,190,62,186,73,92,64,205,204,108,63,246,40,68,64,108,205,204,108,63,139,108,103,63,108,172,28,90,62,139,108,103,63,
108,172,28,90,62,0,0,0,0,99,101,0,0 };
}

struct RangePresets
{
	RangePresets(const File& fileToLoad_):
		fileToLoad(fileToLoad_)
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
		{
			auto v = ValueTree::fromXml(*xml);

			int index = 1;

			for (auto c : v)
			{
				Preset p;
				p.restoreFromValueTree(c);

				p.index = index++;

				presets.add(p);
			}
		}
		else
		{
			createDefaultRange("0-1", { 0.0, 1.0 }, 0.5);
			createDefaultRange("Inverted 0-1", { 0.0, 1.0 }, -1.0, true);
			createDefaultRange("Osc LFO", { 0.0, 10.0, 0.01, 1.0 });
			createDefaultRange("Osc Freq", { 20.0, 20000.0, 0.1 }, 1000.0);
			createDefaultRange("Freq Ratio Harmonics", { 1.0, 16.0, 1.0 });
			createDefaultRange("Freq Ratio Detune Coarse", { 0.5, 2.0, 0.0 }, 1.0);
			createDefaultRange("Freq Ratio Detune Fine", { 1.0 / 1.1, 1.1, 0.0 }, 1.0);
		}
	}

	void createDefaultRange(const String& id, NormalisableRange<double> d, double midPoint = -10000000.0, bool inverted=false)
	{
		Preset p;
		p.id = id;
		p.nr = d;
		p.inverted = inverted;

		p.index = presets.size() + 1;

		if (d.getRange().contains(midPoint))
			p.nr.setSkewForCentre(midPoint);

		presets.add(p);
	}

	~RangePresets()
	{
		ValueTree v("Ranges");

		for (const auto& p : presets)
			v.addChild(p.exportAsValueTree(), -1, nullptr);

		ScopedPointer<XmlElement> xml = v.createXml();
		fileToLoad.replaceWithText(xml->createDocument(""));
	}

	struct Preset: public RestorableObject
	{
		void restoreFromValueTree(const ValueTree& v)
		{
			nr = RangeHelpers::getDoubleRange(v);
			inverted = RangeHelpers::isInverted(v);

			id = v[PropertyIds::ID].toString();
		}

		ValueTree exportAsValueTree() const override
		{
			ValueTree v("Range");
			v.setProperty(PropertyIds::ID, id, nullptr);
			RangeHelpers::storeDoubleRange(v, inverted, nr, nullptr);
			return v;
		}

		NormalisableRange<double> nr;
		String id;
		int index;
		bool inverted = false;
	};

	File fileToLoad;
	Array<Preset> presets;
};


struct ParameterSlider::RangeComponent : public Component,
	public Timer,
	public TextEditor::Listener
{
	enum MousePosition
	{
		Outside,
		Inside,
		Left,
		Right,
		Nothing
	};

	ParameterSlider& parent;

	RangePresets presets;

	ValueTree connectionSource;

	RangeComponent(bool isTemporary, ParameterSlider& parent_):
		temporary(isTemporary),
		parent(parent_),
		presets(File::getSpecialLocation(File::userDesktopDirectory).getChildFile("RangePresets.xml"))
	{
		connectionSource = getParent().getConnectionSourceTree();

		resetRange = getParentRange();
		oldRange = resetRange;
		startTimer(30);
		timerCallback();
	}

	bool shouldClose = false;
	bool shouldFadeIn = true;

	float closeAlpha = 0.f;

	bool temporary = false;

	void timerCallback() override
	{
		if (exitTime != 0)
		{
			auto now = Time::getMillisecondCounter();

			auto delta = now - exitTime;

			if (delta > 500)
			{
				shouldClose = true;
				exitTime = 0;
			}

			return;
		}

		if (shouldFadeIn)
		{
			closeAlpha += JUCE_LIVE_CONSTANT_OFF(0.15f);

			if (closeAlpha >= 1.0f)
			{
				closeAlpha = 1.0f;
				stopTimer();
				shouldFadeIn = false;
			}

			setAlpha(closeAlpha);
			getParent().setAlpha(1.0f - closeAlpha);

			return;
		}

		if (shouldClose)
		{
			closeAlpha -= JUCE_LIVE_CONSTANT_OFF(0.15f);

			setAlpha(closeAlpha);
			getParent().setAlpha(1.0f - closeAlpha);

			if (closeAlpha < 0.1f)
			{
				stopTimer();
				close(0);
			}

			return;
		}

		Range<double> tr(0.0, 1.0);

		auto cs = currentRange.start;
		auto ce = currentRange.end;

		auto ts = tr.getStart();
		auto te = tr.getEnd();

		auto coeff = 0.7;

		auto ns = cs * coeff + ts * (1.0 - coeff);
		auto ne = ce * coeff + te * (1.0 - coeff);

		currentRange.start = ns;
		currentRange.end = ne;
		repaint();

		auto tl = te - ts;
		auto cl = currentRange.end - currentRange.start;

		if (hmath::abs(tl - cl) < 0.01)
		{
			currentRange = { 0.0, 1.0 };
			stopTimer();
			
			if (temporary && !getLocalBounds().contains(getMouseXYRelative()))
			{
				close(300);
			}	
		}
	}

	Rectangle<float> getTotalArea()
	{
		auto sf = 1.0f / UnblurryGraphics::getScaleFactorForComponent(this);

		auto b = getLocalBounds().toFloat();

		b.removeFromBottom(18.0f);

		b = b.reduced(3.0f * sf, 0.0f);
		return b;
	}

	Rectangle<float> getSliderArea()
	{
		auto sf = jmin(1.0f / UnblurryGraphics::getScaleFactorForComponent(this), 1.0f);

		auto b = getLocalBounds().toFloat().removeFromBottom(24.0f);

		b = b.reduced(3.0f * sf, 12.0f - 3.0f * sf);

		return b;
	}

	Rectangle<float> getRangeArea(bool clip)
	{
		auto fullRange = NormalisableRange<double>(0.0, 1.0);

		auto sf = 1.0f / UnblurryGraphics::getScaleFactorForComponent(this);

		auto b = getTotalArea().reduced(3.0f * sf);

		auto fr = fullRange.getRange();
		auto cr = currentRange.getRange();

		auto sNormalised = (cr.getStart() - fr.getStart()) / fr.getLength();
		auto eNormalised = (cr.getEnd() - fr.getStart()) / fr.getLength();

		if (clip)
		{
			sNormalised = jlimit(0.0, 1.0, sNormalised);
			eNormalised = jlimit(0.0, 1.0, eNormalised);
		}

		auto inner = b;

		inner.removeFromLeft(b.getWidth() * sNormalised);
		inner.removeFromRight((1.0 - eNormalised) * b.getWidth());

		return inner;
	}

	MousePosition getMousePosition(Point<int> p)
	{
		if (dragPos != Nothing)
			return dragPos;

		if (!getLocalBounds().contains(p))
			return Nothing;

		auto r = getRangeArea(true);
		auto x = p.toFloat().getX();

		if (hmath::abs(x - r.getX()) < 8)
			return Left;
		else if (hmath::abs(x - r.getRight()) < 8)
			return Right;
		else if (r.contains(p.toFloat()))
			return Inside;
		else
			return Outside;
	}

	

	void close(int delayMilliseconds)
	{
		Component::SafePointer<ParameterSlider> s = &getParent();
		Component::SafePointer<RangeComponent> safeThis(this);

		auto f = [s, safeThis]()
		{
			if (s.getComponent() != nullptr)
			{
				Desktop::getInstance().getAnimator().fadeOut(safeThis.getComponent(), 100);
				s.getComponent()->currentRangeComponent = nullptr;
				s.getComponent()->setAlpha(1.0f);
				s.getComponent()->resized();
			}
		};

		if (delayMilliseconds == 0)
		{
			MessageManager::callAsync(f);
		}
		else
		{
			shouldFadeIn = false;
			shouldClose = true;
			startTimer(30);
		}
	}


	

	NormalisableRange<double> getParentRange()
	{
		NormalisableRange<double> d;
		auto r = getParent().getRange();
		d.start = r.getStart();
		d.end = r.getEnd();
		d.skew = getParent().getSkewFactor();
		d.interval = getParent().getInterval();
		return d;
	}

	void setNewRange(NormalisableRange<double> rangeToUse, bool inverted, NotificationType updateRange)
	{
		auto& s = getParent();

		RangeHelpers::storeDoubleRange(s.parameterToControl->data, inverted, rangeToUse, s.node->getUndoManager());

		if(connectionSource.isValid())
			RangeHelpers::storeDoubleRange(connectionSource, inverted, rangeToUse, s.node->getUndoManager());

		if (updateRange != dontSendNotification)
			oldRange = rangeToUse;

		repaint();
	}

	void setNewRange(NotificationType updateRange)
	{
		auto& s = getParent();

		auto newStart = oldRange.start + currentRange.start * oldRange.getRange().getLength();
		auto newEnd = oldRange.start + currentRange.end * oldRange.getRange().getLength();

		NormalisableRange<double> nr;
		nr.start = newStart;
		nr.end = newEnd;
		nr.interval = s.getInterval();
		nr.skew = skewToUse;

		auto inv = RangeHelpers::isInverted(connectionSource);

		setNewRange(nr, inv, updateRange);
	}

	void setNewValue(const MouseEvent& e)
	{
		auto t = getTotalArea();
		auto nv = jlimit(0.0f, 1.0f, ((float)e.getPosition().getX() - t.getX()) / t.getWidth());

		auto v = getParentRange().convertFrom0to1(nv);
		getParent().setValue(v, sendNotification);
		repaint();
	}


	void textEditorReturnKeyPressed(TextEditor& t)
	{
		auto r = getParentRange();

		auto v = getParent().getValueFromText(t.getText());

		auto inv = RangeHelpers::isInverted(connectionSource);
		auto isLeft = currentTextPos == Left;// && !inv) || (Right && inv);

		if (currentTextPos == Inside)
			r.setSkewForCentre(v);
		else if (currentTextPos == Outside)
			getParent().setValue(v, sendNotificationAsync);
		else if (isLeft)
			r.start = v;
		else
			r.end = v;

		setNewRange(r, inv, sendNotification);
		createLabel(Nothing);
	}

	void textEditorEscapeKeyPressed(TextEditor&)
	{
		createLabel(Nothing);
	}

	void textEditorFocusLost(TextEditor&) override
	{
		createLabel(Nothing);
	}

	void paint(Graphics& g) override
	{
		UnblurryGraphics ug(g, *this, true);

		auto sf = 1.0f / UnblurryGraphics::getScaleFactorForComponent(this);

		//g.fillAll(Colour(0xFF333336));

		auto bg = Colour(0xFF262626);

		auto on = temporary && ((Time::currentTimeMillis() % 1000) > 500);

		auto wh = Colours::white.withAlpha(on ? 0.08f : 0.05f);

		

		auto w1 = Colours::white;
		auto w2 = Colours::white.withAlpha(0.2f);

		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, getTotalArea(), false);

		//g.setColour(Colours::black.withAlpha(0.3f));
		//g.fillRoundedRectangle(getTotalArea(), 2.0f);
		//g.drawRoundedRectangle(getTotalArea().reduced(0.5f), 2.0f, 1.0f);

		auto inner = getRangeArea(true);
		
		{
			auto copy = getRangeArea(false);
			auto w = copy.getWidth();

			for (int i = 0; i < 3; i++)
			{
				copy.removeFromLeft(w / 4.0f);
				ug.draw1PxVerticalLine(copy.getX(), inner.getY(), inner.getBottom());
			}
		}

		g.setColour(Colours::white.withAlpha(0.05f));
		auto p = getMousePosition(getMouseXYRelative());

		g.setColour(Colours::white.withAlpha(0.3f));

		{
			g.saveState();

			auto b = getLocalBounds();
			g.excludeClipRegion(b.removeFromLeft(getTotalArea().getX() + 2.0f));
			g.excludeClipRegion(b.removeFromRight(getTotalArea().getX() + 2.0f));

			auto d = getParentRange();

			Path path;

			

			auto inv = connectionSource.isValid() && RangeHelpers::isInverted(connectionSource);

			Path valuePath;

			{
				auto startValue = (float)d.convertFrom0to1(0.0);
				valuePath.startNewSubPath({ 0.0f, inv ? startValue : 1.0f - startValue});
				path.startNewSubPath({ 0.0f, inv ? startValue : 1.0f - startValue });
			}
			
			auto modValue = getParent().parameterToControl->getValue();
			auto valueAssI = d.convertTo0to1(modValue);

			//auto valueAssI = d.convertTo0to1(getParent().getValue());

			for (float i = 0.0f; i < 1.0f; i += (0.5f / inner.getWidth()))
			{
				auto v = d.convertFrom0to1(i);
				v = d.snapToLegalValue(v);

				path.lineTo(i, inv ? v : 1.0f - v);

				if (i < valueAssI)
					valuePath.lineTo(i, inv ? v : 1.0f - v);
			}

			auto endValue = (float)d.convertFrom0to1(1.0);

			valuePath.startNewSubPath(1.0f, inv ? endValue : 1.0f - endValue);
			path.startNewSubPath(1.0f, inv ? endValue : 1.0f - endValue);

			auto pBounds = getRangeArea(false).reduced(2.0f * sf);

			path.scaleToFit(pBounds.getX(), pBounds.getY(), pBounds.getWidth(), pBounds.getHeight(), false);
			valuePath.scaleToFit(pBounds.getX(), pBounds.getY(), pBounds.getWidth(), pBounds.getHeight(), false);

			g.setColour(Colours::white.withAlpha(0.5f));

			auto psf = jmin(2.0f, sf * 1.5f);

			Path dashed;
			float dl[2] = { psf * 2.0f, psf * 2.0f };
			PathStrokeType(2.0f * psf).createDashedStroke(dashed, path, dl, 2);

			g.fillPath(dashed);

			g.setColour(Colour(0xFF262626).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.25f)));
			g.strokePath(valuePath, PathStrokeType(psf * JUCE_LIVE_CONSTANT_OFF(4.5f), PathStrokeType::curved, PathStrokeType::rounded));

			g.setColour(Colour(0xFF9099AA).withMultipliedBrightness(JUCE_LIVE_CONSTANT_OFF(1.25f)));
			g.strokePath(valuePath, PathStrokeType(psf * JUCE_LIVE_CONSTANT_OFF(3.0f), PathStrokeType::curved, PathStrokeType::rounded));

			g.restoreState();
		}

		float alpha = 0.5f;

		if (isMouseButtonDown())
			alpha += 0.2f;

		if (isMouseOverOrDragging())
			alpha += 0.1f;

		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(alpha));

		g.setFont(GLOBAL_BOLD_FONT());

		if (!isTimerRunning())
		{
			switch (p)
			{
			case Left:
				g.fillRect(inner.removeFromLeft(4.0f * sf));
				break;
			case Right:
				g.fillRect(inner.removeFromRight(4.0f * sf));
				break;
			case Inside:
			{
				break;
			}
			}
		}

		if (editor == nullptr)
		{
			g.setColour(Colours::white);
			g.drawText(getDragText(p), getLocalBounds().toFloat().removeFromBottom(24.0f), Justification::centred);

			if (p == Outside && !connectionSource.isValid())
			{
				auto b = getSliderArea();

				g.setColour(Colours::white.withAlpha(0.1f));
				g.fillRoundedRectangle(b, b.getHeight() / 2.0f);

				auto nv = getParentRange().convertTo0to1(getParent().getValue());

				auto w = jmax<float>(b.getHeight(), b.getWidth() * nv);

				b = b.removeFromLeft(w);

				g.setColour(Colours::white.withAlpha(isMouseButtonDown(true) ? 0.8f : 0.6f));
				g.fillRoundedRectangle(b, b.getHeight() / 2.0f);
			}
		}
	}

	String getDragText(MousePosition p)
	{
		if (isTimerRunning())
			return {};

		auto newStart = oldRange.start + currentRange.start * oldRange.getRange().getLength();
		auto newEnd = oldRange.start + currentRange.end * oldRange.getRange().getLength();

		switch (p)
		{
		case Left: 
		case Right: return getParent().getTextFromValue(newStart) + " - " + getParent().getTextFromValue(newEnd);
		case Inside: return "Mid: " + String(getParentRange().convertFrom0to1(0.5));
		}

		return {};
	}

	ParameterSlider& getParent() { return parent; }

	void createLabel(MousePosition p)
	{
		if (p == Nothing)
		{
			MessageManager::callAsync([&]()
				{
					editor = nullptr;
					resized();
				});
		}
		else
		{
			currentTextPos = p;
			addAndMakeVisible(editor = new TextEditor());
			editor->addListener(this);

			String t;

			switch (p)
			{
			case Left:   t = getParent().getTextFromValue(getParent().getMinimum()); break;
			case Right:  t = getParent().getTextFromValue(getParent().getMaximum()); break;
			case Inside: t = String(getParentRange().convertFrom0to1(0.5)); break;
			case Outside: t = getParent().getTextFromValue(getParent().getValue()); break;
			}

			editor->setColour(Label::textColourId, Colours::white);
			editor->setColour(Label::backgroundColourId, Colours::transparentBlack);
			editor->setColour(Label::outlineColourId, Colours::transparentBlack);
			editor->setColour(TextEditor::textColourId, Colours::white);

			editor->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
			editor->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
			editor->setColour(TextEditor::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
			editor->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
			editor->setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));

			editor->setJustification(Justification::centred);
			editor->setFont(GLOBAL_BOLD_FONT());
			editor->setText(t, dontSendNotification);
			editor->selectAll();

			editor->grabKeyboardFocus();

			resized();
		}
	}

	void resized() override
	{
		if (editor != nullptr)
		{
			editor->setBounds(getLocalBounds().removeFromBottom(24));
		}
	}

	void mouseEnter(const MouseEvent& e) override
	{
		if (exitTime != 0)
		{
			if (shouldClose)
			{
				jassert(isTimerRunning());
				shouldClose = false;
				shouldFadeIn = true;
			}

			exitTime = 0;
			stopTimer();
		}
	}

	void mouseExit(const MouseEvent& e) override
	{
		dragPos = Nothing;

		

		if (temporary && !isTimerRunning())
		{
			exitTime = Time::getMillisecondCounter();
			startTimer(30);
		}
			

		repaint();
	}

	
	
	void mouseDoubleClick(const MouseEvent& e) override
	{
		temporary = !temporary;

		if (!temporary && !e.mods.isAltDown())
		{
			exitTime = Time::getMillisecondCounter();
			startTimer(30);
		}
	}

	void mouseMove(const MouseEvent& e) override
	{
		if (!shouldClose && temporary && !e.mods.isAltDown() && !isTimerRunning())
		{
			close(300);
		}

		switch (getMousePosition(e.getPosition()))
		{
		case Left:	  setMouseCursor(MouseCursor::LeftEdgeResizeCursor); break;
		case Right:   setMouseCursor(MouseCursor::RightEdgeResizeCursor); break;
		case Outside: setMouseCursor(MouseCursor::NormalCursor); break;
		case Inside:  setMouseCursor(MouseCursor::UpDownResizeCursor); break;
		}

		repaint();
	}

	void mouseDown(const MouseEvent& e) override
	{
		if (e.mods.isShiftDown())
		{
			temporary = false;
			createLabel(getMousePosition(e.getPosition()));
			return;
		}

		if (e.mods.isRightButtonDown())
		{
			Component::SafePointer<RangeComponent> rc = this;

			PopupMenu m;
			m.setLookAndFeel(&getParent().laf);

			m.addItem(1, "Make sticky", true, !temporary);

			m.addSeparator();

			PopupMenu ranges;

			for (const auto& p : presets.presets)
				ranges.addItem(9000 + p.index, p.id, true, RangeHelpers::isEqual(getParentRange(), p.nr));

			m.addSubMenu("Load Range Preset", ranges);
			m.addItem(3, "Save Range Preset");
			m.addSeparator();
			m.addItem(4, "Reset Range");
			m.addItem(6, "Reset skew", getParent().getSkewFactor() != 1.0);
			m.addSeparator();
			m.addItem(5, "Invert range", connectionSource.isValid(), RangeHelpers::isInverted(connectionSource));
			m.addItem(7, "Copy range to source", connectionSource.isValid());

			auto r = m.show();

			if (rc.getComponent() == 0)
				return;

			if (r == 0)
			{
				if(temporary && !getLocalBounds().contains(getMouseXYRelative()))
					close(300);
			}

			if (r == 1)
			{
				temporary = !temporary;

				if (temporary)
					close(300);
			}
			if (r == 4)
				setNewRange(resetRange, false, sendNotification);

			if (r == 3)
			{
				auto n = PresetHandler::getCustomName("Range");

				if (n.isNotEmpty())
				{
					auto cr = getParentRange();
					presets.createDefaultRange(n, cr);
				}
			}
			if (r == 5)
			{
				auto cr = getParentRange();
				auto inv = !RangeHelpers::isInverted(connectionSource);
				setNewRange(cr, inv, sendNotification);
			}
			if (r == 6)
			{
				auto cr = getParentRange();
				cr.skew = 1.0;
				auto inv = RangeHelpers::isInverted(connectionSource);
				setNewRange(cr, inv, sendNotification);
			}
			if (r == 7)
			{
				auto cr = getParentRange();

				auto sourceParameterTree = connectionSource.getParent().getParent();

				if (sourceParameterTree.isValid() && sourceParameterTree.getType() == PropertyIds::Parameter)
				{
					auto inv = RangeHelpers::isInverted(connectionSource);
					RangeHelpers::storeDoubleRange(sourceParameterTree, inv, cr, getParent().node->getUndoManager());
				}
			}
			if (r > 9000)
			{
				auto p = presets.presets[r - 9001];
				setNewRange(p.nr, p.inverted, sendNotification);
			}
			
			repaint();
			return;
		}


		dragPos = getMousePosition(e.getPosition());

		oldRange = getParentRange();

		if (dragPos == Outside)
			setNewValue(e);

		currentRangeAtDragStart = currentRange;
		currentRangeAtDragStart.skew = getParent().getSkewFactor();

		skewToUse = currentRangeAtDragStart.skew;
		repaint();
	}

	void mouseUp(const MouseEvent& e) override
	{
		if (e.mods.isRightButtonDown() || e.mods.isShiftDown())
			return;

		auto& s = getParent();

		setNewRange(sendNotification);

		if (dragPos == Left || dragPos == Right)
		{
			shouldClose = false;
			startTimer(30);
		}

		dragPos = Nothing;
		repaint();
	}
	
	void mouseDrag(const MouseEvent& e) override
	{
		if (e.mods.isRightButtonDown() || e.mods.isShiftDown())
			return;

		if (dragPos == Outside)
		{
			if(!connectionSource.isValid())
				setNewValue(e);
		}
		if (dragPos == Inside)
		{
			auto d = (float)e.getDistanceFromDragStartY() / getTotalArea().getHeight();
			auto delta = hmath::pow(2.0f, -1.0f * d);
			auto skewStart = currentRangeAtDragStart.skew;

			skewToUse = jmax(0.001, skewStart * delta);

			currentRange.skew = skewToUse;
			setNewRange(dontSendNotification);
		}
		else if (dragPos == Left || dragPos == Right)
		{
			auto d = (float)e.getDistanceFromDragStartX() / getTotalArea().getWidth();

			if (e.mods.isCommandDown())
			{
				d -= hmath::fmod(d, 0.25f);
			}

			auto r = currentRangeAtDragStart;

			if (dragPos == Left)
			{
				auto v = jmin(currentRangeAtDragStart.end - 0.05, currentRangeAtDragStart.start + d * currentRangeAtDragStart.getRange().getLength());
				currentRange.start = v;
			}
			else
			{
				auto v = jmax(currentRangeAtDragStart.start + 0.05, currentRangeAtDragStart.end + d * currentRangeAtDragStart.getRange().getLength());
				currentRange.end = v;
			}

			setNewRange(dontSendNotification);
		}

		repaint();
	}

	size_t exitTime = 0;

	double skewToUse = 1.0;

	MousePosition dragPos = Nothing;
	NormalisableRange<double> currentRangeAtDragStart;
	NormalisableRange<double> currentRange = { 0.0, 1.0 };

	NormalisableRange<double> oldRange;

	NormalisableRange<double> resetRange;

	MousePosition currentTextPos;
	ScopedPointer<TextEditor> editor;
};


ParameterSlider::ParameterSlider(NodeBase* node_, int index_) :
	SimpleTimer(node_->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	parameterToControl(node_->getParameter(index_)),
	node(node_),
	pTree(node_->getParameter(index_)->getTreeWithValue()),
	index(index_)
{
	addAndMakeVisible(rangeButton);

	setName(pTree[PropertyIds::ID].toString());

	connectionListener.setTypesToWatch({ PropertyIds::Connections, PropertyIds::ModulationTargets });
	connectionListener.setCallback(pTree.getRoot(), valuetree::AsyncMode::Asynchronously,
		BIND_MEMBER_FUNCTION_2(ParameterSlider::updateOnConnectionChange));

	rangeListener.setCallback(pTree, RangeHelpers::getRangeIds(),
		valuetree::AsyncMode::Coallescated,
		BIND_MEMBER_FUNCTION_2(ParameterSlider::updateRange));

	valueListener.setCallback(pTree, { PropertyIds::Value },
		valuetree::AsyncMode::Asynchronously,
		[this](Identifier, var newValue)
	{
		setValue(newValue, dontSendNotification);
		repaint();
	});

	addListener(this);
	setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
	setTextBoxStyle(Slider::TextBoxBelow, false, 100, 18);
	setLookAndFeel(&laf);

	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->updateText();
	}

	checkEnabledState();

	setColour(Slider::ColourIds::textBoxTextColourId, Colours::white);

	setScrollWheelEnabled(false);
}
    
ParameterSlider::~ParameterSlider()
{
    removeListener(this);
}

void ParameterSlider::updateOnConnectionChange(ValueTree p, bool wasAdded)
{
	if (!matchesConnection(p))
		return;

	checkEnabledState();
}

void ParameterSlider::checkEnabledState()
{
	modulationActive = getConnectionSourceTree().isValid();

	setEnabled(!modulationActive);

	if (modulationActive)
		start();
	else
		stop();

	if (auto g = findParentComponentOfClass<DspNetworkGraph>())
		g->repaint();
}

void ParameterSlider::updateRange(Identifier, var)
{
	auto range = RangeHelpers::getDoubleRange(pTree);

	if (range.start > range.end)
		std::swap(range.start, range.end);

	setRange(range.getRange(), range.interval);
	setSkewFactor(range.skew);

	repaint();
}

bool ParameterSlider::isInterestedInDragSource(const SourceDetails& details)
{
	if (details.sourceComponent == this)
		return false;

	if(dynamic_cast<cable::dynamic::editor*>(details.sourceComponent.get()) != nullptr)
		return false;

	return !isReadOnlyModulated;
}

void ParameterSlider::paint(Graphics& g)
{
	Slider::paint(g);

	if (macroHoverIndex != -1)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.drawRect(getLocalBounds(), 1);
	}
}


void ParameterSlider::itemDragEnter(const SourceDetails& )
{
	macroHoverIndex = 1;
	repaint();
}


void ParameterSlider::itemDragExit(const SourceDetails& )
{
	macroHoverIndex = -1;
	repaint();
}



void ParameterSlider::itemDropped(const SourceDetails& dragSourceDetails)
{
	auto sourceNode = dragSourceDetails.sourceComponent->findParentComponentOfClass<NodeComponent>();
	auto thisNode = this->findParentComponentOfClass<NodeComponent>();

	

	macroHoverIndex = -1;

	if (sourceNode == thisNode)
	{
		PresetHandler::showMessageWindow("Can't assign to itself", "You cannot modulate the node with itself", PresetHandler::IconType::Error);

		repaint();
		return;
	}

	if (auto wc = findParentComponentOfClass<WrapperSlot>())
	{
		auto copy = SourceDetails(dragSourceDetails);
		copy.description.getDynamicObject()->setProperty("NumVoices", 8);
		
		currentConnection = parameterToControl->addConnectionFrom(copy.description);
	}

	currentConnection = parameterToControl->addConnectionFrom(dragSourceDetails.description);
	repaint();	
}

Array<NodeContainer::MacroParameter*> ParameterSlider::getConnectedMacroParameters()
{
	Array<NodeContainer::MacroParameter*> list;

	if (parameterToControl == nullptr)
		return list;

	auto unTypedList = parameterToControl->getConnectedMacroParameters();

	for (auto t : unTypedList)
		list.add(dynamic_cast<NodeContainer::MacroParameter*>(t));

	return list;
}

juce::ValueTree ParameterSlider::getConnectionSourceTree()
{
	return parameterToControl->getConnectionSourceTree(true);
}

bool ParameterSlider::matchesConnection(ValueTree& c) const
{
	if (parameterToControl == nullptr)
		return false;

	return parameterToControl->matchesConnection(c);

}

void ParameterSlider::mouseDown(const MouseEvent& e)
{
#if 0
	if (e.mods.isCommandDown())
	{
		addAndMakeVisible(currentRangeComponent = new RangeComponent(false));
		resized();
	}
#endif

	if (e.mods.isShiftDown())
	{
		Slider::showTextBox();
		return;
	}

	if (e.mods.isRightButtonDown())
	{
		auto pe = new MacroPropertyEditor(node, pTree);

		pe->setName("Edit Parameter");

		auto g = findParentComponentOfClass<ZoomableViewport>();
		auto b = g->getLocalArea(this, getLocalBounds());

		g->setCurrentModalWindow(pe, b);
	}
	else
	{
		auto dp = findParentComponentOfClass<DspNetworkGraph>();

		if (dp->probeSelectionEnabled && isEnabled())
		{
			parameterToControl->isProbed = !parameterToControl->isProbed;
			dp->repaint();
			return;
		}

		Slider::mouseDown(e);
	}
}




void ParameterSlider::mouseEnter(const MouseEvent& e)
{
	if (!isEnabled())
	{
		findParentComponentOfClass<DspNetworkGraph>()->repaint();
	}

	if (e.mods.isAltDown())
	{
		showRangeComponent(true);
	}
	else
	{
		Slider::mouseEnter(e);
	}
}

void ParameterSlider::mouseMove(const MouseEvent& e)
{
	if (e.mods.isAltDown() && currentRangeComponent == nullptr)
	{
		showRangeComponent(true);
	}
	else
		Slider::mouseMove(e);

	rangeButton.repaint();
}

void ParameterSlider::mouseExit(const MouseEvent& e)
{
	if (!isEnabled())
	{
		findParentComponentOfClass<DspNetworkGraph>()->repaint();
	}

	Slider::mouseExit(e);
}

void ParameterSlider::mouseDoubleClick(const MouseEvent&)
{
	if (!isEnabled())
	{
		parameterToControl->addConnectionFrom({});

		
		auto v = parameterToControl->getValue();
		
		setValue(v, dontSendNotification);
	}
}

void ParameterSlider::sliderDragStarted(Slider*)
{
	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->startDrag();
	}
}

void ParameterSlider::sliderDragEnded(Slider*)
{
	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->endDrag();
	}
}

void ParameterSlider::sliderValueChanged(Slider*)
{

	if (parameterToControl != nullptr)
	{
		if(isControllingFrozenNode())
		{
			auto n = parameterToControl->parent->getRootNetwork();
			n->getCurrentParameterHandler()->setParameter(index, getValue());
		}
		
		parameterToControl->getTreeWithValue().setProperty(PropertyIds::Value, getValue(), parameterToControl->parent->getUndoManager());
	}

	if (auto nl = dynamic_cast<ParameterKnobLookAndFeel::SliderLabel*>(getTextBox()))
	{
		nl->updateText();
	}
}


juce::String ParameterSlider::getTextFromValue(double value)
{
	if (parameterToControl == nullptr)
		return "Empty";

	if (parameterToControl->valueNames.isEmpty())
		return Slider::getTextFromValue(value);

	int index = (int)value;
	return parameterToControl->valueNames[index];
}

double ParameterSlider::getValueFromText(const String& text)
{
	if (parameterToControl == nullptr)
		return 0.0;

	if (parameterToControl->valueNames.contains(text))
		return (double)parameterToControl->valueNames.indexOf(text);

	return Slider::getValueFromText(text);
}

double ParameterSlider::getValueToDisplay() const
{
	if (parameterToControl != nullptr)
	{
		if (isControllingFrozenNode())
			return getValue();

		return parameterToControl->getValue();
	}
	else
	{
		return getValue();
	}
	
}

bool ParameterSlider::isControllingFrozenNode() const
{
	if (parameterToControl != nullptr)
	{
		auto n = parameterToControl->parent->getRootNetwork();

		return n->getRootNode() == parameterToControl->parent &&
			n->isFrozen();
	}
	
	return false;
}

ParameterKnobLookAndFeel::ParameterKnobLookAndFeel()
{
	cachedImage_smalliKnob_png = ImageProvider::getImage(ImageProvider::ImageType::KnobEmpty); // ImageCache::getFromMemory(BinaryData::knob_empty_png, BinaryData::knob_empty_pngSize);
	cachedImage_knobRing_png = ImageProvider::getImage(ImageProvider::ImageType::KnobUnmodulated); // ImageCache::getFromMemory(BinaryData::ring_unmodulated_png, BinaryData::ring_unmodulated_pngSize);

#if USE_BACKEND
	withoutArrow = ImageCache::getFromMemory(BinaryData::knob_without_arrow_png, BinaryData::knob_without_arrow_pngSize);
#endif
}


juce::Font ParameterKnobLookAndFeel::getLabelFont(Label&)
{
	return GLOBAL_BOLD_FONT();
}

    

juce::Label* ParameterKnobLookAndFeel::createSliderTextBox(Slider& slider)
{
	auto l = new SliderLabel(slider);
	l->refreshWithEachKey = false;

	l->setJustificationType(Justification::centred);
	l->setKeyboardType(TextInputTarget::decimalKeyboard);

	auto tf = slider.findColour(Slider::ColourIds::textBoxTextColourId);

	l->setColour(Label::textColourId, tf);
	l->setColour(Label::backgroundColourId, Colours::transparentBlack);
	l->setColour(Label::outlineColourId, Colours::transparentBlack);
	l->setColour(TextEditor::textColourId, tf);
	l->setColour(TextEditor::backgroundColourId, Colours::transparentBlack);
	l->setColour(TextEditor::outlineColourId, Colours::transparentBlack);
	l->setColour(TextEditor::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
	l->setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
	l->setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));

	return l;
}

void ParameterKnobLookAndFeel::drawRotarySlider(Graphics& g, int , int , int width, int height, float , float , float , Slider& s)
{
	

	const double value = s.getValue();
	const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	const double proportion = pow(normalizedValue, s.getSkewFactor());

	auto ps = dynamic_cast<ParameterSlider*>(&s);

	auto modValue = ps->getValueToDisplay();

	
	const double normalisedModValue = (modValue - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
	float modProportion = jlimit<float>(0.0f, 1.0f, pow((float)normalisedModValue, (float)s.getSkewFactor()));

	modProportion = FloatSanitizers::sanitizeFloatNumber(modProportion);

	bool isBipolar = -1.0 * s.getMinimum() == s.getMaximum();

	auto b = s.getLocalBounds();
	b = b.removeFromTop(48);
	b = b.withSizeKeepingCentre(48, 48).translated(0.0f, 3.0f);

	drawVectorRotaryKnob(g, b.toFloat(), modProportion, isBipolar, s.isMouseOverOrDragging(true), s.isMouseButtonDown(), s.isEnabled(), modProportion);


#if 0
	height = s.getHeight();
	width = s.getWidth();

	int xOffset = (s.getWidth() - 48) / 2;

	const int filmstripHeight = cachedImage_smalliKnob_png.getHeight() / 128;

	
	if (!s.isEnabled())
	{
		g.drawImageWithin(withoutArrow, xOffset, 3, 48, 48, RectanglePlacement::fillDestination);
	}
	else
	{
		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());
		const int stripIndex = (int)(proportion * 127);


		const int offset = stripIndex * filmstripHeight;

		Image clip = cachedImage_smalliKnob_png.getClippedImage(Rectangle<int>(0, offset, filmstripHeight, filmstripHeight));

		g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
		g.drawImage(clip, xOffset, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
	}

	{
		auto ps = dynamic_cast<ParameterSlider*>(&s);
		auto value = ps->parameterToControl->getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = jlimit(0.0, 1.0, pow(normalizedValue, s.getSkewFactor()));
		const int stripIndex = (int)(proportion * 127);

		Image *imageToUse = &cachedImage_knobRing_png;

		Image clipRing = imageToUse->getClippedImage(Rectangle<int>(0, (int)(stripIndex * filmstripHeight), filmstripHeight, filmstripHeight));

		//g.setColour(Colours::black.withAlpha(s.isEnabled() ? 1.0f : 0.5f));
		g.drawImage(clipRing, xOffset, 3, 48, 48, 0, 0, filmstripHeight, filmstripHeight);
	}
#endif
}

MacroParameterSlider::MacroParameterSlider(NodeBase* node, int index) :
	slider(node, index)
{
	addAndMakeVisible(slider);
	setWantsKeyboardFocus(true);

	if (auto mp = dynamic_cast<NodeContainer::MacroParameter*>(slider.parameterToControl.get()))
	{
		setEditEnabled(mp->editEnabled);
	}
}

void MacroParameterSlider::resized()
{
	auto b = getLocalBounds();
	b.removeFromBottom(10);
	slider.setBounds(b);
}

void MacroParameterSlider::mouseDrag(const MouseEvent& )
{
	if (editEnabled)
	{
		if (auto container = DragAndDropContainer::findParentDragContainerFor(this))
		{
			auto details = DragHelpers::createDescription(slider.node->getId(), slider.parameterToControl->getId());

			container->startDragging(details, &slider, ModulationSourceBaseComponent::createDragImageStatic(false));

			findParentComponentOfClass<DspNetworkGraph>()->repaint();
		}
	}
}

void MacroParameterSlider::mouseUp(const MouseEvent& e)
{
	findParentComponentOfClass<DspNetworkGraph>()->repaint();
}

void MacroParameterSlider::paintOverChildren(Graphics& g)
{
	if (editEnabled)
	{
		Path p;
		p.loadPathFromData(ColumnIcons::targetIcon, sizeof(ColumnIcons::targetIcon));

		auto pa = getLocalBounds().toFloat().withSizeKeepingCentre(20.0f, 20.0f).translated(0.0, -8);

		PathFactory::scalePath(p, pa);

		g.setColour(Colours::white.withAlpha(0.3f));
		g.fillPath(p);

		auto b = getLocalBounds().reduced(2).toFloat();
		b.removeFromBottom(8.0f);

		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.05f));
		g.fillRoundedRectangle(b, 3);

		if (hasKeyboardFocus(true))
		{
			g.setColour(Colour(SIGNAL_COLOUR));
			g.drawRoundedRectangle(b, 3, 1.0f);
		}
	}
}

void MacroParameterSlider::setEditEnabled(bool shouldBeEnabled)
{
	slider.setEnabled(!shouldBeEnabled);
	editEnabled = shouldBeEnabled;

	if (auto mp = dynamic_cast<NodeContainer::MacroParameter*>(slider.parameterToControl.get()))
	{
		mp->editEnabled = shouldBeEnabled;
	}

	if (editEnabled)
		slider.addMouseListener(this, true);
	else
		slider.removeMouseListener(this);

	if (shouldBeEnabled)
	{
		slider.setMouseCursor(ModulationSourceBaseComponent::createMouseCursor());
	}
	else
		slider.setMouseCursor({});

	repaint();
}

bool MacroParameterSlider::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
	{
		auto treeToRemove = slider.parameterToControl->data;
		auto um = slider.node->getUndoManager();

		auto f = [treeToRemove, um]()
		{
			treeToRemove.getParent().removeChild(treeToRemove, um);
		};

		MessageManager::callAsync(f);

		return true;
	}

	return false;
}

void ParameterSlider::resized()
{
	rangeButton.setBounds(getLocalBounds().removeFromTop(18.0f).removeFromLeft(18.0f).reduced(2.0f));

	Slider::resized();
}

void ParameterSlider::showRangeComponent(bool temporary)
{
	if (temporary)
	{
		auto dng = findParentComponentOfClass<DspNetworkGraph>();
		Array<RangeComponent*> list;
		DspNetworkGraph::fillChildComponentList<RangeComponent>(list, dng);
		
		for (auto c : list)
		{
			if(c->temporary)
				c->close(100);
		}
	}

	getParentComponent()->addChildComponent(currentRangeComponent = new RangeComponent(temporary, *this));
	currentRangeComponent->setVisible(true);

	currentRangeComponent->setBounds(getBoundsInParent());
}

}

