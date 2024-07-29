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

namespace hise
{

namespace multipage {
using namespace juce;

namespace default_css {

static const char* GLOBAL = R"(
* {
    color: #ddd;
    
	/** Pickup the font from the global selector. */
    font-family: var(--Font);
 
    /** Pickup the font size from the global selector. */
    font-size: var(--FontSize);

    opacity: 1.0;
    color: var(--textColour);

	--triangle-icon: "66.t01PhrCQTd7bCwF..VDQTd7bCwF..ZBQzvgvCwF..d.QTd7bCwVccGAQTd7bCwF..ZBQEZepCw1PhrCQTd7bCMVY";
}

*:disabled
{
 opacity: 0.5;
}

/** Global properties (font, background, etc). */
body
{
    --global-padding: 10px;
    
}

div
{
 gap: 5px;
}

label
{
 text-align: left;
 min-width: 70px;
}

h1, h2, h3, h4
{
 font-size: 1.8rem;
}

#content
{
    background: #333;
}

#title
{
    font-size: 1.5em;
    font-weight: 500;
    
    /** Use the color from the global properties */
    color: var(--headlineColour);
}

::selection
{
 background: var(--headlineColour);
 color: black;
}
)";

static const char* POPUP_MENU = R"(
/** Popup-Menu styling */

/** This CSS class defines the background of the popup menu. */
.popup
{
 background: linear-gradient(to bottom, #222, #161616);
 border: 1px solid #444;
 border-radius: 3px;
}

/** This CSS class defines the default popup menu item style. */
.popup-item
{
 border: 0px solid transparent;
 font-size: 14px;
 font-family: Lato;
 background: transparent;
 color: #ddd;
 text-align: left;
 padding: 3px 15px;
 margin: 3px;
 font-weight: 400;
}

/** The popup menu item that is currently hovered. */
.popup-item:hover
{
 background: rgba(255,255,255, 0.05);
}

/** The currently ticked popup menu item */
.popup-item:active
{
 color: white;
 font-weight: 500;
}

/** A disabled popup menu item */
.popup-item:disabled
{
 color: #555;
}

/** This is the popup menu header (the pseudo element focus is used for this) */
.popup-item:focus
{
 font-weight: 500;
}

/** This is the triangle indicating a submenu. */
.popup-item::after:root
{
 content: '';
 margin: 5px;
 width: 100vh;
 background: #888;
 background-image: var(--triangle-icon);
 transform: rotate(270deg);
}

/** Set the default after pseudo element to zero width */
.popup-item::after
{
 width: 0px;
}

/** Styling the popup menu separator. */
hr
{
 border: 1px solid #555;
})";


static const char* HELP = R"(
.error
{
    padding: 5px;
    border: 1px solid #e44;
    border-radius: 4px;
    background: rgba(255, 0, 0, 0.05);
    padding-right: 40px;
}

.error::after
{
    content: '';
    background: #e44;
    width: 20px;
    height: 20px;
    top: calc(50% - 10px);
    right: 10px;
    background-image: "230.t0F6+YBQ9++OCIl9adCQ9++OCA.fEQD6Od2P..XQDc8+cNjX..XQDM+M.Oj9adCQ...2Cw9elPD..v8PhI.YUPD..v8P..3ADM+M.OD..d.QW+emCIF..d.Qr+3cCI.YUPj+++yPr+mID4+++LzXsIBplPzgiO4Prg84VPDtEi1PrQcVRPjl8q2PrAiFhPjR+y4PrQcVRPjT.x6Prg84VPzMbV7PrIBplPznaX5Pr0FZ1PzMbV7PrAl85PjT.x6PrUgMqPjR+y4PrAl85Pjl8q2Pr0FZ1PDtEi1PrIBplPzgiO4PiUF";
    box-shadow: 0px 0px 5px black;
    
}

.help-button, .retry-button
{
 order: 1000;
 height: 20px;
 width: 24px;
 background: #999;
}

.help-button:hover, .retry-button:hover
{
 background: #aaa;
}

.help-button:checked, .retry-button:checked
{
 background: #bbb;
}


.stop-button
{
 order: 1000;
 height: 24px;
 width: 24px;
}

.help-popup
{
 display: flex;
 height: auto;
 background: #161616;
 margin: 10px;
 padding: 15px;
 margin-top: 15px;
 border-radius: 5px;
 border: 1px solid #353535;
 box-sizing: border-box;
 box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.2);
}

.help-popup::before
{
 background: #161616;
 background-image: "39.t0F++YBQfAfeCwF..VDQR+OuCwF..d.QR+OuCwF++YBQfAfeCMVY";
 content: '';
 width: 20px;
 height: 12px;
 position: absolute;
 top: -10px;
 left: calc(50% - 10px);
}

.help-text
{
	width: calc(100% - 18px);
	height: auto;
	max-height: 300px;
}

.help-close
{
 position: absolute;
 width: 18px;
 height: 18px;
 right: 0px;
 top: 0px;
}

.modal-bg
{
 position: absolute;
 background: rgba(25,25,25, 0.8);
}

.modal-popup
{
 border: 1px solid #555;
 border-radius: 3px;
 box-shadow: 0px 2px 4px black;
}



)";

