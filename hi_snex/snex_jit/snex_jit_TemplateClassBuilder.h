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
namespace jit {
using namespace juce;
USE_ASMJIT_NAMESPACE;


/** This class offers a convenient interface to build up template classes on the C++ side
	that can be added to a jit::Compiler object. 

	In order to use it, create this object, then setup the template arguments and populate
	the template class with methods and members by adding lambda functions to this object.

	Then call flush() and it will create the template and register it to the given Compiler.
*/
struct TemplateClassBuilder
{
	using StatementPtr = Operations::Statement::Ptr;
	using StatementList = Operations::Statement::List;

	/** A function prototype that returns a function for the given struct type. */
	using FunctionBuilder = std::function<FunctionData(StructType*)>;
	
	/** A function prototype that initialises the given struct. This can be used to add members to the class. */
	using InitialiseStructFunction = std::function<void(const TemplateObject::ConstructData&, StructType* st)>;

	/** Create a TemplateClassBuilder. */
	TemplateClassBuilder(Compiler& compiler, const NamespacedIdentifier& parameterName);

	/** Call this to register the template class at the compiler. */
	virtual void flush();

	
	/** Adds an integer template argument with the given id. */
	void addIntTemplateParameter(const Identifier& templateId);

	/** Adds an integer template argument with the given id and default value. */
	void addIntTemplateParameterWithDefault(const Identifier& templateId, int defaultValue);

	/** Adds a type template argument with the given id. */
	void addTypeTemplateParameter(const Identifier& templateId);

	/** Adds a variadic type template argument pack with the given id. */
	void addVariadicTypeTemplateParameter(const Identifier vTypeId);

	/** Adds a function to the template. You need to pass in a FunctionBuilder lambda that creates
		a function data from the given StructType.
	
		This function is called when the template is being instantiated, so you can get the 
		template parameters from the StructType.
	*/
	void addFunction(const FunctionBuilder& f);

	/** Sets the initialiser function data. If you want to add members or instantiate templates for
	    members, pass in a lambda that will process the given StructType while its being created.

		The supplied lambda will be called just before the struct is being finalised and the functions
		are being added.

		Do not add functions here, but rather use the addFunction() method.
	*/
	void setInitialiseStructFunction(const InitialiseStructFunction& f)
	{
		initFunction = f;
	}

	/** Sets a description for the autocomplete menu. */
	void setDescription(const String& newDescription)
	{
		description = newDescription;
	}

	/** This class contains some helper functions and default functions. */
	struct Helpers
	{
		static StatementPtr createBlock(SyntaxTreeInlineData* d);

		static StructType* getStructTypeFromInlineData(InlineData* b);

		static int getTemplateConstant(StructType* st, int index, Result& r);

		/** Just returns a identifier for the (variadic) member called eg. "_p12". */
		/** Creates a function call to the member function of the given type. */
		static StatementPtr createFunctionCall(ComplexType::Ptr converterType, SyntaxTreeInlineData* d, const Identifier& functionId, StatementList originalArgs);

		/** This adds the object pointer to the member at the given memberIndex position. */
		static void addChildObjectPtr(StatementPtr newCall, SyntaxTreeInlineData* d, StructType* parentType, int memberIndex);

		static ComplexType::Ptr getSubTypeFromTemplate(StructType* st, int index);

		/** Helper function that creates the function data for the given member function. */
		static FunctionData getFunctionFromTargetClass(ComplexType::Ptr targetType, const Identifier& id);

		/** Call this when you are messing with the channel amount.

		If your wrap / container template wants to use a fixed amount of
		channels for their processing, call this method and it will

		redirect the processFrame() and process() calls to the channel
		amount that is set as internal property NumChannels.

		For containers this is enabled by default, but you can call it
		on wrappers that require fix channel processing.
		*/
		static void redirectProcessCallbacksToFixChannel(const TemplateObject::ConstructData& cd, StructType* st);
	};

	struct VariadicHelpers
	{
		/** Creates a T::get<Index>() function that returns a reference to the member at Index.

			Use this whenever you create a variadic function template to be able to access the
			individual members.
		*/
		static FunctionData getFunction(StructType* st);


		static Identifier getVariadicMemberIdFromIndex(int index)
		{
			String p = "_p" + String(index + 1);
			return Identifier(p);
		}

		/** Creates members from the variadic template arguments.

			If your template has some fixed arguments, you can supply the Offset to make sure that
			only the variadic types are being added as members (you can still add the other ones manually
			if you want to).
		*/
		template <int Offset> static void initVariadicMembers(const TemplateObject::ConstructData& cd, StructType* st)
		{
			for (int i = Offset; i < cd.tp.size(); i++)
			{
				if (!cd.expectIsComplexType(i))
					return;

				auto t = cd.tp[i].type;
				st->addMember(getVariadicMemberIdFromIndex(i-Offset), t);
			}
		}

		static StatementPtr callEachMember(SyntaxTreeInlineData* d, StructType* st, const Identifier& functionId, int offset=0);
	};

	void addInitFunction(const InitialiseStructFunction& f)
	{
		additionalInitFunctions.add(f);
	}

	void addPostFunctionBuilderInitFunction(const InitialiseStructFunction& f)
	{
		postFunctionBuilderFunctions.add(f);
	}

protected:

	String description;

	TemplateObject createTemplateObject();
	
	InitialiseStructFunction initFunction;

	Array<InitialiseStructFunction> additionalInitFunctions;

	Array<InitialiseStructFunction> postFunctionBuilderFunctions;

	Compiler& c;
	Array<FunctionBuilder> functionBuilders;
	NamespacedIdentifier id;
	Array<TemplateParameter> tp;
};










}
}