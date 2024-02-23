

#include "Exporter.h"

namespace hise {
namespace multipage {
using namespace juce;



var projucer_exporter::exportProjucerProject(State::Job& t, const var& stateObject)
{
	auto exportObj = appState.currentDialog->exportAsJSON();

	auto projectName = exportObj[mpid::Properties][mpid::ProjectName].toString();

	multipage::CodeGenerator cg(rootDir, projectName, exportObj);

	cg.company = exportObj[mpid::Properties][mpid::Company].toString();
	cg.version = exportObj[mpid::Properties][mpid::Version].toString();
	cg.hisePath = stateObject["hisePath"].toString();

	using FT = multipage::CodeGenerator::FileType;

	for(int i = 0; i < (int)FT::numFileTypes; i++)
	{
		auto f = cg.getFile((FT)i);
		f.getParentDirectory().createDirectory();
		f.deleteFile();
		FileOutputStream fos(f);
		cg.write(fos, (FT)i, &t);
	}

	auto batchFile = cg.getFile(CodeGenerator::FileType::BatchFile);

#if JUCE_WINDOWS
	batchFile.startAsProcess();
#elif JUCE_MAC
    String permissionCommand = "chmod +x " + batchFile.getFullPathName().quoted();
    system(permissionCommand.getCharPointer());
    String command = "open " + batchFile.getFullPathName().quoted();
	system(command.getCharPointer().getAddress());
#elif JUCE_LINUX
    // dave...
#endif
	
	return var();
}

using MyClass = projucer_exporter;

Dialog* projucer_exporter::createDialog(State& state)
{
	using namespace factory;
	auto mp_ = new Dialog({}, state, false);
	auto& mp = *mp_;
	mp.setProperty(mpid::Header, "Export Projucer Project");
	mp.setProperty(mpid::Subtitle, "");
	auto& List_0 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& hisePath_1 = List_0.addChild<factory::FileSelector>({
	  { mpid::Text, "HISE Path" }, 
	  { mpid::ID, "hisePath" }, 
	  { mpid::UseInitValue, 0 }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The path to the HISE source code repository folder (the root directory with the `hi_xxx` subdirectories)." }, 
	  { mpid::Directory, 1 }, 
	  { mpid::SaveFile, 0 }
	});

	auto& Spacer_2 = List_0.addChild<factory::Spacer>({
	  { mpid::Text, "LabelText" }, 
	  { mpid::Padding, "30" }
	});

	auto& export_3 = List_0.addChild<factory::LambdaTask>({
	  { mpid::Text, "Export Progress" }, 
	  { mpid::ID, "export" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Function, "exportProjucerProject" }
	});

	// TODO: add var exportProjucerProject(State::Job& t, const var& stateObject) to class
	export_3.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(MyClass::exportProjucerProject));
	
	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_4 = mp.addPage<factory::List>({
	  { mpid::Padding, 10 }
	});

	auto& MarkdownText_5 = List_4.addChild<factory::MarkdownText>({
	  { mpid::Text, "The project was created successfully. Do you want to launch the projucer to continue building the dialog binary?" }, 
	  { mpid::Padding, 0 }
	});

	auto& openProjucer_6 = List_4.addChild<factory::Button>({
	  { mpid::Text, "Open Projucer" }, 
	  { mpid::ID, "openProjucer" }, 
	  { mpid::UseInitValue, 0 }, 
	  { mpid::LabelPosition, "Default" }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& openProjucer_7 = List_4.addChild<factory::Launch>({
	  { mpid::Text, "$hisePath/tools/Projucer.exe" }, 
	  { mpid::ID, "openProjucer" }, 
	  { mpid::CallOnNext, 1 }, 
	  { mpid::Args, "$projectDirectory/$projectName.jucer" }
	});

	// Custom callback for page List_4
	List_4.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	return mp_;
}
} // namespace multipage
} // namespace hise
