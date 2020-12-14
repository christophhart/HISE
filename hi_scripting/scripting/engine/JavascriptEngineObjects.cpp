namespace hise { using namespace juce;


//==============================================================================
struct HiseJavascriptEngine::RootObject::ObjectClass : public DynamicObject
{
	ObjectClass()
	{
		setMethod("dump", dump);
		setMethod("clone", cloneFn);
	}

	static Identifier getClassName()   { static const Identifier i("Object"); return i; }
	static var dump(Args a)          { DBG(JSON::toString(a.thisObject)); ignoreUnused(a); return var::undefined(); }
	static var cloneFn(Args a)        { return a.thisObject.clone(); }
};



//==============================================================================
struct HiseJavascriptEngine::RootObject::ArrayClass : public DynamicObject
{
	ArrayClass()
	{
		setMethod("contains", contains);
		setMethod("remove", remove);
		setMethod("join", join);
		setMethod("push", push);
        setMethod("sort", sort);
        setMethod("sortNatural", sortNatural);
		setMethod("insert", insert);
        setMethod("indexOf", indexOf);
        setMethod("isArray", isArray);
		setMethod("reverse", reverse);
        setMethod("reserve", reserve);
		setMethod("clear", clear);
	}

	static Identifier getClassName()   { static const Identifier i("Array"); return i; }

	static var contains(Args a)
	{
		if (const Array<var>* array = a.thisObject.getArray())
			return array->contains(get(a, 0));

		return false;
	}

	static var remove(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
			array->removeAllInstancesOf(get(a, 0));

		return var();
	}

	static var join(Args a)
	{
		WARN_IF_AUDIO_THREAD(true, IllegalAudioThreadOps::StringCreation);

		StringArray strings;

		if (const Array<var>* array = a.thisObject.getArray())
			for (int i = 0; i < array->size(); ++i)
				strings.add(array->getReference(i).toString());

		return strings.joinIntoString(getString(a, 0));
	}

	static var clear(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
			array->clearQuick();

		return var();
	}

	static var push(Args a)
	{
		

		if (Array<var>* array = a.thisObject.getArray())
		{
			WARN_IF_AUDIO_THREAD(a.numArguments + array->size() >= array->getNumAllocated(), ScriptGuard::ArrayResizing);

			for (int i = 0; i < a.numArguments; ++i)
				array->add(a.arguments[i]);

			return array->size();
		}

		return var();
	}
    
	static var reverse(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			Array<var> reversedArray;

			for (int i = array->size()-1; i >= 0; --i)
			{
				reversedArray.add(array->getUnchecked(i));
			}

			array->swapWith(reversedArray);
		}

		return var();
	}

    static var sort(Args a)
    {
        if (Array<var>* array = a.thisObject.getArray())
        {
            VariantComparator comparator;
            array->sort(comparator);
        }
        
        return var();
    }

    static var sortNatural(Args a)
    {
        if (Array<var>* array = a.thisObject.getArray())
        {
            std::sort (array->begin(), array->end(),
               [] (const String& a, const String& b) { return a.compareNatural (b) < 0; });
        }
        
        return var();
    }
    

    static var reserve(Args a)
    {
        if (Array<var>* array = a.thisObject.getArray())
        {
            array->ensureStorageAllocated(getInt(a, 0));
        }
        
        return var();
    }
    
    

	static var insert(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			int index = getInt(a, 0);

			for (int i = 1; i < a.numArguments; i++)
			{
				array->insert(index++, get(a, i));
			}
		}

		return var();
	}
    
    static var indexOf(Args a)
    {
        if (const Array<var>* array = a.thisObject.getArray())
        {
            const int typeStrictness = getInt(a, 2);
            
            const var target (get (a, 0));
            
            for (int i = (a.numArguments > 1 ? getInt (a, 1) : 0); i < array->size(); ++i)
            {
                if(typeStrictness)
                {
                    if (array->getReference(i).equalsWithSameType(target))
                    {
                        return i;
                    }
                }
                else
                {
                    if (array->getReference(i) == target)
                        return i;
                }
            }
        }
        
        return -1;
        
    }
    
    static var isArray(Args a)
    {
        return get(a, 0).isArray();
    }
    
};

#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable: 4100)
#endif


/** This is a dummy class that contains the array functions. */
class DoxygenArrayFunctions
{
public:

	/** Searches for the element in the array. */
	bool contains(var elementToLookFor) { return false; }

	/** Removes all instances of the given element. */
	var remove(var elementToRemove) { return var(); }

	/** Reverses the order of the elements in the array. */
	void reverse() {}

	/** Reserves the space needed for the given amount of elements. */
	void reserve(int numElements) {}

	/** Joins the array into a string with the given separator. */
	String join(var separatorString) { return String(); }

	/** Adds the given element at the end and returns the size. */
	int push(var elementToInsert) { return 0; }

	/** Sorts the array. */
	void sort() {}

	/** Sorts array of numbers, objects, or strings with "number in string" priority. Can also sort a combination of all types*/
	void sortNatural() {}

	/** Clears the array. */
	void clear() {}

	/** Inserts the given arguments at the firstIndex. */
	void insert(int firstIndex, var argumentList) {}

