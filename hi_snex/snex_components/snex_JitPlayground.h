/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/



#pragma once

#if USE_BACKEND || SNEX_STANDALONE_PLAYGROUND

namespace snex {
namespace jit {
using namespace juce;

namespace SnexIcons
{
	static const unsigned char bugIcon[] = { 110,109,197,160,17,66,82,88,16,67,108,84,227,138,65,12,66,10,67,108,0,0,0,0,166,155,143,66,108,16,88,180,65,123,212,131,66,98,0,0,218,65,47,221,167,66,240,167,255,65,227,229,203,66,240,167,18,66,152,238,239,66,108,195,117,81,66,207,247,249,66,108,20,
	238,40,67,207,247,249,66,108,203,161,56,67,152,238,239,66,108,197,192,70,67,123,212,131,66,98,90,68,78,67,137,193,135,66,240,199,85,67,152,174,139,66,199,75,93,67,166,155,143,66,108,27,239,75,67,12,66,10,67,108,10,55,54,67,242,50,17,67,98,248,51,54,67,
	145,173,28,67,170,49,53,67,109,199,39,67,176,82,51,67,49,72,50,67,108,27,239,75,67,162,37,58,67,108,199,75,93,67,219,153,124,67,108,197,192,70,67,184,62,129,67,108,203,161,56,67,33,112,76,67,108,74,172,45,67,158,239,72,67,98,43,103,33,67,53,126,110,67,
	199,43,9,67,186,9,132,67,59,159,218,66,186,9,132,67,98,180,72,163,66,186,9,132,67,63,53,102,66,16,216,110,67,223,207,52,66,195,181,73,67,108,240,167,18,66,33,112,76,67,108,16,88,180,65,184,62,129,67,108,0,0,0,0,219,153,124,67,108,84,227,138,65,162,37,
	58,67,108,168,198,29,66,10,23,51,67,98,59,223,21,66,246,72,40,67,190,159,17,66,70,214,28,67,190,159,17,66,238,252,16,67,98,190,159,17,66,37,198,16,67,190,159,17,66,27,143,16,67,197,160,17,66,82,88,16,67,99,109,68,107,0,67,8,172,99,66,98,70,86,8,67,193,
	202,1,66,53,62,25,67,205,204,44,65,219,89,49,67,96,229,72,64,108,244,93,59,67,0,0,0,0,98,246,104,61,67,117,147,208,64,248,115,63,67,141,151,80,65,250,126,65,67,164,112,156,65,98,164,176,56,67,31,133,178,65,219,185,47,67,135,22,200,65,106,188,40,67,6,
	129,246,65,98,59,223,27,67,213,248,37,66,164,48,20,67,100,187,104,66,182,51,17,67,197,224,147,66,108,199,139,16,67,248,83,155,66,98,84,163,21,67,215,99,174,66,127,234,25,67,160,154,197,66,178,29,29,67,199,203,223,66,108,4,86,125,66,199,203,223,66,98,
	74,12,134,66,135,150,193,66,205,76,144,66,135,86,167,66,35,155,156,66,35,219,146,66,98,6,1,152,66,217,78,125,66,252,105,145,66,246,40,86,66,219,121,133,66,213,248,48,66,98,172,28,114,66,174,71,10,66,160,154,78,66,162,69,210,65,156,68,33,66,201,118,181,
	65,108,82,184,242,65,176,114,156,65,108,59,223,17,66,0,0,0,0,98,94,58,31,66,25,4,134,63,129,149,44,66,182,243,5,64,164,240,57,66,96,229,72,64,98,82,184,138,66,68,139,38,65,6,193,172,66,90,100,248,65,33,112,189,66,57,180,91,66,98,6,65,199,66,176,114,78,
	66,127,170,209,66,236,81,71,66,51,115,220,66,236,81,71,66,98,219,57,233,66,236,81,71,66,100,123,245,66,248,83,81,66,68,107,0,67,8,172,99,66,99,101,0,0 };