static const char* FOLD_BAR = R"(
/** Styling of the fold bar (the clickable area of a list that
    hides its children if `Foldable` is enabled)
    
    The element is a button so we need to override anything that
    is defined in the default button class!
*/

.fold-bar
{
 margin: 0px;
 margin-bottom: 10px;
 width: 100%;
 height: 34px;
 font-weight: 500;
 background: #202020;
 border-radius: 5px 5px 0px 0px;
 border: 0px;
 color: #ccc;
}

.fold-bar:checked
{
 background: #202020;
 border-radius: 5px;

}

.fold-bar:hover
{
 background: #242424;
}

.fold-bar::before
{
 /** required so that the element shows up */
 content: '';
 position: absolute;
 width: 100vh;
 background-color: #555;
 background-image: var(--triangle-icon);
 margin: 6px;
 top: 3px;
 height: 20px;
 transform: none;
}

.fold-bar::before:hover
{
 background-color: #999;
}

.fold-bar::before:checked
{
 transform: rotate(-90deg);
 transition: transform 0.2s ease-in;
}

.fold-bar::after
{
 display:none;
}
)";

static const char* PROGRESS = R"(
/** This is the appearance of all progress bars
    that indicate a background process. */
progress
{
 margin: 4px;
 height: 36px;
 background: #444;
 border-radius: 50%;
 border: 1px solid #999;
 box-shadow: 0px 1px 3px rgba(0, 0, 0, 0.5);
 color: white;
 font-size: 14px;
 font-weight: 500;
}

progress::before
{
 content: '';
 height: 100%;
 border-radius: 50%;
 margin: 4px;
 position: absolute;
 background: linear-gradient(to bottom, #999, #888);
 width: var(--progress);
}

/** Now we skin the top progress bar */
#total-progress
{
 box-shadow: none;
 border: 0px;
 margin: 0px;
 background: transparent;
 color: #777;
 height: 24px;
 width: 100%;
 font-size: 14px;
 vertical-align: top;
 text-align: right;
}

#total-progress::after:hover
{
 background: white;
 transition: all 0.4s ease-in-out;
}

#total-progress::before
{
 position: absolute;
 margin: 0px;
 content: '';
 width: 100%;
 height: 4px;
 top: 20px;
 background: #181818;
 border-radius: 2px;
}

#total-progress::after
{
 position: absolute;
 left: 2px;
 top: 21px;
 
 content: '';
 width: var(--progress);
 background: #ddd;
 height: 2px;
 max-width: calc(100% - 4px);
 border-radius: 1px;
 box-shadow: 0px 0px 3px rgba(255, 255, 255, 0.1);
 
}

)";

