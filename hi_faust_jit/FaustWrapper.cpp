

namespace scriptnode {
namespace faust {

bool faust_jit_helpers::isValidClassId(String cid)
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

std::string faust_jit_helpers::prefixClassForFaust(std::string _classId)
{
	return "_" + _classId;
}

std::string faust_jit_helpers::genStaticInstanceBoilerplate(std::string dest_dir, std::string _classId)
{
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
		"template <int NV, class ModParameterClass=scriptnode::parameter::empty_list>\n"
		"using " + _classId + " = scriptnode::faust::faust_static_wrapper<NV, ModParameterClass, " + faustClassId + ", " + metaDataClass + ", FAUST_OUTPUTS>;\n"
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

bool faust_jit_helpers::genAuxFile(std::string srcPath, int argc, const char* argv[])
{
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

std::string faust_jit_helpers::genStaticInstanceCode(std::string _classId, std::string srcPath, std::vector<std::string> faustLibraryPaths, std::string dest_dir)
{
	if (!isValidClassId(_classId)) {
		// TODO: error indication
		return "";
	}

	std::string dest_file = _classId + ".cpp";
	std::string faustClassId = prefixClassForFaust(_classId);

	const char* incl = "-I";
	std::vector<const char*> argv = { "-uim", "-nvi", "-rui", "-lang", "cpp", "-scn", "::faust::dsp", "-cn", faustClassId.c_str(), "-O", dest_dir.c_str(), "-o", dest_file.c_str() };
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

} // namespace faust
} // namespace scriptnode


