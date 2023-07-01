/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once



namespace snex {

using namespace juce;

namespace jit
{


struct AsmInlineData;


namespace WrapIds
{
#define DECLARE_ID(x) static const Identifier x(#x);
DECLARE_ID(GetSelfAsObject);
DECLARE_ID(IsObjectWrapper);
DECLARE_ID(ObjectIndex);
DECLARE_ID(IsNode);
DECLARE_ID(NumChannels);
DECLARE_ID(NodeId);
#undef DECLARE_ID
}



struct WrapBuilder : public TemplateClassBuilder
{
	/** The OpaqueType needs to be passed into the constructor of a wrap builder
		and determines how the wrapper can be accessed using getObject() / getWrappedObject()

		- GetSelfAsObject: if a wrapper node needs to be accessible from the outside, use
							this mode. getObject() will then return a reference to the wrap
							class (getWrappedObject() will still forward it to the wrapped class.
							Be aware that this will require that every method that could be called
							from the outside MUST be implemented (or forwarded to the obj member).
		- ForwardToObj:	   if a wrapper node just opposes a different implementation of one of the
							callbacks and doesn't need to be accessible, use this OpaqueType and it
							will return the wrapped object for both calls to getObject() and getWrappedObject()
		- GetObj:		   returns just the top-most object that is wrapped
		- FullOpaque:	   both getObject and getWrappedObject will return the wrapped node (so the inner
		                   object is inaccessible from the outside.
	*/
	enum OpaqueType
	{
		GetSelfAsObject,
		ForwardToObj,
		GetObj,
		FullOpaque,
		numOpaqueTypes
	};

	using CallbackList = Array<Types::ScriptnodeCallbacks::ID>;

	/** This struct holds all the information you need if you want to
		map an external function to a given callback. */
	struct ExternalFunctionMapData
	{
		ExternalFunctionMapData(Compiler& c, AsmInlineData* d);

		/** Returns the amount of channels that this function is using.

			It looks in the last argument, which is usually either a ProcessData or a span<float, NumChannels>
			type for each function where this is relevant.
		*/
		int getChannelFromLastArg() const;

		int getTemplateConstant(int index) const;

		Result insertFunctionPtrToArgReg(void* ptr, int index = 0);

		Result emitRemappedFunction(FunctionData& f);

		FunctionData getCallbackFromObject(Types::ScriptnodeCallbacks::ID cb);

		void* getWrappedFunctionPtr(Types::ScriptnodeCallbacks::ID cb);

		void setExternalFunctionPtrToCall(void* mainFunctionPointer);

	private:

		FunctionData getCallback(TypeInfo t, Types::ScriptnodeCallbacks::ID cb, const Array<TypeInfo>& functionArgs);

		BaseCompiler* compiler;
		void* mainFunction = nullptr;
		Compiler& c;
		WeakReference<BaseScope> scope;
		TypeInfo objectType;
		AsmCodeGenerator& acg;
		TemplateParameter::List tp;

		AssemblyRegister::Ptr target;
		AssemblyRegister::Ptr object;

		AssemblyRegister::List argumentRegisters;


		AssemblyRegister::Ptr createPointerArgument(void* ptr);
	};

	WrapBuilder(Compiler& c, const Identifier& id, int numChannels, OpaqueType opaqueType_, bool addParameterClass=false);

	/** Use this constructor for all wrappers that have an int as first argument before the object. */
	WrapBuilder(Compiler& c, const Identifier& id, const Identifier& constantArg, int numChannels, OpaqueType opaqueType_);

	static NamespacedIdentifier getWrapId(const Identifier& id);

	~WrapBuilder()
	{
		flush();
	}

