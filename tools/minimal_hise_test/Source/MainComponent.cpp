/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#if 0
class CustomDataSource : public perfetto::DataSource<CustomDataSource> {
 public:
  void OnSetup(const SetupArgs&) override {
    // Use this callback to apply any custom configuration to your data source
    // based on the TraceConfig in SetupArgs.
  }

  void OnStart(const StartArgs&) override {
    // This notification can be used to initialize the GPU driver, enable
    // counters, etc. StartArgs will contains the DataSourceDescriptor,
    // which can be extended.
  }

  void OnStop(const StopArgs&) override {
    // Undo any initialization done in OnStart.
  }

  // Data sources can also have per-instance state.
  int my_custom_state = 0;
};

PERFETTO_DECLARE_DATA_SOURCE_STATIC_MEMBERS(CustomDataSource);
#endif

using namespace hise;
using namespace scriptnode;
using namespace snex::Types;
using namespace snex::jit;
using namespace snex;

#ifndef HISE_CHAR_PTR_WARNING_LEVEL
#define HISE_CHAR_PTR_WARNING_LEVEL 0
#endif


#if HISE_CHAR_PTR_WARNING_LEVEL == 0
#define CHAR_PTR_FORCED_HASH_ACTION
#elif HISE_CHAR_PTR_WARNING_LEVEL == 1
#define CHAR_PTR_FORCED_HASH_ACTION DBG("Warning: forced hash comparison");
#elif HISE_CHAR_PTR_WARNING_LEVEL >= 2
#define CHAR_PTR_FORCED_HASH_ACTION jassertfalse;
#endif



/** a char pointer to any type of preallocated string data. */
struct CharPtr
{
    enum class Type
    {
        Empty = 0,
        Identifier,
        RawString,
        ByteArray,  // lifetime not guaranteed
        JuceString, // lifetime not guaranteed
        Wildcard = '*',
        numTypes
    };
    
    constexpr CharPtr() noexcept:
      type(Type::Empty),
      ptr(&nothing),
      numCharacters(0)
    {};
    
    constexpr CharPtr(const CharPtr& other) = default;
    constexpr CharPtr(CharPtr&& other) = default;
    
    /** Creates a CharPtr from a juce::Identifier. This will take the address of the pooled string. */
    CharPtr(const Identifier& id) noexcept:
      type(Type::Identifier),
      ptr(id.getCharPointer().getAddress()),
      numCharacters(id.toString().length())
    {}
    
    /** Creates a CharPtr from a raw literal. */
    CharPtr(const char* rawText) noexcept:
      type(Type::RawString),
      ptr(rawText),
      numCharacters(strlen(ptr))
    {}
    
    CharPtr(Type t):
      type(Type::Wildcard),
      ptr(reinterpret_cast<const char*>(&type)),
      numCharacters(1)
    {
        jassert(t == Type::Wildcard);
    };
    
    /** Creates a CharPtr from a dynamic byte array*/
    CharPtr(const uint8* byteData, size_t numBytes) noexcept:
      type(Type::ByteArray),
      ptr(reinterpret_cast<const char*>(byteData)),
      numCharacters(numBytes)
    {}
    
    explicit CharPtr(const String& s):
      type(Type::JuceString),
      ptr(s.getCharPointer().getAddress()),
      numCharacters(s.length())
    {}
    
    explicit operator bool() const noexcept
    {
        return type != Type::Empty && numCharacters > 0 && *ptr != 0;
    }
    
    operator StringRef () const noexcept { return StringRef(ptr); }
    
    /** The equality for CharPtrs is defined with these conditions:
        - none of them is an empty pointer
        - one of them is a wildcard
        - they have the same type and point to the same address
     */
    template <typename OtherType> bool operator==(const OtherType& other) const noexcept
    {
        if(type == Type::Empty || other.getType() == Type::Empty)
            return false;
        
        if(isWildcard() || other.isWildcard())
            return true;
        
        if(OtherType::isHashed())
        {
            return sameAddressAndLength(other) ||
                   hash() == other.hash();
        }
        
        if(type != other.getType())
            return false;
        
        return sameAddressAndLength(other);
    }
    
    template <typename OtherType> bool operator!=(const OtherType& other) const noexcept
    {
        return !(*this == other);
        
    }
    
