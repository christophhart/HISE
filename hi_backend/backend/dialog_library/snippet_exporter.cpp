// Put in the implementation definitions of every dialog here...


namespace hise {
namespace multipage {
namespace library {
using namespace juce;
var SnippetExporter::createMarkdownFile(State::Job& t, const var& state)
{
    // All variables:
    auto name = state["name"];
    auto author = state["author"];
    auto category = state["category"];
    auto project = state["project"];
    auto API = state["API"];
    auto hello = state["hello"];
    auto advanced = state["advanced"];
    auto framework = state["framework"];
    auto opengl = state["opengl"];
    auto snex = state["snex"];
    auto faust = state["faust"];
    auto sampler = state["sampler"];
    auto panel = state["panel"];
    auto description = state["description"];
    auto previewButton = state["previewButton"];
    auto preview = state["preview"];
    auto targetFile = state["targetFile"];
    auto createTask = state["createTask"];
    auto showFile = state["showFile"];
    
    // ADD CODE here...
    
    return var(""); // return a error
}
Dialog* SnippetExporter::createDialog(State& state)
{
    DynamicObject::Ptr fullData = new DynamicObject();
    fullData->setProperty(mpid::LayoutData, JSON::parse(R"({"StyleSheet": "ModalPopup", "Style": "#footer {\n\tflex-direction: row;\n}\n\n#prev\n{\n\tdisplay: flex;\n}\n\n#header\n{\n\tpadding: 10px 30px;\n}\n\n.tag-button button {\n\ttransform: none;\n\tbackground: #222;\n\tcolor: #999;\n\tborder-radius: 10%;\n\tmargin: 2px;\n\tpadding: 0px;\n}\n\n.tag-button button:hover {\n\tbackground: #262626;\n}\n\n.tag-button button:checked {\n\tbackground: #ddd;\n\tcolor: #222;\n\tborder-radius: 2px solid #222;\n\tmargin: 1px;\n}\n\n.tag-button button::before,\n.tag-button button::after {\n\tdisplay: none;\n}\n\n.tag-button label {\n\tdisplay: none;\t\n}\n\n.color-border\n{\n\tborder-radius: 50%;\n\t\n\tflex-grow: 0;\n\tgap: 0px;\n\tpadding: 2px;\n\twidth: 114px;\n\tmargin-right: 5px;\n\tmargin-bottom: 10px;\n}\n\n.color-border button\n{\n\tbackground: #282828;\n\n\tcolor: #bbb;\n\tfont-family: monospace;\n\tfont-size: 13px;\n\tborder-radius: 50%;\n\n}\n\n.color-border button:active\n{\n\ttransform: scale(98%);\n}\n\n.color-border button:checked\n{\n\tbackground: #fff;\n\t\n}\n\n.markdown-editor\n{\n\theight: 100px;\n}\n\n.markdown-editor input\n{\n\tpadding-top: 10px;\n\tfont-family: monospace;\n\tfont-size: 14px;\n}\n\n.markdown-editor input\n{\n\theight: 100%;\n\tvertical-align: top;\n}\n\n#previewButton\n{\n\twidth: 250px;\n}\n\n#previewButton label\n{\n\tdisplay: none;\n}\n\n#previewButton button\n{\n\tbackground: #222;\n\tborder-radius: 50%;\n\tcolor: white;\n\t\n\tcursor: pointer;\n}\n\n#previewButton button::before,\n#previewButton button::after\n{\n\tdisplay: none;\n}\n\n/** Now we skin the top progress bar */\n#total-progress\n{\n\tdisplay:flex;\n\n\tbox-shadow: none;\n\tborder: 0px;\n\tmargin: 0px;\n\tbackground: transparent;\n\tcolor: transparent;\n\theight: 24px;\n\twidth: 100%;\n\tfont-size: 12px;\n\tvertical-align: top;\n\ttext-align: right;\n}\n\n#total-progress::after:hover\n{\n    background: white;\n    transition: all 0.4s ease-in-out;\n}\n\n#total-progress::before\n{\n    position: absolute;\n    margin: 0px;\n    content: '';\n    width: 100%;\n    height: 4px;\n    top: 20px;\n    background: #181818;\n    border-radius: 2px;\n}\n\n#total-progress::after\n{\n    position: absolute;\n    left: 2px;\n    top: 21px;\n    \n    content: '';\n    width: var(--progress);\n    background: #ddd;\n    height: 2px;\n    max-width: calc(100% - 4px);\n    border-radius: 1px;\n    box-shadow: 0px 0px 3px rgba(255, 255, 255, 0.1);\n    \n}\n", "DialogWidth": 800, "DialogHeight": 600})"));
    fullData->setProperty(mpid::Properties, JSON::parse(R"({"Header": "Snippet Exporter", "Subtitle": "", "Image": "", "ProjectName": "SnippetExporter", "Company": "MyCompany", "Version": "1.0.0", "BinaryName": "My Binary", "UseGlobalAppData": false, "Icon": ""})"));
    using namespace factory;
    auto mp_ = new Dialog(var(fullData.get()), state, false);
    auto& mp = *mp_;
    auto& List_0 = mp.addPage<List>({
      { mpid::Style, "gap: 20px;" }
    });

    List_0.addChild<MarkdownText>({
      { mpid::Text, "You can use this dialog to create a snippet markdown file that can be uploaded to the official HISE snippet repository. Please fill in the metadata and then click OK in order to create the file." }
    });

    List_0.addChild<TextInput>({
      { mpid::Text, "Snippet Name" },
      { mpid::ID, "name" },
      { mpid::Style, "margin-top: 10px;" },
      { mpid::EmptyText, "Enter the snippet name" },
      { mpid::Required, 1 },
      { mpid::ParseArray, 1 },
      { mpid::Height, 80 },
      { mpid::Help, "The snippet name. This should be a descriptive title with less than 80 characters." }
    });

    List_0.addChild<TextInput>({
      { mpid::Text, "Author" },
      { mpid::ID, "author" },
      { mpid::EmptyText, "Enter your name" },
      { mpid::Required, 1 },
      { mpid::Height, 80 },
      { mpid::Help, "The author name (you) that will be shown in the snippet browser. At least try to be serious and pass on the opportunity to put in `HISEFucker2000`..." }
    });

    auto& Column_4 = List_0.addChild<Column>({
      { mpid::Style, "gap: 0px; margin-bottom: 20px;" }
    });

    Column_4.addChild<SimpleText>({
      { mpid::Text, "Category" },
      { mpid::Style, "min-width: 130px; text-align: left;" }
    });

    Column_4.addChild<Button>({
      { mpid::Text, "All" },
      { mpid::ID, "category" },
      { mpid::InitValue, "true" },
      { mpid::UseInitValue, 1 },
      { mpid::Class, ".tag-button" }
    });

    Column_4.addChild<Button>({
      { mpid::Text, "Modules" },
      { mpid::ID, "category" },
      { mpid::Class, ".tag-button" }
    });

    Column_4.addChild<Button>({
      { mpid::Text, "MIDI" },
      { mpid::ID, "category" },
      { mpid::Class, ".tag-button" }
    });

    Column_4.addChild<Button>({
      { mpid::Text, "Scripting" },
      { mpid::ID, "category" },
      { mpid::Class, ".tag-button" }
    });

    Column_4.addChild<Button>({
      { mpid::Text, "Scriptnode" },
      { mpid::ID, "category" },
      { mpid::Class, ".tag-button" }
    });

    Column_4.addChild<Button>({
      { mpid::Text, "UI" },
      { mpid::ID, "category" },
      { mpid::Class, ".tag-button" },
      { mpid::Help, "The category that will be used for the snippet. This is an exclusive property so please select the one that is most suitable for your snippet.  \n> The category will also define how the default appearance of the HISE snippet playground will look like (eg. if you're using the **UI** category it will show the front interface upon loading)." }
    });

    auto& Column_12 = List_0.addChild<Column>({
    });

    Column_12.addChild<SimpleText>({
      { mpid::Text, "Tags" },
      { mpid::Style, "min-width: 120px; text-align: left; width: 120px;" }
    });

    auto& Column_14 = Column_12.addChild<Column>({
    });

    auto& List_15 = Column_14.addChild<List>({
      { mpid::Style, "gap:0px;" }
    });

    auto& Column_16 = List_15.addChild<Column>({
      { mpid::Style, "gap:0px;" }
    });

    Column_16.addChild<Button>({
      { mpid::Text, "project" },
      { mpid::ID, "project" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #9e0142;" },
      { mpid::Help, "Use this tag for projects" }
    });

    Column_16.addChild<Button>({
      { mpid::Text, "API" },
      { mpid::ID, "API" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #d53e4f;" },
      { mpid::Help, "Use this tag when you demonstrate an API method" }
    });

    Column_16.addChild<Button>({
      { mpid::Text, "hello" },
      { mpid::ID, "hello" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #f46d43;" },
      { mpid::Help, "Use this tag for snippets that are a good \"first experience\" (simple hello world things)" }
    });

    Column_16.addChild<Button>({
      { mpid::Text, "advanced" },
      { mpid::ID, "advanced" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #fdae61" },
      { mpid::Help, "Use this tag for advanced examples that demonstrate a complex feature." }
    });

    Column_16.addChild<Button>({
      { mpid::Text, "framework" },
      { mpid::ID, "framework" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #fee08b" },
      { mpid::Help, "Use this tag when the snippet contains a script file that provides helper functions and can be easily imported to your project." }
    });

    auto& Column_22 = List_15.addChild<Column>({
      { mpid::Style, "gap:0px;" }
    });

    Column_22.addChild<Button>({
      { mpid::Text, "Open GL" },
      { mpid::ID, "opengl" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #e6f598" },
      { mpid::Help, "Use this tag when you demonstrate a OpenGL function (shaders etc)." }
    });

    Column_22.addChild<Button>({
      { mpid::Text, "SNEX" },
      { mpid::ID, "snex" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #abdda4" },
      { mpid::Help, "Use this tag when the snippet contains a SNEX class." }
    });

    Column_22.addChild<Button>({
      { mpid::Text, "Faust" },
      { mpid::ID, "faust" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background:  #66c2a5" },
      { mpid::Help, "Use this tag for snippets that contain a Faust example" }
    });

    Column_22.addChild<Button>({
      { mpid::Text, "Sampler" },
      { mpid::ID, "sampler" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #3288bd" },
      { mpid::Help, "Use this tag when demonstrating a sampler feature\n\n> Make sure to use the example assets when creating a sampler snippet so that the user can load it properly." }
    });

    Column_22.addChild<Button>({
      { mpid::Text, "Panel" },
      { mpid::ID, "panel" },
      { mpid::Class, ".tag-button .color-border" },
      { mpid::Style, "background: #5e4fa2" },
      { mpid::Help, "Use this tag when the snippet contains an example using a ScriptPanel." }
    });

    // Custom callback for page List_0
    List_0.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    auto& List_28 = mp.addPage<List>({
      { mpid::Style, "gap: 20px;" }
    });

    List_28.addChild<MarkdownText>({
      { mpid::Text, "Please enter the description for the snippet. It supports markdown & links so you can reference the documentation or other things." }
    });

    List_28.addChild<TextInput>({
      { mpid::Text, "Description" },
      { mpid::ID, "description" },
      { mpid::Code, "// initialisation, will be called on page load\nConsole.print(\"init\");\n\nelement.onValue = function(value)\n{\n    var x = document.getElementById(\"preview\")[0];\n    \n    Console.print(trace(x));\n    \n    \n\n    // Will be called whenever the value changes\n    Console.print(value);\n}\n" },
      { mpid::Class, ".markdown-editor" },
      { mpid::EmptyText, "Please enter the markdown description" },
      { mpid::Height, "200" },
      { mpid::Multiline, 1 },
      { mpid::Help, "The markdown description for the snippet. This information will be shown in the preset browser and can be used to add additional information / explanations about this snippet." },
      { mpid::UseOnValue, 1 }
    });

    auto& Column_31 = List_28.addChild<Column>({
    });

    Column_31.addChild<Button>({
      { mpid::Text, "Preview" },
      { mpid::ID, "previewButton" },
      { mpid::Code, "// initialisation, will be called on page load\nConsole.print(\"init\");\n\nelement.onValue = function(value)\n{\n    var text = state.description;\n    \n    if(text.length == 0)\n        text = \"Click on the left button to preview the markdown content...\";\n    \n\n    document.getElementById(\"preview\")[0].Text = text;\n    \n    document.updateElement(\"preview\");\n\n    // Will be called whenever the value changes\n    Console.print(state.description);\n}\n" },
      { mpid::Trigger, 1 },
      { mpid::UseOnValue, 1 }
    });

    auto& List_33 = Column_31.addChild<List>({
      { mpid::Style, "background: #222;padding: 20px; border: 1px solid #444;box-shadow: 0px 2px 5px rgba(0, 0, 0, 0.5);margin-top: 20px; border-radius: 5px;margin-right: 42px;margin-bottom: 10px;" }
    });

    List_33.addChild<MarkdownText>({
      { mpid::Text, "Click on the left button to preview the markdown content..." },
      { mpid::ID, "preview" },
      { mpid::Style, "width: 500px;" }
    });

    // Custom callback for page List_28
    List_28.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    auto& List_35 = mp.addPage<List>({
      { mpid::Style, "gap:20px;" }
    });

    List_35.addChild<MarkdownText>({
      { mpid::Text, "Please choose a file location that will be used to save your snippet.\n\n> You can either use your local snippet folder or the repository folder for automatically pushing new snippets." }
    });

    List_35.addChild<FileSelector>({
      { mpid::Text, "Target File" },
      { mpid::ID, "targetFile" },
      { mpid::SaveFile, 1 },
      { mpid::Wildcard, "*.md" },
      { mpid::Help, "The target file. This will create a markdown file with the properly formatted YAML metadata header so you can upload it to the repository." },
      { mpid::Directory, 0 }
    });

    auto& createTask_38 = List_35.addChild<LambdaTask>({
      { mpid::Text, "Creating File" },
      { mpid::ID, "createTask" },
      { mpid::CallOnNext, 1 },
      { mpid::Style, "display:none;" },
      { mpid::Function, "createMarkdownFile" }
    });

    // TODO: add var createMarkdownFile(State::Job& t, const var& stateObject) to class
    createTask_38.setLambdaAction(state, BIND_MEMBER_FUNCTION_2(SnippetExporter::createMarkdownFile));
    
    // Custom callback for page List_35
    List_35.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    auto& List_39 = mp.addPage<List>({
    });

    List_39.addChild<MarkdownText>({
      { mpid::Text, "The file was created and written to the location. Press Finish in order to close the dialog." }
    });

    List_39.addChild<Button>({
      { mpid::Text, "Show File" },
      { mpid::ID, "showFile" },
      { mpid::Help, "Opens the file in the Explorer / Finder" }
    });

    List_39.addChild<Launch>({
      { mpid::Text, "$targetFile" },
      { mpid::ID, "showFile" },
      { mpid::CallOnNext, 1 }
    });

    // Custom callback for page List_39
    List_39.setCustomCheckFunction([](Dialog::PageBase* b, const var& obj){

        return Result::ok();

    });
    
    return mp_;
}
} // namespace library
} // namespace multipage
} // namespace hise