	/** This function will replace the given callback with an externally defined function that wraps the original callback.

		The signature of the returning function must be:

			template <typename... As> static void func(void* objPointer, void* functionPointer, As... rest)

		and it will receive the original process function passed into as `functionPointer` so you can implement the custom logic. This way
		you don't need to write a specialisation for each wrapped object, but just use the opaque function pointer instead.

		If this function is templated, you can supply a templateMapFunction which takes a TemplateParameter::List as argument and needs
		to return the matching (compile-time-defined) template function instance:

		auto mapFunction = [](FunctionData& f, const TemplateParameter::List& l)
		{
			int firstConstant = l[0].constant;
			ComplexType::Ptr type = l[1].type;

			if(firstConstant == 16)
			{
				if(type == ProcessDataType<2>)
				{
						f.function = (void*)process_function<16, ProcessDataType<2>;
						return true;
				}

				// ...
			}

			return false;
		};

		Be aware that the template parameter list will have all arguments of the original function call appended after the template class
		parameters, so you can decide which function to use

		The requiredFunctions is a list of all functions that might be called in the external function, so it will check if there is a valid function pointer
		(and compile it to an internal function if not).
	*/
	void mapToExternalTemplateFunction(Types::ScriptnodeCallbacks::ID cb, CallbackList requiredFunctions, const std::function<Result(ExternalFunctionMapData&)>& templateMapFunction);

	void init(Compiler& c, int numChannels);

	/** Call this with function pointers to a function that you want to use instead.

		This will not be inlined, so for high-performance implementations, write a
		custom inliner.
	*/
	void injectExternalFunction(const Identifier& id, void* functionPointer)
	{
		addFunction([id, functionPointer](StructType* st)
			{
				FunctionClass::Ptr fc = st->getFunctionClass();

				Array<FunctionData> matches;

				fc->addMatchingFunctions(matches, st->id.getChildId(id));

				for (auto& m : matches)
					st->injectMemberFunctionPointer(m, functionPointer);

				auto rf = matches[0];
				rf.function = functionPointer;
				return rf;
			});
	}

	struct InnerData
	{
		InnerData(ComplexType* p, OpaqueType t) :
			st(dynamic_cast<StructType*>(p)),
			typeToLookFor(t)
		{

		}

		StructType* st = nullptr;
		int offset = 0;
		OpaqueType typeToLookFor = numOpaqueTypes;

		Result getResult()
		{
			if (st != nullptr)
				return Result::ok();

			return Result::fail("Can't deduce inner type");
		}

		bool resolve()
		{
			return Helpers::getInnerType(*this);
		}

		TypeInfo getRefType() const { return TypeInfo(st, false, true); }
	};

	struct Helpers
	{
		/** Returns the channel amount from the given type info.

			You can use this in the template map lambda to find out which channel to pass into the template function.
		*/
		static Result constructorInliner(InlineData* b);

		static Result addObjReference(SyntaxTreeInlineParser& p);

		static FunctionData constructorFunction(StructType* st)
		{
			FunctionData f;
			f.id = st->id.getChildId(FunctionClass::getSpecialSymbol(st->id, FunctionClass::Constructor));
			f.returnType = TypeInfo(Types::ID::Void);

			f.inliner = Inliner::createHighLevelInliner(f.id, constructorInliner);

			return f;
		}

		static bool checkPropertyExists(StructType* st, const Identifier& id, Result& r);

		static bool getInnerType(InnerData& d);

		/** Sets the NumChannels property from the first template parameter. */
		static void setNumChannelsFromTemplateParameter(const TemplateObject::ConstructData& cd, StructType* st);

		/** Sets the NumChannels property if the immediate child type has it defined. */
		static void setNumChannelsFromObjectType(const TemplateObject::ConstructData& cd, StructType* st);
	};

	/** A function prototype that returns a function for the given struct type. */
	using FunctionBuilder = std::function<FunctionData(StructType*)>;

	static FunctionData createSetFunction(StructType* st);

	static FunctionData createGetObjectFunction(StructType* st);

	static FunctionData createGetWrappedObjectFunction(StructType* st);

	static FunctionData createGetSelfAsObjectFunction(StructType* st);

