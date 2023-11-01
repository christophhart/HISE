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

template <typename T> struct Listener: public Sender<T>::Listener
{
	using SourceType = typename TypedSourceManager<T>::TypedSource;
	Listener(SourceType* source_):
	  source(source_)
	{
		
	}

	virtual ~Listener()
	{
		jassert(!added);
	}

	WeakReference<SourceType> source;

	void addListener()
	{
		if(!added && source != nullptr)
			source->addListener(this);
	}

	void removeListener()
	{
		if(source != nullptr)
			source->removeListener(this);

		added = false;
	}

	bool added = false;

	virtual void changed(Processor& p, uint8* values, size_t numValues) override
	{
		
	}

	NotificationType getNotificationType() const override { return sendNotificationSync; }
	bool shouldInitValues() const override { return false; }
};

} // dispatch
} // hise