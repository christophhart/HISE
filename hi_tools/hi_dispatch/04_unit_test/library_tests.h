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
namespace dummy {
using namespace juce;

struct LibraryTest: public UnitTest,
					public SourceOwner,
					public ListenerOwner
{
    LibraryTest();;
    
    void init(const var& obj);
    void deinit(int numChecksExpected);
    void runTest() override;

	void testSingleValueSources();

    void testHelloWorld(int numAttributeCalls, int numExpected, DispatchType en, DispatchType ln);
	void testHighAttributeCount();

	void testMultipleListeners();

    void testSlotBitMap();
    void testMultipleAttributes(const Array<uint16>& multiParameters);
    void testSuspension(const HashedCharPtr& pathToTest);

	void testOtherChangeMessage();

	void testSigSlotBasics();

    void runTestFiles();

    struct HardcodedTest: public JSONBuilder
	{
		using MidFunction = std::function<void(HardcodedTest&)>;

		static constexpr int AttributeIndex = 5;

		HardcodedTest(LibraryTest& parent_, const String& name, int milliSeconds=1000);

		DynamicObject::Ptr expectNumCallbacks(int numExpectedAttributes, int numExpectedBypassCalls=-1);
		DynamicObject::Ptr addListenerToProcessor(DispatchType n, ThreadType t=ThreadType::Undefined);
		DynamicObject::Ptr addListener(const Data& d);
		DynamicObject::Ptr addSetAttributeCall(double ts, DispatchType n, dummy::ThreadType t=dummy::ThreadType::Undefined);

		void flush();

		JSONBuilder::Data d;
		LibraryTest& parent;
	};

    void testBasicSetup();
    static File getTestDirectory();
    void runEvents(const var& obj);

    ScopedPointer<MainController> mc;
};

} // dummy
} // dispatch
} // hise