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

/** Parameter Preprocessors

	1. Create a enum called Parameters for each parameter
	2. Use DEFINE PARAMETERS in conjunction with DEF_PARAMETER to create boilerplate code that call the functions
	3. Write parameter methods that have the syntax void setXXX(double value)
	4. Use DEFINE_PARAMETER_DATA in the createParameter callback
*/
#define DEFINE_PARAMETERS template <int P> static void setParameter(void* obj, double value)
#define DEF_PARAMETER(ParameterName, ClassName) if (P == (int)Parameters::ParameterName) static_cast<ClassName*>(obj)->set##ParameterName(value);
#define DEFINE_PARAMETERDATA(ClassName, ParameterName) parameter::data p(#ParameterName); p.dbNew = parameter::inner<ClassName, (int)Parameters::ParameterName>(*this);


/** Object Accessors

	/

/** Use this macro to define the type that should be returned by calls to getObject(). Normally you pass in the wrapped object (for non-wrapped classes you should use GET_SELF_AS_OBJECT(). */
#define GET_SELF_OBJECT(x) constexpr auto& getObject() { return x; } \
constexpr const auto& getObject() const { return x; }

/** Use this macro to define the expression that should be used in order to get the most nested type. (usually you pass in obj.getWrappedObject(). */
#define GET_WRAPPED_OBJECT(x) constexpr auto& getWrappedObject() { return x; } \
constexpr const auto& getWrappedObject() const { return x; }

/** Use this macro in order to create the getObject() / getWrappedObject() methods that return the object itself. */
#define SN_GET_SELF_AS_OBJECT(x) GET_SELF_OBJECT(*this); GET_WRAPPED_OBJECT(*this); using ObjectType = x; using WrappedObjectType = x;


#define SN_SELF_AWARE_WRAPPER(x, ObjectClass) GET_SELF_OBJECT(*this); GET_WRAPPED_OBJECT(obj.getWrappedObject()); using ObjectType = x; using WrappedObjectType = typename ObjectClass::WrappedObjectType;

#define SN_OPAQUE_WRAPPER(x, ObjectClass) GET_SELF_OBJECT(obj.getObject()); GET_WRAPPED_OBJECT(obj.getWrappedObject()); using ObjectType = typename ObjectClass::ObjectType; using WrappedObjectType= typename ObjectClass::WrappedObjectType;



/** Callback macros.

	*/

/** Use this definition when you forward a wrapper logic. */
#define INTERNAL_PROCESS_FUNCTION(ObjectClass) template <typename ProcessDataType> static void processInternal(void* obj, ProcessDataType& data) { auto& typed = *static_cast<ObjectClass*>(obj); typed.process(data); }
#define INTERNAL_PREPARE_FUNCTION(ObjectClass) static void prepareInternal(void* obj, PrepareSpecs ps) { auto& typed = *static_cast<ObjectClass*>(obj); typed.prepare(ps); }


/** Use these for default forwarding to the wrapped element. */
#define HISE_DEFAULT_RESET(ObjectType) void reset() { obj.reset(); }
#define HISE_DEFAULT_MOD(ObjectType) bool handleModulation(double& v) { return obj.handleModulation(v); }
#define HISE_DEFAULT_HANDLE_EVENT(ObjectType) void handleHiseEvent(HiseEvent& e) { obj.handleHiseEvent(e); }
#define HISE_DEFAULT_INIT(ObjectType) void initialise(NodeBase* n) { obj.initialise(n); }
#define HISE_DEFAULT_PROCESS(ObjectType) template <typename ProcessDataType> void process(ProcessDataType& d) { obj.process(d); }
#define HISE_DEFAULT_PREPARE(ObjectType) void prepare(PrepareSpecs ps) { obj.prepare(ps); }
#define HISE_DEFAULT_PROCESS_FRAME(ObjectType) template <typename FrameDataType> void processFrame(FrameDataType& data) noexcept { this->obj.processFrame(data); }


/** Stack float array macros. 

	Beware of using to much stack memory !
*/
#define ALLOCA_FLOAT_ARRAY(size) (float*)alloca(size * sizeof(float)); 
#define CLEAR_FLOAT_ARRAY(v, size) memset(v, 0, sizeof(float)*size);


#define CREATE_EXTRA_COMPONENT(className) Component* createExtraComponent(PooledUIUpdater* updater) \
										  { return new className(updater); };


/** Node definition macros. */

#define SET_HISE_POLY_NODE_ID(id) SET_HISE_NODE_ID(id); bool isPolyphonic() const { return NumVoices > 1; };

#define SET_HISE_NODE_ID(id) static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER(id); };
//#define SET_HISE_NODE_EXTRA_HEIGHT(x) int getExtraHeight() const final override { return x; };
//#define SET_HISE_NODE_EXTRA_WIDTH(x) int getExtraWidth() const final override { return x; };
#define SET_HISE_EXTRA_COMPONENT(height, className) SET_HISE_NODE_EXTRA_HEIGHT(height); \
												    CREATE_EXTRA_COMPONENT(className);

/** Node empty callback macros. */

#define HISE_EMPTY_RESET void reset() {}
#define HISE_EMPTY_PREPARE void prepare(PrepareSpecs) {}
#define HISE_EMPTY_PROCESS template <typename ProcessDataType> void process(ProcessDataType&) {}
#define HISE_EMPTY_PROCESS_SINGLE template <typename FrameDataType> void processFrame(FrameDataType& ) {}
#define HISE_EMPTY_CREATE_PARAM void createParameters(ParameterDataList&){}
#define HISE_EMPTY_MOD bool handleModulation(double& ) { return false; }
#define HISE_EMPTY_HANDLE_EVENT void handleEvent(HiseEvent& e) {};

/** Node Factory macros. */

#define DEFINE_EXTERN_MONO_TEMPLATE(monoName, classWithTemplate) extern template class classWithTemplate; using monoName = classWithTemplate;

#define DEFINE_EXTERN_NODE_TEMPLATE(monoName, polyName, className) extern template class className<1>; \
using monoName = className<1>; \
extern template class className<NUM_POLYPHONIC_VOICES>; \
using polyName = className<NUM_POLYPHONIC_VOICES>; 

#define DEFINE_EXTERN_MONO_TEMPIMPL(classWithTemplate) template class classWithTemplate;

#define DEFINE_EXTERN_NODE_TEMPIMPL(className) template class className<1>; template class className<NUM_POLYPHONIC_VOICES>;



/** Snex JIT Preprocessors */

#define FORWARD_PARAMETER_TO_MEMBER(className) DEFINE_PARAMETERS { static_cast<className*>(obj)->setParameter<P>(value); }

/** This is being used to tuck away everything that that JIT compiler can't parse. */
#define DECLARE_NODE(className) SET_HISE_NODE_ID(#className); FORWARD_PARAMETER_TO_MEMBER(className); SN_GET_SELF_AS_OBJECT(className);




}
