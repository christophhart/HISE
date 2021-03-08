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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;

namespace ScriptnodeIcons
{
	static const unsigned char probeIcon[] = { 110,109,176,114,160,64,164,112,173,63,108,180,200,54,64,164,112,173,63,108,180,200,54,64,0,0,0,0,108,115,104,205,64,0,0,0,0,98,109,231,215,64,0,0,0,0,215,163,224,64,143,194,117,62,113,61,226,64,123,20,14,63,108,211,77,226,64,123,20,14,63,108,211,77,226,
64,59,223,15,63,98,152,110,226,64,98,16,24,63,43,135,226,64,18,131,32,63,43,135,226,64,195,245,40,63,108,43,135,226,64,164,112,173,63,108,211,77,226,64,164,112,173,63,108,211,77,226,64,219,249,202,64,98,205,204,172,64,18,131,22,65,141,151,110,64,31,133,
71,65,227,165,3,64,68,139,120,65,98,246,40,252,63,51,51,123,65,66,96,5,64,225,122,126,65,156,196,16,64,82,184,126,65,98,41,92,247,64,18,131,129,65,238,124,83,65,129,149,129,65,225,122,149,65,33,176,126,65,98,250,126,154,65,59,223,125,65,84,227,157,65,
168,198,113,65,53,94,155,65,117,147,104,65,108,145,237,108,65,143,194,205,64,108,195,245,108,65,164,112,173,63,108,72,225,108,65,164,112,173,63,98,72,225,108,65,164,112,173,63,72,225,108,65,215,163,128,63,72,225,108,65,195,245,40,63,98,72,225,108,65,
193,202,33,63,121,233,108,65,190,159,26,63,195,245,108,65,70,182,19,63,108,195,245,108,65,72,225,250,62,108,100,59,109,65,72,225,250,62,98,152,110,110,65,244,253,84,62,68,139,114,65,0,0,0,0,164,112,119,65,0,0,0,0,108,88,57,152,65,0,0,0,0,108,88,57,152,
65,164,112,173,63,108,170,241,134,65,164,112,173,63,108,145,237,134,65,29,90,188,64,108,119,190,169,65,43,135,88,65,98,190,159,177,65,248,83,117,65,199,75,167,65,133,235,142,65,180,200,149,65,229,208,143,65,98,213,120,83,65,225,122,144,65,33,176,246,
64,188,116,144,65,244,253,12,64,229,208,143,65,98,174,71,33,63,51,51,143,65,203,161,5,191,14,45,128,65,217,206,119,62,12,2,105,65,108,176,114,160,64,14,45,186,64,108,176,114,160,64,164,112,173,63,99,109,78,98,240,64,143,194,3,65,108,254,212,128,64,145,
237,108,65,108,0,0,145,65,111,18,107,65,108,59,223,103,65,92,143,6,65,98,59,223,103,65,92,143,6,65,12,2,95,65,129,149,243,64,63,53,58,65,102,102,10,65,98,139,108,21,65,244,253,26,65,78,98,240,64,143,194,3,65,78,98,240,64,143,194,3,65,99,101,0,0 };


	static const unsigned char colourIcon[] = { 110,109,115,104,89,64,143,194,9,65,98,248,83,35,63,63,53,34,65,219,249,126,191,203,161,93,65,186,73,44,63,90,100,135,65,98,66,96,21,64,254,212,159,65,180,200,218,64,106,188,167,65,37,6,27,65,211,77,151,65,98,27,47,7,65,231,251,138,65,186,73,248,64,84,
	227,117,65,209,34,251,64,14,45,86,65,98,203,161,249,64,84,227,85,65,197,32,248,64,129,149,85,65,190,159,246,64,174,71,85,65,98,125,63,181,64,133,235,71,65,49,8,132,64,209,34,43,65,115,104,89,64,143,194,9,65,99,109,229,208,144,65,63,53,6,65,98,182,243,
	143,65,221,36,12,65,254,212,142,65,74,12,18,65,152,110,141,65,35,219,23,65,98,133,235,134,65,47,221,50,65,197,32,118,65,55,137,71,65,117,147,90,65,233,38,83,65,98,201,118,94,65,98,16,108,65,180,200,90,65,172,28,131,65,170,241,76,65,209,34,143,65,98,217,
	206,73,65,59,223,145,65,63,53,70,65,139,108,148,65,113,61,66,65,217,206,150,65,98,166,155,102,65,102,102,162,65,115,104,141,65,106,188,162,65,244,253,157,65,174,71,147,65,98,231,251,173,65,78,98,132,65,123,20,178,65,152,110,84,65,184,30,167,65,4,86,46,
	65,98,37,6,162,65,215,163,28,65,123,20,154,65,240,167,14,65,229,208,144,65,63,53,6,65,99,109,49,8,66,65,23,217,90,65,98,102,102,52,65,119,190,93,65,29,90,38,65,250,126,94,65,117,147,24,65,229,208,92,65,98,252,169,23,65,150,67,107,65,172,28,26,65,123,
	20,122,65,106,188,32,65,209,34,132,65,98,246,40,36,65,193,202,135,65,141,151,40,65,184,30,139,65,217,206,45,65,160,26,142,65,98,35,219,59,65,35,219,132,65,12,2,67,65,147,24,114,65,49,8,66,65,23,217,90,65,99,109,137,65,46,65,131,192,36,65,98,193,202,39,
	65,233,38,45,65,43,135,34,65,18,131,54,65,180,200,30,65,127,106,64,65,98,47,221,40,65,143,194,65,65,184,30,51,65,45,178,65,65,184,30,61,65,197,32,64,65,98,98,16,60,65,12,2,61,65,254,212,58,65,133,235,57,65,115,104,57,65,96,229,54,65,98,78,98,54,65,127,
	106,48,65,190,159,50,65,4,86,42,65,137,65,46,65,131,192,36,65,99,109,8,172,131,65,215,163,252,64,98,2,43,129,65,160,26,251,64,125,63,125,65,4,86,250,64,197,32,120,65,102,102,250,64,98,82,184,98,65,100,59,251,64,141,151,78,65,113,61,6,65,78,98,62,65,84,
	227,19,65,98,139,108,71,65,150,67,31,65,106,188,78,65,98,16,44,65,10,215,83,65,20,174,57,65,98,219,249,92,65,193,202,53,65,6,129,101,65,78,98,48,65,61,10,109,65,41,92,41,65,98,55,137,121,65,70,182,29,65,100,59,129,65,215,163,14,65,8,172,131,65,215,163,
	252,64,99,109,229,208,24,65,201,118,16,65,98,211,77,8,65,170,241,4,65,68,139,232,64,68,139,252,64,219,249,190,64,254,212,252,64,98,72,225,178,64,86,14,253,64,219,249,166,64,141,151,254,64,188,116,155,64,240,167,0,65,98,102,102,158,64,125,63,5,65,221,
	36,162,64,193,202,9,65,82,184,166,64,162,69,14,65,98,180,200,186,64,10,215,33,65,174,71,221,64,4,86,48,65,33,176,2,65,205,204,56,65,98,178,157,3,65,168,198,53,65,33,176,4,65,205,204,50,65,109,231,5,65,10,215,47,65,98,117,147,10,65,152,110,36,65,12,2,
	17,65,59,223,25,65,229,208,24,65,201,118,16,65,99,109,254,212,133,65,217,206,199,64,98,147,24,134,65,70,182,167,64,201,118,132,65,139,108,135,64,156,196,128,65,27,47,85,64,98,12,2,113,65,39,49,168,63,10,215,77,65,244,253,84,188,27,47,41,65,0,0,0,0,98,
	82,184,234,64,37,6,129,61,4,86,146,64,80,141,71,64,121,233,146,64,119,190,203,64,98,51,51,163,64,18,131,200,64,59,223,179,64,23,217,198,64,106,188,196,64,219,249,198,64,98,133,235,249,64,158,239,199,64,84,227,21,65,86,14,221,64,14,45,42,65,43,135,254,
	64,98,43,135,64,65,182,243,217,64,209,34,93,65,246,40,196,64,12,2,123,65,18,131,196,64,98,236,81,128,65,106,188,196,64,160,26,131,65,35,219,197,64,254,212,133,65,217,206,199,64,99,101,0,0 };


