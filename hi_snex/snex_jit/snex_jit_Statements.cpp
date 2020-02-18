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


namespace snex {
namespace jit
{
using namespace juce;

void SyntaxTreeWalker::add(Operations::Statement* s)
{
	statements.add(s);

	if (auto sb = dynamic_cast<Operations::StatementBlock*>(s))
	{
		for (auto s_ : sb->statements)
			add(s_);
	}
	else if (auto st = dynamic_cast<SyntaxTree*>(s))
	{
		for (auto s_ : st->list)
			add(s_);
	}
	else if (auto bl = dynamic_cast<Operations::Loop*>(s))
	{
		// the statement block is not a "real" sub expr (because it shouldn't
		// be subject to the usual codegen)
		add(bl->getSubExpr(0).get());
		add(bl->b);
	}
	else if (auto expr = dynamic_cast<Operations::Expression*>(s))
	{
		for (int i = 0; i < expr->getNumSubExpressions(); i++)
			add(expr->getSubExpr(i).get());
	}
}



}
}