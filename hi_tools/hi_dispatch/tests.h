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

struct LoggerTest: public UnitTest
{
	struct MyTestQueuable: public Queueable
	{
		MyTestQueuable(RootObject& r):
			Queueable(r)
		{};

		String getDispatchId() const override { return "test"; }
	};

	struct MyDanglingQueable: public Queueable
	{
		MyDanglingQueable(RootObject& r):
			Queueable(r)
		{};
		
		String getDispatchId() const override { return "dangling"; }
	};

	// A test object that should never be exeucted.
	struct NeverExecuted: public Queueable
	{
		NeverExecuted(RootObject& r, int id_):
		  Queueable(r),
		  id(id_)
		{};

		operator bool() const { return !wasExecuted; }
		bool wasExecuted = false;

		String getDispatchId() const override { return "never" + String(id); }

		const int id;
	};

	LoggerTest();

	void testLogger();
	void testQueue();
	void testQueueResume();

	void testSourceManager();

	void runTest() override;
};



static LoggerTest loggerTest;

} // dispatch
} // hise