	static const unsigned char cableIcon[] = { 110,109,174,71,121,65,139,108,156,65,98,76,55,113,65,252,169,157,65,240,167,104,65,236,81,158,65,168,198,95,65,186,73,158,65,98,131,192,48,65,221,36,158,65,229,208,10,65,158,239,138,65,160,26,11,65,23,217,102,65,98,90,100,11,65,242,210,55,65,242,210,
	49,65,84,227,17,65,23,217,96,65,14,45,18,65,98,158,239,135,65,201,118,18,65,109,231,154,65,72,225,56,65,131,192,154,65,109,231,103,65,98,57,180,154,65,98,16,120,65,115,104,152,65,104,145,131,65,188,116,148,65,231,251,137,65,108,244,253,203,65,233,38,
	217,65,108,41,92,179,65,152,110,234,65,108,174,71,121,65,139,108,156,65,99,109,248,83,147,65,195,245,221,65,98,217,206,87,65,141,151,234,65,227,165,247,64,201,118,228,65,100,59,119,64,55,137,198,65,98,215,163,128,192,252,169,137,65,199,75,23,63,12,2,
	43,62,190,159,92,65,0,0,0,0,98,178,157,95,65,0,0,0,0,154,153,95,65,0,0,0,0,117,147,98,65,111,18,3,59,98,250,126,189,65,217,206,119,62,147,24,251,65,211,77,56,65,254,212,214,65,14,45,166,65,98,164,112,211,65,147,24,173,65,174,71,207,65,6,129,179,65,238,
	124,202,65,41,92,185,65,108,76,55,184,65,223,79,159,65,98,240,167,198,65,53,94,134,65,238,124,200,65,207,247,75,65,49,8,187,65,88,57,24,65,98,152,110,172,65,246,40,192,64,137,65,144,65,139,108,111,64,12,2,93,65,139,108,111,64,98,178,157,215,64,27,47,
	117,64,207,247,115,63,182,243,65,65,219,249,134,64,61,10,152,65,98,145,237,200,64,205,204,186,65,248,83,57,65,250,126,204,65,221,36,130,65,213,120,197,65,108,248,83,147,65,195,245,221,65,99,101,0,0 };

	static const unsigned char foldIcon[] = { 110,109,123,20,116,65,141,151,178,65,108,109,231,139,65,106,188,160,65,108,211,77,204,65,209,34,225,65,108,176,114,186,65,0,0,243,65,108,109,231,139,65,188,116,196,65,108,135,22,57,65,217,206,243,65,108,41,92,21,65,182,243,225,65,108,123,20,116,65,141,
	151,178,65,99,109,250,126,11,66,233,38,77,65,108,250,126,11,66,100,59,141,65,108,0,0,0,0,100,59,141,65,108,0,0,0,0,233,38,77,65,108,250,126,11,66,233,38,77,65,99,109,135,22,139,65,164,112,189,64,108,176,114,186,65,0,0,0,0,108,211,77,204,65,121,233,14,
	64,108,182,243,156,65,152,110,2,65,108,135,22,139,65,246,40,38,65,108,41,92,21,65,66,96,21,64,108,135,22,57,65,23,217,206,61,108,135,22,139,65,164,112,189,64,99,101,0,0 };

	static const unsigned char gotoIcon[] = { 110,109,111,146,38,66,106,188,168,64,108,35,91,38,66,106,188,168,64,108,35,91,38,66,41,92,217,65,108,215,35,38,66,41,92,217,65,108,215,35,38,66,227,165,218,65,108,57,180,102,65,227,165,218,65,108,57,180,102,65,213,120,176,65,108,150,67,17,66,213,120,
	176,65,108,150,67,17,66,106,188,168,64,108,152,110,104,65,106,188,168,64,108,152,110,104,65,0,0,0,0,108,111,146,38,66,0,0,0,0,108,111,146,38,66,106,188,168,64,99,109,0,0,0,0,219,249,132,65,108,0,0,0,0,199,75,31,65,108,236,81,12,65,199,75,31,65,108,236,
	81,12,65,37,6,113,64,108,16,88,146,65,190,159,84,65,108,236,81,12,65,250,126,182,65,108,236,81,12,65,219,249,132,65,108,0,0,0,0,219,249,132,65,99,101,0,0 };

	static const unsigned char propertyIcon[] = { 110,109,0,0,0,0,0,0,0,0,108,152,110,39,66,0,0,0,0,108,152,110,39,66,57,180,102,65,108,174,71,39,66,57,180,102,65,108,174,71,39,66,201,246,29,66,98,172,28,39,66,178,157,33,66,119,62,36,66,43,135,36,66,68,139,32,66,45,178,36,66,108,127,106,220,63,45,178,
	36,66,98,141,151,78,63,43,135,36,66,10,215,163,61,147,152,33,66,227,165,27,61,201,246,29,66,108,227,165,27,61,57,180,102,65,108,0,0,0,0,57,180,102,65,108,0,0,0,0,0,0,0,0,99,109,223,207,25,66,57,180,102,65,108,133,235,89,64,57,180,102,65,108,133,235,89,
	64,94,58,23,66,108,223,207,25,66,94,58,23,66,108,223,207,25,66,57,180,102,65,99,109,254,212,10,65,39,49,226,65,108,236,81,6,66,39,49,226,65,108,236,81,6,66,209,34,7,66,108,254,212,10,65,209,34,7,66,108,254,212,10,65,39,49,226,65,99,109,254,212,10,65,
	121,233,153,65,108,236,81,6,66,121,233,153,65,108,236,81,6,66,244,253,197,65,108,254,212,10,65,244,253,197,65,108,254,212,10,65,121,233,153,65,99,101,0,0 };

	static const unsigned char zoom[] = { 110,109,223,79,184,65,254,212,218,65,98,47,221,141,65,63,53,245,65,76,55,37,65,166,155,246,65,238,124,167,64,231,251,212,65,98,90,100,139,191,113,61,171,65,41,92,239,191,154,153,43,65,92,143,122,64,233,38,149,64,98,66,96,213,64,248,83,227,63,244,253,
	40,65,193,202,161,61,80,141,105,65,111,18,131,58,98,33,176,108,65,0,0,0,0,8,172,108,65,0,0,0,0,168,198,111,65,111,18,131,58,98,127,106,200,65,57,180,72,62,43,7,5,66,10,215,67,65,82,184,224,65,125,63,176,65,98,178,157,222,65,125,63,180,65,137,65,220,65,
	111,18,184,65,8,172,217,65,94,186,187,65,108,137,193,26,66,180,200,11,66,108,215,163,10,66,102,230,27,66,108,223,79,184,65,254,212,218,65,99,109,137,65,106,65,104,145,85,64,98,121,233,214,64,41,92,95,64,96,229,208,62,254,212,80,65,236,81,144,64,180,200,
	164,65,98,176,114,252,64,18,131,214,65,211,77,139,65,213,120,228,65,61,10,182,65,45,178,188,65,98,215,163,210,65,111,18,162,65,109,231,219,65,76,55,105,65,221,36,203,65,188,116,33,65,98,244,253,187,65,76,55,193,64,106,188,153,65,129,149,83,64,137,65,
	106,65,104,145,85,64,99,101,0,0 };

	static const unsigned char errorIcon[] = { 110,109,186,73,129,65,23,217,150,64,108,225,122,129,65,250,126,54,65,108,55,137,55,65,113,61,129,65,108,145,237,152,64,152,110,129,65,108,182,243,253,60,164,112,55,65,108,0,0,0,0,14,45,154,64,108,219,249,150,64,166,155,196,60,108,141,151,54,65,0,0,0,
	0,98,133,235,79,65,135,22,201,63,125,63,105,65,233,38,73,64,186,73,129,65,23,217,150,64,99,109,27,47,181,64,37,6,17,64,108,158,239,15,64,188,116,183,64,108,115,104,17,64,127,106,40,65,108,152,110,182,64,96,229,94,65,108,43,135,40,65,166,155,94,65,108,
	244,253,94,65,119,190,39,65,108,82,184,94,65,47,221,180,64,108,35,219,39,65,59,223,15,64,98,147,24,14,65,137,65,16,64,8,172,232,64,215,163,16,64,27,47,181,64,37,6,17,64,99,109,55,137,1,65,229,208,206,64,108,137,65,38,65,66,96,133,64,108,162,69,64,65,
	115,104,185,64,108,80,141,27,65,139,108,1,65,108,162,69,64,65,221,36,38,65,108,137,65,38,65,246,40,64,65,108,55,137,1,65,164,112,27,65,108,203,161,185,64,246,40,64,65,108,154,153,133,64,221,36,38,65,108,61,10,207,64,139,108,1,65,108,154,153,133,64,115,
	104,185,64,108,203,161,185,64,66,96,133,64,108,55,137,1,65,229,208,206,64,99,101,0,0 };

