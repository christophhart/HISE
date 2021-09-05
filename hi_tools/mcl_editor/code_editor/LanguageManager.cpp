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

mcl::FoldableLineRange::List LanguageManager::createLineRange(const juce::CodeDocument& doc)
{
    mcl::FoldableLineRange::List lineRanges;

    CodeDocument::Iterator it(doc);

    bool firstInLine = false;
    mcl::FoldableLineRange::WeakPtr currentElement;

    while (auto c = it.nextChar())
    {
        switch (c)
        {
        case '{':
        {
            auto thisLine = it.getLine();

            if (firstInLine)
                thisLine -= 1;

            Range<int> r(thisLine, thisLine);

            mcl::FoldableLineRange::Ptr newElement = new mcl::FoldableLineRange(doc, r);

            if (currentElement == nullptr)
            {
                currentElement = newElement;
                lineRanges.add(newElement);
            }
            else
            {
                newElement->parent = currentElement;
                currentElement->children.add(newElement);
                currentElement = newElement.get();
            }

            break;
        }
        case '}':
        {
            if (currentElement != nullptr)
            {
                currentElement->setEnd(it.getPosition());
                currentElement = currentElement->parent;
            }

            break;
        }
        case '#': it.skipToEndOfLine(); break;
        case '/':
        {
            if (it.peekNextChar() == '*')
            {
                auto lineNumber = it.getLine();

                it.nextChar();

                while ((c = it.nextChar()))
                {
                    if (c == '*')
                    {
                        if (it.peekNextChar() == '/')
                        {
                            auto thisLine = it.getLine();
                            if (thisLine > lineNumber)
                            {
                                mcl::FoldableLineRange::Ptr newElement = new mcl::FoldableLineRange(doc, { lineNumber, it.getLine() });

                                if (currentElement == nullptr)
                                {
                                    lineRanges.add(newElement);
                                }
                                else
                                {
                                    currentElement->children.add(newElement);
                                    newElement->parent = currentElement;
                                    //currentElement = newElement;
                                }
                            }

                            it.nextChar();

                            break;
                        }
                    }
                }
            }
            if (it.peekNextChar() == '/')
                it.skipToEndOfLine();

            break;
        }
        }

        firstInLine = (c == '\n') || (firstInLine && CharacterFunctions::isWhitespace(c));
    }

    return lineRanges;
}

FoldableLineRange::List MarkdownLanguageManager::createLineRange(const CodeDocument& doc)
{
    CodeDocument::Iterator it(doc);

    int currentHeadlineLevel = 0;
    bool addAtNextChar = false;

    bool isInsideCodeBlock = false;

    struct Level
    {
        int line;
        int level;
    };

    Array<Level> levels;

    while (auto c = it.peekNextChar())
    {
        switch (c)
        {
        case '#':
        {
            if (!isInsideCodeBlock)
            {
                if (!addAtNextChar)
                    currentHeadlineLevel = 0;

                currentHeadlineLevel++;
                addAtNextChar = true;
                it.skip();
                break;
            }
        }
        case '-':
        case '`':
        {
            if (it.nextChar() == c && it.nextChar() == c)
            {
                levels.add({ it.getLine(), 9000 });
                isInsideCodeBlock = !isInsideCodeBlock;
                break;
            }
        }
        default:
        {
            if (addAtNextChar)
            {
                levels.add({ it.getLine(), currentHeadlineLevel });
                addAtNextChar = false;
            }

            it.skipToEndOfLine();
            break;
        }
        }
    }

    mcl::FoldableLineRange::WeakPtr currentElement;
    mcl::FoldableLineRange::List lineRanges;

    auto getNextLineWithSameOrLowerLevel = [&](int i)
    {
        auto thisLevel = levels[i].level;

        for (int j = i + 1; j < levels.size(); j++)
        {
            if (levels[j].level <= thisLevel)
                return j;
        }

        return levels.size() - 1;
    };

    auto getCurrentLevel = [&]()
    {
        if (currentElement == nullptr)
            return -1;

        auto lineStart = currentElement->getLineRange().getStart();

        for (int i = 0; i < levels.size(); i++)
        {
            if (lineStart == levels[i].line)
                return levels[i].level;
        }

        jassertfalse;
        return -1;
    };

    isInsideCodeBlock = false;

    for (int i = 0; i < levels.size(); i++)
    {
        auto thisLevel = levels[i].level;



        if (thisLevel == 9000)
        {
            if (isInsideCodeBlock)
            {
                isInsideCodeBlock = false;
                continue;
            }

            isInsideCodeBlock = true;

            if (levels[i + 1].level == 9000)
            {
                Range<int> r(levels[i].line, levels[i + 1].line);
                auto codeRange = new mcl::FoldableLineRange(doc, r);

                if (currentElement != nullptr)
                {
                    currentElement->children.add(codeRange);
                    codeRange->parent = currentElement;
                }
                else
                {
                    // don't add this as current element as there will be no children
                    lineRanges.add(codeRange);
                }
            }

            continue;
        }

        if (thisLevel >= 4)
            continue;

        auto endOfRangeIndex = getNextLineWithSameOrLowerLevel(i);

        Range<int> r(levels[i].line, levels[endOfRangeIndex].line);

        if (r.isEmpty())
            r.setEnd(doc.getNumLines());

        r.setEnd(r.getEnd() - 1);

        auto newRange = new mcl::FoldableLineRange(doc, r);



        if (currentElement == nullptr)
        {
            currentElement = newRange;
            lineRanges.add(currentElement);
        }
        else
        {
            while (getCurrentLevel() >= thisLevel)
            {
                currentElement = currentElement->parent;
            }

            if (currentElement == nullptr)
            {
                currentElement = newRange;
                lineRanges.add(currentElement);
            }
            else
            {
                currentElement->children.add(newRange);
                newRange->parent = currentElement;
                currentElement = newRange;
            }
        }
    }

    return lineRanges;
}