	/** Searches the array and returns the first index. */
	int indexOf(var elementToLookFor, int startOffset, int typeStrictness) {return -1;}

	/** Checks if the given variable is an array. */
	bool isArray(var variableToTest) { return false; }
};

#if JUCE_MSVC
#pragma warning (pop)
#endif


//==============================================================================

struct HiseJavascriptEngine::RootObject::StringClass : public DynamicObject
{
	StringClass()
	{
		setMethod("substring", substring);
		setMethod("indexOf", indexOf);
		setMethod("charAt", charAt);
		setMethod("charCodeAt", charCodeAt);
		setMethod("fromCharCode", fromCharCode);
        setMethod("replace", replace);
		setMethod("split", split);
		setMethod("lastIndexOf", lastIndexOf);
		setMethod("toLowerCase", toLowerCase);
		setMethod("toUpperCase", toUpperCase);
		setMethod("trim", trim);
		setMethod("concat", concat);
	}

	static Identifier getClassName()  { static const Identifier i("String"); return i; }

	
	static var fromCharCode(Args a)  { return String::charToString(getInt(a, 0)); }
	static var substring(Args a)     { return a.thisObject.toString().substring(getInt(a, 0), getInt(a, 1)); }
	static var indexOf(Args a)       { return a.thisObject.toString().indexOf(getString(a, 0)); }
	static var lastIndexOf(Args a)		 { return a.thisObject.toString().lastIndexOf(getString(a, 0)); }
	static var charCodeAt(Args a)    { return (int)a.thisObject.toString()[getInt(a, 0)]; }
    static var replace(Args a)       { return a.thisObject.toString().replace(getString(a, 0), getString(a, 1)); }
	static var charAt(Args a)        { int p = getInt(a, 0); return a.thisObject.toString().substring(p, p + 1); }
	
	static var toUpperCase(Args a) { return a.thisObject.toString().toUpperCase(); };
	static var toLowerCase(Args a) { return a.thisObject.toString().toLowerCase(); };

	static var trim(Args a) { return a.thisObject.toString().trim(); };

	static var concat(Args a)
	{
		String r = a.thisObject.toString();

		for (int i = 0; i < a.numArguments; i++)
		{
			r << getString(a, i);
		}

		return var(r);
	}

	static var split(Args a)
	{
		const String str(a.thisObject.toString());
		const String sep(getString(a, 0));
		StringArray strings;

		if (sep.isNotEmpty())
			strings.addTokens(str, sep.substring(0, 1), "");
		else // special-case for empty separator: split all chars separately
			for (String::CharPointerType pos = str.getCharPointer(); !pos.isEmpty(); ++pos)
				strings.add(String::charToString(*pos));

		var array;
		for (int i = 0; i < strings.size(); ++i)
			array.append(strings[i]);

		return array;
	}
};

#define Array Array<var>

#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable: 4100)
#endif

/** Doxy functions for String operations. */
class DoxygenStringFunctions
{
public:

	/** Returns the substring in the given range. */
	String substring(int startIndex, int endIndex) { return String(); }

	/** Returns the position of the first found occurrence of a specified value in a string. */
	int indexOf(var substring) { return 0; }

	/**	Returns the position of the last found occurrence of a specified value in a string. */
	int lastIndexOf(var substring) { return 0; }

	/** Returns the character at the given position as ASCII number. */
	int charCodeAt(var index) { return 0; }

	/** Returns a copy of the string and replaces all occurences of `a` with `b`. */
	String replace(var substringToLookFor, var replacement) { return String(); }
	
	/** Returns the character at the given index. */
	String charAt(int index) { return String(); }

	/** Splits the string into an array with the given separator. */
	Array split(var separatorString) { return Array(); }

	/** Converts a string to lowercase letters. */
	String toLowerCase() { return String(); }

	/** Converts a string to uppercase letters. */
	String toUpperCase() { return String(); }

	/** Returns a copy of this string with any whitespace characters removed from the start and end. */
	String trim() { return String(); }

	/** Joins two or more strings, and returns a new joined strings. */
	String concat(var stringlist) { return String(); }
};

#if JUCE_MSVC
#pragma warning (pop)
#endif


#undef Array

//==============================================================================
struct HiseJavascriptEngine::RootObject::JSONClass : public DynamicObject
{
	JSONClass()                        { setMethod("stringify", stringify); }
	static Identifier getClassName()   { static const Identifier i("JSON"); return i; }
	static var stringify(Args a)      { return JSON::toString(get(a, 0)); }
};

//==============================================================================
struct HiseJavascriptEngine::RootObject::IntegerClass : public DynamicObject
{
	IntegerClass()                     { setMethod("parseInt", parseInt); }
	static Identifier getClassName()   { static const Identifier i("Integer"); return i; }

	static var parseInt(Args a)
	{
		var v = get(a, 0);

		if (v.isDouble()) { return (int)v; }

		const String s(getString(a, 0).trim());

		return s[0] == '0' ? (s[1] == 'x' ? s.substring(2).getHexValue64() : getOctalValue(s))
			: s.getLargeIntValue();
	}
};


} // namespace hise