	static const unsigned char profileIcon[] = { 110,109,72,225,30,66,0,0,0,0,98,248,211,103,66,197,32,48,62,39,177,149,66,201,118,98,65,217,142,156,66,33,48,0,66,98,195,117,161,66,174,71,51,66,193,202,152,66,131,192,106,66,84,35,134,66,10,151,135,66,98,70,182,96,66,166,219,156,66,109,103,27,66,170,
	241,163,66,113,61,196,65,231,59,152,66,98,201,118,254,64,20,174,138,66,61,10,39,192,98,16,73,66,59,223,15,63,197,32,2,66,98,158,239,111,64,238,124,107,65,47,221,163,65,115,104,209,62,217,206,26,66,66,96,229,59,98,252,41,28,66,111,18,131,58,37,134,29,
	66,111,18,131,186,72,225,30,66,0,0,0,0,99,109,111,18,29,66,100,59,17,65,98,33,176,203,65,223,79,19,65,12,2,71,65,254,212,157,65,113,61,26,65,61,138,5,66,98,100,59,183,64,158,239,81,66,162,69,189,65,96,101,147,66,252,169,53,66,156,196,138,66,98,246,168,
	102,66,246,232,133,66,250,190,135,66,4,214,96,66,94,58,139,66,190,31,47,66,98,57,244,143,66,4,86,215,65,250,126,104,66,25,4,20,65,233,166,30,66,100,59,17,65,98,190,31,30,66,76,55,17,65,154,153,29,66,76,55,17,65,111,18,29,66,100,59,17,65,99,109,244,253,
	241,65,231,251,102,66,108,90,100,200,65,231,251,102,66,108,27,47,60,66,18,131,172,65,108,2,171,81,66,70,182,172,65,108,244,253,241,65,231,251,102,66,99,109,78,226,76,66,31,133,101,66,98,27,175,65,66,31,133,101,66,23,217,56,66,133,235,97,66,59,95,50,66,
	94,186,90,66,98,90,228,43,66,49,136,83,66,233,166,40,66,207,119,74,66,233,166,40,66,43,135,63,66,98,233,166,40,66,72,225,37,66,207,119,52,66,80,13,25,66,160,26,76,66,80,13,25,66,98,137,193,87,66,80,13,25,66,39,177,96,66,129,149,28,66,115,232,102,66,227,
	165,35,66,98,197,32,109,66,70,182,42,66,106,60,112,66,66,224,51,66,106,60,112,66,215,35,63,66,98,106,60,112,66,215,35,74,66,92,15,109,66,186,73,83,66,70,182,102,66,123,148,90,66,98,53,94,96,66,59,223,97,66,137,193,87,66,31,133,101,66,78,226,76,66,31,
	133,101,66,99,109,14,45,77,66,244,253,85,66,98,55,9,83,66,244,253,85,66,176,114,87,66,41,220,83,66,115,104,90,66,147,152,79,66,98,59,95,93,66,254,84,75,66,29,218,94,66,223,207,69,66,29,218,94,66,61,10,63,66,98,29,218,94,66,162,69,56,66,162,69,93,66,236,
	209,50,66,178,29,90,66,27,175,46,66,98,195,245,86,66,80,141,42,66,137,65,82,66,231,123,40,66,6,1,76,66,231,123,40,66,98,137,193,69,66,231,123,40,66,90,100,65,66,49,136,42,66,133,235,62,66,209,162,46,66,98,170,113,60,66,106,188,50,66,63,53,59,66,53,94,
	56,66,63,53,59,66,43,135,63,66,98,63,53,59,66,205,76,70,66,82,184,60,66,106,188,75,66,125,191,63,66,10,215,79,66,98,168,198,66,66,164,240,83,66,131,64,71,66,244,253,85,66,14,45,77,66,244,253,85,66,99,109,4,86,219,65,119,190,33,66,98,172,28,198,65,119,
	190,33,66,127,106,181,65,252,41,30,66,113,61,169,65,6,1,23,66,98,98,16,157,65,10,215,15,66,219,249,150,65,94,186,6,66,219,249,150,65,236,81,247,65,98,219,249,150,65,27,47,225,65,150,67,157,65,242,210,206,65,242,210,169,65,113,61,192,65,98,90,100,182,
	65,240,167,177,65,88,57,199,65,53,94,170,65,223,79,220,65,53,94,170,65,98,236,81,242,65,53,94,170,65,236,209,1,66,43,135,177,65,203,33,8,66,23,217,191,65,98,176,114,14,66,2,43,206,65,166,155,17,66,201,118,224,65,166,155,17,66,94,186,246,65,98,166,155,
	17,66,53,94,6,66,236,81,14,66,25,132,15,66,119,190,7,66,217,206,22,66,98,2,43,1,66,154,25,30,66,182,243,240,65,119,190,33,66,4,86,219,65,119,190,33,66,99,109,145,237,219,65,76,55,18,66,98,137,65,231,65,76,55,18,66,229,208,239,65,154,25,16,66,166,155,
	245,65,53,222,11,66,98,102,102,251,65,215,163,7,66,199,75,254,65,2,43,2,66,199,75,254,65,145,237,246,65,98,199,75,254,65,137,65,233,65,102,102,251,65,27,47,222,65,166,155,245,65,82,184,213,65,98,229,208,239,65,150,67,205,65,236,81,231,65,49,8,201,65,
	184,30,220,65,49,8,201,65,98,133,235,208,65,49,8,201,65,188,116,200,65,248,83,205,65,94,186,194,65,133,235,213,65,98,0,0,189,65,18,131,222,65,209,34,186,65,227,165,233,65,209,34,186,65,236,81,247,65,98,209,34,186,65,250,126,2,66,207,247,188,65,201,246,
	7,66,203,161,194,65,98,16,12,66,98,199,75,200,65,2,43,16,66,94,186,208,65,76,55,18,66,145,237,219,65,76,55,18,66,99,101,0,0 };


}

juce::Path DspNetworkGraph::WrapperWithMenuBar::ActionButton::createPath(const String& url) const
{
	Path p;

	LOAD_PATH_IF_URL("probe", ScriptnodeIcons::probeIcon);
	LOAD_PATH_IF_URL("colour", ScriptnodeIcons::colourIcon);
	LOAD_PATH_IF_URL("cable", ScriptnodeIcons::cableIcon);
	LOAD_PATH_IF_URL("fold", ScriptnodeIcons::foldIcon);
	LOAD_PATH_IF_URL("deselect", EditorIcons::cancelIcon);
	LOAD_PATH_IF_URL("undo", EditorIcons::undoIcon);
	LOAD_PATH_IF_URL("redo", EditorIcons::redoIcon);
	LOAD_PATH_IF_URL("rebuild", ColumnIcons::moveIcon);
	LOAD_PATH_IF_URL("goto", ScriptnodeIcons::gotoIcon);
	LOAD_PATH_IF_URL("properties", ScriptnodeIcons::propertyIcon);
	LOAD_PATH_IF_URL("bypass", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	LOAD_PATH_IF_URL("profile", ScriptnodeIcons::profileIcon);

	LOAD_PATH_IF_URL("copy", SampleMapIcons::copySamples);
	LOAD_PATH_IF_URL("delete", SampleMapIcons::deleteSamples);
	LOAD_PATH_IF_URL("duplicate", SampleMapIcons::duplicateSamples);
	LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_PATH_IF_URL("zoom", ScriptnodeIcons::zoom);
	LOAD_PATH_IF_URL("error", ScriptnodeIcons::errorIcon);
	LOAD_PATH_IF_URL("export", HnodeIcons::freezeIcon);
	LOAD_PATH_IF_URL("wrap", HnodeIcons::mapIcon);
	LOAD_PATH_IF_URL("surround", HnodeIcons::injectNodeIcon);

	return p;
}

DspNetworkGraph::DspNetworkGraph(DspNetwork* n) :
	network(n),
	dataReference(n->getValueTree())
{
	network->addSelectionListener(this);
	rebuildNodes();
	setWantsKeyboardFocus(true);

	cableRepainter.setCallback(dataReference, { PropertyIds::Bypassed },
		valuetree::AsyncMode::Asynchronously,
		[this](ValueTree v, Identifier id)
	{
		if (v[PropertyIds::DynamicBypass].toString().isNotEmpty())
			repaint();
	});

	rebuildListener.setCallback(dataReference, valuetree::AsyncMode::Synchronously,
		[this](ValueTree c, bool)
	{
		if (c.getType() == PropertyIds::Node)
			triggerAsyncUpdate();
	});

	macroListener.setTypeToWatch(PropertyIds::Parameters);
	macroListener.setCallback(dataReference, valuetree::AsyncMode::Asynchronously,
		[this](ValueTree, bool)
	{
		this->rebuildNodes();
	});

	rebuildListener.forwardCallbacksForChildEvents(true);

	resizeListener.setCallback(dataReference, { PropertyIds::Folded, PropertyIds::ShowParameters },
		valuetree::AsyncMode::Asynchronously,
		[this](ValueTree, Identifier)
	{
		this->resizeNodes();
	});

	setOpaque(true);
}

DspNetworkGraph::~DspNetworkGraph()
{
	if (network != nullptr)
		network->removeSelectionListener(this);
}




bool DspNetworkGraph::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
		return Actions::deselectAll(*this);
	if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
		return Actions::deleteSelection(*this);
#if 0
	if ((key.isKeyCode('j') || key.isKeyCode('J')))
		return Actions::showJSONEditorForSelection(*this);
#endif
	if ((key.isKeyCode('z') || key.isKeyCode('Z')) && key.getModifiers().isCommandDown())
		return Actions::undo(*this);
	if ((key.isKeyCode('Y') || key.isKeyCode('Y')) && key.getModifiers().isCommandDown())
		return Actions::redo(*this);
	if ((key.isKeyCode('d') || key.isKeyCode('D')) && key.getModifiers().isCommandDown())
		return Actions::duplicateSelection(*this);
	if ((key.isKeyCode('n') || key.isKeyCode('N')))
		return Actions::showKeyboardPopup(*this, KeyboardPopup::Mode::New);
	if ((key).isKeyCode('f') || key.isKeyCode('F'))
		return Actions::foldSelection(*this);
	if ((key).isKeyCode('u') || key.isKeyCode('U'))
		return Actions::toggleFreeze(*this);
	if ((key).isKeyCode('p') || key.isKeyCode('P'))
		return Actions::editNodeProperty(*this);
	if ((key).isKeyCode('q') || key.isKeyCode('Q'))
		return Actions::toggleBypass(*this);
	if (((key).isKeyCode('c') || key.isKeyCode('C')))
		return Actions::toggleCableDisplay(*this);
	if (((key).isKeyCode('c') || key.isKeyCode('C')) && key.getModifiers().isCommandDown())
		return Actions::copyToClipboard(*this);
	if (key == KeyPress::upKey || key == KeyPress::downKey)
		return Actions::arrowKeyAction(*this, key);

	return false;
}

