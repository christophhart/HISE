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

namespace hise {
using namespace juce;

namespace raw
{


/** This namespace duplicates some HISE enumerations.

The purpose of this is to have one concise place to look for commonly used enumerations along with a description of its values.
*/
namespace IDs
{

/** This namespace contains all major threads used by HISE and can be used with a TaskAfterSuspension object. */
namespace Threads
{

/** The audio thread. This might actually be more than one thread, but it is guaranteed to be a realtime thread. */
constexpr int Audio = (int)MainController::KillStateHandler::TargetThread::AudioThread;

/** The main message thread. */
constexpr int Message = (int)MainController::KillStateHandler::TargetThread::MessageThread;

/** This is the background worker thread that is used for all tasks that take a bit longer. It is also used by the streaming engine to
fetch new samples, so it has a almost real time priority.

Normally you will offload all tasks to this thread, it has a lock free queue that will be processed regularly.
*/
constexpr int Loading = (int)MainController::KillStateHandler::TargetThread::SampleLoadingThread;

/** The thread which executes all calls to the scripting engine - compilation, callback execution (with the exception of MIDI callbacks, which are
happening directly in the audio thread), and even quasi-UI related functions like panel repaints / timer callbacks.

It has an internal prioritisation (compilation > callbacks > UI tasks), so it makes sure that eg. multiple UI calls do not clog more important tasks.
*/
constexpr int Script = (int)MainController::KillStateHandler::TargetThread::ScriptingThread;
}

/** This namespace contains the most important chain indexes used by the Builder object to figure out where to put the processor. */
namespace Chains
{
/** the slot index for adding sound generators directly to containers / groups. */
constexpr int Direct = -1;

/** The slot index for MIDI Processors. */
constexpr int Midi = hise::ModulatorSynth::MidiProcessor;

/** The slot index for Gain modulators. */
constexpr int Gain = hise::ModulatorSynth::GainModulation;

/** the slot index for Pitch modulators. */
constexpr int Pitch = hise::ModulatorSynth::PitchModulation;

/** the slot index for FX. */
constexpr int FX = hise::ModulatorSynth::EffectChain;

/** the frequency modulation for filters. */
constexpr int FilterFrequency = hise::PolyFilterEffect::FrequencyChain;

/** the slot where modulators are put in a GlobalModulatorContainer. */
constexpr int GlobalModulatorSlot = hise::GlobalModulatorContainer::GainModulation;

}

namespace UIWidgets
{
constexpr int Slider = 0;

constexpr int Button = 1;

constexpr int ComboBox = 2;
}

}


}

} // namespace hise;