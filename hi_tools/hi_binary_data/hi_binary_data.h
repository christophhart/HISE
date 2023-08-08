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

#pragma once

#define DECLARE_DATA(id, size) extern const char* id; constexpr size_t id##_Size = size;
#define DEFINE_DATA(id, unused) const char* id = (const char*)data::id;

#define SIZE_OF_PATH(id) id##_Size

// Use this if you have declared the data array as char[] value with a proper size
#define LOAD_PATH_IF_URL(urlName, editorIconName) static_assert(sizeof(editorIconName) > sizeof(char*)); ids.addIfNotAlreadyThere(urlName); if (url == urlName) p.loadPathFromData(editorIconName, sizeof(editorIconName));

// Use this if you have declared the data array as extern data with a char* and the macro DECLARE_EXTERN_PATH_DATA
#define LOAD_EPATH_IF_URL(urlName, editorIconName) ids.addIfNotAlreadyThere(urlName); if (url == urlName) p.loadPathFromData(editorIconName, SIZE_OF_PATH(editorIconName));

#if !HISE_NO_GUI_TOOLS

#include "FrontendBinaryData.h"
#include "ProcessorEditorHeaderBinaryData.h"
#include "LookAndFeelBinaryData.h"
#include "BinaryDataDictionaries.h"


#if !USE_FRONTEND
#include "BackendBinaryData.h"
#endif


namespace ScriptnodeIcons
{
	DECLARE_DATA(mainLogo, 4537);
	DECLARE_DATA(zoomFit, 549);
	DECLARE_DATA(errorIcon, 318);
	DECLARE_DATA(fixIcon, 725);
	DECLARE_DATA(unscaledMod, 576);
	DECLARE_DATA(scaleIcon, 196);
	DECLARE_DATA(pinIcon, 1008);
	DECLARE_DATA(splitIcon, 422);
	DECLARE_DATA(chainIcon, 350);
	DECLARE_DATA(multiIcon, 460);
	DECLARE_DATA(modIcon, 657);
	DECLARE_DATA(frameIcon, 372);
	DECLARE_DATA(os2Icon, 345);
	DECLARE_DATA(os4Icon, 277);
	DECLARE_DATA(os8Icon, 570);
}


namespace HnodeIcons
{
	DECLARE_DATA(dllIcon, 297);
	DECLARE_DATA(testIcon, 442);
	DECLARE_DATA(exportIcon, 366);
	DECLARE_DATA(mapIcon, 142);
	DECLARE_DATA(loopIcon, 286);
	DECLARE_DATA(jit, 59);
	DECLARE_DATA(freezeIcon, 280);
	DECLARE_DATA(branchIcon, 1301);
	DECLARE_DATA(injectNodeIcon, 2319);
	DECLARE_DATA(settings, 964);
	DECLARE_DATA(sealPinsIcon, 317);
	DECLARE_DATA(connectCableIcon, 4415);
}

namespace ProcessorIcons
{
	DECLARE_DATA(midiIcon, 774);
	DECLARE_DATA(gainIcon, 115);
	DECLARE_DATA(pitchIcon, 532);
	DECLARE_DATA(fxIcon, 224);
	DECLARE_DATA(sampleStartIcon, 69);
	DECLARE_DATA(groupFadeIcon, 212);
	DECLARE_DATA(polyFX, 994);
	DECLARE_DATA(voiceStart, 411);
	DECLARE_DATA(timeVariant, 539);
	DECLARE_DATA(envelope, 1597);
	DECLARE_DATA(speaker, 667);
}

namespace MainToolbarIcons
{
	DECLARE_DATA(customPopup, 272);
	DECLARE_DATA(back, 1275);
	DECLARE_DATA(forward, 1275);
	DECLARE_DATA(presetBrowser, 213);
	DECLARE_DATA(macroControlTable, 568);
	DECLARE_DATA(web, 1863);
	DECLARE_DATA(home, 440);
	DECLARE_DATA(mainWorkspace, 142);
	DECLARE_DATA(samplerWorkspace, 383);
	DECLARE_DATA(settings, 964);
	DECLARE_DATA(help, 579);
	DECLARE_DATA(comment, 157);
	DECLARE_DATA(hise, 1219);
}

namespace ExpansionIcons
{
	DECLARE_DATA(encrypted, 773);
	DECLARE_DATA(filebased, 570);
	DECLARE_DATA(intermediate, 685);
}

