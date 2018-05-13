Content.makeFrontInterface(1024, 768);

const var SampleMapLoader = Content.getComponent("SampleMapLoader");

const var ExpansionList = Content.getComponent("ExpansionList");

const var AudioLoopPlayer = Synth.getAudioSampleProcessor("Audio Loop Player");

const var AudioFileLoader = Content.getComponent("AudioFileLoader");


inline function onAudioFileLoaderControl(component, value)
{
	local text = AudioFileLoader.getItemText();
	
	if(text.indexOf("::") != -1)
    {
        local lastPart = text.split("::")[2];
        
        local ref = expHandler.getCurrentExpansion().getReferenceString(lastPart);
        Console.print(ref);
        AudioLoopPlayer.setFile(ref);
        
    }
};

Content.getComponent("AudioFileLoader").setControlCallback(onAudioFileLoaderControl);




const var S1 = Synth.getSampler("S1");

reg list2;

inline function refresh()
{
    local list = Sampler.getSampleMapList();

    SampleMapLoader.set("items", "");
    
    
    for(sampleMap in list)
        SampleMapLoader.addItem(sampleMap);
    
    local expList = expHandler.getExpansionList();
    
    local items = "No Expansion\n";
    
    for(e in expList)
    {
        items = items + e.Name + "\n";
    }
    
    ExpansionList.set("items", items.trim());
    
    local exp = expHandler.getCurrentExpansion();
    
    if(isDefined(exp))
    {
        list2 = exp.getSampleMapList();
        
        for(sampleMap in list2)
            SampleMapLoader.addItem(exp.Name + "::" + sampleMap);
        
        local list3 = exp.getAudioFileList();
        
        AudioFileLoader.set("items", "Empty");
        
        for(a in list3)
        {
            AudioFileLoader.addItem(exp.Name + "::" + a);
        }
        
        local bgName = exp.Name + "_bg";
            
        bgImage.loadImage(exp.getReferenceString("bg.png"), bgName);
        bgImage.setImage(bgName, 0, 0);
        
    }
    else
    {
        bgImage.loadImage("{PROJECT_FOLDER}bg.png", "bg");
        bgImage.setImage("bg", 0, 0);
        
    }
}


const var bgImage = Content.getComponent("bgImage");

bgImage.loadImage("{PROJECT_FOLDER}bg.png", "bg");
bgImage.setImage("bg", 0, 0);



inline function onSampleMapLoaderControl(component, value)
{
    local id = SampleMapLoader.getItemText();
    
    
    
    if(id.indexOf("::") != -1)
    {
        local lastPart = id.split("::")[2];
        
        
        id = expHandler.getCurrentExpansion().getReferenceString(lastPart);
        
    }
    
    Console.print("Loading Samplemap " + id);
    
	S1.loadSampleMap(id);
    
};

Content.getComponent("SampleMapLoader").setControlCallback(onSampleMapLoaderControl);

const var expHandler = Engine.getExpansionHandler();

refresh();

expHandler.setLoadingCallback(function(expansion)
{
	refresh();
	
});

const var ExpansionPopup = Content.getComponent("ExpansionPopup");



inline function onExpansionBrowserButtonControl(component, value)
{
	ExpansionPopup.showAsPopup(false);
};

Content.getComponent("ExpansionBrowserButton").setControlCallback(onExpansionBrowserButtonControl);


inline function onExpansionListControl(component, value)
{
	local name = value == 0 ? "" : expHandler.getExpansionList()[value-1].Name;
	Console.print(name);
	
    expHandler.loadExpansion(name);
	
};

Content.getComponent("ExpansionList").setControlCallback(onExpansionListControl);
function onNoteOn()
{
	
}
function onNoteOff()
{
	
}
function onController()
{
	
}
function onTimer()
{
	
}
function onControl(number, value)
{
	
}