	void setInlinerForCallback(Types::ScriptnodeCallbacks::ID cb, CallbackList requiredFunctions, Inliner::InlineType t, const Inliner::Func& inliner);

	void setEmptyCallback(Types::ScriptnodeCallbacks::ID cb);

private:

	const int WrappedObjectOffset;

	const int numChannels;

	OpaqueType opaqueType;
};
}

namespace Types {

struct WrapLibraryBuilder : public LibraryBuilderBase
{
	WrapLibraryBuilder(Compiler& c, int numChannels) :
		LibraryBuilderBase(c, numChannels)
	{};

	/** Creates the constructor for wrappers with a initialiser. */
	static FunctionData createInitConstructor(StructType* st);

	static void createDefaultInitialiser(const TemplateObject::ConstructData& cd, StructType* st);

	/** Contains replacement functions for the callbacks. */
	struct Callbacks
	{
		struct empty
		{
			static Result noop(InlineData* b);
		};

		struct wrap_event
		{
			static Result process(WrapBuilder::ExternalFunctionMapData& mapData);
		};

		struct fix_block
		{
			static Result prepare(WrapBuilder::ExternalFunctionMapData& mapData);

			static Result process(WrapBuilder::ExternalFunctionMapData& mapData);
		};

		struct frame
		{
			static Result process(InlineData* b);

			static Result prepare(WrapBuilder::ExternalFunctionMapData& mapData);
		};

		struct mod
		{
			static FunctionData checkModValue(StructType* st);
			static FunctionData getParameter(StructType* st);

			static Result process(InlineData* b);
			static Result processFrame(InlineData* b);
		};

		struct core_midi
		{
			static Result prepare(InlineData* b);
			static Result handleHiseEvent(InlineData* b);
			static Result handleModulation(InlineData* b);
		};

		struct fix
		{
			static Result process(InlineData* b);

			static Result processFrame(InlineData* b);

		private:

			static Result createFunctionCall(InlineData* b, FunctionData& f);

			static FunctionData getFunction(InlineData* b, const Identifier& id);

			static TemplateParameter::List createTemplateInstance(Operations::Statement::Ptr object, FunctionData& f)
			{
				auto st = object->getTypeInfo().getTypedComplexType<StructType>();
				auto numChannels = (int)st->getInternalProperty("NumChannels", 0);
				auto wrappedType = TemplateClassBuilder::Helpers::getSubTypeFromTemplate(st, 1);
				auto& nh = object->currentCompiler->namespaceHandler;

				auto classTemplateParameters = dynamic_cast<StructType*>(wrappedType.get())->getTemplateInstanceParameters();

				TemplateParameter::List lToUse;

				if (!f.templateParameters.isEmpty())
				{
					auto cb = ScriptnodeCallbacks::getCallbackId(f.id);

					if (f.templateParameters[0].t == TemplateParameter::TypeTemplateArgument)
					{
						ComplexType::Ptr pd;

						if (cb == ScriptnodeCallbacks::ProcessFunction)
						{
							TemplateInstance tId(NamespacedIdentifier("ProcessData"), {});
							auto r = Result::ok();
							pd = nh.createTemplateInstantiation(tId, { TemplateParameter(numChannels) }, r);
						}
						else
						{
							pd = nh.registerComplexTypeOrReturnExisting(new SpanType(TypeInfo(Types::ID::Float), numChannels));
						}

						lToUse.add(TemplateParameter(TypeInfo(pd)));
					}
					else if (f.templateParameters[0].t == TemplateParameter::IntegerTemplateArgument)
					{
						lToUse.add(TemplateParameter(numChannels));
					}

					TemplateInstance tId(f.id, classTemplateParameters);

					auto r = Result::ok();
					nh.createTemplateFunction(tId, lToUse, r);
				}

				return lToUse;
			}
		};
	};

	Identifier getFactoryId() const override { RETURN_STATIC_IDENTIFIER("wrap"); }

	Result registerTypes() override;

	void registerCoreTemplates();
};


}
}

