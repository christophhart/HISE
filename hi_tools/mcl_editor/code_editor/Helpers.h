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



#define CURSOR_WIDTH 1.5f
#define TEXT_INDENT 6.f

static bool DEBUG_TOKENS = false;



using namespace juce;

/**
	Factoring of responsibilities in the text editor classes:
 */
class CaretComponent;         // draws the caret symbol(s)
class GutterComponent;        // draws the gutter
class GlyphArrangementArray;  // like StringArray but caches glyph positions
class HighlightComponent;     // draws the highlight region(s)
struct Selection;              // stores leading and trailing edges of an editing region
class TextDocument;           // stores text data and caret ranges, supplies metrics, accepts actions
class TextEditor;             // is a component, issues actions, computes view transform
struct Transaction;            // a text replacement, the document computes the inverse on fulfilling it
class CodeMap;
class LanguageManager;		  

//==============================================================================
template <typename ArgType, typename DataType>
class Memoizer
{
public:
	using FunctionType = std::function<DataType(ArgType)>;

	Memoizer(FunctionType f) : f(f) {}
	DataType operator() (ArgType argument) const
	{
		if (map.contains(argument))
		{
			return map.getReference(argument);
		}
		map.set(argument, f(argument));
		return this->operator() (argument);
	}
	FunctionType f;
	mutable juce::HashMap<ArgType, DataType> map;
};



struct ActionHelpers
{
	static bool isLeftClosure(juce_wchar c)
	{
		return String("\"({[<").containsChar(c);
	};

	static bool  isRightClosure(juce_wchar c)
	{
		return String("\")}]>").containsChar(c);
	};

	static bool  isMatchingClosure(juce_wchar l, juce_wchar r)
	{
		return (l == '"' && r == '"') ||
			(l == '[' && r == ']') ||
			(l == '(' && r == ')') ||
			(l == '{' && r == '}') ||
			(l == '<' && r == '>') ;
	};
};

struct Helpers
{
	enum ColourId
	{
		GutterColour,
		EditorBackgroundColour,
		TooltipColour,
		numColours
	};

	static Colour getEditorColour(ColourId c)
	{
		switch (c)
		{
		case GutterColour:           return Colour(0xff2f2f2f);
		case EditorBackgroundColour: return Colour(0xff282829);
            default:                 return Colours::transparentBlack;
		}
	}

	static String replaceTabsWithSpaces(const String& s, int numToInsert)
	{
		if (!s.containsChar('\t'))
			return s;

		String rp;

		auto start = s.getCharPointer();
		auto end = s.getCharPointer() + s.length();

		int index = 0;

		while (start != end)
		{
			if (*start == '\t')
			{
				auto numSpaces = numToInsert - index % numToInsert;
				while (--numSpaces >= 0)
					rp << ' ';
			}
			else
				rp << *start;

			start++;
		}

		return rp;
	}
};



struct CoallescatedCodeDocumentListener : public CodeDocument::Listener
{
	CoallescatedCodeDocumentListener(CodeDocument& doc_) :
		lambdaDoc(doc_)
	{
		lambdaDoc.addListener(this);
	}

	virtual ~CoallescatedCodeDocumentListener()
	{
		lambdaDoc.removeListener(this);
	}

	void codeDocumentTextDeleted(int startIndex, int endIndex) override
	{
		codeChanged(false, startIndex, endIndex);
	}

	void codeDocumentTextInserted(const juce::String& newText, int insertIndex) override
	{
		codeChanged(true, insertIndex, insertIndex + newText.length());
	}

	virtual void codeChanged(bool wasAdded, int startIndex, int endIndex) {};

protected:

	CodeDocument& lambdaDoc;
};



struct LambdaCodeDocumentListener : public CoallescatedCodeDocumentListener
{
	using Callback = std::function<void()>;

	LambdaCodeDocumentListener(CodeDocument& doc_) :
		CoallescatedCodeDocumentListener(doc_)
	{}

	virtual ~LambdaCodeDocumentListener() {};

	void codeChanged(bool, int, int) override
	{
		if (f)
			f();
	}

	void setCallback(const Callback& c)
	{
		f = c;
	}

private:

	Callback f;
};


struct TooltipWithArea : public Component,
	public Timer
{
	struct Data
	{
		bool operator==(const Data& other) const
		{
			return id == other.id;
		}

		bool operator!=(const Data& other) const
		{
			return id != other.id;
		}

		operator bool() const
		{
			return id.isValid();
		}

		Identifier id;
		Point<float> relativePosition;
		String text;
		std::function<void()> clickAction;
	};

	struct Display : public Component
	{
		Display(const Data& d) :
			data(d)
		{
			f = getBasicFont();

			auto w = roundToInt(f.getStringWidthFloat(data.text) + 20.0f);
			setSize(w, f.getHeight() + 10);
		};

		void paint(Graphics& g) override
		{
			UnblurryGraphics ug(g, *this);

			auto b = getLocalBounds().toFloat();


			
			
			g.setColour(Colours::white.withAlpha(0.85f));
			ug.fillUnblurryRect(b);
			g.setFont(f);
			g.setColour(Colours::black);
			ug.draw1PxRect(b);
			g.drawText(data.text, b, Justification::centred);
		}

		Font f;
		Data data;
	};

	TooltipWithArea(Component& root_) :
		root(root_),
		shadow(DropShadow(Colours::black.withAlpha(0.8f), 8, { 0, 3 }))
	{
		root.addMouseListener(this, true);
	}

	struct Client
	{
		virtual ~Client() {};

		virtual Data getTooltip(Point<float> positionInThisComponent) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Client);
	};

	static Font getBasicFont()
	{
		return Font(14.0f, Font::plain);
	}

	bool isBeingShown(const Identifier& id) const
	{
		if (currentDisplay != nullptr)
		{
			return currentDisplay->data.id == id;
		}

		return false;
	}

	void timerCallback() override
	{
		stopTimer();

		if (isBeingShown(currentTooltip.id))
			return;

		if (currentDisplay != nullptr && currentDisplay->isMouseOver(true))
		{
			return;
		}

		clearDisplay();

		if (currentTooltip.id.isValid())
		{
			currentDisplay = new Display(currentTooltip);
			//shadow.setOwner(currentDisplay);
			root.addAndMakeVisible(currentDisplay);
			currentDisplay->setTopLeftPosition(currentPosition);
			
			return;
		}
	}

	void mouseMove(const MouseEvent& e)
	{
		if (auto asClient = dynamic_cast<Client*>(e.eventComponent))
		{
			auto pos = e.position;

			auto tooltip = asClient->getTooltip(pos);

			if (tooltip != currentTooltip)
			{
				currentTooltip = tooltip;
				currentPosition = root.getLocalPoint(e.eventComponent, currentTooltip.relativePosition).toInt();
				startTimer(400);
				return;
			}
		}
	}

	void clearDisplay()
	{
		if (currentDisplay != nullptr)
		{
			Desktop::getInstance().getAnimator().fadeOut(currentDisplay, 300);
			root.removeChildComponent(currentDisplay);

			currentDisplay = nullptr;
		}
	}

	ScopedPointer<Display> currentDisplay;
	Component& root;
	Point<int> currentPosition;
	Data currentTooltip;
	DropShadower shadow;
	Array<WeakReference<Client>> clients;
};

}