static const char* TABLE = R"(
th
{
    border-radius: 0px;
    background: linear-gradient(to bottom, #282828, #242424);
    font-size: 1.2em;
    font-weight: 500;
    padding: 8px 10px;
    text-align: left;
    margin-right: 1px;
}

th:first-child
{
    border-radius: 8px 0px 0px 0px;
}
th:last-child
{
    margin-right: 0px;
    border-radius: 0px 8px 0px 0px;
}

tr
{
    background: transparent;
}

tr:hover
{
    background: #383838;
}

tr:focus,
tr:checked
{
    background: #414141;
}

td
{
    border: 1px solid #282828;
    border-top: 0px;
    border-left: 0px;
    color: #ccc;
    font-weight: 400;
    text-align: left;
    padding: 7px 10px;
}

td:first-child
{
    border-left: 1px;
}

td:checked
{
    color: #fff;
    font-weight: 500;
}
)";

static const char* TAG_BUTTON = R"(
.tag-button
{
    color: #bbb;
    font-size: 0.95em;
    padding: 5px 10px;
    width: auto;
    border-radius: 50%;
    margin: 5px;
    box-shadow: 0px 2px 3px rgba(0, 0, 0, 0.5);
    border: 1px solid #666;
}

.tag-button:checked
{
    background: #bbb;
    border: 1px solid #ddd;
    color: #222;
}

.tag-list
{
    gap: 0px;
}
)";

static const char* propertyCSS = R"(

body {
    font-size: 14px;
}

#header,
#footer
{
 display: none;
}

#content
{
 padding: 5px;
 background: #222;
}

input, select
{
 background: #999;
 border-radius: 3px;
 border: 1px solid #aaa;
 margin: 2px;
 color: #111;
 text-align: left;
 padding-left: 8px;
 padding-right: 8px;
 padding-top: 3px;
}

input:focus
{
 border: 2px solid;
 
 /** Getting a variable doesn't work in a multiproperty line
     so we need to set the border-color property manually. */
 border-color: var(--headlineColour);
}

select::after
{
 content: '';
 background: #333;
 width: 100vh;
 background-image: var(--triangle-icon);
 margin: 8px;
}

select:hover
{
 color: #222;
}

select::after:hover
{
 background: #555;
}

button
{
 background: #282828;
 color: transparent;
 width: 32px;
 margin: 0px;
 box-shadow: none;
 border: 0px;
}

button:hover
{
 background-color: #282828;
}

button::before
{
 position: absolute;
 content: '';
 width: 45px;
 margin: 6px;
 left: 0px;
 border-radius: 50%;
 border: 2px solid #ccc;
 background: transparent;
 box-shadow: 0px 3px 8px rgba(0, 0, 0, 0.3);
}

button::before:hover
{
 border: 2px solid white;
 transition: background 0.2s;
 background: rgba(255, 255, 255, 0.1);
 transform: scale(104%);
}

button::before:active,
button::before:active:checked
{
 transform: scale(99%);
}

button::before:checked
{
 transform: scale(99%);
 background: var(--headlineColour);
 box-shadow: inset 0px 2px 8px black;
}

button::after
{
 position: absolute;
 content: '';
 left: 0px;
 width: 100vh;
 margin: 10px;
 border-radius: 50%;
 background: #ccc;
}

button::after:checked
{
 background: white;

 left: 13px;
 transition: left 0.2s;
}
)";

static const char* darkCSS = R"(

/** Global properties (font, background, etc). */
body
{
 background: #333;
 
 /** This is used for all global containers to get a consistent padding. */
 --global-padding: 30px;
}

#header
{
 background-color: #282828;
 height: auto;
 padding: var(--global-padding);

 display: flex;
 flex-direction: column;
 
 /** aligns to the left */
 align-items: flex-start;
 
 transform: none;
 /** create a shadow */
 box-shadow: inset 0px 0px 5px rgba(0, 0, 0, 0.7);
}

#content
{
 padding: var(--global-padding);
 border-top: 1px solid #444;
 
}

#title
{
 font-size: 2.0em;
 font-weight: 500;
 padding-bottom: 5px;
 
 /** Use the color from the global properties */
 color: var(--headlineColour);
}

#footer
{
 gap: 5px;
 padding: var(--global-padding);
 height: auto;
 margin: 0px;
 
 background: #222;
 box-shadow: inset 0px 0px 5px rgba(0, 0, 0, 0.5);
}

button
{
 padding: 10px 20px;
 background: #444;
 border-radius: 3px;
 margin: 2px;
 border: 1px solid #555;
 box-shadow: 0px 2px 3px rgba(0, 0, 0, 0.2);
}

