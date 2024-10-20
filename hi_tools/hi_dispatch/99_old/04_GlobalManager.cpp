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

#include "JuceHeader.h"

namespace hise {
namespace dispatch {	
using namespace juce;


void GlobalManager::rebuildIndexValuesRecursive()
{
	for(int i = 0; i < getNumChildren(); i++)
	{
		auto c = dynamic_cast<ManagerBase*>(getChildByIndex(i+1));
		c->globalIndex = i+1;

		for(int j = 0; j < c->getNumChildren(); j++)
		{
			auto es = c->getChildByIndex(j+1);
			es->globalIndex = i+1;
			es->sourceIndex = j+1;
		}
	}

	assertIntegrity();
}

manager_int GlobalManager::getNumSlots(manager_int index) const
{
	if(auto e = dynamic_cast<const ManagerBase*>(getChildByIndex(index)))
	{
		return e->getNumChildren();
	}

	return 0;
}

EventSourceManager* GlobalManager::resolve(manager_int index)
{
	if(auto c = dynamic_cast<EventSourceManager*>(getChildByIndex(index)))
		return c;
			
	return nullptr;
}






} // dispatch
} // hise