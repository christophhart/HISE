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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace scriptnode
{
using namespace juce;
using namespace hise;

// We'll define it here once so that it can be used in the initialise() callback
class NodeBase;

/** Parameter Preprocessors

	1. Create a enum called Parameters for each parameter
	2. Use DEFINE PARAMETERS in conjunction with DEF_PARAMETER to create boilerplate code that call the functions
	3. Write parameter methods that have the syntax void setXXX(double value)
	4. Use DEFINE_PARAMETER_DATA in the createParameter callback
*/
#define DEFINE_PARAMETERS template <int P> static void setParameterStatic(void* obj, double value)
#define DEF_PARAMETER(ParameterName, ClassName) if constexpr (P == (int)Parameters::ParameterName) static_cast<ClassName*>(obj)->set##ParameterName(value);
#define DEFINE_PARAMETERDATA(ClassName, ParameterName) scriptnode::parameter::data p(#ParameterName); registerCallback<(int)Parameters::ParameterName>(p); 


#define SN_REGISTER_CALLBACK(className) template <int P> void registerCallback(scriptnode::parameter::data& p) { p.template setParameterCallbackWithIndex<className, P>(this); }
#define SN_PARAMETER_MEMBER_FUNCTION template <int P> void setParameter(double v) { setParameterStatic<P>(this, v); }

/** Object Accessors

 Use this macro to define the type that should be returned by calls to getObject(). Normally you pass in the wrapped object (for non-wrapped classes you should use SN_GET_SELF_AS_OBJECT().
 */
#define SN_GET_SELF_OBJECT(x) constexpr auto& getObject() { return x; } \
constexpr const auto& getObject() const { return x; }

/** Use this macro to define the expression that should be used in order to get the most nested type. (usually you pass in obj.getWrappedObject(). */
#define SN_GET_WRAPPED_OBJECT(x) constexpr auto& getWrappedObject() { return x; } \
constexpr const auto& getWrappedObject() const { return x; }

/** Use this macro in order to create the getObject() / getWrappedObject() methods that return the object itself. */
#define SN_GET_SELF_AS_OBJECT(x) SN_GET_SELF_OBJECT(*this); SN_GET_WRAPPED_OBJECT(*this); using ObjectType = x; using WrappedObjectType = x; SN_REGISTER_CALLBACK(x);

#define SN_DESCRIPTION(x) static juce::String getDescription() { return x; }

/** Use this macro to pass the static ID to the base constructor of the parameter_node class that will define the IsControlNode property to avoid the wrap::mod wrapper. */
#define SN_PARAMETER_NODE_CONSTRUCTOR(ClassId, ParameterId) ClassId() : control::pimpl::parameter_node_base<ParameterId>(getStaticId()) {};

#define SN_TEMPLATED_MODE_PARAMETER_NODE_CONSTRUCTOR(ClassId, ParameterId, namespaceId) ClassId(): \
	control::pimpl::parameter_node_base<ParameterId>(getStaticId()),\
    control::pimpl::templated_mode(getStaticId(), namespaceId) {};

#define SN_SELF_AWARE_WRAPPER(x, ObjectClass) SN_GET_SELF_OBJECT(*this); SN_GET_WRAPPED_OBJECT(this->obj.getWrappedObject()); using ObjectType = x; using WrappedObjectType = typename ObjectClass::WrappedObjectType; SN_REGISTER_CALLBACK(ObjectClass);

#define SN_OPAQUE_WRAPPER(x, ObjectClass) SN_GET_SELF_OBJECT(this->obj.getObject()); SN_GET_WRAPPED_OBJECT(this->obj.getWrappedObject()); using ObjectType = typename ObjectClass::ObjectType; using WrappedObjectType= typename ObjectClass::WrappedObjectType; SN_REGISTER_CALLBACK(ObjectClass);


/** Use this to define an optional function that will be forwarded to the object. */
#define OPTIONAL_BOOL_CLASS_FUNCTION(functionName) bool functionName() const { \
		if constexpr (prototypes::check::functionName<WrappedObjectType>::value) \
			return getWrappedObject().functionName(); \
		else return false; };


/** Callback macros.

	*/


/** Use these for default forwarding to the wrapped element. */
#define SN_DEFAULT_RESET(ObjectType) void reset() { obj.reset(); }
#define SN_DEFAULT_MOD(ObjectType) bool handleModulation(double& v) { \
	if constexpr(prototypes::check::handleModulation<ObjectType>::value) \
		return obj.handleModulation(v); \
	else return false; }


#define SN_DEFAULT_INIT(ObjectType)  void initialise(NodeBase* n)  { \
	if constexpr(prototypes::check::initialise<ObjectType>::value) \
		obj.initialise(n); }

#define SN_DEFAULT_HANDLE_EVENT(ObjectType) void handleHiseEvent(HiseEvent& e) { obj.handleHiseEvent(e); }
#define SN_DEFAULT_PROCESS(ObjectType) template <typename ProcessDataType> void process(ProcessDataType& d) { obj.process(d); }
#define SN_DEFAULT_PREPARE(ObjectType) void prepare(PrepareSpecs ps) { \
    if constexpr (prototypes::check::prepare<ObjectType>::value) \
        obj.prepare(ps); }

