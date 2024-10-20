/*
 * This file is part of the HISE loris_library codebase (https://github.com/christophhart/loris-tools).
 * Copyright (c) 2023 Christoph Hart
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

// This file contains the public function API for calling into the dynamic library
// It's implemented as pure C API to avoid the usual C++ hassles

#pragma once

struct LorisLibrary
{

	/** Creates a new instance of a loris state that holds all analysed files. 

		While it's theoretically possible to use multiple instances, it's not recommended
		as there are a few functions (mainly logging) that rely on the internal loris
		logging functions which use a global state.

		Returns an opaque pointer that you need to pass into all other functions.

		Call destroyLorisState to deallocate it when you're done.
	*/
	static void* createLorisState();

	/** Deallocates the state provided by createLorisState(). */
	static void destroyLorisState(void* stateToDestroy);

	/** Returns the Library version to check for mismatches. */
	static const char* getLibraryVersion();

	/** Returns the Loris version. */
	static const char* getLorisVersion();


	/** Analyses the given file with the given root frequency. You need to call this method before processing and synthesizing.
	 
	    - state the state context created with createLorisState().
	    - file the full path name
	    - rootFrequency - the estimated frequency of the samples. This will be used for setting the frequency resolution and frequency drift
	 
	    Depending on the `enablecache` optiion, the analysed partials will be cached and reused when the function is called again
	    with the same filename.
	*/
	static bool loris_analyze(void* state, char* file, double rootFrequency);

	/** Processes the analyzed partials with a predefined function.
	 
	    - state: the state context pointer
	    - file: the full path name
	    - command: the function you want to execute.
	 
	    For an overview of the available functions and JSON configuration take a look at ScopedPartialList::process.
	*/
	static bool loris_process(void* state, const char* file,
	                              const char* command, const char* json);

	/** Processes the analyzed partials with a custom function. */
	static bool loris_process_custom(void* state, const char* file, void* obj, void* function);

	static bool loris_set(void* state, const char* setting, const char* value);

	static double loris_get(void* state, const char* setting);

	/** Returns the number of bytes that you need to allocate before calling loris_synthesize or loris_create_envelope. */
	static size_t getRequiredBytes(void* state, const char* file);

	/** Synthesises the partial list for the given file. 

		state - the state context
		file - the full path name
		dst - a preallocated buffer that will be written to as float array. Use getRequiredBytes
		      in order to obtain the size of the buffer.
		numChannels - will be set to the number of channels
		numSamples - will be set to the number of samples
	*/
	static bool loris_synthesize(void* state, const char* file, float* dst, int& numChannels, int& numSamples);

	/** Creates a audio-rate envelope for each channel of the given parameter (bandwidth, phase, frequency, amp) for the given file.
	 
	    If you've channelized the file, you can also pass in "parameter[idx]" to get the envelope for the given harmonic for the given index. */
	static bool loris_create_envelope(void* state, const char* file, const char* parameter, int label, float* dst, int& numChannels, int& numSamples);

	/** Creates a snapshot (=list of values for all partials) of the given parameter at the given time.

	    Useful for creating wavetables from the analysed data.
	*/
	static bool loris_snapshot(void* state, const char* file, double time, const char* parameter, double* buffer, int& numChannels, int& numHarmonics);

	/** Prepares an audio file for morphing. If removeUnlabeled is true, then all non-labeled partials will be removed. */
	static bool loris_prepare(void* state, const char* file, bool removeUnlabeled);

	static bool getLastMessage(void* state, char* buffer, int maxlen);

	static void getIdList(char* buffer, int maxlen, bool getOptions);

	static const char* getLastError(void* state);

	static void setThreadController(void* state, void* t);
	
}; // extern "C"
