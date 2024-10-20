

#include "Exporter.h"

namespace hise {
namespace multipage {
using namespace juce;



var projucer_exporter::exportProjucerProject(State::Job& t, const var& stateObject)
{
    appState.callEventListeners("save", {});
    
	if(!exportObj.isObject())
		exportObj = appState.getFirstDialog()->exportAsJSON();

	auto projectName = exportObj[mpid::Properties][mpid::ProjectName].toString();

	multipage::CodeGenerator cg(rootDir, projectName, exportObj);

	cg.company = exportObj[mpid::Properties][mpid::Company].toString();
	cg.version = exportObj[mpid::Properties][mpid::Version].toString();
	cg.hisePath = stateObject["hisePath"].toString();
    cg.teamId = stateObject["teamID"].toString();

	auto startCompile = !(bool)stateObject["skipCompilation"];

	{
		ScopedSetting ss;
		ss.set("hisePath", cg.hisePath);
		ss.set("teamID", stateObject["teamID"]);
	}

	using FT = multipage::CodeGenerator::FileType;

	for(int i = 0; i < (int)FT::numFileTypes; i++)
	{
		auto f = cg.getFile((FT)i);
		f.getParentDirectory().createDirectory();
		f.deleteFile();
		FileOutputStream fos(f);
		cg.write(fos, (FT)i, &t);
	}

	if(!startCompile)
	{
		return var(0);
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
	
	return var(0);
}



void projucer_exporter::postInit()
{
	ScopedSetting ss;
	setProperty("hisePath", ss.get("hisePath", ""));
	setProperty("teamID", ss.get("teamID"));
}

using MyClass = projucer_exporter;

Dialog* projucer_exporter::createDialog(State& state)
{
	
	DynamicObject::Ptr fullData = new DynamicObject();
	fullData->setProperty(mpid::StyleData, JSON::parse(R"({"Font": "Lato Regular", "BoldFont": "<Sans-Serif>", "FontSize": 18.0, "bgColour": 4279834905, "codeBgColour": 864585864, "linkBgColour": 8947967, "textColour": 4289901234, "codeColour": 4294967295, "linkColour": 4289374975, "tableHeaderBgColour": 864059520, "tableLineColour": 864059520, "tableBgColour": 864059520, "headlineColour": 4287692721, "UseSpecialBoldFont": false, "buttonTabBackgroundColour": 570425344, "buttonBgColour": 4289374890, "buttonTextColour": 4280624421, "modalPopupBackgroundColour": 4281545523, "modalPopupOverlayColour": 3995214370, "modalPopupOutlineColour": 4289374890, "pageProgressColour": 2013265919})"));
	fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"TopHeight": 56, "ButtonTab": 40, "ButtonMargin": 5, "OuterPadding": 50, "LabelWidth": 120.0, "LabelHeight": 32, "DialogWidth": 700, "DialogHeight": 600, "LabelPosition": "Default"})"));
	fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Export Projucer Project", "Subtitle": "", "Image": "", "ProjectName": "", "Company": "", "Version": ""})"));
	using namespace factory;
	auto mp_ = new Dialog(var(fullData.get()), state, false);
	auto& mp = *mp_;
	auto& List_0 = mp.addPage<factory::List>({
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& hisePath_1 = List_0.addChild<factory::FileSelector>({
	  { mpid::Text, "HISE Path" }, 
	  { mpid::ID, "hisePath" }, 
	  { mpid::UseInitValue, 0 }, 
	  { mpid::Required, 1 }, 
	  { mpid::Help, "The path to the HISE source code repository folder (the root directory with the `hi_xxx` subdirectories)." }, 
	  { mpid::Directory, 1 }, 
	  { mpid::SaveFile, 0 }
	});

	auto& teamID_2 = List_0.addChild<factory::TextInput>({
	  { mpid::Text, "Team ID" }, 
	  { mpid::ID, "teamID" }, 
	  { mpid::UseInitValue, 0 }, 
	  { mpid::EmptyText, "Enter Team Development ID " }, 
	  { mpid::Required, 0 }, 
	  { mpid::ParseArray, 0 }, 
	  { mpid::Height, 80 }, 
	  { mpid::Help, "The macOS Team Development ID for signing the compiled binary." }, 
	  { mpid::Multiline, 0 }
	});

	auto& Spacer_3 = List_0.addChild<factory::Spacer>({
	  { mpid::Text, "LabelText" }, 
	});

	auto& export_4 = List_0.addChild<factory::LambdaTask>({
	  { mpid::Text, "Export Progress" }, 
	  { mpid::ID, "export" }, 
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Function, "exportProjucerProject" }
	});

	// TODO: add var exportProjucerProject(State::Job& t, const var& stateObject) to class
	export_4.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(MyClass::exportProjucerProject));
	
	// Custom callback for page List_0
	List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	auto& List_5 = mp.addPage<factory::List>({
	  { mpid::Foldable, 0 }, 
	  { mpid::Folded, 0 }
	});

	auto& MarkdownText_6 = List_5.addChild<factory::MarkdownText>({
	  { mpid::Text, "The project was created successfully. Do you want to launch the projucer to continue building the dialog binary?" }, 
	});

	auto& openProjucer_7 = List_5.addChild<factory::Button>({
	  { mpid::Text, "Open Projucer" }, 
	  { mpid::ID, "openProjucer" }, 
	  { mpid::UseInitValue, 0 }, 
	  { mpid::Required, 0 }, 
	  { mpid::Trigger, 0 }
	});

	auto& openProjucer_8 = List_5.addChild<factory::Launch>({
	  { mpid::Text, "$hisePath/tools/Projucer.exe" }, 
	  { mpid::ID, "openProjucer" }, 
	  { mpid::EventTrigger, "OnSubmit" }, 
	  { mpid::Args, "\"$projectDirectory/$projectName.jucer\"" }
	});

	// Custom callback for page List_5
	List_5.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

		return Result::ok();

	});
	
	return mp_;
}
} // namespace multipage
} // namespace hise