button:hover
{
 background: #555;
 transition: all 0.1s ease-in-out;
}

button:active
{
 box-shadow: none;
 transform: translate(0px, 1px);
 
}

input, select
{
 height: 40px;
 background: #999;
 border-radius: 3px;
 border: 1px solid #aaa;
 margin: 2px;
 color: #111;
 text-align: left;
 padding-left: 8px;
 padding-right: 8px;
 padding-top: 3px;
}

input:focus
{
 border: 2px solid;
 
 /** Getting a variable doesn't work in a multiproperty line
     so we need to set the border-color property manually. */
 border-color: var(--headlineColour);
}

select::after
{
 content: '';
 background: #333;
 width: 100vh;
 background-image: var(--triangle-icon);
 margin: 10px;
}

select:hover
{
 color: #333;
}

select::after:hover
{
 background: #555;
}

.toggle-button
{
 background: #282828;
 color: transparent;
 width: auto;
 margin: 0px;
 box-shadow: none;
 border: 0px;
 text-align: left;
 padding-left: 10px;
 
}

.toggle-button:hover
{
 background-color: #282828;
}

.toggle-button:checked
{
 
}

.toggle-button::before
{
 position: initial;
 content: '';
 width: 32px;
 margin: 6px;
 left: 0px;
 border-radius: 5px;
 border: 2px solid #ccc;
 background: transparent;
 box-shadow: 0px 3px 8px rgba(0, 0, 0, 0.3);
}

.toggle-button::before:hover
{
 border: 2px solid white;
 transition: background 0.5s;
 background: rgba(255, 255, 255, 0.1);
 transform: scale(104%);
}

.toggle-button::before:active
{
 transform: scale(99%);
}

.toggle-button::after
{
 position: absolute;

 content: '';
 left: 0px;
 width: 100vh;
 margin: 10px;
 border-radius: 2px;
 background: transparent;
}

.toggle-button::after
{
 background: transparent;
}

.toggle-button::after:checked
{
 background: #ccc;
 
}

.no-label
{
 color: white; 
 width: 100%;
}

)";

static const char* brightCSS = R"(

*
{
	color: #333;
}

body
{
	background-color: #cccccf;
}

#header
{
	background-color: #aaa;
}

#content
{
	border-top: 0px;
	background-color: transparent;
	padding: 30px 100px;
}

#footer
{
	background-color: #aaa;
}

button
{
	background: #bbb;
	border: 1px solid #999;
}

button:hover
{
	background: #ccc;
}

.nav-button
{
	background-color: #bbb;
	border: 1px solid #888;
	cursor: pointer;
}

.nav-button:hover
{
	background-color: #eee;
	transition: background-color 0.1s ease-in-out;
}


.text-button:checked
{
	background: #ddd;
}

.text-button: hover
{
	background-color: #ccc;
	border: 1px solid #999;
}

.toggle-button
{
	margin-left: 2px;
	margin-right: 2px;
	background: rgba(0, 0, 0, 0.1);
}

.toggle-button:hover
{
	background: rgba(0, 0, 0, 0.15);
}

.toggle-button::before
{
	box-shadow: unset;
	border-color: #444;
}

.toggle-button::before:hover
{
	border-color: #555;
}




.toggle-button::after:checked
{
	background: #444;
}

input, select
{
	background: rgba(0, 0, 0, 0.1);
}

input:focus
{
	border-color: #eee;
}

.popup
{
	background: #ddd;
	border-color: #888;
}

.popup-item,
.popup-item:active
{
	color: #333;
}

::selection
{
 background: var(--headlineColour);
 color: #ddd;
}

.tag-button
{
	background: #666;
	border-color: #555;
	
}

.tag-button:hover
{
	background: #777;
	
}

.tag-button:checked
{
	background: #fff;
	border-color: #333;
}

.help-button,
.stop-button,
.retry-button
{
	background-color: #444;
}

.error
{
	background: rgba(255, 0, 0, 0.2);
}

progress
{
	background: #bbb;
	color: #333;
	box-shadow: unset;
}

progress::before
{
	background: #ddd;
	margin: 3px;
	color: blue;
}