#define SN_DEFAULT_PROCESS_FRAME(ObjectType) template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept { this->obj.processFrame(data); }




/** Stack float array macros. 

	Beware of using to much stack memory !
*/
#define ALLOCA_FLOAT_ARRAY(size) (float*)alloca(size * sizeof(float)); 
#define CLEAR_FLOAT_ARRAY(v, size) memset(v, 0, sizeof(float)*size);


/** Node definition macros. */

#define SN_POLY_NODE_ID(id) SN_NODE_ID(id); static constexpr bool isPolyphonic() { return NumVoices > 1; };

#define SN_NODE_ID(id) static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER(id); };


/** Node empty callback macros. */

#define SN_EMPTY_INITIALISE void initialise(NodeBase* ) {}
#define SN_EMPTY_PREPARE void prepare(PrepareSpecs) {}
#define SN_EMPTY_RESET void reset() {}
#define SN_EMPTY_PROCESS template <typename ProcessDataType> void process(ProcessDataType&) {}
#define SN_EMPTY_PROCESS_FRAME template <typename FrameDataType> void processFrame(FrameDataType& ) {}
#define SN_EMPTY_MOD bool handleModulation(double& ) { return false; } static constexpr bool isNormalisedModulation() { return false; }
#define SN_EMPTY_HANDLE_EVENT void handleHiseEvent(HiseEvent& ) {};
#define SN_EMPTY_SET_EXTERNAL_DATA void setExternalData(const ExternalData& , int) {};

#define SN_EMPTY_CREATE_PARAM void createParameters(ParameterDataList&){}
#define SN_EMPTY_SET_PARAMETER template <int P> static void setParameterStatic(void* , double ) {} template <int P> void setParameter(double) {}
#define SN_NO_PARAMETERS SN_EMPTY_CREATE_PARAM SN_EMPTY_SET_PARAMETER

/** Node Factory macros. */

#define DEFINE_EXTERN_MONO_TEMPLATE(monoName, classWithTemplate) using monoName = classWithTemplate;    
#define DEFINE_EXTERN_NODE_TEMPLATE(monoName, polyName, className) using monoName = className<1>; \
using polyName = className<NUM_POLYPHONIC_VOICES>;
#define DEFINE_EXTERN_MONO_TEMPIMPL(classWithTemplate)
#define DEFINE_EXTERN_NODE_TEMPIMPL(className) 

/** SNEX Metadata macros to be used in metadata subclass for wrap::node. */

#define SNEX_METADATA_PARAMETERS(number, ...) static StringArray getParameterIds() { return StringArray({__VA_ARGS__}); }
#define SNEX_METADATA_ID(x) static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER(#x); }
#define SNEX_METADATA_NUM_CHANNELS(x) static constexpr int NumChannels = x;
#define SNEX_METADATA_ENCODED_PARAMETERS(NumElements) const snex::Types::span<unsigned int, NumElements> encodedParameters =

/** Snex JIT Preprocessors */

#define SN_FORWARD_PARAMETER_TO_MEMBER(className) DEFINE_PARAMETERS { static_cast<className*>(obj)->setParameter<P>(value); }

/** This is being used to tuck away everything that that JIT compiler can't parse. */
#define DECLARE_NODE(className) SN_NODE_ID(#className); SN_FORWARD_PARAMETER_TO_MEMBER(className); SN_GET_SELF_AS_OBJECT(className); hmath Math; SN_EMPTY_INITIALISE SN_EMPTY_CREATE_PARAM


/** If your node has only a single parameter called value, you can use this macro and then just add a method `void setValue(double v)`. */
#define SN_ADD_SET_VALUE(ClassType) enum Parameters { Value }; \
									  DEFINE_PARAMETERS { DEF_PARAMETER(Value, ClassType); } \
									  SN_PARAMETER_MEMBER_FUNCTION; \
									  void createParameters(ParameterDataList& data) { DEFINE_PARAMETERDATA(ClassType, Value); p.setRange({ 0.0, 1.0 }); data.add(std::move(p)); }

#if defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_FULL_VER >= 190023918 && _MSC_VER < 2000
	// Selectively enable the empty base optimization for a given type.
	// __declspec(empty_bases) was added in VC++ 2015 Update 2 and is expected to become unnecessary in the next ABI-breaking release.
#define EMPTY_BASES __declspec(empty_bases)
#else // defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_FULL_VER >= 190023918 && _MSC_VER < 2000
#define EMPTY_BASES
#endif // defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_FULL_VER >= 190023918 && _MSC_VER < 2000




// Use this in every node to add the boiler plate for C++ compilation
#define SNEX_NODE(className) hmath Math; SN_GET_SELF_AS_OBJECT(className); SN_NODE_ID(#className); SN_FORWARD_PARAMETER_TO_MEMBER(className); SN_EMPTY_INITIALISE;


}