namespace EditorIcons
{
	DECLARE_DATA(searchIcon2, 485);
	DECLARE_DATA(resizeIcon, 230);
	DECLARE_DATA(moveIcon, 196);
	DECLARE_DATA(bookIcon, 195);
	DECLARE_DATA(connectIcon, 3380);
	DECLARE_DATA(penShape, 183);
	DECLARE_DATA(lockShape, 393);
	DECLARE_DATA(urlIcon, 806);
	DECLARE_DATA(newFile, 319);
	DECLARE_DATA(openFile, 451);
	DECLARE_DATA(saveFile, 289);
	DECLARE_DATA(deleteIcon, 584);
	DECLARE_DATA(imageIcon, 616);
	DECLARE_DATA(tableIcon, 377);
	DECLARE_DATA(backIcon, 1275);
	DECLARE_DATA(forwardIcon, 1275);
	DECLARE_DATA(searchIcon, 349);
	DECLARE_DATA(dragIcon, 230);
	DECLARE_DATA(selectIcon, 122);
	DECLARE_DATA(sunIcon, 1606);
	DECLARE_DATA(nightIcon, 189);
	DECLARE_DATA(pasteIcon, 499);
	DECLARE_DATA(compileIcon, 107);
	DECLARE_DATA(cancelIcon, 113);
	DECLARE_DATA(swapIcon, 196);
	DECLARE_DATA(selectAll, 360);
	DECLARE_DATA(undoIcon, 125);
	DECLARE_DATA(redoIcon, 125);
	DECLARE_DATA(verticalAlign, 188);
	DECLARE_DATA(horizontalAlign, 188);
	DECLARE_DATA(xyAlign, 206);
	DECLARE_DATA(warningIcon, 1549);
};

namespace SampleToolbarIcons
{
	DECLARE_DATA(envelope, 458);
	DECLARE_DATA(zoomIn, 435);
	DECLARE_DATA(zoomOut, 354);
	DECLARE_DATA(normaliseOn, 587);
	DECLARE_DATA(toggleFirst, 1087);
	DECLARE_DATA(normaliseOff, 587);
	DECLARE_DATA(loopOff, 50);
	DECLARE_DATA(loopOn, 286);
	DECLARE_DATA(selectMidi, 495);
	DECLARE_DATA(selectMouse, 68);
	DECLARE_DATA(sampleStartArea, 1076);
	DECLARE_DATA(playArea, 679);
	DECLARE_DATA(loopArea, 351);
	DECLARE_DATA(more, 334);
	DECLARE_DATA(zero, 224);
	DECLARE_DATA(smooth_loop, 696);
	DECLARE_DATA(tabIcon, 123);
}

namespace LoopIcons
{
	DECLARE_DATA(apply, 202);
	DECLARE_DATA(find, 628);
	DECLARE_DATA(preview, 68);
}

namespace SampleMapIcons
{
	DECLARE_DATA(cutSamples, 938);
	DECLARE_DATA(copySamples, 371);
	DECLARE_DATA(pasteSamples, 499);
	DECLARE_DATA(autoPreview, 701);
	DECLARE_DATA(deleteSamples, 584);
	DECLARE_DATA(duplicateSamples, 600);
	DECLARE_DATA(newSampleMap, 319);
	DECLARE_DATA(loadSampleMap, 451);
	DECLARE_DATA(saveSampleMap, 289);
	DECLARE_DATA(zoomIn, 435);
	DECLARE_DATA(zoomOut, 354);
	DECLARE_DATA(fillNoteGaps, 95);
	DECLARE_DATA(fillVelocityGaps, 95);
	DECLARE_DATA(sfzImport, 4289);
	DECLARE_DATA(monolith, 187);
	DECLARE_DATA(refreshCrossfade, 78);
	DECLARE_DATA(selectAll, 6748);
	DECLARE_DATA(trimSampleStart, 132);
};

namespace WaveformIcons
{
	DECLARE_DATA(sine, 114);
	DECLARE_DATA(triangle, 50);
	DECLARE_DATA(saw, 50);
	DECLARE_DATA(square, 68);
	DECLARE_DATA(noise, 167);
}

namespace MpeIcons
{
	DECLARE_DATA(stroke, 104);
	DECLARE_DATA(press, 1458);
	DECLARE_DATA(glide, 150);
	DECLARE_DATA(lift, 104);
	DECLARE_DATA(slide, 150);
}

#endif