progress::after
{
	background: #eee;
}


#total-progress
{
	color: #333;
}

#total-progress::before
{
	background: #888;
}

#total-progress::after
{
	background: #fff;
}

.fold-bar,
.fold-bar:checked,
.fold-bar:hover
{
	background: #aaa;
	border: 1px solid #999;
	color: #333;
}

.help-popup
{
 background: #888;
 border-color: #777;
}

.help-popup::before
{
 background: #888;
}

.modal-bg
{
 background: rgba(200, 200, 200, 0.8);
}

.modal-popup
{
	background: #aaa;
 	border: 1px solid #888;
 	box-shadow: unset; 	
}

)";

#if 0
static const char* brightCSS = R"(

/** Global properties (font, background, etc). */
body
{
 background: #999;
 
 /** This is used for all global containers to get a consistent padding. */
 --global-padding: 30px;
}

#header
{
 background: linear-gradient(to bottom, #ddd, #bbb);
 box-shadow: inset 0px -2px 10px rgba(0, 0, 0, 0.2);
 height: auto;
 display: flex;
 flex-direction: column;
 align-items: flex-start;
 padding: var(--global-padding);
 border-bottom: 1px solid #777;

}

#title
{
 font-size: 2.0em;
 font-weight: 500;
 padding-bottom: 5px;
 
 /** Use the color from the global properties */
 color: var(--headlineColour);
}

#content
{
 padding: 30px;
}

#footer
{
 background: #222;
 box-shadow: inset 0px 2px 3px black;
 gap: 10px;
 padding: 20px;
}

button
{
 font-size: 16px;

 background: #555;
 padding: 5px 10px;
 border: 1px solid #666;
 margin: 4px;
 box-shadow: 0px 1px 3px rgba(0, 0, 0, 0.4);
 border-radius: 3px;
 color: #aaa;
}

button:hover
{
 background: #666;
}

.toggle-button
{
 background: transparent;
 box-shadow: none;
 border: 0px;
 color: transparent;
 text-align: left;
 padding-left: 52px;
 height: 40px;
}

.toggle-button:hover
{
 background: rgba(0, 0, 0, 0.05);
 border-radius: 50%;
 transition: background 0.1s;
 
}

.toggle-button::after
{
 content: '';
 width: 30px;
 height: 30px;
 background: linear-gradient(to bottom, #ddd, #bbb);
 
 left: 0px;
 margin:7px;
 border-radius: 50%;
}

.toggle-button::after:checked
{
 content: '';
 width: 30px;
 height: 30px;
 
 left: 20px;
 margin:7px;
 border-radius: 50%;
 transition: left 0.2s ease-in-out;
}

.toggle-button::before
{
 position: absolute;
 box-shadow: inset 0px 2px 4px rgba(0, 0, 0, 0.2);
 border: 2px solid rgba(0, 0, 0, 0.2);
 content: '';
 width: 50px;
 height: 30px;
 left: 0px;
 background: #888;
 margin: 5px;
 border-radius: 50%;
}

.toggle-button::before:checked
{
 background: var(--headlineColour);
}

input
{
 box-shadow: inset 0px 2px 4px rgba(0, 0, 0, 0.2);
 border: 2px solid rgba(0, 0, 0, 0.2);
 content: '';
 height: 40px;
 left: 0px;
 background: #aaa;
 margin: 5px;
 padding-top: 0px;
 padding-left: 10px;
 padding-right: 10px;
 border-radius: 5px;
}

input:focus
{
 border: 3px solid #4C6F8E;
 border-color: var(--headlineColour);
 background: #ddd;
 transition: background 0.4s;
}

)";
#endif

static const char* rawHTML = R"(
*
{
   color: black;
}

#content
{
	background: transparent;

}

#header,
#footer
{
	display: none;
}

body
{
	background: white;
})";

static const char* modalPopup = R"(
/** Global properties (font, background, etc). */
body
{
    border: 1px solid #555;
   
    background: #333;
    
    /** This is used for all global containers to get a consistent padding. */
    --global-padding: 30px;
}

