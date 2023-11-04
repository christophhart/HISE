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



namespace hise {
namespace dispatch {	
using namespace juce;


LibraryTest::LibraryTest():
	UnitTest("library test", "dispatch")
{}

void LibraryTest::init(const var& obj)
{
	TRACE_DISPATCH("start library test");
	MessageManagerLock mm;

	mc = new dummy::MainController();

	try
	{
		mc->setActions(obj[dummy::ActionIds::actions]);
		mc->start();
	}
	catch(String& s)
	{
		expect(false, s);
		mc = nullptr;
	}
}

void LibraryTest::deinit()
{
	if(mc != nullptr)
	{
		TRACE_DISPATCH("shutdown library test");
		MessageManagerLock mm;
		mc = nullptr;
	}
	
}

void LibraryTest::runTest()
{
	File rootDirectory = getTestDirectory();

	auto list = rootDirectory.findChildFiles(File::findFiles, true, "*.json");

	for(auto l: list)
	{
		var obj;

		auto r = JSON::parse(l.loadFileAsString(), obj);
		expect(r.wasOk(), r.getErrorMessage());

		if(r.wasOk())
		{
			runEvents(obj);
		}
	}
}


static LibraryTest libraryTest;


} // dispatch
} // hise