	static const unsigned char asmIcon[] = { 110,109,176,114,148,64,8,172,122,65,98,74,12,134,64,47,221,130,65,14,45,90,64,31,133,134,65,55,137,33,64,31,133,134,65,98,156,196,144,63,31,133,134,65,0,0,0,0,170,241,122,65,0,0,0,0,240,167,100,65,98,0,0,0,0,53,94,78,65,156,196,144,63,186,73,60,65,55,
	137,33,64,186,73,60,65,98,14,45,90,64,186,73,60,65,74,12,134,64,154,153,67,65,176,114,148,64,240,167,78,65,108,102,102,190,64,240,167,78,65,108,102,102,190,64,139,108,41,65,98,102,102,190,64,80,141,11,65,72,225,238,64,190,159,230,64,223,79,21,65,190,
	159,230,64,108,240,167,78,65,190,159,230,64,108,240,167,78,65,176,114,148,64,98,154,153,67,65,74,12,134,64,186,73,60,65,14,45,90,64,186,73,60,65,55,137,33,64,98,186,73,60,65,156,196,144,63,53,94,78,65,0,0,0,0,240,167,100,65,0,0,0,0,98,170,241,122,65,
	0,0,0,0,31,133,134,65,156,196,144,63,31,133,134,65,55,137,33,64,98,31,133,134,65,14,45,90,64,47,221,130,65,74,12,134,64,8,172,122,65,176,114,148,64,108,8,172,122,65,190,159,230,64,108,199,75,169,65,190,159,230,64,108,199,75,169,65,176,114,148,64,98,156,
	196,163,65,74,12,134,64,172,28,160,65,14,45,90,64,172,28,160,65,55,137,33,64,98,172,28,160,65,156,196,144,63,246,40,169,65,0,0,0,0,211,77,180,65,0,0,0,0,98,164,112,191,65,0,0,0,0,238,124,200,65,156,196,144,63,238,124,200,65,55,137,33,64,98,238,124,200,
	65,14,45,90,64,254,212,196,65,74,12,134,64,211,77,191,65,176,114,148,64,108,211,77,191,65,190,159,230,64,108,150,67,235,65,190,159,230,64,108,150,67,235,65,176,114,148,64,98,106,188,229,65,74,12,134,64,123,20,226,65,14,45,90,64,123,20,226,65,55,137,33,
	64,98,123,20,226,65,156,196,144,63,197,32,235,65,0,0,0,0,162,69,246,65,0,0,0,0,98,63,181,0,66,0,0,0,0,94,58,5,66,156,196,144,63,94,58,5,66,55,137,33,64,98,94,58,5,66,14,45,90,64,102,102,3,66,74,12,134,64,209,162,0,66,176,114,148,64,108,209,162,0,66,190,
	159,230,64,108,86,14,14,66,190,159,230,64,98,37,134,21,66,190,159,230,64,129,149,27,66,80,141,11,65,129,149,27,66,139,108,41,65,108,129,149,27,66,240,167,78,65,108,119,190,33,66,240,167,78,65,98,68,139,35,66,154,153,67,65,252,169,38,66,186,73,60,65,57,
	52,42,66,186,73,60,65,98,168,198,47,66,186,73,60,65,205,76,52,66,53,94,78,65,205,76,52,66,240,167,100,65,98,205,76,52,66,170,241,122,65,168,198,47,66,31,133,134,65,57,52,42,66,31,133,134,65,98,252,169,38,66,31,133,134,65,68,139,35,66,47,221,130,65,119,
	190,33,66,8,172,122,65,108,129,149,27,66,8,172,122,65,108,129,149,27,66,199,75,169,65,108,119,190,33,66,199,75,169,65,98,68,139,35,66,156,196,163,65,252,169,38,66,172,28,160,65,57,52,42,66,172,28,160,65,98,168,198,47,66,172,28,160,65,205,76,52,66,246,
	40,169,65,205,76,52,66,211,77,180,65,98,205,76,52,66,164,112,191,65,168,198,47,66,238,124,200,65,57,52,42,66,238,124,200,65,98,252,169,38,66,238,124,200,65,68,139,35,66,254,212,196,65,119,190,33,66,211,77,191,65,108,129,149,27,66,211,77,191,65,108,129,
	149,27,66,150,67,235,65,108,119,190,33,66,150,67,235,65,98,68,139,35,66,106,188,229,65,252,169,38,66,123,20,226,65,57,52,42,66,123,20,226,65,98,168,198,47,66,123,20,226,65,205,76,52,66,197,32,235,65,205,76,52,66,162,69,246,65,98,205,76,52,66,63,181,0,
	66,168,198,47,66,94,58,5,66,57,52,42,66,94,58,5,66,98,252,169,38,66,94,58,5,66,68,139,35,66,102,102,3,66,119,190,33,66,209,162,0,66,108,129,149,27,66,209,162,0,66,108,129,149,27,66,162,69,10,66,98,129,149,27,66,106,188,17,66,37,134,21,66,199,203,23,66,
	86,14,14,66,199,203,23,66,108,209,162,0,66,199,203,23,66,108,209,162,0,66,119,190,33,66,98,102,102,3,66,68,139,35,66,94,58,5,66,252,169,38,66,94,58,5,66,57,52,42,66,98,94,58,5,66,168,198,47,66,63,181,0,66,205,76,52,66,162,69,246,65,205,76,52,66,98,197,
	32,235,65,205,76,52,66,123,20,226,65,168,198,47,66,123,20,226,65,57,52,42,66,98,123,20,226,65,252,169,38,66,106,188,229,65,68,139,35,66,150,67,235,65,119,190,33,66,108,150,67,235,65,199,203,23,66,108,211,77,191,65,199,203,23,66,108,211,77,191,65,119,
	190,33,66,98,254,212,196,65,68,139,35,66,238,124,200,65,252,169,38,66,238,124,200,65,57,52,42,66,98,238,124,200,65,168,198,47,66,164,112,191,65,205,76,52,66,211,77,180,65,205,76,52,66,98,246,40,169,65,205,76,52,66,172,28,160,65,168,198,47,66,172,28,160,
	65,57,52,42,66,98,172,28,160,65,252,169,38,66,156,196,163,65,68,139,35,66,199,75,169,65,119,190,33,66,108,199,75,169,65,199,203,23,66,108,8,172,122,65,199,203,23,66,108,8,172,122,65,119,190,33,66,98,47,221,130,65,68,139,35,66,31,133,134,65,252,169,38,
	66,31,133,134,65,57,52,42,66,98,31,133,134,65,168,198,47,66,170,241,122,65,205,76,52,66,240,167,100,65,205,76,52,66,98,53,94,78,65,205,76,52,66,186,73,60,65,168,198,47,66,186,73,60,65,57,52,42,66,98,186,73,60,65,252,169,38,66,154,153,67,65,68,139,35,
	66,240,167,78,65,119,190,33,66,108,240,167,78,65,199,203,23,66,108,223,79,21,65,199,203,23,66,98,72,225,238,64,199,203,23,66,102,102,190,64,106,188,17,66,102,102,190,64,162,69,10,66,108,102,102,190,64,209,162,0,66,108,176,114,148,64,209,162,0,66,98,74,
	12,134,64,102,102,3,66,14,45,90,64,94,58,5,66,55,137,33,64,94,58,5,66,98,156,196,144,63,94,58,5,66,0,0,0,0,63,181,0,66,0,0,0,0,162,69,246,65,98,0,0,0,0,197,32,235,65,156,196,144,63,123,20,226,65,55,137,33,64,123,20,226,65,98,14,45,90,64,123,20,226,65,
	74,12,134,64,106,188,229,65,176,114,148,64,150,67,235,65,108,102,102,190,64,150,67,235,65,108,102,102,190,64,211,77,191,65,108,176,114,148,64,211,77,191,65,98,74,12,134,64,254,212,196,65,14,45,90,64,238,124,200,65,55,137,33,64,238,124,200,65,98,156,196,
	144,63,238,124,200,65,0,0,0,0,164,112,191,65,0,0,0,0,211,77,180,65,98,0,0,0,0,246,40,169,65,156,196,144,63,172,28,160,65,55,137,33,64,172,28,160,65,98,14,45,90,64,172,28,160,65,74,12,134,64,156,196,163,65,176,114,148,64,199,75,169,65,108,102,102,190,
	64,199,75,169,65,108,102,102,190,64,8,172,122,65,108,176,114,148,64,8,172,122,65,99,109,14,45,6,66,123,20,1,66,108,14,45,6,66,127,106,76,65,108,6,129,53,65,127,106,76,65,108,6,129,53,65,123,20,1,66,108,14,45,6,66,123,20,1,66,99,101,0,0 };