#header
{
	display: flex;
    background-color: #282828;
    height: auto;
    padding: 20px;
    margin: 1px;

    flex-direction: column;
    align-items: center;
    transform: none;
    /** create a shadow */
    box-shadow: inset 0px 0px 5px rgba(0, 0, 0, 0.7);
}

#content
{
    padding: var(--global-padding);
    border-top: 1px solid #444;
}

#subtitle
{
    display: none;
}

#footer
{
    gap: 5px;
    padding: 20px;
    height: auto;
    margin: 1px;
	display: flex;
    flex-direction: row;
    background: #222;
    box-shadow: inset 0px 0px 5px rgba(0, 0, 0, 0.5);
}

button
{
    padding: 10px 20px;
    background: #444;
    border-radius: 3px;
    margin: 2px;
    border: 1px solid #555;
    box-shadow: 0px 2px 3px rgba(0, 0, 0, 0.2);
}

button:hover
{
    background: #555;
    transition: all 0.1s ease-in-out;
}

button:active
{
    box-shadow: none;
    transform: translate(0px, 1px);
}

input, select
{
    height: 40px;
    background: #999;
    border-radius: 3px;
    border: 1px solid #aaa;
    margin: 2px;
    color: #111;
    text-align: left;
    padding-left: 8px;
	padding-right: 8px;
    padding-top: 3px;
}

input:focus
{
    border: 2px solid;
    
    /** Getting a variable doesn't work in a multiproperty line
        so we need to set the border-color property manually. */
    border-color: var(--headlineColour);
}

select::after
{
    content: '';
    background: #333;
    width: 100vh;
    background-image: var(--triangle-icon);
    margin: 10px;
}

select:hover
{
    color: #333;
}

select::after:hover
{
    background: #555;
}

.toggle-button
{
    background: #282828;
    color: transparent;
    width: 32px;
    margin: 0px;
    box-shadow: none;
    border: 0px;
}

.toggle-button:hover
{
    background-color: #282828;
}

.toggle-button:checked
{
    
}

.toggle-button::before
{
    position: absolute;
    content: '';
    width: 32px;
    margin: 6px;
    right: 0px;
    border-radius: 5px;
    border: 2px solid #ccc;
    background: transparent;
    box-shadow: 0px 3px 8px rgba(0, 0, 0, 0.3);
}

.toggle-button::before:hover
{
    border: 2px solid white;
    transition: background 0.5s;
    background: rgba(255, 255, 255, 0.1);
    transform: scale(104%);
}

.toggle-button::before:active
{
    transform: scale(99%);
}

.toggle-button::after
{
    position: absolute;

    content: '';
    right: 0px;
    width: 100vh;
    margin: 10px;
    border-radius: 2px;
    background: transparent;
}

.toggle-button::after
{
    background: transparent;
}

.toggle-button::after:checked
{
    background: #ccc;
    
}

.no-label
{
 color: white;
 width: 100%;
}

)";

} // default_css

String DefaultCSSFactory::getTemplate(Template t)
{
    String s;
    
    s << default_css::GLOBAL;


	switch(t)
	{
	case Template::None:
        return s;
	case Template::PropertyEditor: 
        s << default_css::propertyCSS;
        break;
	case Template::RawHTML:
        s << default_css::rawHTML;
        break;
	case Template::Dark:
        s << default_css::darkCSS;
        break;
	case Template::Bright:
        s << default_css::darkCSS;
        
        break;
    case Template::ModalPopup:
        s << default_css::modalPopup;
        break;
	case Template::numTemplates: break;
	default: ;
	}

    
    s << default_css::POPUP_MENU;
    s << default_css::TABLE;
    s << default_css::HELP;
    s << default_css::PROGRESS;
    s << default_css::FOLD_BAR;
    s << default_css::TAG_BUTTON;

    // overwrite everything with bright colours
    if(t == Template::Bright)
        s << default_css::brightCSS; 
    
    
	return s;
}

simple_css::StyleSheet::Collection DefaultCSSFactory::getTemplateCollection(Template t, const String& additionalStyle)
{
	using namespace simple_css;

    auto code = getTemplate(t);
	code << additionalStyle;

	Parser p(code);
	
	p.parse();
	return p.getCSSValues();
}

}
}
