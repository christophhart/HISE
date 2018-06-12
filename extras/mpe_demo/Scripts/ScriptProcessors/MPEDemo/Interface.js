Content.makeFrontInterface(1024, 768);
Synth.deferCallbacks(true);

const var ChannelLabel = Content.getComponent("ChannelLabel");


const var upperData = {
    "Type": "Keyboard",
  "ColourData": {
   "bgColour": "0xFF222222",
   "itemColour1": "0x88FFFFFF",
  },
  "KeyWidth": 14,
  "DisplayOctaveNumber": false,
  "LowKey": 48,
  "HiKey": 127,
  "CustomGraphics": false,
  "DefaultAppearance": true,
  "BlackKeyRatio": 0.7,
  "ToggleMode": false,
  "MidiChannel": 1,
  "MPEKeyboard": true,
  "MPEStartChannel": 2,
  "MPEEndChannel": 16
};


const var lowerData = {
  "Type": "Keyboard",
  
  "ColourData": {
   "bgColour": "0xFF222222",
   "itemColour1": "0x88FFFFFF",
  },
  
  
  "KeyWidth": 14,
  "DisplayOctaveNumber": false,
  "LowKey": 48,
  "HiKey": 127,
  "CustomGraphics": false,
  "DefaultAppearance": true,
  "BlackKeyRatio": 0.69999999,
  "ToggleMode": false,
  "MidiChannel": 1,
  "MPEKeyboard": true,
  "MPEStartChannel": 2,
  "MPEEndChannel": 16
};


inline function onUpperStartControl(component, value)
{
	upperData.MPEStartChannel = parseInt(component.getItemText());
	UpperKeyboard.setContentData(upperData);
};

Content.getComponent("UpperStart").setControlCallback(onUpperStartControl);

inline function onUpperEndControl(component, value)
{
	upperData.MPEEndChannel = parseInt(component.getItemText());
	UpperKeyboard.setContentData(upperData);
};

Content.getComponent("UpperEnd").setControlCallback(onUpperEndControl);

inline function onLowerStartControl(component, value)
{
    lowerData.MPEStartChannel = parseInt(component.getItemText());
	LowerKeyboard.setContentData(lowerData);
};

Content.getComponent("LowerStart").setControlCallback(onLowerStartControl);

inline function onLowerEndControl(component, value)
{
	lowerData.MPEEndChannel = parseInt(component.getItemText());
	LowerKeyboard.setContentData(lowerData);
};

Content.getComponent("LowerEnd").setControlCallback(onLowerEndControl);


const var UpperKeyboard = Content.getComponent("UpperKeyboard");
const var LowerKeyboard = Content.getComponent("LowerKeyboard");

UpperKeyboard.setContentData(upperData);
LowerKeyboard.setContentData(lowerData);

const var Settings = Content.getComponent("Settings");


const var settingsData = {
  "Type": "Tabs",
  "StyleData": {
  },
  "Font": "",
  "FontSize": 14,
  "LayoutData": {
    "ID": "anonymous",
    "Size": -0.33062551,
    "Folded": -1,
    "Visible": true,
    "ForceFoldButton": 0,
    "MinSize": -1
  },
  "ColourData": {
  },
  "Content": [
    {
      "Type": "CustomSettings",
      "StyleData": {
      },
      "Font": "",
      "FontSize": 14,
      "LayoutData": {
        "ID": "anonymous",
        "Size": -0.5,
        "Folded": 0,
        "Visible": true,
        "ForceFoldButton": 0,
        "MinSize": -1
      },
      "ColourData": {
      },
      "Driver": true,
      "Device": true,
      "Output": true,
      "BufferSize": true,
      "SampleRate": true,
      "GlobalBPM": true,
      "StreamingMode": false,
      "GraphicRendering": false,
      "ScaleFactor": false,
      "SustainCC": true,
      "VoiceAmountMultiplier": true,
      "ClearMidiCC": true,
      "SampleLocation": false,
      "DebugMode": true,
      "ScaleFactorList": [
        0.5,
        0.75,
        1,
        1.25,
        1.5,
        2
      ]
    },
    {
      "Type": "MidiSources",
      "StyleData": {
      },
      "Font": "",
      "FontSize": 14,
      "LayoutData": {
        "ID": "anonymous",
        "Size": -0.5,
        "Folded": 0,
        "Visible": true,
        "ForceFoldButton": 0,
        "MinSize": -1
      },
      "ColourData": {
      }
    }
  ],
  "CurrentTab": 0
};


Settings.setPopupData(settingsData, [60, 30, 500, 500]);function onNoteOn()
{
    ChannelLabel.set("text", "Channel " + Message.getChannel());
    


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