	static const unsigned char optimizeIcon[] = { 110,109,12,2,148,65,141,151,197,65,108,70,182,101,65,164,112,164,65,108,250,126,180,65,154,153,69,65,108,70,182,101,65,166,155,132,64,108,12,2,148,65,0,0,0,0,108,129,149,246,65,233,38,69,65,108,41,92,246,65,154,153,69,65,108,129,149,246,65,49,8,70,65,
	108,12,2,148,65,141,151,197,65,99,109,166,155,132,64,141,151,197,65,108,0,0,0,0,164,112,164,65,108,174,71,3,65,154,153,69,65,108,0,0,0,0,166,155,132,64,108,166,155,132,64,0,0,0,0,108,94,186,131,65,233,38,69,65,108,6,129,131,65,154,153,69,65,108,94,186,
	131,65,49,8,70,65,108,166,155,132,64,141,151,197,65,99,101,0,0 };

	static const unsigned char debugPanel[] = { 110,109,130,248,113,67,59,111,13,68,108,130,248,113,67,185,123,14,68,98,155,142,112,67,138,172,14,68,188,90,111,67,230,205,14,68,227,92,110,67,204,223,14,68,98,9,95,109,67,178,241,14,68,2,77,108,67,165,250,14,68,207,38,107,67,165,250,14,68,98,12,142,
105,67,165,250,14,68,58,19,104,67,89,235,14,68,89,182,102,67,194,204,14,68,98,248,164,101,67,83,181,14,68,134,205,100,67,182,151,14,68,2,48,100,67,233,115,14,68,98,126,146,99,67,28,80,14,68,87,1,99,67,58,27,14,68,143,124,98,67,65,213,13,68,98,199,247,
97,67,72,143,13,68,99,181,97,67,3,65,13,68,99,181,97,67,114,234,12,68,108,99,181,97,67,242,89,12,68,98,99,181,97,67,131,212,11,68,28,88,98,67,102,93,11,68,143,157,99,67,155,244,10,68,98,245,82,101,67,182,102,10,68,224,165,103,67,196,31,10,68,79,150,106,
67,195,31,10,68,98,2,113,107,67,195,31,10,68,165,64,108,67,76,37,10,68,55,5,109,67,92,48,10,68,98,200,201,109,67,109,59,10,68,150,132,110,67,6,76,10,68,162,53,111,67,39,98,10,68,98,97,160,111,67,18,74,10,68,133,8,112,67,7,62,10,68,16,110,112,67,7,62,
10,68,98,56,227,112,67,7,62,10,68,64,66,113,67,30,72,10,68,40,139,113,67,75,92,10,68,98,13,212,113,67,121,112,10,68,128,248,113,67,165,146,10,68,130,248,113,67,207,194,10,68,108,130,248,113,67,169,68,11,68,98,129,248,113,67,212,116,11,68,103,211,113,
67,41,151,11,68,52,137,113,67,170,171,11,68,98,255,62,113,67,43,192,11,68,81,223,112,67,107,202,11,68,40,106,112,67,107,202,11,68,98,109,12,112,67,107,202,11,68,182,187,111,67,66,195,11,68,6,120,111,67,240,180,11,68,98,243,67,111,67,45,171,11,68,87,24,
111,67,136,149,11,68,50,245,110,67,3,116,11,68,98,11,210,110,67,126,82,11,68,87,170,110,67,223,59,11,68,21,126,110,67,40,48,11,68,98,200,55,110,67,72,29,11,68,118,187,109,67,133,12,11,68,31,9,109,67,224,253,10,68,98,197,86,108,67,59,239,10,68,243,142,
107,67,233,231,10,68,165,177,106,67,232,231,10,68,98,2,116,105,67,232,231,10,68,7,96,104,67,46,248,10,68,181,117,103,67,185,24,11,68,98,20,207,102,67,207,48,11,68,119,53,102,67,202,90,11,68,224,168,101,67,172,150,11,68,98,72,28,101,67,142,210,11,68,252,
213,100,67,165,19,12,68,252,213,100,67,241,89,12,68,108,252,213,100,67,113,234,12,68,98,252,213,100,67,215,85,13,68,78,82,101,67,93,167,13,68,243,74,102,67,4,223,13,68,98,151,67,103,67,171,22,14,68,175,206,104,67,126,50,14,68,58,236,106,67,126,50,14,
68,98,84,91,108,67,126,50,14,68,227,169,109,67,63,33,14,68,232,215,110,67,191,254,13,68,108,232,215,110,67,57,111,13,68,108,176,167,107,67,57,111,13,68,98,5,231,106,67,57,111,13,68,174,93,106,67,243,101,13,68,172,11,106,67,102,83,13,68,98,168,185,105,
67,217,64,13,68,167,144,105,67,154,40,13,68,167,144,105,67,169,10,13,68,98,166,144,105,67,184,236,12,68,168,185,105,67,121,212,12,68,172,11,106,67,236,193,12,68,98,174,93,106,67,95,175,12,68,5,231,106,67,25,166,12,68,176,167,107,67,25,166,12,68,108,61,
53,113,67,18,167,12,68,98,230,245,113,67,18,167,12,68,61,127,114,67,47,176,12,68,65,209,114,67,104,194,12,68,98,67,35,115,67,162,212,12,68,68,76,115,67,183,236,12,68,70,76,115,67,168,10,13,68,98,69,76,115,67,23,34,13,68,72,48,115,67,151,54,13,68,79,248,
114,67,42,72,13,68,98,84,192,114,67,189,89,13,68,16,107,114,67,194,102,13,68,130,248,113,67,56,111,13,68,99,109,51,249,80,67,53,21,14,68,108,51,249,80,67,46,6,11,68,108,58,124,80,67,46,6,11,68,98,144,187,79,67,46,6,11,68,57,50,79,67,232,252,10,68,54,
224,78,67,91,234,10,68,98,51,142,78,67,206,215,10,68,49,101,78,67,143,191,10,68,49,101,78,67,158,161,10,68,98,49,101,78,67,173,131,10,68,51,142,78,67,110,107,10,68,54,224,78,67,225,88,10,68,98,57,50,79,67,84,70,10,68,144,187,79,67,14,61,10,68,58,124,
80,67,14,61,10,68,108,150,83,88,67,14,61,10,68,98,164,50,90,67,14,61,10,68,248,179,91,67,106,94,10,68,146,215,92,67,33,161,10,68,98,43,251,93,67,217,227,10,68,248,140,94,67,24,51,11,68,249,140,94,67,222,142,11,68,98,248,140,94,67,122,186,11,68,31,107,
94,67,124,227,11,68,111,39,94,67,227,9,12,68,98,189,227,93,67,74,48,12,68,230,124,93,67,196,83,12,68,233,242,92,67,79,116,12,68,98,116,239,93,67,16,154,12,68,221,172,94,67,41,198,12,68,36,43,95,67,154,248,12,68,98,105,169,95,67,12,43,13,68,140,232,95,
67,42,100,13,68,141,232,95,67,243,163,13,68,98,140,232,95,67,184,214,13,68,252,186,95,67,233,5,14,68,221,95,95,67,133,49,14,68,98,43,28,95,67,183,82,14,68,218,200,94,67,20,109,14,68,236,101,94,67,154,128,14,68,98,34,225,93,67,240,155,14,68,105,62,93,
67,59,178,14,68,192,125,92,67,123,195,14,68,98,21,189,91,67,187,212,14,68,65,204,90,67,91,221,14,68,66,171,89,67,91,221,14,68,108,58,124,80,67,91,221,14,68,98,144,187,79,67,91,221,14,68,57,50,79,67,20,212,14,68,54,224,78,67,136,193,14,68,98,51,142,78,
67,251,174,14,68,49,101,78,67,188,150,14,68,49,101,78,67,203,120,14,68,98,49,101,78,67,129,91,14,68,217,142,78,67,149,67,14,68,42,226,78,67,8,49,14,68,98,123,53,79,67,123,30,14,68,42,190,79,67,53,21,14,68,58,124,80,67,53,21,14,68,99,109,205,25,84,67,
34,41,12,68,108,130,136,87,67,34,41,12,68,98,137,195,88,67,34,41,12,68,50,201,89,67,72,21,12,68,124,153,90,67,148,237,11,68,98,19,38,91,67,228,210,11,68,95,108,91,67,178,177,11,68,96,108,91,67,254,137,11,68,98,95,108,91,67,216,102,11,68,251,41,91,67,
23,72,11,68,52,165,90,67,186,45,11,68,98,106,32,90,67,94,19,11,68,135,77,89,67,48,6,11,68,136,44,88,67,47,6,11,68,108,205,25,84,67,47,6,11,68,99,109,205,25,84,67,54,21,14,68,108,77,128,89,67,54,21,14,68,98,190,197,90,67,54,21,14,68,220,170,91,67,44,9,
14,68,165,47,92,67,22,241,13,68,98,46,149,92,67,220,222,13,68,243,199,92,67,211,196,13,68,244,199,92,67,250,162,13,68,98,243,199,92,67,159,122,13,68,183,99,92,67,235,82,13,68,61,155,91,67,221,43,13,68,98,194,210,90,67,207,4,13,68,16,179,89,67,73,241,
12,68,39,60,88,67,72,241,12,68,108,205,25,84,67,72,241,12,68,99,109,50,161,61,67,54,21,14,68,108,50,161,61,67,47,6,11,68,98,247,247,60,67,136,5,11,68,254,122,60,67,239,251,10,68,72,42,60,67,98,233,10,68,98,146,217,59,67,213,214,10,68,55,177,59,67,234,
190,10,68,55,177,59,67,159,161,10,68,98,55,177,59,67,174,131,10,68,57,218,59,67,111,107,10,68,60,44,60,67,226,88,10,68,98,63,126,60,67,85,70,10,68,150,7,61,67,15,61,10,68,64,200,61,67,15,61,10,68,108,167,83,68,67,10,62,10,68,98,2,88,69,67,10,62,10,68,
130,86,70,67,21,74,10,68,39,79,71,67,42,98,10,68,98,202,71,72,67,63,122,10,68,157,15,73,67,215,152,10,68,160,166,73,67,241,189,10,68,98,46,25,74,67,238,217,10,68,75,144,74,67,85,0,11,68,247,11,75,67,38,49,11,68,98,161,135,75,67,247,97,11,68,182,228,75,
67,117,146,11,68,51,35,76,67,159,194,11,68,98,174,97,76,67,202,242,11,68,236,128,76,67,178,45,12,68,238,128,76,67,87,115,12,68,108,238,128,76,67,5,211,12,68,98,237,128,76,67,163,39,13,68,166,71,76,67,167,114,13,68,24,213,75,67,17,180,13,68,98,137,98,
75,67,123,245,13,68,45,204,74,67,219,42,14,68,6,18,74,67,48,84,14,68,98,222,87,73,67,133,125,14,68,198,168,72,67,243,155,14,68,193,4,72,67,122,175,14,68,98,100,0,71,67,18,206,14,68,59,175,69,67,93,221,14,68,68,17,68,67,93,221,14,68,108,65,200,61,67,93,
221,14,68,98,151,7,61,67,93,221,14,68,64,126,60,67,22,212,14,68,61,44,60,67,138,193,14,68,98,58,218,59,67,253,174,14,68,56,177,59,67,190,150,14,68,56,177,59,67,205,120,14,68,98,56,177,59,67,131,91,14,68,58,218,59,67,109,67,14,68,61,44,60,67,141,48,14,
68,98,64,126,60,67,173,29,14,68,146,250,60,67,144,20,14,68,51,161,61,67,55,21,14,68,99,109,204,193,64,67,54,21,14,68,108,42,21,68,67,54,21,14,68,98,204,82,69,67,54,21,14,68,84,66,70,67,210,9,14,68,192,227,70,67,10,243,13,68,98,163,182,71,67,25,213,13,
68,116,85,72,67,5,175,13,68,53,192,72,67,207,128,13,68,98,243,42,73,67,152,82,13,68,82,96,73,67,183,22,13,68,83,96,73,67,41,205,12,68,108,83,96,73,67,116,110,12,68,98,82,96,73,67,81,47,12,68,40,48,73,67,254,247,11,68,212,207,72,67,122,200,11,68,98,209,
56,72,67,70,126,11,68,239,142,71,67,88,75,11,68,45,210,70,67,174,47,11,68,98,106,21,70,67,5,20,11,68,36,41,69,67,48,6,11,68,90,13,68,67,47,6,11,68,108,203,193,64,67,47,6,11,68,99,101,0,0 };

}

struct CallbackStateComponent : public Component,
								public CallbackCollection::Listener
{
	CallbackStateComponent() :
		r("")
	{
		r.getStyleData().fontSize = 14.0f;
	}

	void initialised(const CallbackCollection& c) override
	{
		frameCallback = c.getBestCallback(CallbackCollection::FrameProcessing);
		blockCallback = c.getBestCallback(CallbackCollection::BlockProcessing);
		currentCallback = frameProcessing ? frameCallback : blockCallback;

		rebuild();
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT());

		r.draw(g, getLocalBounds().reduced(5).toFloat());
	}