    const char* get() const noexcept { return ptr; }
    size_t length()   const noexcept { return numCharacters; }
    int hash()        const noexcept { return calculateHash(); }
    Type getType()    const noexcept { return type; }
    bool isDynamic()  const noexcept { return type == Type::ByteArray || type == Type::JuceString; }
    bool isWildcard() const noexcept { return type == Type::Wildcard; }
    static constexpr bool isHashed() { return false; }
    
private:
    
    int calculateHash() const noexcept
    {
        int hash = 0;
        enum { multiplier = sizeof (int) > 4 ? 101 : 31 };
        
        for(int i = 0; i < length(); i++)
            hash = multiplier * hash + (int)get()[i];
        return hash;
    }
    
    template <typename OtherType> bool sameAddressAndLength(const OtherType& other) const noexcept
    {
        auto thisAddress = reinterpret_cast<const void*>(get());
        auto otherAddress = reinterpret_cast<const void*>(other.get());
        
        return thisAddress == otherAddress &&
               numCharacters == other.length();
    }
    
    const char* ptr;
    size_t numCharacters;
    const Type type;
    static char nothing;
};

char CharPtr::nothing = 0;

struct HashedCharPtr
{
public:
    
    using Type = CharPtr::Type;
    
    HashedCharPtr():
      cpl(),
      hashed(0)
    {};
    
    HashedCharPtr(Type):
      cpl(Type::Wildcard),
      hashed((int)Type::Wildcard)
    {};
    
    HashedCharPtr(const CharPtr& cpl_) noexcept:
      cpl(cpl_),
      hashed(cpl.hash())
    {};
    
    HashedCharPtr(uint8* values, size_t numBytes) noexcept:
      cpl(values, numBytes),
      hashed(cpl.hash())
    {};
    
    HashedCharPtr(const char* rawText) noexcept:
      cpl(rawText),
      hashed(cpl.hash())
    {};
    
    HashedCharPtr(const String& s) noexcept:
      cpl(s),
      hashed(cpl.hash())
    {};
    
    explicit operator bool() const { return (bool)cpl; }
    operator StringRef () const noexcept { return StringRef(cpl.get()); }
    
    template <typename OtherType> bool operator==(const OtherType& other) const noexcept
    {
        if(getType() == Type::Empty || other.getType() == Type::Empty)
            return false;
        
        if(isWildcard() || other.isWildcard())
            return true;
        
        return hashed == other.hash();
    }
    
    template <typename OtherType> bool operator!=(const OtherType& other) const noexcept
    {
        return !(*this == other);
    }
    
    const char*  get()      const noexcept { return cpl.get(); }
    size_t length()         const noexcept { return cpl.length(); }
    int hash()              const noexcept { return hashed; };
    CharPtr::Type getType() const noexcept { return cpl.getType(); }
    bool isDynamic()        const noexcept { return cpl.isDynamic(); }
    bool isWildcard()       const noexcept { return cpl.isWildcard(); }
    static constexpr bool isHashed() { return true; }
    
private:
    
    CharPtr cpl;
    const int hashed = 0;
};

struct StringBuilder
{
    static constexpr int SmallBufferSize = 13;
    
    StringBuilder(size_t numToPreallocate=0)
    {
        if(numToPreallocate != 0)
            data.setSize(numToPreallocate);
    }
    
    StringBuilder& operator<<(const char* rawLiteral)
    {
        auto num = strlen(rawLiteral);
        memcpy(getWriteHeadAndAdvance(num), rawLiteral, num);
        return *this;
    }
    
    StringBuilder& operator<<(const CharPtr& p)
    {
        auto num = p.length();
        memcpy(getWriteHeadAndAdvance(num), p.get(), num);
        return *this;
    }
    
    StringBuilder& operator<<(const HashedCharPtr& p)
    {
        auto num = p.length();
        memcpy(getWriteHeadAndAdvance(num), p.get(), num);
        return *this;
    }
    
    StringBuilder& operator<<(const String& s)
    {
        auto num = s.length();
        memcpy(getWriteHeadAndAdvance(num), s.getCharPointer().getAddress(), num);
        return *this;
    }
    
    StringBuilder& operator<<(int number)
    {
        ensureAllocated(8); // the limit is 99 999 999 digits yo...
        auto ptr = get();
        getWriteHeadAndAdvance(snprintf(ptr, 8, "%d", number));
        return *this;
    }
    
