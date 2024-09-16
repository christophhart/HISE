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
	typedef var(*ScopedNativeFunction)(Args, const Scope&);

	// This is annoying, but in order to get the scope we have to hack this stuff here...
	static ScopedNativeFunction getScopedFunction(const Identifier& id)
	{
		static const Array<Identifier> scopedFunctions =
		{
			"find",
			"findIndex",
			"some",
			"map",
			"filter",
			"forEach",
			"every",
			"sort"
		};

		static constexpr int NumFunctionPointers = 8;

		static const ScopedNativeFunction scopedFunctionPointers[NumFunctionPointers] =
		{
			ArrayClass::find,
			ArrayClass::findIndex,
			ArrayClass::some,
			ArrayClass::map,
			ArrayClass::filter,
			ArrayClass::forEach,
			ArrayClass::every,
			ArrayClass::sort
		};

		auto idx = scopedFunctions.indexOf(id);

		if (isPositiveAndBelow(idx, NumFunctionPointers))
		{
			return scopedFunctionPointers[idx];
		}
		
		return nullptr;
	}

private:

	static bool isFunctionObject(const var& f)
	{
		if (dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*>(f.getObject()))
			return true;

		if (dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(f.getObject()))
			return true;

		if (f.isMethod())
			return true;

		return false;
	}

	static int getNumArgs(const var& f)
	{
		if (auto fo = dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*>(f.getObject()))
		{
			return fo->parameters.size();
		}
		if (auto ilf = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(f.getObject()))
		{
			return ilf->parameterNames.size();
		}

		return 0;
	}
	
