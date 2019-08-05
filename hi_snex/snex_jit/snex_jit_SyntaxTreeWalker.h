/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace snex {
namespace jit
{
using namespace juce;


struct SyntaxTreeWalker
{
	SyntaxTreeWalker(const Operations::Statement* statement, bool searchFromRoot = true)
	{
		if (searchFromRoot)
		{
			while (statement->parent != nullptr)
				statement = statement->parent;
		}

		add(const_cast<Operations::Statement*>(statement));
	}

	Operations::Statement* getNextStatement()
	{
		return statements[index++].get();
	}

	template <class T> T* getNextStatementOfType()
	{
		while (auto s = getNextStatement())
		{
			if (auto typed = dynamic_cast<T*>(s))
				return typed;
		}

		return nullptr;
	}

private:

	void add(Operations::Statement* s);

	Array<WeakReference<Operations::Statement>> statements;
	int index = 0;
};



}
}