    StringBuilder& operator<<(const StringBuilder& other)
    {
        auto num = other.length();
        memcpy(getWriteHeadAndAdvance(num), other.get(), num);
        return *this;
    }
    
#if 0
    StringBuilder& operator<<(const hise::dispatch::Queue::FlushArgument& f)
    {
        using namespace hise::dispatch;
        
        auto& s = *this;
        
        s << f.source->getDispatchId() << ": ";

        if(f.eventType == EventType::Add)
            s << " ADD " << (int)*f.data;
        else if (f.eventType == EventType::Remove)
            s << " REM " << (int)*f.data;
        else
        {
            s << "RAW: ";
            memcpy(pos, f.data, f.numBytes);
            pos += f.numBytes;
            
            s << "[ ";
            for(int i = 0; i < f.numBytes; i++)
            {
                s << (int)f.data[i];

                if(i != f.numBytes)
                    s << ", ";
            }
            s << " ]";
        }
        
        return *this;
    }
#endif
    
    char* get() const noexcept { return (char*)data.getObjectPtr(); }
    char* end()   const noexcept { return (char*)data.getObjectPtr() + position; }
    size_t length() const noexcept { return position; }
    
private:
    
    void ensureAllocated(size_t num)
    {
        data.ensureAllocated(position + num, true);
    }
    
    char* getWriteHead() const
    {
        return (char*)data.getObjectPtr() + position;
    }
    
    char* getWriteHeadAndAdvance(size_t numToWrite)
    {
        ensureAllocated(numToWrite);
        auto ptr = getWriteHead();
        position += numToWrite;
        *(ptr+position) = 0;
        return ptr;
    }
    
    ObjectStorage<SmallBufferSize, 0> data;
    int position = 0;
};

struct CharPtrTest: public UnitTest
{
    CharPtrTest():
      UnitTest("testing char pointer classes", "dispatch")
    {};
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same(CharPtrType1 p1, CharPtrType2 p2)
    {
        String t1 = CharPtrType1::isHashed() ? "HashedPtr p1" : "CharPtr p1";
        String t2 = CharPtrType2::isHashed() ? "HashedPtr p2" : "CharPtr p2";
        expect(p1 == p2, t1 + " == " + t2);
        expect(p2 == p1, t2 + " == " + t1);
    }
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff(CharPtrType1 p1, CharPtrType2 p2)
    {
        String t1 = CharPtrType1::isHashed() ? "HashedPtr p1" : "CharPtr p1";
        String t2 = CharPtrType2::isHashed() ? "HashedPtr p2" : "CharPtr p2";
        expect(p1 != p2, t1 + " != " + t2);
        expect(p2 != p1, t2 + " != " + t1);
    }
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same_length(CharPtrType1 p1, CharPtrType2 p2)
    { expectEquals(p1.length(), p2.length(), "same length"); }
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff_length(CharPtrType1 p1, CharPtrType2 p2)
    { expectNotEquals(p1.length(), p2.length(), "not same length"); }
    
    template <typename CharPtrType1, typename CharPtrType2>
    void same_hash(CharPtrType1 p1, CharPtrType2 p2)
    { expectEquals(p1.hash(), p2.hash(), "same hash"); }
    
    template <typename CharPtrType1, typename CharPtrType2>
    void diff_hash(CharPtrType1 p1, CharPtrType2 p2)
    { expectNotEquals(p1.hash(), p2.hash(), "not same hash"); }

    // if hashed then same else diff
    template <typename CharPtrType1, typename CharPtrType2>
    void HASH(CharPtrType1 p1, CharPtrType2 p2)
    {
        if constexpr (CharPtrType1::isHashed() || CharPtrType2::isHashed())
            same(p1, p2);
        else
            diff(p1, p2);
    }
    
    
    template <typename CharPtrType> void testCharPtr()
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
        CharPtrType r2 ("value");
        CharPtrType r3 ("property");
        
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
        
        CharPtrType u1 (b1, sizeof(b1));
        CharPtrType u0 (b0, sizeof(b0));
        CharPtrType u2 (b2, sizeof(b2));
        CharPtrType u3 (b3, sizeof(b3));
        
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
    
    void expectStringResult(const StringBuilder& b, const String& e)
    {
        expectEquals(String(b.get(), b.length()), e);
        expectEquals((int)b.length(), e.length());
    }
    
