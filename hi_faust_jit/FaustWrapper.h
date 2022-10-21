#include "hi_core/hi_core/HiseSettings.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cctype>
using namespace std::chrono_literals;

#ifndef __FAUST_JIT_WRAPPER_H
#define __FAUST_JIT_WRAPPER_H

namespace scriptnode {
namespace faust {

// All static functions for faust codegeneration etc. are moved to this class to avoid template bloat
struct faust_jit_helpers
{
	static bool isValidClassId(String cid);
	static std::string prefixClassForFaust(std::string _classId);
	static std::string genStaticInstanceBoilerplate(std::string dest_dir, std::string _classId);
	static bool genAuxFile(std::string srcPath, int argc, const char* argv[]);
	static std::string genStaticInstanceCode(std::string _classId, std::string srcPath, std::vector<std::string> faustLibraryPaths, std::string dest_dir);
};


// wrapper struct for faust types to avoid name-clash
template <int NV> struct faust_jit_wrapper : public faust_base_wrapper<NV> 
{
	faust_jit_wrapper():
		faust_base_wrapper<NV>(),
#if !HISE_FAUST_USE_LLVM_JIT
		interpreterFactory(nullptr),
#else // HISE_FAUST_USE_LLVM_JIT
		jitFactory(nullptr),
#endif // !HISE_FAUST_USE_LLVM_JIT
		classId("")
	{ }

	void deleteFaustObjects()
	{
		if (this->initialisedOk())
		{
			for (auto& fdsp : this->faustDsp)
			{
				if (fdsp != nullptr)
					delete fdsp;

				fdsp = nullptr;
			}
		}
	}

	~faust_jit_wrapper()
	{
		deleteFaustObjects();

#if !HISE_FAUST_USE_LLVM_JIT
		if (interpreterFactory)
			::faust::deleteInterpreterDSPFactory(interpreterFactory);
#else // HISE_FAUST_USE_LLVM_JIT
		if (jitFactory)
			::faust::deleteDSPFactory(jitFactory);
#endif // !HISE_FAUST_USE_LLVM_JIT
	}

	std::string code;
	std::string errorMessage;
	int jitOptimize = 0; // -1 is maximum optimization
#if !HISE_FAUST_USE_LLVM_JIT
	::faust::interpreter_dsp_factory* interpreterFactory;
#else // HISE_FAUST_USE_LLVM_JIT
	::faust::llvm_dsp_factory* jitFactory;
#endif // !HISE_FAUST_USE_LLVM_JIT

	// Mutex for synchronization of compilation and processing
	juce::CriticalSection jitLock;

	bool setup(std::vector<std::string> faustLibraryPaths, std::string& error_msg) {
		juce::ScopedLock sl(jitLock);
		// cleanup old code and factories
		// make sure faustDsp is nullptr in case we fail to recompile
		// so we don't use an old deallocated faustDsp in process (checks
		// for faustDsp == nullptr)
		deleteFaustObjects();

#if !HISE_FAUST_USE_LLVM_JIT
		if (interpreterFactory != nullptr) {
			::faust::deleteInterpreterDSPFactory(interpreterFactory);
			// no need to set interpreterFactory=nullptr, as it will be overwritten in the next step
		}
#else // HISE_FAUST_USE_LLVM_JIT
		if (jitFactory != nullptr) {
			::faust::deleteDSPFactory(jitFactory);
			// no need to set jitFactory=nullptr, as it will be overwritten in the next step
		}
#endif // !HISE_FAUST_USE_LLVM_JIT

		this->ui.reset();

		const char* incl = "-I";
		std::vector<const char*> llvm_argv = {"-rui"};
		for (const std::string &p : faustLibraryPaths) {
			llvm_argv.push_back(incl);
			llvm_argv.push_back(p.c_str());
		}
		llvm_argv.push_back(nullptr);

#if !HISE_FAUST_USE_LLVM_JIT
		interpreterFactory = ::faust::createInterpreterDSPFactoryFromString("faust", code, (int)llvm_argv.size() - 1,
		  &(llvm_argv[0]), errorMessage);
		if (interpreterFactory == nullptr) {
			// error indication
			error_msg = errorMessage;
			return false;
		}
		DBG("Faust interpreter instantiation successful");

		for(auto& fdsp: this->faustDsp)
			fdsp = interpreterFactory->createDSPInstance();

#else // HISE_FAUST_USE_LLVM_JIT
		jitFactory = ::faust::createDSPFactoryFromString("faust", code, (int)llvm_argv.size() - 1, &(llvm_argv[0]),
														 "", errorMessage, jitOptimize);
		if (jitFactory == nullptr) {
			// error indication
			error_msg = errorMessage;
			return false;
		}
		DBG("Faust jit compilation successful");

		for (auto& fdsp : this->faustDsp)
			fdsp = jitFactory->createDSPInstance();

#endif // !HISE_FAUST_USE_LLVM_JIT
		if (!this->initialisedOk()) {
			error_msg = "Faust DSP instantiation failed";
			return false;
		}

		DBG("Faust DSP instantiation successful");

		faust_base_wrapper<NV>::setup();
		return true;
	}

	String getClassId() {
		return classId;
	}

	bool setCode(String newClassId, std::string newCode) {
		if (!faust_jit_helpers::isValidClassId(newClassId))
			return false;
		classId = newClassId;
		code = newCode;
		return true;
	}

	void process(ProcessDataDyn& data)
	{
		// run jitted code only while holding the corresponding lock:
		juce::ScopedTryLock stl(jitLock);
		if (stl.isLocked() && this->initialisedOk()) {
			faust_base_wrapper<NV>::process(data);
		} else {
			// std::cout << "Faust: dsp was not initialized" << std::endl;
		}
	}

	template <class FrameDataType> void processFrame(FrameDataType& data)
	{
		// run jitted code only while holding the corresponding lock:
		juce::ScopedTryLock stl(jitLock);
		if (stl.isLocked() && this->initialisedOk()) {
			faust_base_wrapper<NV>::template processFrame<FrameDataType>(data);
		} else {
			// std::cout << "Faust: dsp was not initialized" << std::endl;
		}
	}

	std::string classIdForFaustClass() {
		return faust_jit_helpers::prefixClassForFaust(classId.toStdString());
	}

private:
	String classId;
};

} // namespace faust
} // namespace scriptnode

#endif // __FAUST_JIT_WRAPPER_H
