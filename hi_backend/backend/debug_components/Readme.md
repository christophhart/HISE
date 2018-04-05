# How to add documentation to a method

The extended documentation is stored in the file `ExtendedApiDocumentation.cpp` in this directory.

This file uses macros to define a method which reduce the amount of boilerplate text to a minimum. Every method documentation can have a selection of predefined blocks which are added using one of these markup tags:

- `METHOD`: Ths method name. This must be the exact name of the method (obviously).
- `DESCRIPTION`: a extended Description of the method. You can use multiple paragraphs including bulletpoint lists, quote blocks and basic Markdown syntax.

- `CODE`: A line of code that will be added to the example code block. You can call this multiple times.
- `PARAMETER`: defines a parameter. Must have three parameters that define the type, the name and the description. You can use one of these C++ types directly: (`int`, `double`, `String`) as well as `Array` and `Object`.
- `RETURN`: defines the return type with the type and the description. The description can have Markdown syntax, although it will be embedded into a table so you can't use headlines or paragraphs.

### Example

This example shows a full definition of the method `Engine.getSamplesForMilliSeconds()`:

```cpp
CLASS(Engine);

METHOD(getSamplesForMilliSeconds);
DESCRIPTION("Converts milli seconds to samples.");
DESCRIPTION("This uses the current sample rate so the result may vary depending on your audio settings.");
PARAMETER(double, "milliSeconds", "The time in **millisconds**");

CODE("// returns 44100.0");
CODE("const var time = Engine.getSamplesForMilliSeconds(1000.0);");

RETURN(double, "The time in samples");
```

## How to contribute

You can help extending the API by writing the documentation of a method and post it in this forum topic:

[API Forum Topic](https://forum.hise.audio/topic/653/call-for-documentation/)

Or you could make a pull request with the changes to the file `ExtendedApiDocumentation.cpp`. Posting it in the HISE forum is the recommended way though - I am kind of lazy when it comes to pull requests :)

## Style guide

If you add a documentation, please stick to the following style guide to avoid inconsistencies:

- Try to describe the whole functionality of the method, including pre-conditions to the parameters.
- The code examples should be as short as possible but demonstrate the usage of the function. Results can be added as comments as shown above. If there are best practices in how to use this function, make sure you mention it here.
- not every parameter needs to be documented. Please try to avoid redudancy, so if the parameter name is self-explanatory there is no need to duplicate it using a sentence. The example above violates that rule (`milliSeconds` should be very clear), but it's there for demonstrating purposes
- same with the return type. 
- Since the parameter description is shown in a table cell, restrict the formatting to **bold** and `code` only.

## Supported Markdown features

Every text will be parsed using a Markdown parser which supports the most basic features:

| Markup | Syntax | Description |
| --- | --- | ---- |
| **Bold** | `**Bold**` | Uses the bold font |
| `Code` | ``` `Code` ``` | Use a monospace font 
| Headlines* | `#` / `##` / `###` | This line will be treated as headline. The more `#` you use the smaller is the headline (up to `####` is supported)
| List* | `-` | this line will be treated as item of a bullet point list
| Quoted Block* | `>` | This line will be formatted as quoted block.


### Markdown Syntax

### Bold

`This is some text. This is **bold**.`

This is some text. This is **bold**.

#### Headlines

 This line will be treated as headline. The more `#` you use the smaller is the headline (up to `####` is supported

`##### This is a Headline`

##### This is a headline

#### Quoted Block

`> This is a quoted **block**`

> This is a quoted **block**

---

#### List


`- This is a list`
`- another item`

- This is a list
- another item

---

#### Code Block

Use the `CODE` markup tag for the default code examples. If you want to add additional code, use this syntax:

	```javascript
	var x = 5;
	```

```javascript
var x = 5;
```