    void testStringBuilder()
    {
        beginTest("test StringBuilder << operators");
        
        String s;
        
        StringBuilder b;
        int n = 1;
        b << n;
        s << String(n);
        expectStringResult(b, s);
        
        auto r = ", ";
        b << r;
        s << r;
        expectStringResult(b, s);
        
        String j("juce::String, ");
        b << j;
        s << j;
        expectStringResult(b, s);
        
        CharPtr c("CharPtr, ");
        b << c;
        s << (StringRef)c;
        expectStringResult(b, s);
        
        HashedCharPtr h("HashedCharPtr, ");
        b << h;
        s << (StringRef)h;
        expectStringResult(b, s);
        
        StringBuilder b2;
        b2 << b;
        expectStringResult(b2, s);
        
        jassertfalse;
    }
    
    void runTest() override
    {
        testStringBuilder();
        //testCharPtr<CharPtr>();
        //testCharPtr<HashedCharPtr>();
        
    }
};

#define DECLARE_HASHED_ID(x)   static const HashedCharPtr x(CharPtr(Identifier(#x)));
#define DECLARE_HASHED_ENUM(enumclass, x) static const HashedCharPtr x(Identifier(#x));
namespace enum_strings
{




}


static CharPtrTest charPtrTest;

//==============================================================================
MainComponent::MainComponent():
  startButton("start", nullptr, f),
  cancelButton("cancel", nullptr, f),
  data(new hise::WebViewData(File())),
  webview(data)
{
    UnitTestRunner r;
    r.setAssertOnFailure(true);
    r.setPassesAreLogged(true);
    r.runTests({&charPtrTest});
    
    //r.runTestsInCategory("dispatch");
    addAndMakeVisible(webview);
    addAndMakeVisible(dragger);
    addAndMakeVisible(startButton);
    addAndMakeVisible(cancelButton);
    
    startButton.setToggleModeWithColourChange(true);
    
	startTimer(150);
	

#if PERFETTO
	startButton.onClick = [&]()
	{
		if(startButton.getToggleState())
		{
			MelatoninPerfetto::get().beginSession();
		}
		else
		{
			MelatoninPerfetto::get().endSession(true);
            dragger.setFile(MelatoninPerfetto::get().lastFile);
		}
        
        repaint();
	};
#endif
    
#if PERFETTO
    cancelButton.onClick = [&]()
    {
        if(startButton.getToggleState())
        {
            MelatoninPerfetto::get().endSession(false);
            dragger.setFile(File());
            startButton.setToggleStateAndUpdateIcon(false);
        }
        
        repaint();
    };
#endif

#if JUCE_WINDOWS
    context.attachTo(*this);
	setSize (2560, 1080);
#else
	setSize(1440, 900);
#endif
    
    Timer::callAfterDelay(100, [&]()
    {
        webview.navigateToURL(URL("https://ui.perfetto.dev"));
        webview.refreshBounds(1.0f);
        resized();
    });
    
    
}

MainComponent::~MainComponent()
{
#if JUCE_WINDOWS
	context.detach();
#endif
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF19212b));
    
    if(startButton.getToggleState())
    {
        auto b = getLocalBounds().removeFromTop(48).toFloat();
        b.removeFromLeft(cancelButton.getRight() + 20);
        b.removeFromRight(dragger.getWidth() + 20);
        
        g.setColour(Colour(HISE_WARNING_COLOUR));
        g.fillRoundedRectangle(b.reduced(10), 3.0f);
        g.setColour(Colours::black);
        g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));
        g.drawText("Profiling in process...", b, Justification::centred);
    }
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    
    auto top = area.removeFromTop(48);
	startButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(10));
    cancelButton.setBounds(top.removeFromLeft(top.getHeight()).reduced(10));
    dragger.setBounds(top.removeFromRight(180));
    webview.setBounds(area);
    webview.refreshBounds(1.0f);
    
}

void MainComponent::timerCallback()
{
	
    TRACE_DISPATCH(perfetto::DynamicString(String(counter++).toStdString()));
    
    
    
	
	

	Random r;

	for(int i = 0; i < 100000 + r.nextInt(1000); i++)
	{
		if(i % 10000 == 0)
		{
			String s2;
			s2 << "SUBEVENT" << String(i);
            TRACE_DISPATCH("asdsa");

			for(int j = 0; j < r.nextInt(10000) + 100000; j++)
			{
				hmath::sin(0.5);
			}
		}

		

		hmath::sin((float)i);
	}
}
