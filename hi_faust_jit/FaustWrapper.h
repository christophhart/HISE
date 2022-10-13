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

class faust_jit_node;
// wrapper struct for faust types to avoid name-clash
struct faust_jit_wrapper : public faust_base_wrapper {

	static bool isValidClassId(String cid)
	{
		if (cid.length() <= 0)
			return false;

		if (!isalpha(cid[0]) && cid[0] != '_')
			return false;

		for (auto c : cid) {
			if (!isalnum(c) && c != '_')
				return false;
		}

		return true;
	}

	faust_jit_wrapper():
		faust_base_wrapper(nullptr),
#if !HISE_FAUST_USE_LLVM_JIT
		interpreterFactory(nullptr),
#else // HISE_FAUST_USE_LLVM_JIT
		jitFactory(nullptr),
#endif // !HISE_FAUST_USE_LLVM_JIT
		classId("")
	{ }

	~faust_jit_wrapper()
	{
		if(faustDsp)
			delete faustDsp;
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
		if (faustDsp != nullptr) {
			delete faustDsp;
			faustDsp = nullptr;
		}
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

		ui.reset();

		const char* incl = "-I";
		std::vector<const char*> llvm_argv = {"-rui"};
		for (const std::string &p : faustLibraryPaths) {
			llvm_argv.push_back(incl);
			llvm_argv.push_back(p.c_str());
		}
		llvm_argv.push_back(nullptr);

#if !HISE_FAUST_USE_LLVM_JIT
		interpreterFactory = ::faust::createInterpreterDSPFactoryFromString("faust", code, llvm_argv.size() - 1,
		  &(llvm_argv[0]), errorMessage);
		if (interpreterFactory == nullptr) {
			// error indication
			error_msg = errorMessage;
			return false;
		}
		DBG("Faust interpreter instantiation successful");
		faustDsp = interpreterFactory->createDSPInstance();
#else // HISE_FAUST_USE_LLVM_JIT
		jitFactory = ::faust::createDSPFactoryFromString("faust", code, (int)llvm_argv.size() - 1, &(llvm_argv[0]),
														 "", errorMessage, jitOptimize);
		if (jitFactory == nullptr) {
			// error indication
			error_msg = errorMessage;
			return false;
		}
		DBG("Faust jit compilation successful");
		faustDsp = jitFactory->createDSPInstance();
#endif // !HISE_FAUST_USE_LLVM_JIT
		if (faustDsp == nullptr) {
			error_msg = "Faust DSP instantiation failed";
			return false;
		}

		DBG("Faust DSP instantiation successful");

		faust_base_wrapper::setup();
		return true;
	}


	String getClassId() {
		return classId;
	}

	bool setCode(String newClassId, std::string newCode) {
		if (!isValidClassId(newClassId))
			return false;
		classId = newClassId;
		code = newCode;
		return true;
	}

	void process(ProcessDataDyn& data)
	{
		// run jitted code only while holding the corresponding lock:
		juce::ScopedTryLock stl(jitLock);
		if (stl.isLocked() && faustDsp) {
			faust_base_wrapper::process(data);
		} else {
			// std::cout << "Faust: dsp was not initialized" << std::endl;
		}
	}

	template <class FrameDataType> void processFrame(FrameDataType& data)
	{
		// run jitted code only while holding the corresponding lock:
		juce::ScopedTryLock stl(jitLock);
		if (stl.isLocked() && faustDsp) {
			faust_base_wrapper::processFrame<FrameDataType>(data);
		} else {
			// std::cout << "Faust: dsp was not initialized" << std::endl;
		}
	}

	static std::string prefixClassForFaust(std::string _classId) {
		return "_" + _classId;
	}

	std::string classIdForFaustClass() {
		return prefixClassForFaust(classId.toStdString());
	}