void DspNetworkGraph::handleAsyncUpdate()
{
	rebuildNodes();
}

void DspNetworkGraph::rebuildNodes()
{
	addAndMakeVisible(root = dynamic_cast<NodeComponent*>(network->signalPath->createComponent()));
	resizeNodes();
}

void DspNetworkGraph::resizeNodes()
{
	auto b = network->signalPath->getPositionInCanvas({ UIValues::NodeMargin, UIValues::NodeMargin });
	setSize(b.getWidth() + 2 * UIValues::NodeMargin, b.getHeight() + 2 * UIValues::NodeMargin);
	resized();
}

void DspNetworkGraph::updateDragging(Point<int> position, bool copyNode)
{
	copyDraggedNode = copyNode;

	if (auto c = dynamic_cast<ContainerComponent*>(root.get()))
	{
		c->setDropTarget({});
	}

	if (auto hoveredComponent = root->getComponentAt(position))
	{
		auto container = dynamic_cast<ContainerComponent*>(hoveredComponent);

		if (container == nullptr)
			container = hoveredComponent->findParentComponentOfClass<ContainerComponent>();

		if (container != nullptr)
		{
			currentDropTarget = container;
			DBG(container->getName());
			auto pointInContainer = container->getLocalPoint(this, position);
			container->setDropTarget(pointInContainer);
		}
	}
}

void DspNetworkGraph::finishDrag()
{
	if (currentDropTarget != nullptr)
	{
		currentDropTarget->insertDraggedNode(currentlyDraggedComponent, copyDraggedNode);
		currentlyDraggedComponent = nullptr;
	}
}

void DspNetworkGraph::paint(Graphics& g)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xff1d1d1d)));
	return;
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF444444)));

	Colour lineColour = Colours::white;

	for (int x = 15; x < getWidth(); x += 10)
	{
		g.setColour(lineColour.withAlpha(((x - 5) % 100 == 0) ? 0.12f : 0.05f));
		g.drawVerticalLine(x, 0.0f, (float)getHeight());
	}

	for (int y = 15; y < getHeight(); y += 10)
	{
		g.setColour(lineColour.withAlpha(((y - 5) % 100 == 0) ? 0.12f : 0.05f));
		g.drawHorizontalLine(y, 0.0f, (float)getWidth());
	}
}

void DspNetworkGraph::resized()
{
	if (root != nullptr)
	{
		root->setBounds(getLocalBounds().reduced(UIValues::NodeMargin));
		root->setTopLeftPosition({ UIValues::NodeMargin, UIValues::NodeMargin });
	}

	if (auto sp = findParentComponentOfClass<ScrollableParent>())
		sp->centerCanvas();
}

template <class T> void fillChildComponentList(Array<T*>& list, Component* c)
{
	for (int i = 0; i < c->getNumChildComponents(); i++)
	{
		auto child = c->getChildComponent(i);

		if (!child->isShowing())
			continue;

		if (auto typed = dynamic_cast<T*>(child))
		{
			list.add(typed);
		}

		fillChildComponentList(list, child);
	}
}

static Colour getFadeColour(int index, int numPaths)
{
	if (numPaths == 0)
		return Colours::transparentBlack;

	auto hue = (float)index / (float)numPaths;

	return Colour::fromHSV(hue, 0.2f, 0.8f, 0.4f);
}

Colour getSpecialColour(Component* c, Colour defaultColour)
{
	if (NodeComponent* nc = c->findParentComponentOfClass<NodeComponent>())
	{
		if(nc->header.colour.getSaturation() != 0.0)
			return nc->header.colour;
	}

	return defaultColour;
}

