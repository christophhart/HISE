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


#pragma once

namespace mcl
{
using namespace juce;



//==============================================================================
class GutterComponent : public juce::Component,
							 public FoldableLineRange::Listener,
							 public Value::Listener
{
public:

	struct BreakpointListener
	{
		virtual ~BreakpointListener() {};

		virtual void breakpointsChanged(GutterComponent& g) {}

		JUCE_DECLARE_WEAK_REFERENCEABLE(BreakpointListener);
	};

	GutterComponent(TextDocument& document);
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	float getGutterWidth() const
	{
		auto numRows = document.getNumRows();
		auto digits = numRows > 0 ? (int)(log10(numRows)) : 0;

		auto w = document.getCharacterRectangle().getWidth() * (digits+4);

		w += breakpoints.isEmpty() ? 0.0f : (document.getCharacterRectangle().getHeight() * 0.6f);

		return w * scaleFactor;
	}

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged) override
	{
		repaint();
	}

	void rootWasRebuilt(FoldableLineRange::WeakPtr newRoot) override
	{
		repaint();
	}

	Rectangle<float> getRowBounds(const TextDocument::RowData& r) const;

	void mouseMove(const MouseEvent& event) override
	{
		repaint();
	}

	void mouseDown(const MouseEvent& e) override;

	bool hitTest(int x, int y) override;

	void valueChanged(Value& value) override
	{
		sendBreakpointChangeMessage();
	}

	void sendBlinkMessage(int n);

	void setBreakpointsEnabled(bool shouldBeEnabled)
	{
		breakpointsEnabled = shouldBeEnabled;
		repaint();
	}

	void sendBreakpointChangeMessage()
	{
		if (recompileOnBreakpointChange)
		{
			for (auto l : listeners)
				l->breakpointsChanged(*this);
		}

		repaint();
	}

	Path createArrow()
	{
		Path p;
		Line<float> l(0.0f, 0.0f, 1.0f, 0.0f);
		p.addArrow(l, 0.3f, 1.0f, 0.5f);
		return p;
	}

	//==========================================================================
	void paint(juce::Graphics& g) override;

	bool injectBreakPoints(String& s)
	{
		blinkHandler.clear();

		Component::SafePointer<GutterComponent> st(this);

		MessageManager::callAsync([st]()
		{
			if (st)
				st.getComponent()->repaint();
		});

		if (breakpoints.isEmpty())
			return false;

		auto lines = StringArray::fromLines(s);

		for (auto bp : breakpoints)
		{
			if (isPositiveAndBelow(bp->getLineNumber(), lines.size()))
				lines.set(*bp, bp->processLine(lines[*bp]));
		}

		s = lines.joinIntoString("\n");
		return true;
	}

	void setError(int lineNumber, const String& error)
	{
		errorLine = lineNumber;
        
        if(error.isEmpty())
            errorLine = -1;
        
		errorMessage = error;
		repaint();
	}