public:
	ArrayClass()
	{
		setMethod("contains", contains);
		setMethod("remove", remove);
        setMethod("removeElement", removeElement);
		setMethod("join", join);
		setMethod("push", push);
		setMethod("pushIfNotAlreadyThere", pushIfNotAlreadyThere);
		setMethod("pop", pop);
        setMethod("sortNatural", sortNatural);
		setMethod("insert", insert);
		setMethod("concat", concat);
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

    static var removeElement(Args a)
    {
        if (Array<var>* array = a.thisObject.getArray())
            array->removeRange((int)get(a, 0), 1);

        return var();
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

	static var pushIfNotAlreadyThere(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			WARN_IF_AUDIO_THREAD(a.numArguments + array->size() >= array->getNumAllocated(), ScriptGuard::ArrayResizing);

			for (int i = 0; i < a.numArguments; ++i)
				array->addIfNotAlreadyThere(a.arguments[i]);

			return array->size();
		}
        
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

	static var pop(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			auto v = array->getLast();
			array->removeLast();
			return v;
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

    static var sort(Args a, const Scope& parent)
    {
        if (Array<var>* array = a.thisObject.getArray())
        {
			auto sortFunction = get(a, 0);

			if(sortFunction.isObject())
			{
				auto fo = dynamic_cast<FunctionObject*>(sortFunction.getObject());
				auto ilf = dynamic_cast<InlineFunction::Object*>(sortFunction.getObject());

				struct CustomComparator
				{
					CustomComparator(const Scope& parent_, FunctionObject* fo_, InlineFunction::Object* ifo_):
					  parent(parent_),
					  fo(fo_),
					  ifo(ifo_),
					  scopeObject(new DynamicObject())
					{}

					DynamicObject::Ptr scopeObject;

					bool operator() (const var& a, const var& b)
					{
						var args_[2];

						args_[0] = a;
						args_[1] = b;

						var::NativeFunctionArgs args(thisObject, args_, 2);

						Scope thisScope(&parent, parent.root.get(), scopeObject.get());

						if (fo)
							return fo->invokeWithoutAllocation(thisScope, args, scopeObject.get()) < var(0);
						if (ifo)
							return ifo->performDynamically(thisScope, args.arguments, args.numArguments) < var(0);

						return false;
					};

					var thisObject;
					const Scope& parent;
					FunctionObject* fo = nullptr;
					InlineFunction::Object* ifo = nullptr;
				};

				CustomComparator comparator(parent, fo, ilf);
				
				try
				{
					std::stable_sort(array->begin(), array->end(), comparator);
				}
				catch(std::exception& e)
				{
					throw String(e.what());
				}
			}
			else
			{
				VariantComparator comparator;
				array->sort(comparator);
			}
        }
        
        return a.thisObject;
    }

    static var sortNatural(Args a)
    {
        if (Array<var>* array = a.thisObject.getArray())
        {
            std::sort (array->begin(), array->end(),
               [] (const String& a, const String& b) { return a.compareNatural (b) < 0; });
        }
        
        return a.thisObject;
    }

	

	static var find(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			if(elementReturnValue)
			{
				*totalReturnValue = elementValue;
				return true;
			}
				
			return false;
		});
	}

	static var findIndex(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			if(elementReturnValue)
			{
				*totalReturnValue = index;
				return true;
			}
				
			return false;
		});
	}

	static var some(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			*totalReturnValue = false;

			if(elementReturnValue)
			{
				*totalReturnValue = true;
				return true;
			}
				
			return false;
		});
	}

	static var map(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			if(!totalReturnValue->isArray())
			{
				*totalReturnValue = var(Array<var>());
			}

			totalReturnValue->getArray()->add(elementReturnValue);
	
			return false;
		});
	}

	

	static var filter(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			if(!totalReturnValue->isArray())
			{
				*totalReturnValue = var(Array<var>());
			}

			if(elementReturnValue)
				totalReturnValue->getArray()->add(elementValue);
	
			return false;
		});
	}

	static var forEach(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{

			return false;
		});
	}

	static var every(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [&](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			*totalReturnValue = true;

			if(!elementReturnValue)
			{
				*totalReturnValue = false;
				return true;
			}
			
			return false;
		});
	}

	static var reduce(Args a, const Scope& parent)
	{
		return callForEach(a, parent, [&](int index, const var& elementReturnValue, const var& elementValue, var* totalReturnValue)
		{
			*totalReturnValue = a.thisObject;
			
			return false;
		});
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
    
	static var concat(Args a)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			for (int i = 0; i < a.numArguments; i++)
			{
				var newElements = get(a, i);
				
				for (int j = 0; j < newElements.size(); j++)
				{
					array->insert(-1, newElements[j]);
				}	
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

private:

	using ReturnFunction = std::function<bool(int index, const var& functionReturnValue, const var& elementValue, var* totalReturnValue)>;

	static var callForEach(Args a, const Scope& parent, const ReturnFunction& rf)
	{
		if (Array<var>* array = a.thisObject.getArray())
		{
			auto f = get(a, 0);

			if (isFunctionObject(f))
			{
				int numArgs = getNumArgs(f);

				auto thisObject = get(a, 1);

				DynamicObject::Ptr scopeObject = new DynamicObject();

				static const Identifier thisIdent("this");
				scopeObject->setProperty(thisIdent, thisObject);

				var arg[3];
				arg[2] = a.thisObject;

				HiseJavascriptEngine::RootObject::Scope thisScope(&parent, parent.root.get(), scopeObject.get());

				int numToExecute = array->size();

				var::NativeFunctionArgs args(thisObject, arg, numArgs);

				auto fo = dynamic_cast<HiseJavascriptEngine::RootObject::FunctionObject*>(f.getObject());
				auto ilf = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(f.getObject());

				var totalReturnValue;

				for (int i = 0; i < numToExecute; i++)
				{
					auto element = array->operator[](i);

					if (element.isUndefined() || element.isVoid())
						continue;
					
					arg[0] = element;
					arg[1] = i;

					var elementReturnValue;

					if (fo)
						elementReturnValue = fo->invokeWithoutAllocation(thisScope, args, scopeObject.get());
					else if (ilf)
						elementReturnValue = ilf->performDynamically(thisScope, args.arguments, args.numArguments);

					if(rf(i, elementReturnValue, element, &totalReturnValue))
						break;
				}

				return totalReturnValue;
			}
			else
			{
				throw String("not a function");
			}
		}

		return var();
	}
};



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

    /** Removes the element at the given position. */
    var removeElement(int index) { return {}; }

	/** Creates a deep copy of the array. */
	var clone() { return {}; }

	/** Joins the array into a string with the given separator. */
	String join(var separatorString) { return String(); }

	/** Adds the given element at the end and returns the size. */
	int push(var elementToInsert) { return 0; }

	/** Adds the given element at the end and returns the size. */
	int pushIfNotAlreadyThere(var elementToInsert) { return 0; }

	/** Sorts the array. */
	void sort() {}

	/** Sorts array of numbers, objects, or strings with "number in string" priority. Can also sort a combination of all types*/
	void sortNatural() {}

	/** Clears the array. */
	void clear() {}

	/** Inserts the given arguments at the firstIndex. */
	void insert(int firstIndex, var argumentList) {}
	
	/** Concatenates (joins) two or more arrays */
	void concat(var argumentList) {}

	/** Searches the array and returns the first index. */
	int indexOf(var elementToLookFor, int startOffset, int typeStrictness) {return -1;}

	/** Checks if the given variable is an array. */
	bool isArray(var variableToTest) { return false; }

	/** Returns the value of the first element that passes the function test. */
	var find(var testFunction, var optionalThisObject) { return var(); }

	/** Returns the index of the first element that passes the function test. */
	var findIndex(var testFunction, var optionalThisObject) { return var(); }

	/** Creates a new array from calling a function for every array element. */
	var map(var testFunction, var optionalThisObject) { return var(); }

	/** Checks if any array elements pass a function test. */
	var some(var testFunction, var optionalThisObject) { return var(); }

	/** Checks if all array elements pass a function test. */
	var every(var testFunction, var optionalThisObject) { return var(); }

	/** Calls a function for each element. */
	var forEach(var testFunction, var optionalThisObject) { return var(); }

	/** Creates a new array filled with elements that pass the function test. */
	var filter(var testFunction, var optionalThisObject) { return var(); }

	/** Removes and returns the last element. */
	var pop() { return var(); }
};


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
		setMethod("splitCamelCase", splitCamelCase);
		setMethod("lastIndexOf", lastIndexOf);
		setMethod("toLowerCase", toLowerCase);
		setMethod("toUpperCase", toUpperCase);
		setMethod("capitalize", capitalize);
		setMethod("parseAsJSON", parseAsJSON);
		setMethod("trim", trim);
		setMethod("concat", concat);
		setMethod("encrypt", encrypt);
		setMethod("decrypt", decrypt);
		setMethod("contains", contains);

		setMethod("getTrailingIntValue", getTrailingIntValue);
		setMethod("getIntValue", getIntValue);
		setMethod("hash", hash);
		setMethod("fromFirstOccurrenceOf", fromFirstOccurrenceOf);
		setMethod("fromLastOccurrenceOf", fromLastOccurrenceOf);
		setMethod("upToFirstOccurrenceOf", upToFirstOccurrenceOf);
		setMethod("upToLastOccurrenceOf", upToLastOccurrenceOf);
	}

	static Identifier getClassName()  { static const Identifier i("String"); return i; }

	static var contains(Args a)		 { return a.thisObject.toString().contains(getString(a, 0)); }
	static var fromCharCode(Args a)  { return String::charToString(getInt(a, 0)); }
	static var substring(Args a)     { return a.thisObject.toString().substring(getInt(a, 0), getInt(a, 1)); }
	static var indexOf(Args a)       { return a.thisObject.toString().indexOf(getString(a, 0)); }
	static var lastIndexOf(Args a)		 { return a.thisObject.toString().lastIndexOf(getString(a, 0)); }
	static var charCodeAt(Args a)    { return (int)a.thisObject.toString()[getInt(a, 0)]; }
	static var replace(Args a)       { return a.thisObject.toString().replace(getString(a, 0), getString(a, 1)); }
	static var charAt(Args a)        { int p = getInt(a, 0); return a.thisObject.toString().substring(p, p + 1); }
	static var parseAsJSON(Args a)   { return JSON::parse(a.thisObject.toString()); }	
	static var toUpperCase(Args a) { return a.thisObject.toString().toUpperCase(); };
	static var toLowerCase(Args a) { return a.thisObject.toString().toLowerCase(); };
	static var trim(Args a) { return a.thisObject.toString().trim(); };

	static var getTrailingIntValue(Args a) { return a.thisObject.toString().getTrailingIntValue(); }
	static var getIntValue(Args a) { return a.thisObject.toString().getLargeIntValue(); }
	static var hash(Args a) { return a.thisObject.toString().hashCode64(); }
	static var fromFirstOccurrenceOf(Args a) { return a.thisObject.toString().fromFirstOccurrenceOf(getString(a, 0), false, false); }
	static var fromLastOccurrenceOf(Args a) { return a.thisObject.toString().fromLastOccurrenceOf(getString(a, 0), false, false); }
	static var upToFirstOccurrenceOf(Args a) { return a.thisObject.toString().upToFirstOccurrenceOf(getString(a, 0), false, false); }
	static var upToLastOccurrenceOf(Args a) { return a.thisObject.toString().upToLastOccurrenceOf(getString(a, 0), false, false); }

	static var concat(Args a)
	{
		String r = a.thisObject.toString();

		for (int i = 0; i < a.numArguments; i++)
			r << getString(a, i);

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
	
	static var splitCamelCase(Args a)
	{
		auto trimmed = a.thisObject.toString().removeCharacters(" \t\n\r");
		auto current = trimmed.begin();
		auto end = trimmed.end();
		
		Array<var> list;

		String currentToken;

		auto flush = [&]()
		{
			if (currentToken.isNotEmpty())
			{
				list.add(currentToken);
				currentToken = {};
			}
		};

		while (current != end)
		{
			if (CharacterFunctions::isDigit(*current))
			{
				flush();

				while (CharacterFunctions::isDigit(*current))
					currentToken << *current++;

				continue;
			}
			
			if (CharacterFunctions::isUpperCase(*current))
			{
				flush();

				while (CharacterFunctions::isUpperCase(*current))
					currentToken << *current++;
				
				continue;
			}
			

			currentToken << *current++;
		}

		flush();

		return var(list);
	}

	static var capitalize(Args a)
	{
		const String str(a.thisObject.toString());

		StringArray strings;
		strings.addTokens(str, " ", "");

		StringArray result;
		String firstLetter;
		
		for (int i = 0; i < strings.size(); i++)
		{
			firstLetter = strings[i].substring(0, 1);
			firstLetter = firstLetter.toUpperCase();
			result.add(firstLetter + strings[i].substring(1));
		}

		return var(result.joinIntoString(" ", 0, -1));
	}

	static var encrypt(Args a)
	{
		const String str(a.thisObject.toString());
		const String key(getString(a, 0));

		auto data = key.getCharPointer().getAddress();
		auto size = jlimit(0, 72, key.length());

		BlowFish bf(data, size);

		MemoryOutputStream mos;
		mos.writeString(str);
		mos.flush();
		
		auto out = mos.getMemoryBlock();

		bf.encrypt(out);

		return out.toBase64Encoding();
	}

	static var decrypt(Args a)
	{
		const String encStr(a.thisObject.toString());
		const String key(getString(a, 0));

		auto data = key.getCharPointer().getAddress();
		auto size = jlimit(0, 72, key.length());

		BlowFish bf(data, size);

		MemoryBlock in;
		
		in.fromBase64Encoding(encStr);
		bf.decrypt(in);

		return in.toString();
	}
};

