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
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#include "DialogLibrary.h"
#include "MainComponent.h"



namespace hise {
namespace multipage {
namespace library {
using namespace juce;
var CreateCSSFromTemplate::createFile(State::Job& t, const var& state)
{
    // All variables:
    auto file = File(state["file"].toString());
    auto templateIndex = (int)state["templateIndex"];
    auto addAsAsset = (bool)state["addAsAsset"];

    auto c = DefaultCSSFactory::getTemplate((DefaultCSSFactory::Template)templateIndex);

    file.replaceWithText(c);
    
    if(addAsAsset)
    {
        MessageManagerLock mm;
        auto& am = findParentComponentOfClass<MainComponent>()->assetManager;
        auto a = am->addAsset(file);
        a->id = file.getFileName();
    }
    
    
    
    return var(""); // return a error
}
Dialog* CreateCSSFromTemplate::createDialog(State& state)
{
    DynamicObject::Ptr fullData = new DynamicObject();
    fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "ModalPopup", "Style": "", "DialogWidth": 700, "DialogHeight": 330})"));
    fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Create Stylesheet", "Subtitle": "", "Image": "", "ProjectName": "CreateCSSFromTemplate", "Company": "HISE", "Version": "1.0.0", "BinaryName": "", "UseGlobalAppData": false, "Icon": ""})"));
    using namespace factory;
    auto mp_ = new Dialog(var(fullData.get()), state, false);
    auto& mp = *mp_;
    auto& List_0 = mp.addPage<List>({
    });

    List_0.addChild<FileSelector>({
      { mpid::Text, "CSS File" },
      { mpid::ID, "file" },
      { mpid::Required, 1 },
      { mpid::Wildcard, "*.css" },
      { mpid::SaveFile, 1 },
      { mpid::Help, "The CSS file to be created.  \n> it's highly recommended to pick a file that is relative to the `json` file you're using to create this dialog!." },
      { mpid::Directory, 0 }
    });

    List_0.addChild<Choice>({
      { mpid::Text, "Template" },
      { mpid::ID, "templateIndex" },
      { mpid::InitValue, "Dark" },
      { mpid::UseInitValue, 1 },
      { mpid::Custom, 0 },
      { mpid::ValueMode, "Index" },
      { mpid::Help, "The template to be used by the style sheet." },
      { mpid::Items, DefaultCSSFactory::getTemplateList() }
    });

    List_0.addChild<Button>({
      { mpid::Text, "Add as asset" },
      { mpid::ID, "addAsAsset" },
      { mpid::InitValue, "true" },
      { mpid::UseInitValue, 1 },
      { mpid::Help, "Whether to add this file as asset to the current dialog." }
    });

    auto& createFile_4 = List_0.addChild<LambdaTask>({
      { mpid::ID, "createFile" },
      { mpid::EventTrigger, "OnSubmit" },
      { mpid::Style, "display: none;" },
      { mpid::Function, "createFile" }
    });

    // TODO: add var createFile(State::Job& t, const var& stateObject) to class
    createFile_4.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(CreateCSSFromTemplate::createFile));
    
    // Custom callback for page List_0
    List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise


namespace hise {
namespace multipage {
namespace library {
using namespace juce;
var ProjectExporter::exportProjucerProject(State::Job& t, const var& state)
{
    // All variables:
    auto hisePath = state["HisePath"];
    auto teamID = state["teamID"];
    auto exportId = state["export"];
    auto openProjucer = state["openProjucer"];
    

    var exportObj;
    
    appState.callEventListeners("save", {});
    
    if(!exportObj.isObject())
        exportObj = appState.currentDialog->exportAsJSON();

    auto projectName = exportObj[mpid::Properties][mpid::ProjectName].toString();

    multipage::CodeGenerator cg(rootDir, projectName, exportObj);

    cg.company = exportObj[mpid::Properties][mpid::Company].toString();
    cg.version = exportObj[mpid::Properties][mpid::Version].toString();
    cg.hisePath = state["HisePath"].toString();
    cg.teamId = state["teamID"].toString();

    auto startCompile = !(bool)state["skipCompilation"];

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

Dialog* ProjectExporter::createDialog(State& state)
{
    DynamicObject::Ptr fullData = new DynamicObject();
    fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "ModalPopup", "Style": "#header\n{\n\n\tdisplay: flex;\n}\n", "UseViewport": true, "DialogWidth": 700, "DialogHeight": 400})"));
    fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Export Projucer Project", "Subtitle": "", "Image": "", "ProjectName": "ProjectExporter", "Company": "HISE", "Version": "1.0.0", "BinaryName": "", "UseGlobalAppData": false, "Icon": ""})"));
    using namespace factory;
    auto mp_ = new Dialog(var(fullData.get()), state, false);
    auto& mp = *mp_;
    auto& List_0 = mp.addPage<List>({
      { mpid::Style, "gap: 15px;" }
    });

    List_0.addChild<PersistentSettings>({
      { mpid::ID, "CompilerSettings" },
      { mpid::Filename, "compilerSettings" },
      { mpid::UseChildState, 1 },
      { mpid::Items, R"(HisePath: "")" },
      { mpid::UseProject, 0 },
      { mpid::ParseJSON, 0 }
    });

    List_0.addChild<FileSelector>({
      { mpid::Text, "HISE Path" },
      { mpid::ID, "HisePath" },
      { mpid::Required, 1 },
      { mpid::Help, "The path to the HISE source code repository folder (the root directory with the `hi_xxx` subdirectories)." },
      { mpid::Directory, 1 },
      { mpid::SaveFile, 0 },
      { mpid::NoLabel, 0 }
    });

    List_0.addChild<TextInput>({
      { mpid::Text, "Team ID" },
      { mpid::ID, "teamID" },
      { mpid::EmptyText, "Enter Team Development ID " },
      { mpid::Height, 80 },
      { mpid::Help, "The macOS Team Development ID for signing the compiled binary." },
      { mpid::Code, R"(// initialisation, will be called on page load
Console.print("init");

element.onValue = function(value)
{
    // Will be called whenever the value changes
    Console.print(value);
}
)" }
    });

    List_0.addChild<Spacer>({
    });

    auto& export_5 = List_0.addChild<LambdaTask>({
      { mpid::Text, "Export Progress" },
      { mpid::ID, "export" },
      { mpid::EventTrigger, "OnSubmit" },
      { mpid::Function, "exportProjucerProject" }
    });

    // TODO: add var exportProjucerProject(State::Job& t, const var& stateObject) to class
    export_5.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(ProjectExporter::exportProjucerProject));
    
    // Custom callback for page List_0
    List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    auto& List_6 = mp.addPage<List>({
    });

    List_6.addChild<MarkdownText>({
      { mpid::Text, "The project was created successfully. Do you want to launch the projucer to continue building the dialog binary?" }
    });

    List_6.addChild<Button>({
      { mpid::Text, "Open Projucer" },
      { mpid::ID, "openProjucer" }
    });

    List_6.addChild<Launch>({
      { mpid::Text, "$hisePath/tools/Projucer.exe" },
      { mpid::ID, "openProjucer" },
      { mpid::EventTrigger, "OnSubmit" },
      { mpid::Args, R"("$projectDirectory/$projectName.jucer")" }
    });

    // Custom callback for page List_6
    List_6.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise
