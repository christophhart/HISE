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


#define BEGIN_TEST(x) TRACE_DISPATCH(DYNAMIC_STRING(x)); beginTest(x);

struct LoggerTest: public UnitTest,
				   public SourceOwner,
				   public ListenerOwner
{
	struct MyTestQueuable: public Queueable
	{
		MyTestQueuable(RootObject& r);
        ~MyTestQueuable() { clearFromRoot(); }
		HashedCharPtr getDispatchId() const override { return "test"; }
	};

	struct MyDanglingQueable: public Queueable
	{
		MyDanglingQueable(RootObject& r);
        ~MyDanglingQueable() { clearFromRoot(); }
		HashedCharPtr getDispatchId() const override { return "dangling"; }
	};

	// A test object that should never be exeucted.
	struct NeverExecuted: public Queueable
	{
		NeverExecuted(RootObject& r, const HashedCharPtr& id_);;
        ~NeverExecuted() { clearFromRoot(); }
		operator bool() const { return !wasExecuted; }
		bool wasExecuted = false;
		HashedCharPtr getDispatchId() const override { return id; }
        const HashedCharPtr id;
	};

	LoggerTest();

	void testLogger();
	void testQueue();
	void testQueueResume();
	void testSourceManager();

	void runTest() override;
};

struct CharPtrTest: public UnitTest
{
    CharPtrTest();;
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same_length(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff_length(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same_hash(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff_hash(CharPtrType1 p1, CharPtrType2 p2);

    // if hashed then same else diff
    template <typename CharPtrType1, typename CharPtrType2>
    void HASH(CharPtrType1 p1, CharPtrType2 p2);
    
    template <typename CharPtrType> void testCharPtr();
    
    void expectStringResult(const StringBuilder& b, const String& e);
    void testStringBuilder();

    void testHashedPath();

    void runTest() override;
};

} // dispatch
} // hise