	juce::String getCallbackName() const
	{
		switch (currentCallback)
		{
		case CallbackTypes::Channel:	return "processChannel()";
		case CallbackTypes::Frame:		return "processFrame()";
		case CallbackTypes::Sample:	return "processSample()";
		default:							return "inactive";
		}
	}

	void setFrameProcessing(int mode)
	{
		frameProcessing = mode == CallbackCollection::ProcessType::FrameProcessing;
		currentCallback = frameProcessing ? frameCallback : blockCallback;

		rebuild();
	}

	void rebuild()
	{
		juce::String s;
		s << "**Samplerate**: " << juce::String(samplerate, 1) << "  ";
		s << "**Blocksize**: " << juce::String(blockSize) << "  ";
		s << "**NumChannels**: " << juce::String(numChannels) << "  ";
		s << "**Frame processing**: " << (frameProcessing ? "Yes" : "No") << "  ";
		s << "**Used Callback**: `" << getCallbackName() << "`";

		r.setNewText(s);
		auto h = r.getHeightForWidth((float)(getWidth() + 10), true);

		setSize(getWidth(), h + 10);

		repaint();
	}

	void prepare(double samplerate_, int blockSize_, int numChannels_) override
	{
		samplerate = samplerate_;
		blockSize = blockSize_;
		numChannels = numChannels_;
		
		MessageManager::callAsync([this]
		{
			rebuild();
		});
	}