void MarkdownLanguageManager::setupEditor(mcl::TextEditor* editor)
{
    editor->setEnableAutocomplete(false);
    editor->setEnableBreakpoint(false);
}

FoldableLineRange::List XmlLanguageManager::createLineRange(const CodeDocument& doc)
{
    CodeDocument::Iterator it(doc);
    bool isParsingTag = false;
    
    struct Tag
    {
        String name;
        bool isCloseTag = false;
        bool isSelfClose = false;
        bool isMeta = false;
        int lineNumber;
    };
    
    Tag currentTag;
    Array<Tag> tags;
    
    
    while (auto c = it.nextChar())
    {
        switch (c)
        {
        case '<':
            currentTag = {};
            currentTag.lineNumber = it.getLine();
            currentTag.isMeta = it.peekNextChar() == '?';
            isParsingTag = true;
                
            break;
        case '/':
            currentTag.isCloseTag = true;
            currentTag.isSelfClose = currentTag.name.isNotEmpty();
            break;
        case ' ':
        case '\t':
            isParsingTag = false;
            break;
        case '>':
            isParsingTag = false;
                
            if(!currentTag.isMeta)
                tags.add(currentTag);
            break;
        case '\'':
        case '"':
        {
            auto quoteChar = c;
            
            while((c = it.nextChar()))
            {
                if(c == quoteChar)
                    break;
            }
            
            break;
        }
        default:
            if(isParsingTag)
            {
                if(CharacterFunctions::isLetter(c) ||
                   CharacterFunctions::isDigit(c))
                    currentTag.name << c;
            }
            break;
        }
    }

    FoldableLineRange::List lineRanges;
    mcl::FoldableLineRange::WeakPtr currentElement;
    
    int index = -1;
    
    for(const auto& t: tags)
    {
        DBG(t.name);
        index++;
        
        if(t.isSelfClose)
        {
            Range<int> r(t.lineNumber, tags[index+1].lineNumber-1);
            
            if(!r.isEmpty() && currentElement != nullptr)
            {
                auto newElement = new FoldableLineRange(doc, r);
                currentElement->children.add(newElement);
                newElement->parent = currentElement;
            }
            
            continue;
        }
        
        auto isOpen = !t.isCloseTag;
        
        if(isOpen)
        {
            int numOpen = 1;
            
            for(int i = index+1; i < tags.size(); i++)
            {
                auto next = tags[i];
                
                if(next.isSelfClose)
                    continue;
                
                if(next.name == t.name)
                {
                    
                    
                    if(next.isCloseTag)
                        numOpen--;
                    else
                        numOpen++;
                    
                    if(numOpen == 0)
                    {
                        Range<int> r(t.lineNumber, next.lineNumber);
                        
                        auto newElement = new FoldableLineRange(doc, r);
                        
                        if(currentElement == nullptr)
                        {
                            currentElement = newElement;
                            lineRanges.add(newElement);
                        }
                        else
                        {
                            currentElement->children.add(newElement);
                            newElement->parent = currentElement;
                            currentElement = newElement;
                        }
                        
                        break;
                    }
                }
            }
        }
        else
        {
            if(currentElement != nullptr)
                currentElement = currentElement->parent;
        }
    }
    
    return lineRanges;
}

void XmlLanguageManager::setupEditor(mcl::TextEditor* editor)
{
    editor->setEnableBreakpoint(false);
}

}
