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
using namespace juce;


void debug::MathFunctionProvider::addTokens(mcl::TokenCollection::List& l)
{
	FunctionClass::Ptr fc = new snex::jit::MathFunctions(false);

	Array<NamespacedIdentifier> functions;
	fc->getAllFunctionNames(functions);

	for (auto& f : functions)
	{
		Array<FunctionData> matches;
		fc->addMatchingFunctions(matches, f);

		for (auto m : matches)
		{
			l.add(new MathFunction(m));
		}
	}

	for (int i = 0; i < l.size(); i++)
	{
		if (l[i]->tokenContent.isEmpty())
		{
			l.remove(i--);
			continue;
		}

		for (int j = i + 1; j < l.size(); j++)
		{
			if (*l[j] == *l[i])
			{
				auto s = l[j]->tokenContent;
				l.remove(j--);
			}
		}
	}
}

}