void DspNetworkGraph::paintOverChildren(Graphics& g)
{
	Array<ModulationSourceBaseComponent*> modSourceList;
	fillChildComponentList(modSourceList, this);

	for (auto modSource : modSourceList)
	{
		if (!modSource->getSourceNodeFromParent()->isBodyShown())
			continue;

		auto start = getCircle(modSource, false);

		g.setColour(Colours::black);
		g.fillEllipse(start);
		g.setColour(Colour(0xFFAAAAAA));
		g.drawEllipse(start, 2.0f);
	}

	float alpha = showCables ? 1.0f : 0.1f;

	Array<ParameterSlider*> sliderList;
	fillChildComponentList(sliderList, this);

	Array<MultiOutputDragSource*> multiOutputList;
	fillChildComponentList(multiOutputList, this);

	for (auto sourceSlider : sliderList)
	{
		if (probeSelectionEnabled || sourceSlider->parameterToControl->isProbed)
		{
			auto b = getLocalArea(sourceSlider, sourceSlider->getLocalBounds()).reduced(1).toFloat();

			float alpha = 0.0f;

			if (probeSelectionEnabled)
			{
				g.setColour(Colours::white.withAlpha(0.06f));
				g.fillRoundedRectangle(b, 5.0f);
				alpha += 0.05f;
			}

			if (sourceSlider->parameterToControl->isProbed)
				alpha += 0.3f;

			Path p;
			p.loadPathFromData(ScriptnodeIcons::probeIcon, sizeof(ScriptnodeIcons::probeIcon));
			auto top = b.removeFromRight(17.0f).removeFromTop(17.0f).reduced(2.0f);
			PathFactory::scalePath(p, top);
			g.setColour(Colours::white.withAlpha(alpha));
			g.fillPath(p);
		}

		auto cableColour = getSpecialColour(sourceSlider, Colour(MIDI_PROCESSOR_COLOUR));
		
		if (auto macro = dynamic_cast<NodeContainer::MacroParameter*>(sourceSlider->parameterToControl.get()))
		{
			for (auto c : macro->connections)
			{
				for (auto targetSlider : sliderList)
				{
					auto target = targetSlider->parameterToControl;

					if (target == nullptr || !target->parent->isBodyShown())
						continue;

					if (c->matchesTarget(target))
					{
						auto start = getCircle(sourceSlider);
						auto end = getCircle(targetSlider);

						Colour hc = targetSlider->isMouseOver(true) ? Colours::red : Colour(0xFFAAAAAA);

						paintCable(g, start, end, cableColour, alpha, hc);
					}
				}
			}
		}
	}

	for (auto multiSource : multiOutputList)
	{
		for (auto s : sliderList)
		{
			if (multiSource->matchesParameter(s->parameterToControl))
			{
				auto start = getCircle(multiSource->asComponent(), false);
				auto end = getCircle(s);

				auto index = multiSource->getOutputIndex();
				auto numOutputs = multiSource->getNumOutputs();

				auto c = MultiOutputDragSource::getFadeColour(index, numOutputs).withAlpha(1.0f);

				auto cableColour = getSpecialColour(dynamic_cast<Component*>(multiSource), c);

				Colour hc = s->isMouseOver(true) ? Colours::red : Colour(0xFFAAAAAA);

				paintCable(g, start, end, c, alpha, hc);
			}
		}
	}

	

	for (auto modSource : modSourceList)
	{
		auto start = getCircle(modSource, false);

		auto cableColour = getSpecialColour(modSource, Colour(0xffbe952c));

		if (auto sourceNode = modSource->getSourceNodeFromParent())
		{
			if (!sourceNode->isBodyShown())
				continue;

			auto modTargets = sourceNode->getModulationTargetTree();

			for (auto c : modTargets)
			{
				for (auto s : sliderList)
				{
					if (s->parameterToControl == nullptr)
						continue;

					if (!s->parameterToControl->parent->isBodyShown())
						continue;

					auto parentMatch = s->parameterToControl->parent->getId() == c[PropertyIds::NodeId].toString();
					auto paraMatch = s->parameterToControl->getId() == c[PropertyIds::ParameterId].toString();

					if (parentMatch && paraMatch)
					{
						auto end = getCircle(s);

						Colour hc = s->isMouseOver(true) ? Colours::red : Colour(0xFFAAAAAA);

						paintCable(g, start, end, cableColour, alpha, hc);
						break;
					}
				}
			}
		}

	}

	Array<NodeComponent::Header*> bypassList;
	fillChildComponentList(bypassList, this);

	for (auto b : bypassList)
	{
		auto n = b->parent.node.get();

		if (n == nullptr)
			continue;

		if (!n->isBodyShown())
			continue;

		auto connection = n->getValueTree().getProperty(PropertyIds::DynamicBypass).toString();

		if (connection.isNotEmpty())
		{
			auto nodeId = connection.upToFirstOccurrenceOf(".", false, false);
			auto pId = connection.fromFirstOccurrenceOf(".", false, false);

			for (auto multiSource : multiOutputList)
			{
				if (!multiSource->getNode()->isBodyShown())
					continue;

				if (multiSource->getNode()->getId() == nodeId)
				{
					if (multiSource->getOutputIndex() == pId.getIntValue())
					{
						auto start = getCircle(multiSource->asComponent(), false);
						auto end = getCircle(&b->powerButton).translated(0.0, -60.0f);

						auto c = n->isBypassed() ? Colours::grey : Colour(SIGNAL_COLOUR).withAlpha(0.8f);

						Colour hc = b->isMouseOver(true) ? Colours::red : Colour(0xFFAAAAAA);

						paintCable(g, start, end, c, alpha, hc);
					}
				}
			}

			for (auto sourceSlider : sliderList)
			{
				auto c = n->isBypassed() ? Colours::grey : Colour(SIGNAL_COLOUR).withAlpha(0.8f);

				c = getSpecialColour(sourceSlider, c);

				if (!sourceSlider->parameterToControl->parent->isBodyShown())
					continue;

				if (sourceSlider->parameterToControl->getId() == pId &&
					sourceSlider->parameterToControl->parent->getId() == nodeId)
				{
					auto start = getCircle(sourceSlider);
					auto end = getCircle(&b->powerButton).translated(0.0, -60.0f);

					Colour hc = sourceSlider->isMouseOver(true) ? Colours::red : Colour(0xFFAAAAAA);

					paintCable(g, start, end, c, alpha, hc);
					break;
				}
			}
		}
	}

	Array<cable::dynamic::editor*> sendList;
	fillChildComponentList(sendList, this);

	for (auto s : sendList)
	{
		auto nc = s->findParentComponentOfClass<NodeComponent>();

		auto c = getSpecialColour(s, Colours::white);


		if (!nc->node->isBodyShown())
			continue;

		if (auto sn = s->getAsSendNode())
		{
			for (auto r : sendList)
			{
				nc = r->findParentComponentOfClass<NodeComponent>();

				if (!nc->node->isBodyShown())
					continue;

				if (auto rn = r->getAsReceiveNode())
				{
					if (&sn->cable == rn->source)
					{
						float deltaY = JUCE_LIVE_CONSTANT_OFF(-11.5f);
						float deltaXS = JUCE_LIVE_CONSTANT_OFF(-127.0f);
						float deltaXE = JUCE_LIVE_CONSTANT_OFF(-49.0f);

						auto start = getCircle(s, false).translated(deltaXS, deltaY);
						auto end = getCircle(r, false).translated(deltaXE, deltaY);

						if (start.getY() > end.getY())
							std::swap(start, end);

						Colour hc = r->isMouseOver(true) ? Colours::red : Colour(0xFFAAAAAA);

						paintCable(g, start, end, c, alpha, hc);
					}
				}
			}
		}
	}

	
}

scriptnode::NodeComponent* DspNetworkGraph::getComponent(NodeBase::Ptr node)
{
	Array<NodeComponent*> nodes;
	fillChildComponentList(nodes, this);

	for (auto nc : nodes)
		if (nc->node == node)
			return nc;

	return nullptr;
}

bool DspNetworkGraph::setCurrentlyDraggedComponent(NodeComponent* n)
{
	if (auto parentContainer = dynamic_cast<ContainerComponent*>(n->getParentComponent()))
	{
		n->setBufferedToImage(true);
		auto b = n->getLocalArea(parentContainer, n->getBounds());
		parentContainer->removeDraggedNode(n);
		addAndMakeVisible(currentlyDraggedComponent = n);

		n->setBounds(b);
		return true;
	}

	return false;
}




void DspNetworkGraph::Actions::selectAndScrollToNode(DspNetworkGraph& g, NodeBase::Ptr node)
{
	g.network->addToSelection(node, {});

	if (auto nc = g.getComponent(node))
	{
		auto viewport = g.findParentComponentOfClass<Viewport>();

		auto nodeArea = viewport->getLocalArea(nc, nc->getLocalBounds());
		auto viewArea = viewport->getViewArea();

		if (!viewArea.contains(nodeArea))
		{
			int deltaX = 0;
			int deltaY = 0;

			if (nodeArea.getX() < viewArea.getX())
				deltaX = nodeArea.getX() - viewArea.getX();
			else if (nodeArea.getRight() > viewArea.getRight() && viewArea.getWidth() > nodeArea.getWidth())
				deltaX = nodeArea.getRight() - viewArea.getRight();


			if (nodeArea.getY() < viewArea.getY())
				deltaY = nodeArea.getY() - viewArea.getY();
			else if (nodeArea.getBottom() > viewArea.getBottom() && viewArea.getHeight() > nodeArea.getHeight())
				deltaY = nodeArea.getBottom() - viewArea.getBottom();


			viewport->setViewPosition(viewArea.getX() + deltaX, viewArea.getY() + deltaY);

		}


	}
}

