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

		HashedCharPtr getDispatchId() const override { return "test"; }
	};

	struct MyDanglingQueable: public Queueable
	{
		MyDanglingQueable(RootObject& r):
			Queueable(r)
		{};

		HashedCharPtr getDispatchId() const override { return "dangling"; }
	};

	// A test object that should never be exeucted.
	struct NeverExecuted: public Queueable
	{
		NeverExecuted(RootObject& r, const HashedCharPtr& id_):
		  Queueable(r),
          id(id_)
		{};

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
    CharPtrTest():
      UnitTest("testing char pointer classes", "dispatch")
    {};
    
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
    
    void runTest() override
    {
        testStringBuilder();
        testCharPtr<CharPtr>();
        testCharPtr<HashedCharPtr>();
        
    }
};


// Implementations of template functions
	template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::same(CharPtrType1 p1, CharPtrType2 p2)
{
    String t1 = CharPtrType1::isHashed() ? "HashedPtr p1" : "CharPtr p1";
    String t2 = CharPtrType2::isHashed() ? "HashedPtr p2" : "CharPtr p2";
    expect(p1 == p2, t1 + " == " + t2);
    expect(p2 == p1, t2 + " == " + t1);
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::diff(CharPtrType1 p1, CharPtrType2 p2)
{
    String t1 = CharPtrType1::isHashed() ? "HashedPtr p1" : "CharPtr p1";
    String t2 = CharPtrType2::isHashed() ? "HashedPtr p2" : "CharPtr p2";
    expect(p1 != p2, t1 + " != " + t2);
    expect(p2 != p1, t2 + " != " + t1);
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::same_length(CharPtrType1 p1, CharPtrType2 p2)
{
    expectEquals(p1.length(), p2.length(), "same length");
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::diff_length(CharPtrType1 p1, CharPtrType2 p2)
{
    expectNotEquals(p1.length(), p2.length(), "not same length");
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::same_hash(CharPtrType1 p1, CharPtrType2 p2)
{
    expectEquals(p1.hash(), p2.hash(), "same hash");
}

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::diff_hash(CharPtrType1 p1, CharPtrType2 p2)
{
    expectNotEquals(p1.hash(), p2.hash(), "not same hash");
}


// if hashed then same else diff

template<typename CharPtrType1, typename CharPtrType2>
void CharPtrTest::HASH(CharPtrType1 p1, CharPtrType2 p2)
{
    if constexpr (CharPtrType1::isHashed() || CharPtrType2::isHashed())
        same(p1, p2);
    else
        diff(p1, p2);
}

template<typename CharPtrType>
void CharPtrTest::testCharPtr()
{
    String t = CharPtrType::isHashed() ? "HashedCharPtr" : "CharPtr";

    beginTest("test " + t + " juce::Identifier constructor");

    // 0 == 1, 1.length() == 3.length()
    CharPtr p1(hise::dispatch::IDs::event::bypassed);
    CharPtr p0(hise::dispatch::IDs::event::bypassed);
    CharPtr p2(hise::dispatch::IDs::event::value);
    CharPtr p3(hise::dispatch::IDs::event::property);

    expect(!p1.isDynamic());
    expect(!p1.isWildcard());

    same(p1, p0); same_hash(p1, p0); same_length(p1, p0);
    diff(p1, p2); diff_hash(p1, p2); diff_length(p1, p2);
    diff(p1, p3); diff_hash(p1, p3); same_length(p1, p3);

    beginTest("test " + t + " copy constructor");

    CharPtrType c1(p1);
    CharPtrType c0(p0);
    CharPtrType c2(p2);
    CharPtrType c3(p3);

    {
        auto& t1 = c1;
        auto& t0 = c0;
        auto& t2 = c2;
        auto& t3 = c3;

        expect(!t1.isDynamic());
        expect(!t1.isWildcard());

        same(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        same(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        same(t2, p2); same_hash(t2, p2); same_length(t2, p2);
        same(t3, p3); same_hash(t3, p3); same_length(t3, p3);
    }

    beginTest("test " + t + " raw literal constructor");

    CharPtrType r1("bypassed");
    CharPtrType r0("bypassed");
    CharPtrType r2("value");
    CharPtrType r3("property");

    {
        auto& t1 = r1;
        auto& t0 = r0;
        auto& t2 = r2;
        auto& t3 = r3;

        expect(!t1.isDynamic());
        expect(!t1.isWildcard());

        same(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        HASH(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        diff(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        diff(t1, p3); diff_hash(t1, p3); same_length(t1, p3);
        HASH(t1, c0); same_hash(t1, c0); same_length(t1, c0); // cmp to copy
        diff(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        diff(t1, c3); diff_hash(t1, c3); same_length(t1, c3);
    }

    beginTest("test " + t + " constructor from uint8* and size_t");

    uint8 b1[8]; uint8 b0[8]; uint8 b2[5]; uint8 b3[8];
    memcpy(b1, "bypassed", sizeof(b1));
    memcpy(b0, "bypassed", sizeof(b0));
    memcpy(b2, "value", sizeof(b2));
    memcpy(b3, "property", sizeof(b3));

    CharPtrType u1(b1, sizeof(b1));
    CharPtrType u0(b0, sizeof(b0));
    CharPtrType u2(b2, sizeof(b2));
    CharPtrType u3(b3, sizeof(b3));

    {
        auto& t1 = u1;
        auto& t0 = u0;
        auto& t2 = u2;
        auto& t3 = u3;

        expect(t1.isDynamic());
        expect(!t1.isWildcard());

        HASH(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        HASH(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        diff(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        diff(t1, p3); diff_hash(t1, p3); same_length(t1, p3);
        HASH(t1, c0); same_hash(t1, c0); same_length(t1, c0); // cmp to copy
        diff(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        diff(t1, c3); diff_hash(t1, c3); same_length(t1, c3);
        HASH(t1, r0); same_hash(t1, r0); same_length(t1, r0); // cmp to raw
        diff(t1, r2); diff_hash(t1, r2); diff_length(t1, r2);
        diff(t1, r3); diff_hash(t1, r3); same_length(t1, r3);
    }

    beginTest("test " + t + " with juce::String");

    juce::String j1_("bypassed");
    juce::String j0_("bypassed");
    juce::String j2_("value");
    juce::String j3_("property");

    CharPtrType j1(j1_);
    CharPtrType j0(j0_);
    CharPtrType j2(j2_);
    CharPtrType j3(j3_);

    {
        auto& t1 = j1;
        auto& t0 = j0;
        auto& t2 = j2;
        auto& t3 = j3;

        expect(t1.isDynamic());
        expect(!t1.isWildcard());

        HASH(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        diff(t1, t2); diff_hash(t1, t2); diff_length(t1, t2);
        diff(t1, t3); diff_hash(t1, t3); same_length(t1, t3);
        HASH(t1, p0); same_hash(t1, p0); same_length(t1, p0); // cmp to original
        diff(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        diff(t1, p3); diff_hash(t1, p3); same_length(t1, p3);
        HASH(t1, c0); same_hash(t1, c0); same_length(t1, c0); // cmp to copy
        diff(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        diff(t1, c3); diff_hash(t1, c3); same_length(t1, c3);
        HASH(t1, r0); same_hash(t1, r0); same_length(t1, r0); // cmp to raw
        diff(t1, r2); diff_hash(t1, r2); diff_length(t1, r2);
        diff(t1, r3); diff_hash(t1, r3); same_length(t1, r3);
        HASH(t1, u0); same_hash(t1, u0); same_length(t1, u0); // cmp to byte array
        diff(t1, u2); diff_hash(t1, u2); diff_length(t1, u2);
        diff(t1, u3); diff_hash(t1, u3); same_length(t1, u3);
    }

    beginTest("test " + t + " Wildcard");

    CharPtrType w1(CharPtrType::Type::Wildcard);
    CharPtrType w0(CharPtrType::Type::Wildcard);
    CharPtrType w2(CharPtrType::Type::Wildcard);
    CharPtrType w3(CharPtrType::Type::Wildcard);

    {
        auto& t1 = w1;
        auto& t0 = w0;
        auto& t2 = w2;
        auto& t3 = w3;

        expect(!t1.isDynamic());
        expect(t1.isWildcard());

        same(t1, t0); same_hash(t1, t0); same_length(t1, t0); // cmp to self
        same(t1, t2); same_hash(t1, t2); same_length(t1, t2);
        same(t1, t3); same_hash(t1, t3); same_length(t1, t3);
        same(t1, p0); diff_hash(t1, p0); diff_length(t1, p0); // cmp to original
        same(t1, p2); diff_hash(t1, p2); diff_length(t1, p2);
        same(t1, p3); diff_hash(t1, p3); diff_length(t1, p3);
        same(t1, c0); diff_hash(t1, c0); diff_length(t1, c0); // cmp to copy
        same(t1, c2); diff_hash(t1, c2); diff_length(t1, c2);
        same(t1, c3); diff_hash(t1, c3); diff_length(t1, c3);
        same(t1, r0); diff_hash(t1, r0); diff_length(t1, r0); // cmp to raw
        same(t1, r2); diff_hash(t1, r2); diff_length(t1, r2);
        same(t1, r3); diff_hash(t1, r3); diff_length(t1, r3);
        same(t1, u0); diff_hash(t1, u0); diff_length(t1, u0); // cmp to byte array
        same(t1, u2); diff_hash(t1, u2); diff_length(t1, u2);
        same(t1, u3); diff_hash(t1, u3); diff_length(t1, u3);
        same(t1, j0); diff_hash(t1, j0); diff_length(t1, j0); // cmp to juce::String
        same(t1, j2); diff_hash(t1, j2); diff_length(t1, j2);
        same(t1, j3); diff_hash(t1, j3); diff_length(t1, j3);
    }
}

} // dispatch
} // hise