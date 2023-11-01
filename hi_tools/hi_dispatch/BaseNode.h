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
namespace dispatch {	
using namespace juce;


class BaseNode: public ObjectUnderGlobalManager
{
public:

	BaseNode(BaseNode* parent_, GlobalManager* gm_, Category c);;

	~BaseNode() override {};

	Event createEvent(EventAction a) const;
	Event createEmptyEvent() const;
	manager_int getIndexForCategory() const;
	void updateIndex(Category c, manager_int v);
	void setIndexes(manager_int gi, manager_int src, manager_int slot);
	bool shouldPerform(Event e) const;
	bool performInternal(Event e);

	const Category category;

	manager_int globalIndex = 0;		// < 1 - 3, index in global manager
	manager_int sourceIndex = 0;	    // < 1 - numElements (eg. Processor)
	manager_int slotIndex = 0;			// < 1 - numSlots

	BaseNode* parent = nullptr;

protected:

	friend class ManagerBase;

	/** Performs the event if it matches. */
	virtual bool performMatch(Event e) = 0;
};


// The base class that handles the trickle-down logic of the event categories.
struct ManagerBase: public BaseNode
{
	struct ScopedDelayer // Move to Sender
	{
		ScopedDelayer(BaseNode* m, EventAction actionType);
		~ScopedDelayer();

		GlobalManager* m;
		Event e;
	};

	ManagerBase(BaseNode* parent, GlobalManager* gm_, Category c);;
	virtual ~ManagerBase() {};

	virtual bool performManagerEvent(Event e);
	bool performMatch(Event e) override;

	template <typename T> bool callRecursive(const std::function<bool(T&)>& f)
	{
		if(auto typed = dynamic_cast<T*>(this))
		{
			if(f(*typed))
				return true;
		}

		for(int i = 0; i < getNumChildren(); i++)
		{
			if(auto container = dynamic_cast<ManagerBase*>(getChildByIndex(i-1)))
			{
				if(container->callRecursive(f))
					return true;
			}
		}

		return false;
	}

	void addChild(BaseNode* child);
	void removeChild(BaseNode* child);

	SimpleReadWriteLock iteratorLock;
	manager_int getNumChildren() const;
	BaseNode* getChildByIndex(manager_int index);
	const BaseNode* getChildByIndex(manager_int index) const;
	void assertIntegrity();

protected:

	virtual void childAdded(BaseNode* child) {};
	virtual void childRemoved(BaseNode* child) {};

private:

	SpinLock addLock;
	
	Array<BaseNode*> childrenToBeAdded;
	Array<BaseNode*> childrenToBeRemoved;
	Array<BaseNode*> children;
};

} // dispatch
} // hise