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

namespace scriptnode { using namespace juce;


namespace prototypes
{
	using namespace snex;
	using namespace Types;

	typedef void(*prepare)(void*, PrepareSpecs*);
	typedef void(*reset)(void*);
	typedef void(*setParameter)(void*, double);
	typedef void(*handleHiseEvent)(void*, HiseEvent*);
	typedef void(*initialise)(void*, NodeBase*);
	typedef void(*destruct)(void*);
	typedef int(*handleModulation)(void*, double*);
	typedef void(*setExternalData)(void*, const ExternalData*, int);

	template <typename ProcessDataType> using process = void(*)(void*, ProcessDataType*);
	template <typename FrameDataType> using processFrame = void(*)(void*, FrameDataType*);

	namespace check
	{
		template <typename T> class setExternalData
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::setExternalData));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class initialise
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::initialise));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class processSample
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::processSample));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class isPolyphonic
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::isPolyphonic));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class isSuspendedOnSilence
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::isSuspendedOnSilence));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class hasTail
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::hasTail));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class createParameters
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::createParameters));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class prepare
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::prepare));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};
    
        template <typename T> class process
        {
            typedef char one; struct two { char x[2]; };
            template <typename C> static one test(decltype(&C::process));
            template <typename C> static two test(...);
        public:
            enum { value = sizeof(test<T>(0)) == sizeof(char) };
        };

        template <typename T> class processFrame
        {
            typedef char one; struct two { char x[2]; };
            template <typename C> static one test(decltype(&C::processFrame));
            template <typename C> static two test(...);
        public:
            enum { value = sizeof(test<T>(0)) == sizeof(char) };
        };
    
		template <typename T> class reset
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::reset));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class getDescription
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::getDescription));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class getDefaultValue
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::getDefaultValue));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class isNormalisedModulation
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::isNormalisedModulation));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class handleModulation
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::handleModulation));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class getFixChannelAmount
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::getFixChannelAmount));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};

		template <typename T> class isProcessingHiseEvent
		{
			typedef char one; struct two { char x[2]; };
			template <typename C> static one test(decltype(&C::isProcessingHiseEvent));
			template <typename C> static two test(...);
		public:
			enum { value = sizeof(test<T>(0)) == sizeof(char) };
		};
	}

	template <typename T> struct static_wrappers
	{
		static T* create(void* obj) { return new (obj)T(); };
		static void destruct(void* obj) { static_cast<T*>(obj)->~T(); }

		static void prepare(void* obj, PrepareSpecs* ps) 
		{ 
#if THROW_SCRIPTNODE_ERRORS
			static_cast<T*>(obj)->prepare(*ps);
#else
			try
			{
				static_cast<T*>(obj)->prepare(*ps);
			}
			catch (scriptnode::Error& e)
			{
				DynamicLibraryErrorStorage::currentError = e;
			}
#endif
		};

		template <typename ProcessDataType> static void process(void* obj, ProcessDataType* data) { static_cast<T*>(obj)->process(*data); }
		template <typename FrameDataType> static void processFrame(void* obj, FrameDataType* data) { static_cast<T*>(obj)->processFrame(*data); };
		static void reset(void* obj) { static_cast<T*>(obj)->reset(); }
		static void handleHiseEvent(void* obj, HiseEvent* e) { static_cast<T*>(obj)->handleHiseEvent(*e); };
		static void initialise(void* obj, NodeBase* n) { static_cast<T*>(obj)->initialise(n); };
		static int handleModulation(void* obj, double* modValue) { return (int)static_cast<T*>(obj)->handleModulation(*modValue); }
		static void setExternalData(void* obj, const ExternalData* d, int index) { static_cast<T*>(obj)->setExternalData(*d, index); }
	};

	struct noop
	{
		static void prepare(void* obj, PrepareSpecs* ps) { ignoreUnused(obj, ps); }
		template <typename ProcessDataType> static void process(void* obj, ProcessDataType* data) { ignoreUnused(obj, data); }
		template <typename FrameDataType> static void processFrame(void* obj, FrameDataType* data) { ignoreUnused(obj, data); };
		static void reset(void* obj) { ignoreUnused(obj); }
		static void handleHiseEvent(void* obj, HiseEvent* e) { ignoreUnused(obj, e); };
		static void initialise(void* obj, NodeBase* n) { ignoreUnused(obj, n); };
		static int handleModulation(void* obj, double* modValue) { ignoreUnused(obj, modValue); return 0; }
		static void setExternalData(void* obj, const ExternalData* d, int index) { ignoreUnused(obj, d, index); }
	};
}





} 
