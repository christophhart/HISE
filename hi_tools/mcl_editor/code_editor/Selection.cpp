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



//==============================================================================
mcl::Selection::Selection(const String& content)
{
	int rowSpan = 0;
	int n = 0, lastLineStart = 0;
	auto c = content.getCharPointer();

	while (*c != '\0')
	{
		if (*c == '\n')
		{
			++rowSpan;
			lastLineStart = n + 1;
		}
		++c; ++n;
	}

	head = { 0, 0 };
	tail = { rowSpan, content.length() - lastLineStart };
}

bool mcl::Selection::isOriented() const
{
	return !(head.x > tail.x || (head.x == tail.x && head.y > tail.y));
}

mcl::Selection mcl::Selection::oriented() const
{
	if (!isOriented())
		return swapped();

	return *this;
}

mcl::Selection mcl::Selection::swapped() const
{
	Selection s = *this;
	std::swap(s.head, s.tail);
	return s;
}

mcl::Selection mcl::Selection::horizontallyMaximized(const TextDocument& document) const
{
	Selection s = *this;

	if (isOriented())
	{
		s.head.y = 0;
		s.tail.y = document.getNumColumns(s.tail.x);
	}
	else
	{
		s.head.y = document.getNumColumns(s.head.x);
		s.tail.y = 0;
	}
	return s;
}

mcl::Selection mcl::Selection::measuring(const String& content) const
{
	Selection s(content);

	if (isOriented())
	{
		return Selection(content).startingFrom(head);
	}
	else
	{
		return Selection(content).startingFrom(tail).swapped();
	}
}

mcl::Selection mcl::Selection::startingFrom(Point<int> index) const
{
	Selection s = *this;

	/*
	 Pull the whole selection back to the origin.
	 */
	s.pullBy(Selection({}, isOriented() ? head : tail));

	/*
	 Then push it forward to the given index.
	 */
	s.pushBy(Selection({}, index));

	return s;
}

void mcl::Selection::pullBy(Selection disappearingSelection)
{
	disappearingSelection.pull(head);
	disappearingSelection.pull(tail);
}

void mcl::Selection::pushBy(Selection appearingSelection)
{
	appearingSelection.push(head);
	appearingSelection.push(tail);
}

void mcl::Selection::pull(Point<int>& index) const
{
	const auto S = oriented();

	/*
	 If the selection tail is on index's row, then shift its column back,
	 either by the difference between our head and tail column indexes if
	 our head and tail are on the same row, or otherwise by our tail's
	 column index.
	 */
	if (S.tail.x == index.x && S.head.y <= index.y)
	{
		if (S.head.x == S.tail.x)
		{
			index.y -= S.tail.y - S.head.y;
		}
		else
		{
			index.y -= S.tail.y;
		}
	}

	/*
	 If this selection starts on the same row or an earlier one,
	 then shift the row index back by our row span.
	 */
	if (S.head.x <= index.x)
	{
		index.x -= S.tail.x - S.head.x;
	}
}

void mcl::Selection::push(Point<int>& index) const
{
	const auto S = oriented();

	/*
	 If our head is on index's row, then shift its column forward, either
	 by our head to tail distance if our head and tail are on the
	 same row, or otherwise by our tail's column index.
	 */
	if (S.head.x == index.x && S.head.y <= index.y)
	{
		if (S.head.x == S.tail.x)
		{
			index.y += S.tail.y - S.head.y;
		}
		else
		{
			index.y += S.tail.y;
		}
	}

	/*
	 If this selection starts on the same row or an earlier one,
	 then shift the row index forward by our row span.
	 */
	if (S.head.x <= index.x)
	{
		index.x += S.tail.x - S.head.x;
	}
}



//==============================================================================
class mcl::Transaction::Undoable : public UndoableAction
{
public:
	Undoable(TextDocument& document, Callback callback, Transaction forward)
		: document(document)
		, callback(callback)
		, forward(forward) {}

	bool perform() override
	{
		callback(reverse = document.fulfill(forward));
		return true;
	}

	bool undo() override
	{
		callback(forward = document.fulfill(reverse));
		return true;
	}

	TextDocument& document;
	Callback callback;
	Transaction forward;
	Transaction reverse;
};




//==============================================================================
mcl::Transaction mcl::Transaction::accountingForSpecialCharacters(const TextDocument& document) const
{
	Transaction t = *this;
	auto& s = t.selection;


	if ((int)content.getLastCharacter() == KeyPress::backspaceKey)
	{
		if (s.head.y == s.tail.y)
		{
			document.navigateLeftRight(s.head, false);
		}
		t.content.clear();
	}
	else if ((int)content.getLastCharacter() == KeyPress::deleteKey)
	{
		if (s.head.y == s.tail.y)
		{
			document.navigateLeftRight(s.head, true);
		}
		t.content.clear();
	}
	return t;
}

UndoableAction* mcl::Transaction::on(TextDocument& document, Callback callback)
{
	return new Undoable(document, callback, *this);
}

Selection::Selection(const juce::CodeDocument& doc, int headChar, int tailChar)
{
	auto h = juce::CodeDocument::Position(doc, headChar);
	auto t = juce::CodeDocument::Position(doc, tailChar);

	head.x = h.getLineNumber();
	head.y = h.getIndexInLine();

	tail.x = t.getLineNumber();
	tail.y = t.getIndexInLine();
}

}