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
      { mpid::CallOnNext, 1 },
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


