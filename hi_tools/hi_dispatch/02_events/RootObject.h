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

struct Queueable;

struct RootObject
{
	struct Child
	{
		Child(RootObject& root_);;
		virtual ~Child();

		/** return the ID of this object. */
		virtual HashedCharPtr getDispatchId() const = 0;

		RootObject& getRootObject() { return root; }
		const RootObject& getRootObject() const { return root; }
		
	private:

		RootObject& root;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Child);
	};

	explicit RootObject(PooledUIUpdater* updater_);
	~RootObject();

	void addChild(Child* c);
	void removeChild(Child* c);

	/** Removes the element from all queues. */
	int clearFromAllQueues(Queueable* q, DanglingBehaviour danglingBehaviour);

	/** Call this periodically to ensure that the queues are minimized. */
	void minimiseQueueOverhead();

	PooledUIUpdater* getUpdater();
	void setLogger(Logger* l);
	Logger* getLogger() const { return currentLogger; }

	int getNumChildren() const { return childObjects.size(); }

	void setState(State newState);
	State getState() const { return currentState; }

	/** Iterates all registered queues and calls the given function. */
	bool callForAllQueues(const std::function<bool(Queue&)>& qf);

private:

	State currentState = State::Running;

	Logger* currentLogger = nullptr;

	PooledUIUpdater* updater;
	Array<Child*> childObjects;
};

} // dispatch
} // hise