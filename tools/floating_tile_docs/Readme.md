@mainpage

## Introduction

The *FloatingTile* is a widget in HISE that can be populated with all kinds of ready made components and enhance the customizability of the user interfaces built with HISE. Basically there are two ways of using this system within a scripted interface:

1. Add a "permament" FloatingTile widget directly on the interface
2. Add a popup to a ScriptPanel which contains the FloatingTile

So which option you choose is dependent on your UX design. The procedure for both approaches is similar:

1. Create a JSON object that describes the FloatingTile's content.
2. Pass it to the FloatingTile or the ScriptPanel.

### Example usage

```{.js}

Content.makeFrontInterface(600, 500);

// The JSON data for a simple virtual Keyboard
const var data = {"Type": "Keyboard"};

// Create a FloatingTile widget
const var FloatingTile = Content.addFloatingTile("FloatingTile", 0, 0);

// Set its dimension to match the keyboard
FloatingTile.set("width", 200);
FloatingTile.set("height", 72);

// Pass the JSON data to the widget and it will show the keyboard.
FloatingTile.setContentData(data);



// Create a ScriptPanel
const var Panel = Content.addPanel("Panel", 230, 6);

// Set the callback level to anything above `No callbacks` in order to receive mouse clicks
Panel.set("allowCallbacks", "Context Menu");

// Pass the JSON to the panel and it will open a popup when clicked
// The second argument is the position and size of the popup: [x-pos, y-pos, width, height]
Panel.setPopupData(data, [50, 50, 200, 72]);

```

You should now see the keyboard and an empty ScriptPanel. Clicking on the ScriptPanel opens a popup with another keyboard:

![](http://hise.audio/images/floating_tile_gifs/floatingTileDemo.gif)

> The popup will not work if you preview the interface in a popup itself (you can't open nested popups in HISE). Use **Tools -> Add Interface Preview** instead or use the interface designer in the **Scripting Workspace**.

This example shows a very basic use case, but you can customize almost every widget by defining special properties in the JSON. You can even combine multiple components and build complex arrangements using the containers available (HorizontalTile, VerticalTile and Tabs).

If you don't define properties, it will use the default values so you don't have to spell out everything. There are a few properties which are common to all components, which are listed in the base class description:

The FloatingTile content base class.