#define Array Array<var>


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

	/** Checks if the string contains the given substring. */
	bool contains(String otherString) { return false; }

	/** Converts a string to uppercase letters. */
	String toUpperCase() { return String(); }
	
	/** Converts a string to start case (first letter of every word is uppercase). */
	String capitalize() { return String(); }

	/** Splits the string at uppercase characters (so MyValue becomes ["My", "Value"]. */
	Array splitCamelCase();

	/** Returns a copy of this string with any whitespace characters removed from the start and end. */
	String trim() { return String(); }

	/** Joins two or more strings, and returns a new joined strings. */
	String concat(var stringlist) { return String(); }

	/** Encrypt a string using Blowfish encryption. */
	String encrypt(var key) { return String(); }

	/** Decrypt a string from Blowfish encryption. */
	String decrypt(var key) { return String(); }

	/** Attempts to parse a integer number at the end of the string. */
	int getTrailingIntValue() { return 0; }

	/** Attempts to parse the string as integer number. */
	int getIntValue() { return 0; };

	/** Creates a unique hash from the string. */
	int64 hash() { return 0; }

	/* Returns a section of the string starting from a given substring. */
	String fromFirstOccurrenceOf(String subString) { return {}; }

	/* Returns a section of the string starting from a given substring. */
	String fromLastOccurrenceOf(String subString) { return {}; }

	/* Returns a section of the string up to a given substring. */
	String upToFirstOccurrenceOf(String subString) { return {}; }

	/* Returns a section of the string up to a given substring. */
	String upToLastOccurrenceOf(String subString) { return {}; }
};



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
	IntegerClass() 
	{
		setMethod("parseInt", parseInt); 
		setMethod("parseFloat", parseFloat);
	}
	static Identifier getClassName()   { static const Identifier i("Integer"); return i; }

	static var parseFloat(Args a)
	{
		var v = get(a, 0);

		if (v.isDouble() || v.isInt() || v.isInt64()) { return var((double)v); }

		const String s(getString(a, 0).trim());

		return var(s.getDoubleValue());
	}

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
