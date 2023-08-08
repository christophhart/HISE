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

#pragma once

namespace hise {
using namespace juce;

namespace raw
{

template <class T>
raw::Reference<T> hise::raw::Builder::find(const String& name)
{
	return { mc, name };
}


template <class T> 
raw::Reference<T> hise::raw::Builder::findWithIndex(const Identifier& id, int index)
{
	String s = id.toString();
	s << String(index + 1);

	return find<T>(s);
}

template <class T>
T* hise::raw::Builder::add(T* processor, Processor* parent, int chainIndex /*= -1*/)
{
	Chain* c = nullptr;

	if (chainIndex == -1)
		c = dynamic_cast<Chain*>(parent);
	else
		c = dynamic_cast<Chain*>(parent->getChildProcessor(chainIndex));

	if (c == nullptr)
	{
		jassertfalse;
		return nullptr;
	}

	return addInternal<T>(processor, c);
}


template <class T>
T* hise::raw::Builder::addInternal(Processor* p, Chain* c)
{
	if (ProcessorHelpers::is<ModulatorSynth>(p) && 
		dynamic_cast<ModulatorSynthGroup*>(c) == nullptr)
		dynamic_cast<ModulatorSynth*>(p)->addProcessorsWhenEmpty();

	c->getHandler()->add(p, nullptr);

	PresetHandler::setUniqueIdsForProcessor(p);

	return dynamic_cast<T*>(p);
}


template <class T>
bool hise::raw::Builder::remove(Processor* p)
{
	auto c = dynamic_cast<Chain*>(p->getParentProcessor(false, true));
	
	c->getHandler()->remove(p);

	return true;
}



template <class T>
T* hise::raw::Builder::create(Processor* parent, int chainIndex /*= -1*/)
{
	const Identifier id = T::getClassType();

	return dynamic_cast<T*>(create(parent, id, chainIndex));
}

}

} // namespace hise;
