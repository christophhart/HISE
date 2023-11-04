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

struct LibraryTest: public UnitTest
{
    LibraryTest();;
    
    void init(const var& obj);
    void deinit();
    void runTest() override;

    static File getTestDirectory()
    {
        auto f = File::getSpecialLocation(File::currentExecutableFile);
        f = f.getParentDirectory().getParentDirectory().getParentDirectory();
        f = f.getParentDirectory().getParentDirectory().getParentDirectory();
        f = f.getParentDirectory().getParentDirectory();
        
        auto dir = f.getChildFile("hi_tools/hi_dispatch/04_unit_test/json");
        jassert(dir.isDirectory());
        return dir;
    }

    void runEvents(const var& obj)
    {
        beginTest(obj[dummy::ActionIds::description].toString());

        // must not run on the message thread!
		jassert(!MessageManager::getInstance()->isThisTheMessageThread());

		init(obj);

        if(mc != nullptr)
		{
			TRACE_DISPATCH("run test");
			Thread::getCurrentThread()->wait(500);

			while(!mc->isFinished())
			{
				Thread::getCurrentThread()->wait(500);
			}
		}
    }

    

    ScopedPointer<dummy::MainController> mc;
};

} // dispatch
} // hise