	static std::string genStaticInstanceBoilerplate(std::string dest_dir, std::string _classId) {
		if (!isValidClassId(_classId)) {
			// TODO: error indication
			return "";
		}
		std::string dest_file = _classId + ".h";
		std::string metaDataClass = _classId + "MetaData";
		std::string faustClassId = prefixClassForFaust(_classId);
		std::string body =
			"#pragma once\n"
			"#include \"hi_faust/hi_faust.h\"\n"
			"using Meta = ::faust::Meta;\n"
			"using UI = ::faust::UI;\n"
			"#define FAUST_UIMACROS\n"
			" // define dummy macros\n"
			"#define FAUST_ADDCHECKBOX(...)\n"
			"#define FAUST_ADDNUMENTRY(...)\n"
			"#define FAUST_ADDBUTTON(...)\n"
			"#define FAUST_ADDHORIZONTALSLIDER(...)\n"
			"#define FAUST_ADDVERTICALSLIDER(...)\n"
			"#define FAUST_ADDVERTICALBARGRAPH(...)\n"
			"#define FAUST_ADDHORIZONTALBARGRAPH(...)\n"
			"#define FAUST_ADDSOUNDFILE(...)\n"
			"#include \"src/" + _classId + ".cpp\"\n"
			"#if (FAUST_INPUTS - FAUST_OUTPUTS) > 0\n"
			"#error Number of inputs and outputs in faust code must match!\n"
			"#endif\n"
			"namespace project {\n"
			"struct " + metaDataClass + " {\n"
			"		SN_NODE_ID(\"" + _classId + "\");\n"
			"};\n"
			"template <int NV>\n"
			"using " + _classId + " = scriptnode::faust::faust_static_wrapper<1, " + faustClassId + " , " + metaDataClass + ", FAUST_OUTPUTS>;\n"
			"} // namespace project\n"
			" // undef dummy macros\n"
			"#undef FAUST_UIMACROS\n"
			"#undef FAUST_ADDCHECKBOX\n"
			"#undef FAUST_ADDNUMENTRY\n"
			"#undef FAUST_ADDBUTTON\n"
			"#undef FAUST_ADDHORIZONTALSLIDER\n"
			"#undef FAUST_ADDVERTICALSLIDER\n"
			"#undef FAUST_ADDVERTICALBARGRAPH\n"
			"#undef FAUST_ADDHORIZONTALBARGRAPH\n"
			"#undef FAUST_ADDSOUNDFILE\n"
			" // undef faust ui macros\n"
			"#undef FAUST_FILE_NAME\n"
			"#undef FAUST_CLASS_NAME\n"
			"#undef FAUST_COMPILATION_OPIONS\n"
			"#undef FAUST_INPUTS\n"
			"#undef FAUST_OUTPUTS\n"
			"#undef FAUST_ACTIVES\n"
			"#undef FAUST_PASSIVES\n"
			"#undef FAUST_LIST_ACTIVES\n"
			"#undef FAUST_LIST_PASSIVES\n";

		auto dir = juce::File(dest_dir);
		if (!dir.isDirectory())
			return "";

		auto dest = dir.getChildFile(dest_file);
		dest.replaceWithText(body);

		DBG("Static body file generation successful: " + dest.getFullPathName());

		return dest_file;
	}

	static bool genAuxFile(std::string srcPath, int argc, const char* argv[]) {
		std::string aux_content = "none";
		std::string error_msg;

		if (!::faust::generateAuxFilesFromFile(srcPath, argc, argv, error_msg)) {
			// TODO replace DBG with appropriate error logging function
			DBG("hi_faust_jit: Aux file generation failed:");
			DBG("argv: ");
			while (*argv) {
				DBG(std::string("\t") + (*argv++));
			}
			DBG("result: " + error_msg);
			return false;
		}
		return true;
	}

	static std::string genStaticInstanceCode(std::string _classId, std::string srcPath, std::vector<std::string> faustLibraryPaths, std::string dest_dir) {
		if (!isValidClassId(_classId)) {
			// TODO: error indication
			return "";
		}

		std::string dest_file = _classId + ".cpp";
		std::string faustClassId = prefixClassForFaust(_classId);

		const char* incl = "-I";
		std::vector<const char*> argv = {"-uim", "-nvi", "-rui", "-lang", "cpp", "-scn", "::faust::dsp", "-cn", faustClassId.c_str(), "-O", dest_dir.c_str(), "-o", dest_file.c_str()};
		for (const std::string &p : faustLibraryPaths) {
			argv.push_back(incl);
			argv.push_back(p.c_str());
		}
		argv.push_back(nullptr);

		if (genAuxFile(srcPath, (int)argv.size() - 1, &(argv[0]))) {
			DBG("hi_faust_jit: Static code generation successful: " + dest_file);
			return dest_file;
		}
		return "";
	}

private:
	String classId;
};

} // namespace faust
} // namespace scriptnode

#endif // __FAUST_JIT_WRAPPER_H