	void addBreakpointListener(BreakpointListener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeBreakpointListener(BreakpointListener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void setCurrentBreakline(int n)
	{
		if (n != -1)
		{
			currentBreakLine = CodeDocument::Position(document.getCodeDocument(), n, 0);
			currentBreakLine.setPositionMaintained(true);
		}
		else
		{
			currentBreakLine = {};
		}

		

		MessageManager::callAsync([this]()
		{
			this->repaint();
		});
	}

private:

	struct Breakpoint: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Breakpoint>;
		using List = ReferenceCountedArray<Breakpoint>;

		Breakpoint(Value::Listener* listener, int l, CodeDocument& doc) : 
			pos(doc, l, 0),
			condition(String("true")), 
			useCondition(var(1)), 
			enabled(var(1)),
			breakIfHit(var(1)),
			blinkIfHit(var(0))
		{
			pos.setPositionMaintained(true);
			condition.addListener(listener);
			useCondition.addListener(listener);
			enabled.addListener(listener);
			logExpression.addListener(listener);
			breakIfHit.addListener(listener);
			blinkIfHit.addListener(listener);
		};

		String processLine(const String& s)
		{
			if (enabled.getValue())
			{
				String m;

				auto e = logExpression.toString();

				if (e.isNotEmpty() || blinkIfHit.getValue())
				{
					if (getCondition() != "true")
						m << "if(" << getCondition() << "){ ";

					if (blinkIfHit.getValue())
						m << "Console.blink(); ";

					if(e.isNotEmpty())
						m << "Console.print(" << e << "); ";

					if (getCondition() != "true")
						m << "}";
				}
					
				if (breakIfHit.getValue())
					m << "Console.stop(" << getCondition() << "); ";

				m << s;

				return m;
			}
			else
				return s;
		}

		int getLineNumber() const
		{
			return pos.getLineNumber();
		}

		bool operator==(int i) const { return getLineNumber() == i; }
		operator int() const { return getLineNumber(); }

		Path createPath()
		{
			Path p;
			if (hasCustomCondition())
			{
				p.startNewSubPath({ 0.0f, 0.5f });
				p.lineTo({ 0.5f, 0.0f });
				p.lineTo({ 1.0f, 0.5f });
				p.lineTo({ 0.5f, 1.0f });
				p.closeSubPath();
			}
			else
			{
				p.addEllipse({ 0.0f, 0.0f, 1.0f, 1.0f });
			}
			return p;
		}

		bool hasCustomCondition() const
		{
			return condition.toString() != "true";
		}

		String getCondition() const { return (bool)useCondition.getValue() ? condition.toString() : String("true"); }

		Value condition;
		Value useCondition;
		Value enabled;
		Value logExpression;
		Value breakIfHit;
		Value blinkIfHit;
		
		CodeDocument::Position pos;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Breakpoint);
	};

	bool breakpointsEnabled = false;

	struct BlinkState
	{
		bool blinkCallback()
		{
			alpha *= JUCE_LIVE_CONSTANT_OFF(0.8f);

			if (alpha < 0.001f)
			{
				alpha = 0.0f;
				return false;
			}

			return true;
		}

		void draw(GutterComponent& gutter, const Array<TextDocument::RowData>& rows, Graphics& g)
		{
			if (alpha == 0.0f)
				return;

			for (auto& r : rows)
			{
				if ((r.rowNumber == lineNumber - 1))
				{
					auto b = gutter.getRowBounds(r);
					b.setWidth((float)gutter.getWidth());
					g.setColour(Colours::white.withAlpha(alpha));
					g.fillRect(b);
				}
			}
		}

		int lineNumber = -1;
		float alpha = 0.4f;
	};

	Breakpoint* getBreakpoint(int lineNumber)
	{
		for (auto bp : breakpoints)
		{
			if (*bp == lineNumber)
				return bp;
		}

		return nullptr;
	}

	struct BlinkHandler : public Timer
	{
		BlinkHandler(GutterComponent& parent_) :
			parent(parent_)
		{};

		void addBlinkState(int n)
		{
			BlinkState bs;
			bs.lineNumber = n;
			startTimer(30);
			blinkStates.add(bs);
			parent.repaint();
		}

		void clear()
		{
			blinkStates.clear();
			stopTimer();
		}

		void timerCallback() override
		{
			for (int i = 0; i < blinkStates.size(); i++)
			{
				if (!blinkStates.getReference(i).blinkCallback())
					blinkStates.remove(i--);
			}

			parent.repaint();

			if (blinkStates.isEmpty())
				stopTimer();
		}

		void draw(Graphics& g, const Array<TextDocument::RowData>& rowData)
		{
			for (auto& bs : blinkStates)
				bs.draw(parent, rowData, g);
		}

		Array<BlinkState> blinkStates;

		GutterComponent& parent;
	} blinkHandler;

	
	bool recompileOnBreakpointChange = true;
	

	CodeDocument::Position currentBreakLine;

	Array<WeakReference<BreakpointListener>> listeners;

	Breakpoint::List breakpoints;

	TextDocument::RowData hoveredData;

    int errorLine = -1;
	String errorMessage;

	float scaleFactor = 1.0f;

	juce::GlyphArrangement getLineNumberGlyphs(int row) const;
	//==========================================================================
	TextDocument& document;
	juce::AffineTransform transform;
	Memoizer<int, juce::GlyphArrangement> memoizedGlyphArrangements;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GutterComponent);
};





}