bool DspNetworkGraph::Actions::freezeNode(NodeBase::Ptr node)
{
	auto freezedId = node->getValueTree()[PropertyIds::FreezedId].toString();

	if (freezedId.isNotEmpty())
	{
		if (auto fn = dynamic_cast<NodeBase*>(node->getRootNetwork()->get(freezedId).getObject()))
		{
			node->getRootNetwork()->deselect(fn);

			auto newTree = fn->getValueTree();
			auto oldTree = node->getValueTree();
			auto um = node->getUndoManager();

			auto f = [oldTree, newTree, um]()
			{
				auto p = oldTree.getParent();

				int position = p.indexOf(oldTree);
				p.removeChild(oldTree, um);
				p.addChild(newTree, position, um);
			};

			MessageManager::callAsync(f);

			auto nw = node->getRootNetwork();
			auto s = [newTree, nw]()
			{
				auto newNode = nw->getNodeForValueTree(newTree);
				nw->deselectAll();
				nw->addToSelection(newNode, ModifierKeys());
			};

			MessageManager::callAsync(s);
		}

		return true;
	}

	auto freezedPath = node->getValueTree()[PropertyIds::FreezedPath].toString();

	if (freezedPath.isNotEmpty())
	{
		auto newNode = node->getRootNetwork()->create(freezedPath, node->getId() + "_freezed");

		if (auto nn = dynamic_cast<NodeBase*>(newNode.getObject()))
		{
			auto newTree = nn->getValueTree();
			auto oldTree = node->getValueTree();
			auto um = node->getUndoManager();

			auto f = [oldTree, newTree, um]()
			{
				auto p = oldTree.getParent();

				int position = p.indexOf(oldTree);
				p.removeChild(oldTree, um);
				p.addChild(newTree, position, um);
			};

			MessageManager::callAsync(f);

			auto nw = node->getRootNetwork();
			auto s = [newTree, nw]()
			{
				auto newNode = nw->getNodeForValueTree(newTree);
				nw->deselectAll();
				nw->addToSelection(newNode, ModifierKeys());
			};

			MessageManager::callAsync(s);
		}

		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::unfreezeNode(NodeBase::Ptr node)
{
#if 0
	if (auto hc = node->getAsRestorableNode())
	{
		// Check if there is already a node that was unfrozen

		for (auto n : node->getRootNetwork()->getListOfUnconnectedNodes())
		{
			if (n->getValueTree()[PropertyIds::FreezedId].toString() == node->getId())
			{
				auto newTree = n->getValueTree();
				auto oldTree = node->getValueTree();
				auto um = node->getUndoManager();

				auto f = [oldTree, newTree, um]()
				{
					auto p = oldTree.getParent();

					int position = p.indexOf(oldTree);
					p.removeChild(oldTree, um);
					p.addChild(newTree, position, um);
				};

				MessageManager::callAsync(f);

				auto nw = node->getRootNetwork();

				auto s = [newTree, nw]()
				{
					auto newNode = nw->getNodeForValueTree(newTree);
					nw->deselectAll();
					nw->addToSelection(newNode, ModifierKeys());
				};

				MessageManager::callAsync(s);

				return true;
			}
		}

		auto t = hc->getSnippetText();

		if (t.isNotEmpty())
		{
			auto newTree = ValueTreeConverters::convertBase64ToValueTree(t, true);
			newTree = node->getRootNetwork()->cloneValueTreeWithNewIds(newTree);
			newTree.setProperty(PropertyIds::FreezedPath, node->getValueTree()[PropertyIds::FactoryPath], nullptr);
			newTree.setProperty(PropertyIds::FreezedId, node->getId(), nullptr);

			{
				auto oldTree = node->getValueTree();
				auto um = node->getUndoManager();

				auto newNode = node->getRootNetwork()->createFromValueTree(true, newTree, true);

				auto f = [oldTree, newTree, um]()
				{
					auto p = oldTree.getParent();

					int position = p.indexOf(oldTree);
					p.removeChild(oldTree, um);
					p.addChild(newTree, position, um);
				};

				MessageManager::callAsync(f);

				auto nw = node->getRootNetwork();

				auto s = [newNode, nw]()
				{
					nw->deselectAll();
					nw->addToSelection(newNode, ModifierKeys());
				};

				MessageManager::callAsync(s);
			}
		}

		return true;
	}
#endif

	return false;
}

bool DspNetworkGraph::Actions::toggleBypass(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	if (selection.isEmpty())
		return false;

	bool oldState = selection.getFirst()->isBypassed();

	for (auto n : selection)
	{
		n->setBypassed(!oldState);
	}

	return true;
}

bool DspNetworkGraph::Actions::toggleFreeze(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	if (selection.isEmpty())
		return false;

	auto f = selection.getFirst();

#if 0
	if (auto r = f->getAsRestorableNode())
	{
		unfreezeNode(f);
		return true;
	}
	else if (freezeNode(f))
	{
		return true;
	}
#endif

	return false;
}

bool DspNetworkGraph::Actions::toggleProbe(DspNetworkGraph& g)
{
	g.toggleProbeMode();
	return true;
}

bool DspNetworkGraph::Actions::setRandomColour(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	auto sat = JUCE_LIVE_CONSTANT_OFF(0.5f);
	auto br = JUCE_LIVE_CONSTANT_OFF(0.7f);

	auto c = Colour::fromHSV(Random::getSystemRandom().nextFloat(), sat, br, 1.0f);
	auto v = (int64)c.getARGB();

	for (auto n : selection)
	{
		n->getValueTree().setProperty(PropertyIds::NodeColour, v, g.network->getUndoManager());
	}

	return true;
}

bool DspNetworkGraph::Actions::copyToClipboard(DspNetworkGraph& g)
{
	if (auto n = g.network->getSelection().getFirst())
	{
		g.getComponent(n)->handlePopupMenuResult((int)NodeComponent::MenuActions::ExportAsSnippet);
		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::toggleCableDisplay(DspNetworkGraph& g)
{
	g.showCables = !g.showCables;
	g.repaint();
	return true;
}

bool DspNetworkGraph::Actions::toggleCpuProfiling(DspNetworkGraph& g)
{
	auto& b = g.network->getCpuProfileFlag();
	b = !b;
	
	g.enablePeriodicRepainting(b);

	return true;
}

bool DspNetworkGraph::Actions::editNodeProperty(DspNetworkGraph& g)
{
	if (auto n = g.network->getSelection().getFirst())
	{
		g.getComponent(n)->handlePopupMenuResult((int)NodeComponent::MenuActions::EditProperties);
		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::foldSelection(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	if (selection.isEmpty())
		return false;

	auto shouldBeFolded = !(bool)selection.getFirst()->getValueTree()[PropertyIds::Folded];

	for (auto n : selection)
		n->setValueTreeProperty(PropertyIds::Folded, shouldBeFolded);

	return true;
}

bool DspNetworkGraph::Actions::arrowKeyAction(DspNetworkGraph& g, const KeyPress& k)
{
	auto node = g.network->getSelection().getFirst();

	if (node == nullptr || g.network->getSelection().size() > 1)
		return false;

	auto network = g.network;

	bool swapAction = k.getModifiers().isShiftDown();

	if (swapAction)
	{
		auto swapWithPrev = k == KeyPress::upKey;

		auto tree = node->getValueTree();
		auto parent = tree.getParent();
		auto index = node->getIndexInParent();

		if (swapWithPrev)
			parent.moveChild(index, index - 1, node->getUndoManager());
		else
			parent.moveChild(index, index + 1, node->getUndoManager());

		return true;
	}
	else
	{


		auto selectPrev = k == KeyPress::upKey;
		auto index = node->getIndexInParent();

		if (selectPrev)
		{
			auto container = dynamic_cast<NodeContainer*>(node->getParentNode());

			if (container == nullptr)
				return false;

			if (index == 0)
				selectAndScrollToNode(g, node->getParentNode());
			else
			{
				auto prevNode = container->getNodeList()[index - 1];

				if (auto prevContainer = dynamic_cast<NodeContainer*>(prevNode.get()))
				{
					if (auto lastChild = prevContainer->getNodeList().getLast())
					{
						selectAndScrollToNode(g, lastChild);
						return true;
					}
				}

				selectAndScrollToNode(g, prevNode);
				return true;
			}


			return true;
		}
		else
		{
			auto container = dynamic_cast<NodeContainer*>(node.get());

			if (container != nullptr && node->isBodyShown())
			{
				if (auto firstChild = container->getNodeList()[0])
				{
					selectAndScrollToNode(g, firstChild);
					return true;
				}
			}

			container = dynamic_cast<NodeContainer*>(node->getParentNode());

			if (container == nullptr)
				return false;

			if (auto nextSibling = container->getNodeList()[index + 1])
			{
				selectAndScrollToNode(g, nextSibling);
				return true;
			}

			node = node->getParentNode();

			container = dynamic_cast<NodeContainer*>(node->getParentNode());

			if (container == nullptr)
				return false;

			index = node->getIndexInParent();

			if (auto nextSibling = container->getNodeList()[index + 1])
			{
				selectAndScrollToNode(g, nextSibling);
				return true;
			}
		}
	}

	return false;
}

bool DspNetworkGraph::Actions::showKeyboardPopup(DspNetworkGraph& g, KeyboardPopup::Mode)
{
	auto firstInSelection = g.network->getSelection().getFirst();

	NodeBase::Ptr containerToLookFor;

	int addPosition = -1;

	bool somethingSelected = dynamic_cast<NodeContainer*>(firstInSelection.get()) != nullptr;
	bool stillInNetwork = somethingSelected && firstInSelection->getParentNode() != nullptr;


	if (somethingSelected && stillInNetwork)
		containerToLookFor = firstInSelection;
	else if (firstInSelection != nullptr)
	{
		containerToLookFor = firstInSelection->getParentNode();
		addPosition = firstInSelection->getIndexInParent() + 1;
	}

	Array<ContainerComponent*> list;

	fillChildComponentList(list, &g);

	bool mouseOver = g.isMouseOver(true);

	if (mouseOver)
	{
		int hoverPosition = -1;
		NodeBase* hoverContainer = nullptr;

		for (auto nc : list)
		{
			auto thisAdd = nc->getCurrentAddPosition();

			if (thisAdd != -1)
			{
				hoverPosition = thisAdd;
				hoverContainer = nc->node;
				break;
			}
		}

		if (hoverPosition != -1)
		{
			containerToLookFor = nullptr;
			addPosition = -1;
		}
	}

	for (auto nc : list)
	{
		auto thisAddPosition = nc->getCurrentAddPosition();


		bool containerIsSelected = nc->node == containerToLookFor;
		bool nothingSelectedAndAddPositionMatches = (containerToLookFor == nullptr && thisAddPosition != -1);

		if (containerIsSelected || nothingSelectedAndAddPositionMatches)
		{
			if (addPosition == -1)
				addPosition = thisAddPosition;

			KeyboardPopup* newPopup = new KeyboardPopup(nc->node, addPosition);

			auto midPoint = nc->getInsertRuler(addPosition);

			auto sp = g.findParentComponentOfClass<ScrollableParent>();

			auto r = sp->getLocalArea(nc, midPoint.toNearestInt());



			sp->setCurrentModalWindow(newPopup, r);
		}
	}

	return true;
}

bool DspNetworkGraph::Actions::duplicateSelection(DspNetworkGraph& g)
{
	int insertIndex = 0;

	for (auto n : g.network->getSelection())
	{
		auto tree = n->getValueTree();
		insertIndex = jmax(insertIndex, tree.getParent().indexOf(tree));
	}

	for (auto n : g.network->getSelection())
	{
		auto tree = n->getValueTree();
		auto copy = n->getRootNetwork()->cloneValueTreeWithNewIds(tree);
		n->getRootNetwork()->createFromValueTree(true, copy, true);
		tree.getParent().addChild(copy, insertIndex, n->getUndoManager());
		insertIndex = tree.getParent().indexOf(copy);
	}

	return true;
}

bool DspNetworkGraph::Actions::deselectAll(DspNetworkGraph& g)
{
	g.network->deselectAll();

	return true;
}

bool DspNetworkGraph::Actions::deleteSelection(DspNetworkGraph& g)
{
	for (auto n : g.network->getSelection())
	{
		if (n == nullptr)
			continue;

		auto tree = n->getValueTree();
		tree.getParent().removeChild(tree, n->getUndoManager());
	}

	g.network->deselectAll();

	return true;
}

bool DspNetworkGraph::Actions::showJSONEditorForSelection(DspNetworkGraph& g)
{
	Array<var> list;
	NodeBase::List selection = g.network->getSelection();

	if (selection.size() != 1)
		return false;

	for (auto node : selection)
	{
		auto v = ValueTreeConverters::convertScriptNodeToDynamicObject(node->getValueTree());
		list.add(v);
	}

	JSONEditor* editor = new JSONEditor(var(list));

	editor->setEditable(true);

	auto callback = [&g, selection](const var& newData)
	{
		if (auto n = selection.getFirst())
		{
			if (newData.isArray())
			{
				auto newTree = ValueTreeConverters::convertDynamicObjectToScriptNodeTree(newData.getArray()->getFirst());
				n->getValueTree().copyPropertiesAndChildrenFrom(newTree, n->getUndoManager());
			}

		}

		return;
	};

	editor->setCallback(callback, true);
	editor->setName("Editing JSON");
	editor->setSize(400, 400);

	Component* componentToPointTo = &g;

	if (list.size() == 1)
	{
		if (auto fn = g.network->getSelection().getFirst())
		{
			Array<NodeComponent*> ncList;
			fillChildComponentList<NodeComponent>(ncList, &g);

			for (auto nc : ncList)
			{
				if (nc->node == fn)
				{
					componentToPointTo = nc;
					break;
				}
			}
		}
	}

	auto p = g.findParentComponentOfClass<ScrollableParent>();
	auto b = p->getLocalArea(componentToPointTo, componentToPointTo->getLocalBounds());

	p->setCurrentModalWindow(editor, b);

	return true;
}

bool DspNetworkGraph::Actions::undo(DspNetworkGraph& g)
{
	if (auto um = g.network->getUndoManager())
		return um->undo();

	return false;
}

bool DspNetworkGraph::Actions::redo(DspNetworkGraph& g)
{
	if (auto um = g.network->getUndoManager())
		return um->redo();

	return false;
}

DspNetworkGraph::ScrollableParent::ScrollableParent(DspNetwork* n)
{

	addAndMakeVisible(viewport);
	viewport.setViewedComponent(new DspNetworkGraph(n), true);
	addAndMakeVisible(dark);
	dark.setVisible(false);
	//context.attachTo(*this);
	setOpaque(true);
}

DspNetworkGraph::ScrollableParent::~ScrollableParent()
{
	//context.detach();
}

void DspNetworkGraph::ScrollableParent::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	if (e.mods.isCommandDown())
	{
		if (wheel.deltaY > 0)
			zoomFactor += 0.1f;
		else
			zoomFactor -= 0.1f;

		zoomFactor = jlimit(0.25f, 2.0f, zoomFactor);
		auto trans = AffineTransform::scale(zoomFactor);
		viewport.getViewedComponent()->setTransform(trans);
		centerCanvas();
	}
}

void DspNetworkGraph::ScrollableParent::resized()
{
	viewport.setBounds(getLocalBounds());
	dark.setBounds(getLocalBounds());
	centerCanvas();
}

void DspNetworkGraph::ScrollableParent::centerCanvas()
{
	auto contentBounds = viewport.getViewedComponent()->getLocalBounds();
	auto sb = getLocalBounds();

	int x = 0;
	int y = 0;

	if (contentBounds.getWidth() < sb.getWidth())
		x = (sb.getWidth() - contentBounds.getWidth()) / 2;

	if (contentBounds.getHeight() < sb.getHeight())
		y = (sb.getHeight() - contentBounds.getHeight()) / 2;

	viewport.setTopLeftPosition(x, y);
}

juce::Identifier NetworkPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

juce::Component* NetworkPanel::createContentComponent(int index)
{
	if (auto holder = dynamic_cast<DspNetwork::Holder*>(getConnectedProcessor()))
	{
		auto sa = holder->getIdList();

		auto id = sa[index];

		if (id.isNotEmpty())
		{
			auto network = holder->getOrCreate(id);

			return createComponentForNetwork(network);


		}
	}

	return nullptr;
}

void NetworkPanel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<ProcessorWithScriptingContent>(moduleList);
}

void NetworkPanel::fillIndexList(StringArray& sa)
{
	if (auto holder = dynamic_cast<DspNetwork::Holder*>(getConnectedProcessor()))
	{
		auto sa2 = holder->getIdList();

		sa.clear();
		sa.addArray(sa2);
	}
}

KeyboardPopup::Help::Help(DspNetwork* n) :
	renderer("", nullptr)
{
#if USE_BACKEND

	auto bp = dynamic_cast<BackendProcessor*>(n->getScriptProcessor()->getMainController_())->getDocProcessor();

	rootDirectory = bp->getDatabaseRootDirectory();
	renderer.setDatabaseHolder(bp);

	initGenerator(rootDirectory, bp);
	renderer.setImageProvider(new doc::ScreenshotProvider(&renderer));
	renderer.setLinkResolver(new doc::Resolver(rootDirectory));

#endif


}


void KeyboardPopup::Help::showDoc(const String& text)
{
	ignoreUnused(text);

#if USE_BACKEND
	if (text.isEmpty())
	{
		renderer.setNewText("> no search results");
		return;
	}

	String link;

	link << doc::ItemGenerator::getWildcard();
	link << "/list/";
	link << text.replace(".", "/");

	MarkdownLink url(rootDirectory, link);
	renderer.gotoLink(url);
	rebuild(getWidth());
#endif
}

bool KeyboardPopup::Help::initialised = false;

void KeyboardPopup::Help::initGenerator(const File& root, MainController* mc)
{
	ignoreUnused(root, mc);

#if USE_BACKEND
	if (initialised)
		return;

	auto bp = dynamic_cast<BackendProcessor*>(mc);

	static doc::ItemGenerator gen(root, *bp);
	gen.createRootItem(bp->getDatabase());

	initialised = true;
#endif
}


void KeyboardPopup::addNodeAndClose(String path)
{
	auto sp = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();

	auto container = dynamic_cast<NodeContainer*>(node.get());
	auto ap = addPosition;

	if (path.startsWith("ScriptNode"))
	{
		auto f = [sp, container, ap]()
		{
			sp->setCurrentModalWindow(nullptr, {});

			auto clipboard = SystemClipboard::getTextFromClipboard();
			auto data = clipboard.fromFirstOccurrenceOf("ScriptNode", false, false);
			auto newTree = ValueTreeConverters::convertBase64ToValueTree(data, true);

			if (newTree.isValid())
			{
				var newNode;
				auto network = container->asNode()->getRootNetwork();

				newNode = network->createFromValueTree(network->isPolyphonic(), newTree, true);

				container->assign(ap, newNode);

				network->deselectAll();
				network->addToSelection(dynamic_cast<NodeBase*>(newNode.getObject()), ModifierKeys());
			}

			sp->setCurrentModalWindow(nullptr, {});
		};

		MessageManager::callAsync(f);
	}
	else
	{
		auto f = [sp, path, container, ap]()
		{
			if (path.isNotEmpty())
			{
				var newNode;

				auto network = container->asNode()->getRootNetwork();

				newNode = network->get(path);

				if (!newNode.isObject())
					newNode = network->create(path, {});

				container->assign(ap, newNode);

				network->deselectAll();
				network->addToSelection(dynamic_cast<NodeBase*>(newNode.getObject()), ModifierKeys());
			}

			sp->setCurrentModalWindow(nullptr, {});
		};

		MessageManager::callAsync(f);
	}




}

bool KeyboardPopup::keyPressed(const KeyPress& k, Component*)
{
	if (k == KeyPress::F1Key)
	{
		buttonClicked(&helpButton);
		return true;
	}
	if (k == KeyPress::escapeKey)
	{
		addNodeAndClose({});
	}
	if (k == KeyPress::upKey)
	{
		auto pos = list.selectNext(false);
		scrollToMakeVisible(pos);
		nodeEditor.setText(list.getCurrentText(), dontSendNotification);
		updateHelp();
		return true;
	}
	else if (k == KeyPress::downKey)
	{
		auto pos = list.selectNext(true);
		scrollToMakeVisible(pos);
		nodeEditor.setText(list.getCurrentText(), dontSendNotification);
		updateHelp();
		return true;
	}
	else if (k == KeyPress::returnKey)
	{
		addNodeAndClose(list.getTextToInsert());
		return true;
	}

	return false;
}

KeyboardPopup::PopupList::Item::Item(const Entry& entry_, bool isSelected_) :
	entry(entry_),
	selected(isSelected_),
	deleteButton("delete", this, f)
{
	if (entry.t == ExistingNode)
		addAndMakeVisible(deleteButton);

	static const StringArray icons = { "clipboard", "oldnode", "newnode" };

	p = f.createPath(icons[(int)entry.t]);
}

void KeyboardPopup::PopupList::Item::buttonClicked(Button*)
{
	auto plist = findParentComponentOfClass<PopupList>();

	plist->network->deleteIfUnused(entry.insertString);

	MessageManager::callAsync([plist]()
	{
		plist->rebuildItems();
	});
}

void KeyboardPopup::PopupList::Item::mouseDown(const MouseEvent&)
{
	findParentComponentOfClass<PopupList>()->setSelected(this);
}

void KeyboardPopup::PopupList::Item::mouseUp(const MouseEvent& event)
{
	if (!event.mouseWasDraggedSinceMouseDown())
	{
		findParentComponentOfClass<KeyboardPopup>()->addNodeAndClose(entry.insertString);
	}
}

void KeyboardPopup::PopupList::Item::paint(Graphics& g)
{
	if (selected)
	{
		g.fillAll(Colours::white.withAlpha(0.2f));
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);
	}

	auto b = getLocalBounds().toFloat();

	auto icon = b.removeFromLeft(b.getHeight());

	PathFactory::scalePath(p, icon.reduced(6.0f));

	b.removeFromLeft(10.0f);

	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.8f));
	g.drawText(entry.displayName, b.reduced(3.0f), Justification::centredLeft);

	g.setColour(Colours::white.withAlpha(0.3f));
	g.fillPath(p);

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void KeyboardPopup::PopupList::Item::resized()
{
	deleteButton.setBounds(getLocalBounds().removeFromRight(getHeight()).reduced(3));
}



void DspNetworkGraph::WrapperWithMenuBar::addButton(const String& name)
{
	auto p = dynamic_cast<DspNetworkGraph*>(canvas.viewport.getViewedComponent());

	ActionButton* b = new ActionButton(p, name);

	if (name == "probe")
	{
		b->actionFunction = Actions::toggleProbe;
		b->stateFunction = [](DspNetworkGraph& g) { return g.probeSelectionEnabled; };
		b->setTooltip("Enable parameter list selection");
	}

	if (name == "colour")
	{
		b->actionFunction = Actions::setRandomColour;
		b->enabledFunction = selectionEmpty;
		b->setTooltip("Randomize colours for selection");
	}
	if (name == "cable")
	{
		b->actionFunction = Actions::toggleCableDisplay;
		b->stateFunction = [](DspNetworkGraph& g) { return g.showCables; };
		b->setTooltip("Show / Hide cables [C]");
	}
	if (name == "profile")
	{
		b->actionFunction = Actions::toggleCpuProfiling;
		b->stateFunction = [](DspNetworkGraph& g) { return g.network->getCpuProfileFlag(); };
		b->setTooltip("Activate CPU profiling");
	}
	if (name == "add")
	{
		b->actionFunction = [](DspNetworkGraph& g)
		{
			Actions::showKeyboardPopup(g, KeyboardPopup::New);
			return true;
		};

		b->enabledFunction = selectionEmpty;
		b->setTooltip("Create node after selection [N]");
	}
	if (name == "wrap")
	{
		b->enabledFunction = selectionEmpty;

		b->actionFunction = [](DspNetworkGraph& g)
		{
			PopupLookAndFeel plaf;
			PopupMenu m;
			m.setLookAndFeel(&plaf);

			m.addItem((int)NodeComponent::MenuActions::WrapIntoChain, "Wrap into chain");

			m.addItem((int)NodeComponent::MenuActions::WrapIntoFrame, "Wrap into frame processing container");

			m.addItem((int)NodeComponent::MenuActions::WrapIntoMulti, "Wrap into multichannel container");

			m.addItem((int)NodeComponent::MenuActions::WrapIntoSplit, "Wrap into split container");

			m.addItem((int)NodeComponent::MenuActions::WrapIntoOversample4, "Wrap into 4x oversample container");

			int result = m.show();

			Array<NodeComponent*> list;

			fillChildComponentList(list, &g);

			for (auto n : list)
			{
				if (n->isSelected())
				{
					n->handlePopupMenuResult(result);
					return true;
				}
			}

			return true;
		};
	}
	if (name == "error")
	{
		b->stateFunction = [](DspNetworkGraph& g)
		{
			return !g.network->getExceptionHandler().isOk();
		};

		b->setColour(TextButton::ColourIds::buttonOnColourId, Colour(0xFFAA4444));
		b->setTooltip("Select nodes with error");

		b->enabledFunction = b->stateFunction;

		b->startTimer(1000);

		b->actionFunction = [](DspNetworkGraph& g)
		{
			auto& exceptionHandler = g.network->getExceptionHandler();
			auto list = g.network->getListOfNodesWithType<NodeBase>(false);

			g.network->deselectAll();

			for (auto& n : list)
			{
				auto e = exceptionHandler.getErrorMessage(n);

				if (e.isNotEmpty())
					g.network->addToSelection(n, ModifierKeys::commandModifier);
			}

			return true;

		};
	}
	if (name == "zoom")
	{
		b->actionFunction = [](DspNetworkGraph& g)
		{
			g.setTransform({});
			return true;
		};

		b->setTooltip("Reset Zoom");
	}
	if (name == "fold")
	{
		b->actionFunction = Actions::foldSelection;
		b->stateFunction = [](DspNetworkGraph& g)
		{
			auto selection = g.network->getSelection();

			if (selection.isEmpty())
				return false;

			return (bool)selection.getFirst()->getValueTree()[PropertyIds::Folded];
		};
		b->enabledFunction = selectionEmpty;
		b->setTooltip("Fold the selected nodes [F]");
	}
	if (name == "deselect")
	{
		b->actionFunction = Actions::deselectAll;
		b->enabledFunction = selectionEmpty;
		b->setTooltip("Deselect all nodes [Esc]");
	}
	if (name == "undo")
	{
		b->actionFunction = Actions::undo;
		b->setTooltip("Undo the last action [Ctrl+Z]");
	}
	if (name == "redo")
	{
		b->actionFunction = Actions::redo;
		b->setTooltip("Redo the last action [Ctrl+Y]");
	}
	if (name == "copy")
	{
		b->actionFunction = Actions::copyToClipboard;
		b->enabledFunction = selectionEmpty;
		b->setTooltip("Copy nodes to clipboard [Ctrl+C]");
	}
	if (name == "delete")
	{
		b->actionFunction = Actions::deleteSelection;
		b->enabledFunction = selectionEmpty;
		b->setTooltip("Delete selected nodes [Del]");
	}
	if (name == "duplicate")
	{
		b->actionFunction = Actions::duplicateSelection;
		b->enabledFunction = selectionEmpty;
		b->setTooltip("Duplicate node [Ctrl+D]");
	}
	if (name == "bypass")
	{
		b->actionFunction = Actions::toggleBypass;
		b->enabledFunction = selectionEmpty;

		b->stateFunction = [](DspNetworkGraph& g)
		{
			if (auto f = g.network->getSelection().getFirst())
			{
				return !f->isBypassed();
			}

			return false;
		};

		b->setTooltip("Bypass the selected nodes");
	}
	if (name == "properties")
	{
		b->setTooltip("Show node properties [P]");
		b->actionFunction = Actions::editNodeProperty;
		b->enabledFunction = selectionEmpty;
	}
	if (name == "goto")
	{
		b->enabledFunction = selectionEmpty;

		b->actionFunction = [](DspNetworkGraph& g)
		{
			if (auto fn = g.network->getSelection().getFirst())
			{
				if (!fn->isBodyShown())
				{
					auto p = fn;

					while (p != nullptr)
					{
						p->setValueTreeProperty(PropertyIds::Folded, false);
						p = p->getParentNode();
					}
				}

				Actions::selectAndScrollToNode(g, fn);
			}

			return true;
		};
	}

	actionButtons.add(b);
	addAndMakeVisible(b);
}

}