	juce::String processSpecs;

	double samplerate = 0.0;
	int blockSize = 0;
	int numChannels = 0;

	int frameCallback = CallbackTypes::Inactive;
	int blockCallback = CallbackTypes::Inactive;

	int currentCallback = CallbackTypes::Inactive;
	bool frameProcessing = false;
	bool active = false;

	BreakpointHandler* handler = nullptr;
	MarkdownRenderer r;
};

struct SnexPathFactory: public hise::PathFactory
{
	juce::String getId() const override { return "Snex"; }
    Path createPath(const juce::String& id) const override;
};
  


/** Quick and dirty assembly syntax highlighter.

Definitely not standard conform (don't know nothing about assembly lol).
*/
class AssemblyTokeniser : public juce::CodeTokeniser
{
public:

	enum Tokens
	{
		Unknown,
		Comment,
		Location,
		Number,
		Label,
		Instruction,
        Register,
        Type,
        Local
	};

	static SparseSet<int> applyDiff(const String& oldAsm, String& newAsm);

	int readNextToken(CodeDocument::Iterator& source) override;

    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
    
    Tokens currentState;
};

class BreakpointDataProvider : public ApiProviderBase,
							   public ApiProviderBase::Holder
{
public:

	BreakpointDataProvider(GlobalScope& m) :
		handler(m.getBreakpointHandler()),
		scope(m)
	{};

	ApiProviderBase* getProviderBase() override { return this; }

	int getNumDebugObjects() const override
	{
		return infos.size();
	}

	DebugInformationBase::Ptr getDebugInformation(int index)
	{
		return infos[index];
	}

	void getColourAndLetterForType(int type, Colour& colour, char& letter) override
	{
		ApiHelpers::getColourAndLetterForType(type, colour, letter);
	}

	void rebuild() override;

	DebugInformationBase::List infos;
	GlobalScope& scope;
	BreakpointHandler& handler;
};


/** A simple background thread that does the compilation. */
class BackgroundCompileThread: public Thread,
	public ui::WorkbenchData::CompileHandler
{
public:

	

	BackgroundCompileThread(ui::WorkbenchData::Ptr data_) :
		Thread("SnexPlaygroundThread"),
		CompileHandler(data_.get())
	{
		getParent()->getGlobalScope().getBreakpointHandler().setExecutingThread(this);
		setPriority(4);
	}

	~BackgroundCompileThread()
	{
		stopThread(3000);
	}

	virtual JitCompiledNode::Ptr getLastNode() = 0;

	ui::WorkbenchData::CompileResult compile(const String& s) override
	{
		// You need to override that method...
		jassertfalse;
		return { };
	}

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override
	{
		// You need to override that method...
		jassertfalse;
	}

	

	bool triggerCompilation() override
	{
		getParent()->getGlobalScope().getBreakpointHandler().abort();

		if (isThreadRunning())
			stopThread(3000);

		startThread();

		return false;
	}

	void run() override
	{
		try
		{
			getParent()->handleCompilation();
		}
		catch (...)
		{
			jassertfalse;
		}
	}

protected:

};


class TestCompileThread : public BackgroundCompileThread
{
public:

	TestCompileThread(ui::WorkbenchData::Ptr data) :
		BackgroundCompileThread(data)
	{
		
	}

	ui::WorkbenchData::CompileResult compile(const String& s) override;

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override;

	JitCompiledNode::Ptr getLastNode() override
	{
		if (lastTest != nullptr)
		{
			return lastTest->nodeToTest;
		}

		return nullptr;
	}

	void processTestParameterEvent(int parameterIndex, double value) override
	{
		jassert(lastTest != nullptr);
		jassert(lastTest->nodeToTest != nullptr);

		if (isPositiveAndBelow(parameterIndex, lastResult.parameters.size()))
			lastResult.parameters.getReference(parameterIndex).callback.call(value);
	}

	void processTest(ProcessDataDyn& data) override
	{
		jassert(lastTest != nullptr);
		jassert(lastTest->nodeToTest != nullptr);

		lastTest->nodeToTest->process(data);
	}

	Result prepareTest(PrepareSpecs ps, const Array<ui::WorkbenchData::TestData::ParameterEvent>& initialParameters) override
	{
		jassert(lastTest != nullptr);
		jassert(lastTest->nodeToTest != nullptr);

		lastTest->nodeToTest->prepare(ps);
		lastTest->nodeToTest->reset();
        
        return Result::ok();
	}

private:

	ui::WorkbenchData::CompileResult lastResult;

	AudioSampleBuffer empty;

	ScopedPointer<JitFileTestCase> lastTest;
};


class JitNodeCompileThread : public BackgroundCompileThread
{
public:

	JitNodeCompileThread(ui::WorkbenchData::Ptr d) :
		BackgroundCompileThread(d)
	{
		
	};

	Result prepareTest(PrepareSpecs ps, const Array<ui::WorkbenchData::TestData::ParameterEvent>& initialParameters)
	{
		if (lastNode != nullptr)
			lastNode->prepare(ps);
        
        return Result::ok();
	}

	void processTestParameterEvent(int parameterIndex, double value) override
	{
		if (isPositiveAndBelow(parameterIndex, lastResult.parameters.size()))
			lastResult.parameters.getReference(parameterIndex).callback.call(value);
	}

	void processTest(ProcessDataDyn& data) override
	{
		if (lastNode != nullptr)
			lastNode->process(data);
	}

	ui::WorkbenchData::CompileResult compile(const String& code)
	{
		auto p = getParent();

		lastResult = {};

		auto instanceId = p->getInstanceId();

		if (instanceId.isValid())
		{
			Compiler::Ptr cc = createCompiler();

			auto numChannels = getParent()->getTestData().testSourceData.getNumChannels();

			if (numChannels == 0)
				numChannels = 2;

			lastNode = new JitCompiledNode(*cc, code, instanceId.toString(), numChannels);

			lastResult.assembly = cc->getAssemblyCode();
			lastResult.compileResult = lastNode->r;
			lastResult.obj = lastNode->getJitObject();
			lastResult.lastNode = lastNode;

			lastResult.parameters.clear();
			lastResult.parameters.addArray(lastNode->getParameterList());

			return lastResult;
		}

		lastResult.compileResult = Result::fail("Didn't specify file");
		return lastResult;
	}

	void postCompile(ui::WorkbenchData::CompileResult& lastResult) override
	{
		if (lastNode != nullptr && lastResult.compiledOk())
		{
			auto& testData = getParent()->getTestData();

			if (testData.shouldRunTest())
			{
                PrepareSpecs ps;
                ps.sampleRate = 44100.0;
                ps.blockSize = 512;
                ps.numChannels = 2;
                
				testData.initProcessing(ps);
				testData.processTestData(getParent());
			}
		}
	}

	JitCompiledNode::Ptr getLastNode() override { return lastNode; }

private:

	ui::WorkbenchData::CompileResult lastResult;

	
	JitCompiledNode::Ptr lastNode;
};


class SnexPlayground : public ui::WorkbenchComponent,
	public CodeDocument::Listener,
	public ButtonListener,
	public BreakpointHandler::Listener,
	public mcl::GutterComponent::BreakpointListener
{
public:

	enum class ContextActions
	{
		BackgroundParsing = 11000,
		Preprocessor,
		Goto
	};

	struct TestCodeProvider : public ui::WorkbenchData::CodeProvider
	{
		TestCodeProvider(SnexPlayground& p, const File& f_):
			CodeProvider(p.getWorkbench()),
			parent(p),
			f(f_)
		{}

		static String getTestTemplate();

		String loadCode() const override;

		bool saveCode(const String& s) override;

		Identifier getInstanceId() const override 
		{ 
			if (f.existsAsFile())
				return Identifier(f.getFileNameWithoutExtension());

			return Identifier("UnsavedTest");
		}

		SnexPlayground& parent;

		File f;
	};

	struct PreprocessorUpdater: public Timer,
								public CodeDocument::Listener,
		public snex::DebugHandler
	{
		PreprocessorUpdater(SnexPlayground& parent_):
			parent(parent_)
		{
			parent.doc.addListener(this);
		}

		void timerCallback() override;

		void codeDocumentTextInserted(const juce::String&, int) override
		{
			startTimer(1200);
		}

		void logMessage(int level, const juce::String& s);

		void codeDocumentTextDeleted(int, int) override
		{
			startTimer(1200);
		}

		~PreprocessorUpdater()
		{
			parent.doc.removeListener(this);
		}

		SparseSet<int> lastRange;

		SnexPlayground& parent;
	};

	void codeDocumentTextInserted(const juce::String& , int ) override
	{
		auto lineToShow = jmax(0, consoleContent.getNumLines() - console.getNumLinesOnScreen());
		console.scrollToLine(lineToShow);
	}

	void codeDocumentTextDeleted(int , int ) override
	{

	}

	void breakpointsChanged(mcl::GutterComponent& g) override
	{
		
		getWorkbench()->triggerRecompile();
	}

	

	SnexPlayground(ui::WorkbenchData* data, bool addDebugComponents=false);

	~SnexPlayground();

	void paint(Graphics& g) override;
	void resized() override;

	void mouseDown(const MouseEvent& event) override;

	void addPopupMenuItems(mcl::TextEditor& te, PopupMenu &m, const MouseEvent& e)
	{
		m.addItem((int)ContextActions::Goto, "Goto definition", true, false);
		m.addItem((int)ContextActions::BackgroundParsing, "Background C++ parsing", true, te.enableLiveParsing);
		m.addItem((int)ContextActions::Preprocessor, "Parse preprocessor conditions", true, te.enablePreprocessorParsing);
	}

	bool performPopupMenu(mcl::TextEditor& te, int result)
	{
		switch ((ContextActions)result)
		{
		case ContextActions::Goto: te.gotoDefinition(te.getTextDocument().getSelection(0)); return true;
		case ContextActions::BackgroundParsing: te.enableLiveParsing = !te.enableLiveParsing; return true;
		case ContextActions::Preprocessor: te.enablePreprocessorParsing = !te.enablePreprocessorParsing; return true;
		}

		return false;
	}

	bool keyPressed(const KeyPress& k) override;
	void createTestSignal();

	bool preprocess(String& code) override
	{
		return editor.injectBreakpointCode(code);
	}

	struct Spacer : public Component
	{
		Spacer(const juce::String& n) :
			Component(n)
		{};

		void paint(Graphics& g) override
		{
			g.setColour(Colours::black.withAlpha(0.4f));
			g.fillAll();
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(getName(), getLocalBounds().toFloat(), Justification::centred);
		}
	};

	void setReadOnly(bool shouldBeReadOnly)
	{
		isReadOnly = shouldBeReadOnly;
		editor.setReadOnly(!isReadOnly);
	}

	void updateTextFromCodeProvider()
	{
		auto c = getWorkbench()->getCode();
		doc.replaceAllContent(c);
		doc.clearUndoHistory();
	}

	void buttonClicked(Button* b) override;
	
	

	

private:

	bool isReadOnly = false;

	ScopedPointer<ui::WorkbenchData::CodeProvider> previousProvider;

	File currentTestFile;

	int currentBreakpointLine = -1;

	juce::String currentParameter;
	std::function<void(void)> pendingParam;

	bool dirty = false;

	void recalculateInternal();

	struct ButtonLaf : public LookAndFeel_V3
	{
		void drawButtonBackground(Graphics& g, Button& b, const Colour& , bool over, bool down)
		{
			float alpha = 0.0f;

			if (over)
				alpha += 0.2f;
			if (down)
				alpha += 0.2f;

			if (b.getToggleState())
			{
				g.setColour(Colours::white.withAlpha(0.5f));
				g.fillRoundedRectangle(b.getLocalBounds().toFloat(), 3.0f);
			}

			g.setColour(Colours::white.withAlpha(alpha));
			g.fillRoundedRectangle(b.getLocalBounds().toFloat(), 3.0f);
		}

		void drawButtonText(Graphics&g, TextButton& b, bool , bool )
		{
			auto c = !b.getToggleState() ? Colours::white : Colours::black;
			g.setColour(c.withAlpha(0.8f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
		}
	} blaf;

	void logMessage(ui::WorkbenchData::Ptr d, int level, const juce::String& s) override;

	void eventHappened(BreakpointHandler* handler, BreakpointHandler::EventType type) override
	{
		currentBreakpointLine = (int)*handler->getLineNumber();

		if (type == BreakpointHandler::Resume)
			currentBreakpointLine = -1;

		editor.setCurrentBreakline(currentBreakpointLine);

		bpProvider.rebuild();
	}

	void preCompile() override
	{
		auto& ed = editor.editor;
		ed.clearWarningsAndErrors();

		if (isReadOnly)
		{
			doc.replaceAllContent(getWorkbench()->getCode());
			doc.clearUndoHistory();
		}
		else
		{
			assemblyDoc.replaceAllContent({});
			consoleContent.replaceAllContent({});
			consoleContent.clearUndoHistory();
			editor.setCurrentBreakline(-1);
		}
	}

	void debugModeChanged(bool isEnabled) override
	{
		bugButton.setToggleStateAndUpdateIcon(isEnabled);
		editor.enableBreakpoints(isEnabled);
	}

	void recompiled(ui::WorkbenchData::Ptr p) override;

	void postPostCompile(ui::WorkbenchData::Ptr wb) override;

	const bool testMode;

	CodeDocument doc;
	mcl::TextDocument mclDoc;
	PreprocessorUpdater conditionUpdater;
	juce::CPlusPlusCodeTokeniser tokeniser;
	BreakpointDataProvider bpProvider;
	mcl::FullEditor editor;
	AssemblyTokeniser assemblyTokeniser;
	CodeDocument assemblyDoc;
	CodeEditorComponent assembly;
	bool saveTest = false;

	juce::Label resultLabel;

	DebugHandler::Tokeniser consoleTokeniser;
	CodeDocument consoleContent;
	CodeEditorComponent console;
	ScriptWatchTable watchTable;

	ScopedPointer<ui::OptimizationProperties> stateViewer;

    SnexPathFactory factory;
    Path snexIcon;

	HiseShapeButton showAssembly;
	HiseShapeButton showConsole;
	HiseShapeButton showInfo;
	HiseShapeButton bugButton;
	HiseShapeButton showWatch;
	TextButton compileButton;
	TextButton resumeButton;
	

	std::atomic<int> currentSampleIndex = { 0 };

	Spacer spacerAssembly;
	Spacer spacerInfo;
	Spacer spacerConsole;
	Spacer spacerWatch;

	

	Array<Range<int>> scopeRanges;
	ScopedPointer<TestCodeProvider> testProvider;
};

}
}

#endif
