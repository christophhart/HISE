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

namespace hise { using namespace juce;

namespace ScriptBroadcasterMapIcons
{
	static const unsigned char commentIcon[] = { 110,109,10,63,148,68,61,138,215,68,108,184,214,148,68,215,107,214,68,108,143,42,156,68,61,42,218,68,108,246,208,148,68,20,222,221,68,108,41,52,148,68,41,172,220,68,108,51,83,153,68,154,17,218,68,108,10,63,148,68,61,138,215,68,99,101,0,0 };

	static const unsigned char complexDataIcon[] = { 110,109,102,214,170,68,72,233,196,68,108,154,41,158,68,72,233,196,68,108,154,41,158,68,246,184,233,68,108,154,249,194,68,246,184,233,68,108,154,249,194,68,41,12,221,68,108,102,214,170,68,72,233,196,68,99,109,123,252,187,68,92,63,208,68,108,184,230,181,
68,154,41,202,68,108,133,83,188,68,205,188,195,68,108,31,173,182,68,102,22,190,68,108,174,15,200,68,102,22,190,68,108,174,15,200,68,246,120,207,68,108,72,105,194,68,143,210,201,68,108,123,252,187,68,92,63,208,68,99,101,0,0 };

	static const unsigned char valueIcon[] = { 110,109,188,212,54,68,225,130,220,68,98,190,223,48,68,0,232,216,68,188,68,49,68,31,141,211,68,231,3,56,68,195,45,208,68,98,102,46,63,68,82,152,204,68,23,209,74,68,82,152,204,68,150,251,81,68,195,45,208,68,98,4,38,89,68,225,194,211,68,4,38,89,68,41,148,
217,68,150,251,81,68,154,41,221,68,98,211,77,75,68,82,128,224,68,53,190,64,68,143,186,224,68,121,137,57,68,0,216,221,68,108,160,18,60,68,133,147,220,68,98,59,223,65,68,113,197,222,68,178,45,74,68,41,140,222,68,92,119,79,68,92,231,219,68,98,86,62,85,68,
215,3,217,68,86,62,85,68,51,83,212,68,92,119,79,68,174,111,209,68,98,115,176,73,68,41,140,206,68,10,79,64,68,41,140,206,68,16,136,58,68,174,111,209,68,98,205,44,53,68,113,29,212,68,6,201,52,68,174,87,216,68,139,92,57,68,10,63,219,68,108,188,212,54,68,
225,130,220,68,99,109,162,133,60,68,61,170,217,68,98,109,167,57,68,215,163,215,68,131,8,58,68,82,208,212,68,246,168,61,68,82,0,211,68,98,227,181,65,68,154,249,208,68,154,73,72,68,154,249,208,68,135,86,76,68,82,0,211,68,98,100,99,80,68,184,6,213,68,100,
99,80,68,164,80,216,68,135,86,76,68,10,87,218,68,98,0,200,72,68,20,30,220,68,201,70,67,68,195,85,220,68,152,62,63,68,113,253,218,68,108,129,109,71,68,195,229,214,68,108,4,190,68,68,20,142,213,68,108,162,133,60,68,61,170,217,68,99,101,0,0 };

	static const unsigned char moduleIcon[] = { 110,109,98,208,7,68,215,131,240,68,108,231,139,254,67,72,201,244,68,98,90,140,1,68,0,40,246,68,217,102,1,68,61,50,248,68,127,170,253,67,225,122,249,68,108,221,84,218,67,41,40,1,69,98,45,226,212,67,143,214,1,69,88,9,204,67,143,214,1,69,135,150,198,67,
41,40,1,69,108,49,216,178,67,164,96,253,68,98,96,101,173,67,215,3,252,68,96,101,173,67,195,205,249,68,49,216,178,67,72,113,248,68,108,178,45,214,67,215,155,239,68,98,229,80,219,67,225,82,238,68,121,121,227,67,82,64,238,68,221,244,232,67,133,99,239,68,
108,186,9,250,67,20,30,235,68,108,127,218,244,67,61,210,233,68,98,180,8,236,67,195,157,231,68,168,182,221,67,195,157,231,68,221,228,212,67,61,210,233,68,108,143,178,155,67,10,31,248,68,98,197,224,146,67,51,83,250,68,197,224,146,67,0,232,253,68,143,178,
155,67,20,14,0,69,108,49,168,187,67,205,12,4,69,98,252,121,196,67,10,39,5,69,8,204,210,67,10,39,5,69,211,157,219,67,205,12,4,69,108,16,104,10,68,31,205,249,68,98,246,208,14,68,164,152,247,68,246,208,14,68,41,4,244,68,16,104,10,68,174,207,241,68,108,98,
208,7,68,215,131,240,68,99,109,197,176,213,67,195,157,0,69,108,195,197,230,67,102,246,252,68,98,213,56,226,67,92,151,251,68,248,131,226,67,113,141,249,68,43,167,231,67,123,68,248,68,108,86,126,5,68,10,111,239,68,98,190,55,8,68,143,18,238,68,41,164,12,
68,143,18,238,68,145,93,15,68,10,111,239,68,108,188,60,25,68,184,94,244,68,98,37,246,27,68,133,187,245,68,37,246,27,68,154,241,247,68,188,60,25,68,102,78,249,68,108,236,145,7,68,236,17,1,69,98,98,0,5,68,61,182,1,69,8,236,0,68,174,191,1,69,205,92,252,
67,20,46,1,69,108,207,71,235,67,164,80,3,69,108,43,119,240,67,143,246,3,69,98,246,72,249,67,205,16,5,69,129,205,3,68,205,16,5,69,102,54,8,68,143,246,3,69,108,141,207,36,68,164,160,249,68,98,115,56,41,68,41,108,247,68,115,56,41,68,174,215,243,68,141,207,
36,68,51,163,241,68,108,188,212,20,68,195,165,233,68,98,215,107,16,68,72,113,231,68,209,66,9,68,72,113,231,68,236,217,4,68,195,165,233,68,108,137,129,208,67,61,242,247,68,98,190,175,199,67,184,38,250,68,190,175,199,67,51,187,253,68,137,129,208,67,174,
239,255,68,108,197,176,213,67,195,157,0,69,99,101,0,0 };

	static const unsigned char radioGroupIcon[] = { 110,109,250,158,20,68,113,173,214,68,98,250,158,20,68,72,41,213,68,115,40,18,68,195,237,211,68,190,31,15,68,195,237,211,68,98,27,23,12,68,195,237,211,68,131,160,9,68,72,41,213,68,131,160,9,68,113,173,214,68,98,131,160,9,68,236,49,216,68,27,23,12,68,31,
109,217,68,190,31,15,68,31,109,217,68,98,115,40,18,68,31,109,217,68,250,158,20,68,236,49,216,68,250,158,20,68,113,173,214,68,99,109,193,98,28,68,143,98,217,68,98,178,37,28,68,102,102,217,68,125,231,27,68,164,104,217,68,66,168,27,68,164,104,217,68,98,
139,164,24,68,164,104,217,68,252,49,22,68,92,47,216,68,252,49,22,68,113,173,214,68,98,252,49,22,68,215,43,213,68,139,164,24,68,143,242,211,68,66,168,27,68,143,242,211,68,98,248,171,30,68,143,242,211,68,135,30,33,68,215,43,213,68,135,30,33,68,113,173,
214,68,98,135,30,33,68,82,8,216,68,205,36,31,68,82,40,217,68,119,142,28,68,92,95,217,68,108,119,142,28,68,195,117,216,68,98,217,38,30,68,143,66,216,68,113,85,31,68,154,137,215,68,113,85,31,68,113,173,214,68,98,113,85,31,68,236,169,213,68,190,175,29,68,
10,215,212,68,66,168,27,68,10,215,212,68,98,213,160,25,68,10,215,212,68,18,251,23,68,236,169,213,68,18,251,23,68,113,173,214,68,98,18,251,23,68,72,177,215,68,213,160,25,68,41,132,216,68,66,168,27,68,41,132,216,68,98,33,232,27,68,41,132,216,68,119,38,
28,68,246,128,216,68,193,98,28,68,225,122,216,68,108,193,98,28,68,143,98,217,68,99,109,252,121,2,68,164,104,217,68,98,203,113,2,68,164,104,217,68,170,105,2,68,164,104,217,68,121,97,2,68,164,104,217,68,98,133,187,254,67,164,104,217,68,102,214,249,67,92,
47,216,68,102,214,249,67,113,173,214,68,98,102,214,249,67,215,43,213,68,133,187,254,67,143,242,211,68,121,97,2,68,143,242,211,68,98,47,101,5,68,143,242,211,68,190,215,7,68,215,43,213,68,190,215,7,68,113,173,214,68,98,190,215,7,68,215,35,216,68,186,137,
5,68,20,86,217,68,178,165,2,68,0,104,217,68,108,178,165,2,68,225,130,216,68,98,96,141,4,68,72,113,216,68,168,14,6,68,195,165,215,68,168,14,6,68,113,173,214,68,98,168,14,6,68,236,169,213,68,246,104,4,68,10,215,212,68,121,97,2,68,10,215,212,68,98,12,90,
0,68,10,215,212,68,147,104,253,67,236,169,213,68,147,104,253,67,113,173,214,68,98,147,104,253,67,72,177,215,68,12,90,0,68,41,132,216,68,121,97,2,68,41,132,216,68,98,170,105,2,68,41,132,216,68,219,113,2,68,41,132,216,68,252,121,2,68,215,131,216,68,108,
252,121,2,68,164,104,217,68,99,101,0,0 };

	static const unsigned char mouseIcon[] = { 110,109,23,1,25,68,143,130,188,68,108,246,56,6,68,184,238,193,68,108,244,237,8,68,184,70,196,68,98,117,67,11,68,41,76,198,68,213,112,16,68,195,253,198,68,199,123,20,68,51,211,197,68,108,145,157,24,68,154,161,196,68,98,131,168,28,68,10,119,195,68,166,
11,30,68,82,224,192,68,20,182,27,68,225,218,190,68,108,23,1,25,68,143,130,188,68,99,109,188,44,5,68,225,250,183,68,108,143,170,4,68,82,32,184,68,98,158,159,0,68,51,75,185,68,246,120,254,67,236,225,187,68,252,145,1,68,92,231,189,68,108,115,136,5,68,195,
85,193,68,108,211,93,13,68,225,18,191,68,108,86,118,12,68,143,74,190,68,98,215,131,11,68,195,61,190,68,82,160,10,68,72,249,189,68,63,29,10,68,0,136,189,68,108,8,124,7,68,246,64,187,68,98,246,248,6,68,92,207,186,68,25,244,6,68,143,74,186,68,158,87,7,68,
51,219,185,68,108,188,44,5,68,225,250,183,68,99,109,133,59,8,68,246,24,183,68,108,115,96,10,68,41,244,184,68,98,160,90,11,68,195,253,184,68,76,71,12,68,225,66,185,68,20,206,12,68,174,183,185,68,108,76,111,15,68,184,254,187,68,98,20,246,15,68,133,115,
188,68,109,247,15,68,123,252,188,68,2,139,15,68,113,109,189,68,108,156,108,16,68,246,48,190,68,108,164,80,24,68,236,233,187,68,108,45,90,20,68,51,123,184,68,98,172,4,18,68,195,117,182,68,76,215,12,68,41,196,181,68,90,204,8,68,10,239,182,68,108,133,59,
8,68,246,24,183,68,99,109,76,71,12,68,184,118,186,68,98,51,195,11,68,41,4,186,68,70,158,10,68,31,221,185,68,121,185,9,68,10,31,186,68,98,172,212,8,68,72,97,186,68,53,134,8,68,133,243,186,68,78,10,9,68,20,102,187,68,108,8,164,10,68,246,200,188,68,98,33,
40,11,68,51,59,189,68,14,77,12,68,143,98,189,68,219,49,13,68,82,32,189,68,98,168,22,14,68,102,222,188,68,31,101,14,68,215,75,188,68,6,225,13,68,154,217,187,68,108,76,71,12,68,184,118,186,68,99,101,0,0 };

	static const unsigned char tagIcon[] = { 110,109,43,55,18,68,193,106,145,67,108,113,45,18,68,193,106,170,67,98,113,45,18,68,193,106,170,67,117,107,0,68,184,238,205,67,35,139,242,67,160,58,220,67,98,119,254,240,67,76,199,221,67,215,227,238,67,63,165,222,67,209,178,236,67,18,163,222,67,98,203,
129,234,67,6,161,222,67,213,104,232,67,27,191,221,67,27,223,230,67,158,47,220,67,98,176,82,219,67,76,119,208,67,76,151,193,67,29,90,182,67,98,192,181,67,70,86,170,67,98,31,85,178,67,244,221,166,67,94,90,178,67,219,73,161,67,41,204,181,67,16,216,157,67,
98,66,32,196,67,25,132,143,67,23,57,230,67,70,214,90,67,23,57,230,67,70,214,90,67,108,82,64,10,68,70,214,90,67,98,246,160,14,68,70,214,90,67,113,45,18,68,49,8,105,67,113,45,18,68,2,139,122,67,108,113,45,18,68,39,33,129,67,108,211,77,11,68,39,33,129,67,
98,53,238,9,68,217,174,120,67,55,89,7,68,219,57,114,67,106,100,4,68,219,57,114,67,98,152,254,255,67,219,57,114,67,166,219,248,67,223,63,128,67,166,219,248,67,29,10,137,67,98,166,219,248,67,90,212,145,67,152,254,255,67,76,247,152,67,106,100,4,68,76,247,
152,67,98,49,64,7,68,76,247,152,67,111,194,9,68,111,242,149,67,29,42,11,68,193,106,145,67,108,43,55,18,68,193,106,145,67,99,109,45,250,23,68,80,173,132,67,108,160,146,3,68,80,173,132,67,108,160,146,3,68,111,34,142,67,108,45,250,23,68,111,34,142,67,108,
45,250,23,68,80,173,132,67,99,101,0,0 };

	static const unsigned char propertyIcon[] = { 110,109,14,29,184,67,215,27,180,68,98,12,242,186,67,215,27,180,68,152,62,189,67,10,175,180,68,152,62,189,67,41,100,181,68,98,152,62,189,67,154,25,182,68,12,242,186,67,205,172,182,68,14,29,184,67,205,172,182,68,98,16,72,181,67,205,172,182,68,133,251,178,
67,154,25,182,68,133,251,178,67,41,100,181,68,98,133,251,178,67,10,175,180,68,16,72,181,67,215,27,180,68,14,29,184,67,215,27,180,68,99,109,133,187,228,67,102,46,180,68,108,70,198,192,67,102,46,180,68,108,70,198,192,67,61,154,182,68,108,133,187,228,67,
61,154,182,68,108,133,187,228,67,102,46,180,68,99,109,14,29,184,67,31,85,176,68,98,12,242,186,67,31,85,176,68,152,62,189,67,82,232,176,68,152,62,189,67,113,157,177,68,98,152,62,189,67,225,82,178,68,12,242,186,67,20,230,178,68,14,29,184,67,20,230,178,
68,98,16,72,181,67,20,230,178,68,133,251,178,67,225,82,178,68,133,251,178,67,113,157,177,68,98,133,251,178,67,82,232,176,68,16,72,181,67,31,85,176,68,14,29,184,67,31,85,176,68,99,109,133,187,228,67,174,103,176,68,108,70,198,192,67,174,103,176,68,108,
70,198,192,67,133,211,178,68,108,133,187,228,67,133,211,178,68,108,133,187,228,67,174,103,176,68,99,109,14,29,184,67,10,111,172,68,98,12,242,186,67,10,111,172,68,152,62,189,67,236,1,173,68,152,62,189,67,92,183,173,68,98,152,62,189,67,123,108,174,68,12,
242,186,67,174,255,174,68,14,29,184,67,174,255,174,68,98,16,72,181,67,174,255,174,68,133,251,178,67,123,108,174,68,133,251,178,67,92,183,173,68,98,133,251,178,67,236,1,173,68,16,72,181,67,10,111,172,68,14,29,184,67,10,111,172,68,99,109,133,187,228,67,
72,129,172,68,108,70,198,192,67,72,129,172,68,108,70,198,192,67,31,237,174,68,108,133,187,228,67,31,237,174,68,108,133,187,228,67,72,129,172,68,99,101,0,0 };

	static const unsigned char otherBroadcasterIcon[] = { 110,109,197,88,100,68,72,201,131,68,108,176,178,83,68,123,236,118,68,108,176,114,66,68,102,22,132,68,108,197,24,83,68,72,105,140,68,108,197,88,100,68,72,201,131,68,99,109,150,179,98,68,31,5,126,68,108,25,148,92,68,129,229,119,68,108,59,255,99,68,94,122,
112,68,108,190,79,94,68,225,202,106,68,108,53,206,111,68,225,202,106,68,108,53,206,111,68,154,73,124,68,108,184,30,106,68,219,153,118,68,108,150,179,98,68,31,5,126,68,99,109,238,164,52,68,109,71,111,68,108,106,196,58,68,240,39,105,68,108,141,47,66,68,
18,147,112,68,108,10,223,71,68,150,227,106,68,108,10,223,71,68,236,97,124,68,108,147,96,54,68,236,97,124,68,108,16,16,60,68,143,178,118,68,108,238,164,52,68,109,71,111,68,99,101,0,0 };

	static const unsigned char dimIcon[] = { 110,109,0,128,141,68,16,128,15,68,108,61,2,136,68,16,128,15,68,108,61,2,136,68,184,166,38,68,108,0,128,141,68,184,166,38,68,108,0,128,141,68,16,128,15,68,99,109,51,11,166,68,92,79,10,68,108,236,201,168,68,221,204,0,68,108,215,195,158,68,18,115,234,67,
108,205,4,156,68,49,120,253,67,108,51,11,166,68,92,79,10,68,99,109,193,250,114,68,49,120,253,67,108,47,125,109,68,18,115,234,67,108,147,112,89,68,221,204,0,68,108,53,238,94,68,92,79,10,68,108,193,250,114,68,49,120,253,67,99,109,14,181,112,68,59,239,198,
67,98,166,59,113,68,201,54,159,67,225,194,128,68,182,147,126,67,72,193,138,68,182,147,126,67,98,236,233,148,68,182,147,126,67,154,41,157,68,240,71,160,67,154,41,157,68,2,235,200,67,98,154,41,157,68,53,142,241,67,236,233,148,68,37,70,9,68,72,193,138,68,
37,70,9,68,98,31,173,128,68,37,70,9,68,96,245,112,68,221,20,242,67,143,178,112,68,215,227,201,67,108,33,64,121,68,215,227,201,67,98,143,130,121,68,246,168,232,67,0,8,131,68,197,184,0,68,72,193,138,68,197,184,0,68,108,72,193,138,68,156,100,144,67,98,195,
29,131,68,156,100,144,67,121,201,121,68,45,162,168,67,117,67,121,68,59,239,198,67,108,14,181,112,68,59,239,198,67,99,109,47,125,109,68,18,99,167,67,108,193,250,114,68,244,93,148,67,108,53,238,94,68,217,110,122,67,108,147,112,89,68,106,60,144,67,108,47,
125,109,68,18,99,167,67,99,109,236,201,168,68,106,60,144,67,108,51,11,166,68,217,110,122,67,108,205,4,156,68,244,93,148,67,108,215,195,158,68,18,99,167,67,108,236,201,168,68,106,60,144,67,99,109,0,128,141,68,104,17,9,67,108,61,2,136,68,104,17,9,67,108,
61,2,136,68,8,172,101,67,108,0,128,141,68,8,172,101,67,108,0,128,141,68,104,17,9,67,99,101,0,0 };

	static const unsigned char andIcon[] = { 110,109,80,77,12,68,4,46,48,68,108,166,67,11,68,6,129,49,68,98,121,193,10,68,137,65,49,68,123,68,10,68,252,233,48,68,172,204,9,68,143,122,48,68,98,240,111,9,68,227,205,48,68,129,13,9,68,84,11,49,68,96,165,8,68,242,50,49,68,98,47,61,8,68,127,90,49,68,
55,193,7,68,86,110,49,68,121,49,7,68,86,110,49,68,98,8,20,6,68,86,110,49,68,129,61,5,68,20,30,49,68,178,173,4,68,178,125,48,68,98,70,62,4,68,186,1,48,68,135,6,4,68,2,115,47,68,135,6,4,68,137,209,46,68,98,135,6,4,68,168,62,46,68,78,50,4,68,160,186,45,
68,203,137,4,68,113,69,45,68,98,72,225,4,68,66,208,44,68,8,100,5,68,111,106,44,68,252,17,6,68,248,19,44,68,98,215,195,5,68,76,183,43,68,6,137,5,68,250,94,43,68,104,97,5,68,35,11,43,68,98,219,57,5,68,76,183,42,68,4,38,5,68,223,103,42,68,4,38,5,68,221,
28,42,68,98,4,38,5,68,78,146,41,68,129,93,5,68,221,28,41,68,106,204,5,68,139,188,40,68,98,100,59,6,68,41,92,40,68,121,217,6,68,248,43,40,68,168,166,7,68,248,43,40,68,98,156,108,8,68,248,43,40,68,76,7,9,68,135,94,40,68,184,118,9,68,150,195,40,68,98,53,
230,9,68,147,40,41,68,227,29,10,68,8,164,41,68,227,29,10,68,227,53,42,68,98,227,29,10,68,143,146,42,68,78,2,10,68,160,234,42,68,18,203,9,68,244,61,43,68,98,231,147,9,68,72,145,43,68,106,36,9,68,141,239,43,68,188,124,8,68,197,88,44,68,108,117,187,9,68,
14,253,45,68,98,246,224,9,68,94,186,45,68,72,1,10,68,100,99,45,68,90,28,10,68,33,248,44,68,108,55,169,11,68,193,82,45,68,98,170,129,11,68,92,223,45,68,76,95,11,68,53,70,46,68,29,66,11,68,92,135,46,68,98,238,36,11,68,115,200,46,68,178,5,11,68,217,254,
46,68,90,228,10,68,160,42,47,68,98,80,21,11,68,115,88,47,68,156,84,11,68,176,138,47,68,45,162,11,68,104,193,47,68,98,207,239,11,68,16,248,47,68,213,40,12,68,74,28,48,68,80,77,12,68,4,46,48,68,99,109,252,161,7,68,180,80,43,68,108,180,24,8,68,20,246,42,
68,98,49,112,8,68,94,178,42,68,248,155,8,68,43,111,42,68,248,155,8,68,123,44,42,68,98,248,155,8,68,57,244,41,68,217,134,8,68,90,196,41,68,172,92,8,68,188,156,41,68,98,127,50,8,68,47,117,41,68,121,249,7,68,88,97,41,68,154,177,7,68,88,97,41,68,98,199,107,
7,68,88,97,41,68,31,53,7,68,209,114,41,68,129,13,7,68,178,149,41,68,98,244,229,6,68,147,184,41,68,45,210,6,68,246,224,41,68,45,210,6,68,201,14,42,68,98,45,210,6,68,254,68,42,68,133,243,6,68,152,134,42,68,37,54,7,68,182,211,42,68,108,252,161,7,68,180,
80,43,68,99,109,20,246,6,68,63,45,45,68,98,23,145,6,68,66,96,45,68,129,69,6,68,195,157,45,68,133,19,6,68,162,229,45,68,98,137,225,5,68,129,45,46,68,131,200,5,68,233,118,46,68,131,200,5,68,236,193,46,68,98,131,200,5,68,180,32,47,68,141,231,5,68,20,110,
47,68,129,37,6,68,252,169,47,68,98,117,99,6,68,227,229,47,68,135,182,6,68,199,3,48,68,184,30,7,68,199,3,48,68,98,117,99,7,68,199,3,48,68,162,165,7,68,70,246,47,68,47,229,7,68,51,219,47,68,98,172,36,8,68,33,192,47,68,252,105,8,68,84,147,47,68,254,180,
8,68,205,84,47,68,108,20,246,6,68,63,45,45,68,99,101,0,0 };

	static const unsigned char notIcon[] = { 110,109,37,70,15,68,215,251,98,68,108,131,192,17,68,215,251,98,68,108,131,192,17,68,205,44,101,68,108,37,70,15,68,205,44,101,68,108,37,70,15,68,215,251,98,68,99,109,188,228,18,68,45,58,92,68,108,98,240,21,68,45,58,92,68,108,227,133,23,68,225,250,94,68,
108,98,16,25,68,45,58,92,68,108,74,20,28,68,45,58,92,68,108,84,75,25,68,141,143,96,68,108,125,87,28,68,205,44,101,68,108,41,60,25,68,205,44,101,68,108,207,119,23,68,221,76,98,68,108,117,179,21,68,205,44,101,68,108,221,156,18,68,205,44,101,68,108,2,179,
21,68,2,131,96,68,108,188,228,18,68,45,58,92,68,99,109,184,46,15,68,45,58,92,68,108,102,214,17,68,45,58,92,68,108,102,214,17,68,8,68,94,68,108,35,83,17,68,123,92,98,68,108,217,174,15,68,123,92,98,68,108,184,46,15,68,8,68,94,68,108,184,46,15,68,45,58,
92,68,99,101,0,0 };

	static const unsigned char zoomWidthIcon[] = { 110,109,205,156,217,67,51,35,225,68,108,205,156,217,67,0,104,228,68,108,92,127,197,67,164,96,223,68,108,205,156,217,67,72,89,218,68,108,205,156,217,67,20,158,221,68,108,242,138,4,68,20,158,221,68,108,242,138,4,68,72,89,218,68,108,170,153,14,68,164,96,
223,68,108,242,138,4,68,0,104,228,68,108,242,138,4,68,51,35,225,68,108,205,156,217,67,51,35,225,68,99,109,166,27,211,67,113,197,209,68,98,96,53,210,67,236,9,209,68,154,185,209,67,51,67,208,68,154,185,209,67,205,116,207,68,98,154,185,209,67,143,26,203,
68,74,220,223,67,236,145,199,68,129,69,241,67,236,145,199,68,98,76,87,1,68,236,145,199,68,164,104,8,68,143,26,203,68,164,104,8,68,205,116,207,68,98,164,104,8,68,82,232,208,68,207,159,7,68,41,68,210,68,72,65,6,68,92,111,211,68,108,129,197,14,68,154,177,
215,68,108,178,133,10,68,72,209,217,68,108,109,71,2,68,61,178,213,68,98,246,56,255,67,143,186,214,68,16,136,248,67,0,88,215,68,129,69,241,67,0,88,215,68,98,176,50,227,67,0,88,215,68,215,67,215,67,246,8,213,68,254,52,211,67,236,217,209,68,108,2,251,221,
67,225,106,209,68,98,188,20,225,67,236,81,211,68,211,141,232,67,154,169,212,68,129,69,241,67,154,169,212,68,98,51,195,252,67,154,169,212,68,199,11,3,68,123,84,210,68,199,11,3,68,205,116,207,68,98,199,11,3,68,113,149,204,68,51,195,252,67,82,64,202,68,
129,69,241,67,82,64,202,68,98,174,199,229,67,82,64,202,68,84,115,220,67,113,149,204,68,84,115,220,67,205,116,207,68,98,84,115,220,67,184,30,208,68,209,242,220,67,72,193,208,68,35,219,221,67,184,86,209,68,108,166,27,211,67,113,197,209,68,99,101,0,0 };

	static const unsigned char neighbourIcon[] = { 110,109,31,69,131,68,41,60,212,68,108,236,161,138,68,205,252,224,68,108,102,62,107,68,123,176,2,69,108,150,51,23,68,123,176,2,69,108,106,92,218,67,205,252,224,68,108,150,51,23,68,164,152,188,68,108,102,62,107,68,164,152,188,68,108,195,53,122,68,102,142,
201,68,108,139,252,93,68,41,180,209,68,98,170,217,80,68,195,133,203,68,213,168,60,68,133,19,202,68,129,61,44,68,246,208,206,68,98,168,46,24,68,51,155,212,68,172,76,17,68,225,114,225,68,72,225,28,68,143,122,235,68,98,227,117,40,68,236,129,245,68,162,37,
66,68,225,242,248,68,123,52,86,68,164,40,243,68,98,180,120,102,68,102,118,238,68,176,18,110,68,246,32,229,68,131,48,106,68,133,107,220,68,108,31,69,131,68,41,60,212,68,99,109,0,24,133,68,154,241,196,68,108,123,52,147,68,215,203,188,68,98,225,34,145,68,
123,4,180,68,102,238,148,68,31,141,170,68,215,35,157,68,0,208,165,68,98,133,43,167,68,113,5,160,68,51,3,180,68,102,118,163,68,113,205,185,68,20,126,173,68,98,0,152,191,68,113,133,183,68,10,39,188,68,31,93,196,68,92,31,178,68,174,39,202,68,98,113,253,
169,68,154,217,206,68,154,1,160,68,154,121,205,68,92,111,153,68,82,112,199,68,108,61,66,139,68,92,159,207,68,108,10,159,146,68,0,96,220,68,108,123,164,188,68,0,96,220,68,108,10,167,209,68,215,251,183,68,108,123,164,188,68,174,151,147,68,108,10,159,146,
68,174,151,147,68,108,246,56,123,68,215,251,183,68,108,0,24,133,68,154,241,196,68,99,101,0,0 };

static const unsigned char queueIcon[] = { 110,109,221,68,34,67,92,119,149,68,108,221,68,34,67,41,124,140,68,108,27,223,144,67,133,251,155,68,108,221,68,34,67,225,122,171,68,108,221,68,34,67,174,127,162,68,108,244,221,87,67,133,251,155,68,108,221,68,34,67,92,119,149,68,99,109,61,202,133,67,92,
119,149,68,108,61,202,133,67,41,124,140,68,108,233,134,197,67,133,251,155,68,108,61,202,133,67,225,122,171,68,108,61,202,133,67,174,127,162,68,108,168,150,160,67,133,251,155,68,108,61,202,133,67,92,119,149,68,99,109,186,25,239,67,92,119,149,68,108,186,
25,239,67,41,124,140,68,108,51,107,23,68,133,251,155,68,108,186,25,239,67,225,122,171,68,108,186,25,239,67,174,127,162,68,108,35,243,4,68,133,251,155,68,108,186,25,239,67,92,119,149,68,99,109,12,114,186,67,92,119,149,68,108,12,114,186,67,41,124,140,68,
108,184,46,250,67,133,251,155,68,108,12,114,186,67,225,122,171,68,108,12,114,186,67,174,127,162,68,108,119,62,213,67,133,251,155,68,108,12,114,186,67,92,119,149,68,99,101,0,0 };

}

namespace ScriptingObjects {



ScriptBroadcasterMap::MessageWatcher::MessageWatcher(ScriptBroadcasterMap& map) :
	parent(map)
{
	parent.filteredBroadcasters.clear();

	for (auto b : parent.allBroadcasters)
	{
		parent.filteredBroadcasters.add(b->metadata);
		times.add(LastTime(b));
	}

	parent.rebuild();
	parent.findParentComponentOfClass<ZoomableViewport>()->zoomToRectangle(parent.getLocalBounds());
	startTimer(300);
}



void ScriptBroadcasterMap::MessageWatcher::timerCallback()
{
	bool doSomething = false;

	for (auto& t : times)
	{
		if (t.hasChanged())
		{
			parent.filteredBroadcasters.removeAllInstancesOf(t.bc->metadata);
			doSomething = true;
		}
	}

	if (doSomething)
	{
		parent.rebuild();
		parent.findParentComponentOfClass<ZoomableViewport>()->zoomToRectangle(parent.getLocalBounds());
	}
}

ScriptBroadcasterMap::MessageWatcher::LastTime::LastTime(ScriptBroadcaster* b) :
	bc(b),
	prevTime(b->lastMessageTime)
{

}

bool ScriptBroadcasterMap::MessageWatcher::LastTime::hasChanged()
{
	if (bc != nullptr)
	{
		auto c = bc->lastMessageTime != prevTime;

		if (c)
			prevTime = bc->lastMessageTime;

		return c;
	}

	return false;
}







ScriptBroadcasterMap::ScriptBroadcasterMap(JavascriptProcessor* p_, bool active_) :
	ControlledObject(dynamic_cast<Processor*>(p_)->getMainController()),
	p(p_),
	factory(*this),
	active(active_)
{
	padding = 20;

	marginTop = 30;
	marginLeft = 30;
	marginRight = 30;
	marginBottom = 30;

	childLayout = ComponentWithPreferredSize::Layout::ChildrenAreRows;

	

	getMainController()->addScriptListener(this);

	factory.registerWithCreate<SimpleVarBody>();
	//factory.registerWithCreate<JSONBody>();

	setInterceptsMouseClicks(false, true);

	tagBroadcaster.addListener(*this, [](ScriptBroadcasterMap& m, Array<int64> unused) 
	{
		m.updateTagFilter();
	}, false);

	p->addCallbackObjectClearListener<ScriptBroadcasterMap>(*this, [](ScriptBroadcasterMap& m, bool)
	{
		m.children.clear();
		m.addChildWithPreferredSize(new PrefferedSizeWrapper<EmptyDisplay, 400, 400>("Recompiling..."));
		m.resetSize();
	});

	rebuild();
}



void ScriptBroadcasterMap::rebuild()
{
	availableTags.clear();

	allBroadcasters = createBroadcasterList();
	children.clear();

	if(!active)
	{
		addChildWithPreferredSize(new PrefferedSizeWrapper<EmptyDisplay, 400, 400>("Broadcaster map is deactivated."));
		resetSize();
		zoomToWidth();
		return;
	}

	for (auto br : allBroadcasters)
	{
		br->errorBroadcaster.addListener(*this, [](ScriptBroadcasterMap& m, ScriptBroadcaster::ItemBase* i, const String& e)
		{
			m.ok |= !e.isEmpty();
		});
	}
	
	for (auto b : allBroadcasters)
	{
		if (filteredBroadcasters.contains(b->metadata))
			continue;

		if(!b->metadata.visible)
			continue;


		auto be = new BroadcasterRow(factory, b);

		addChildWithPreferredSize(be);
	}

	if (children.isEmpty())
		addChildWithPreferredSize(new PrefferedSizeWrapper<EmptyDisplay, 400, 400>("No broadcasters available"));

	updateTagFilter();

	callRecursive<TagItem::TagButton>(this, [this](TagItem::TagButton* tb)
	{
		tb->setBroadcasterMap(this);
		return false;
	});

	callRecursive<ComponentWithMetadata>(this, [this](ComponentWithMetadata* tb)
	{
		for (auto tag : tb->metadata.tags)
			availableTags.addIfNotAlreadyThere(tag.toString());
		
		return false;
	});
}

void ScriptBroadcasterMap::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF282828));

    GlobalHiseLookAndFeel::draw1PixelGrid(g, this, getLocalBounds());
    
}

void ScriptBroadcasterMap::updateTagFilter()
{
	callRecursive<ComponentWithMetadata>(this, [&](ComponentWithMetadata* b)
	{
		auto matchesTag = currentTags.isEmpty();

		for (auto tag : currentTags)
		{
			if (matchesTag)
				break;

			matchesTag |= b->matchesTag(tag, tagFilterOptions);
		}

		auto asComponent = dynamic_cast<Component*>(b);
		
		if (tagFilterOptions.dimOpacity)
		{
			asComponent->setVisible(true);
			asComponent->setAlpha(matchesTag ? 1.0f : 0.1f);
		}
		else
		{
			asComponent->setAlpha(1.0f);
			asComponent->setVisible(matchesTag);
		}
			
		return false;
	});

	
	if (!tagFilterOptions.dimOpacity)
	{
		resetSize();

		if (currentTags.isEmpty())
			zoomToWidth();
		else
			showAll();
		
	}

	repaint();
}

void ScriptBroadcasterMap::showAll()
{
	if (auto vp = findParentComponentOfClass<ZoomableViewport>())
		vp->zoomToRectangle(getLocalBounds());
}

void ScriptBroadcasterMap::zoomToWidth()
{
	if (auto vp = findParentComponentOfClass<ZoomableViewport>())
	{
		auto b = vp->getLocalBounds().reduced(50);

		auto widthToUse = jmax(b.getWidth(), getLocalBounds().getWidth() + 100);

		vp->zoomToRectangle(getLocalBounds().withSizeKeepingCentre( widthToUse, b.getHeight()));
	}
}

void ScriptBroadcasterMap::forEachDebugInformation(DebugInformationBase::Ptr di, const std::function<void(DebugInformationBase::Ptr)>& f)
{
	f(di);

	for (int i = 0; i < di->getNumChildElements(); i++)
	{
		if (auto c = di->getChildElement(i))
		{
			f(di->getChildElement(i));
		}
		else
			jassertfalse;
		
	}
		
}

hise::ScriptingObjects::ScriptBroadcasterMap::BroadcasterList ScriptBroadcasterMap::createBroadcasterList()
{
	BroadcasterList list;

	for (int i = 0; i < p->getNumRegisteredCallableObjects(); i++)
	{
		if(auto b = p->getRegisteredCallableObject<ScriptBroadcaster>(i))
			list.addIfNotAlreadyThere(b);
	}
	
	return list;
}

void ScriptBroadcasterMap::scriptWasCompiled(JavascriptProcessor *processor)
{
	if (p == processor)
	{
		triggerAsyncUpdate();
	}
}

float getAlphaRecursive(Component* p, float startAlpha=1.0f)
{
	if (p == nullptr)
		return startAlpha;

	startAlpha *= p->getAlpha();

	return getAlphaRecursive(p->getParentComponent(), startAlpha);
}

void ScriptBroadcasterMap::paintCablesForOutputs(Graphics& g, EntryBase* b)
{
	if (!b->isShowing())
		return;

	auto outputBox = getLocalArea(b, b->getLocalBounds().reduced(3));
	outputBox = outputBox.removeFromRight(PinWidth);

	auto inputOpacity = getAlphaRecursive(b);

	RectangleList<float> outputPins;

	for (int i = 0; i < b->outputPins.size(); i++)
		outputPins.addWithoutMerging(outputBox.removeFromTop(PinWidth).toFloat());

	int outputIndex = 0;

	for (auto output : b->outputPins)
	{
		if (!output->isVisible())
			continue;

		auto outputOpacity = getAlphaRecursive(output);

		RectangleList<float> inputPins;

		auto inputBox = getLocalArea(output, output->getLocalBounds().reduced(3));
		inputBox = inputBox.removeFromLeft(PinWidth);

		for (int i = 0; i < output->inputPins.size(); i++)
			inputPins.addWithoutMerging(inputBox.removeFromTop(PinWidth).toFloat());

		auto inputIndex = output->inputPins.indexOf(b);

		auto start = outputPins.getRectangle(outputIndex);
		auto end = inputPins.getRectangle(inputIndex);

		auto thisAlpha = jmin(inputOpacity, outputOpacity);

		auto midPoint = GlobalHiseLookAndFeel::paintCable(g, start.reduced(7), end.reduced(7), Colours::white.withAlpha(thisAlpha), thisAlpha, Colours::grey.withAlpha(thisAlpha), true, false);

        ignoreUnused(midPoint);
        
		outputIndex++;
	}
}

void ScriptBroadcasterMap::paintOverChildren(Graphics& g)
{
	Component::callRecursive<EntryBase>(this, [&](EntryBase* b)
	{
		paintCablesForOutputs(g, b);
		return false;
	});
}

void ScriptBroadcasterMap::setShowComments(bool shouldShowComments)
{
	commentsShown = shouldShowComments;

	Component::callRecursive<CommentDisplay>(this, [this, shouldShowComments](CommentDisplay* c)
	{
		auto matchesTag = currentTags.isEmpty();

		for (auto h : currentTags)
			matchesTag |= c->matchesTag(h, tagFilterOptions);

		c->setVisible(shouldShowComments && matchesTag);
		return false;
	});

	resetSize();
}

struct ScriptBroadcasterMapViewport : public WrapperWithMenuBarBase
{
	struct TagEditor : public Component,
					   public ControlledObject,
					   public TextEditor::Listener
	{
		static constexpr int HeaderHeight = 40;

		using TagButton = ScriptBroadcasterMap::TagItem::TagButton;

		TagEditor(ScriptBroadcasterMap* b):
			Component("Tag Filter"),
			ControlledObject(b->getMainController()),
			map(b)
		{
			addAndMakeVisible(searchBar);
			searchBar.addListener(this);
			GlobalHiseLookAndFeel::setTextEditorColours(searchBar);

			searchBar.setEscapeAndReturnKeysConsumed(true);

			grabKeyboardFocusAsync();

			struct Item
			{
				Identifier id;
				Colour c;

				bool operator==(const Item& other) const
				{
					return id == other.id;
				}

				bool operator<(const Item& other) const
				{
					return id.toString() < other.id.toString();
				}
			};

			Array<Item> ids;

			callRecursive([&ids](ComponentWithMetadata* m)
			{
				for (auto newTag : m->metadata.tags)
				{
					ids.addIfNotAlreadyThere({ newTag, m->metadata.c });
				}

				return false;
			});

			ids.sort();

			int totalWidth = 0;

			for (auto id : ids)
			{
				auto nb = new TagButton(id.id, id.c, 14.0f);
				totalWidth += nb->getTagWidth() + 10;
				tags.add(nb);

				nb->setParentList(&tags);
				nb->setBroadcasterMap(map.getComponent());
				addAndMakeVisible(nb);
			}

			int numRows = 1;

			if (totalWidth > 400)
			{
				numRows = (totalWidth / 400) + 1;
				totalWidth = 400;
			}
				
			int totalHeight = 24 * numRows + HeaderHeight;

			setSize(jmax(200, totalWidth), totalHeight);
		}

		Component::SafePointer<ScriptBroadcasterMap> map;

		bool callRecursive(const std::function<bool(ComponentWithMetadata*)>& f)
		{
			if (map != nullptr)
			{
				return Component::callRecursive<ComponentWithMetadata>(map.getComponent(), f);
			}
			
			return false;
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().removeFromTop(HeaderHeight);

			g.setColour(Colours::white.withAlpha(0.05f));
			g.fillRect(b);

			Path p;

			p.loadPathFromData(EditorIcons::searchIcon, SIZE_OF_PATH(EditorIcons::searchIcon));
			p.applyTransform(AffineTransform::rotation(float_Pi));

			PathFactory::scalePath(p, b.removeFromLeft(b.getHeight()).reduced(10).toFloat());

			g.setColour(Colours::white.withAlpha(0.2f));
			g.fillPath(p);
		}

		void textEditorTextChanged(TextEditor&) override
		{
			resized();
		}

		void textEditorEscapeKeyPressed(TextEditor&) override
		{
			map->grabKeyboardFocusAsync();
			findParentComponentOfClass<FloatingTilePopup>()->deleteAndClose();
		}

		void textEditorReturnKeyPressed(TextEditor&) override
		{
			// select all tags;

			auto searchTerm = searchBar.getText().toLowerCase();

			for (auto t : tags)
			{
				bool isIncluded = !searchTerm.isEmpty() && t->id.toString().toLowerCase().contains(searchTerm);

				if (t->on)
					isIncluded = false;
				
				t->sendMessage(isIncluded);
			}
		}

		void resized() override
		{
			auto b = getLocalBounds();
			auto topBar = b.removeFromTop(HeaderHeight);

			topBar.removeFromLeft(HeaderHeight);

			searchBar.setBounds(topBar.reduced(8));

			auto currentRow = b.removeFromTop(24);

			auto searchTerm = searchBar.getText().toLowerCase();

			b.removeFromTop(5);

			for (auto t : tags)
			{
				bool isIncluded = searchTerm.isEmpty() || t->id.toString().toLowerCase().contains(searchTerm);

				t->setVisible(isIncluded);

				if (!isIncluded)
					continue;

				if (currentRow.getWidth() < t->getTagWidth())
					currentRow = b.removeFromTop(24);

				t->setBounds(currentRow.removeFromLeft(t->getTagWidth()));
			}
		}

		

		TagButton::List tags;

		TextEditor searchBar;

	};

	struct Factory : public PathFactory
	{
		Path createPath(const String& url) const override
		{
			Path p;

			LOAD_EPATH_IF_URL("watch", BackendBinaryData::ToolbarIcons::viewPanel);
			LOAD_PATH_IF_URL("clear", ColumnIcons::moveIcon);
			LOAD_EPATH_IF_URL("error", ScriptnodeIcons::errorIcon);
			LOAD_EPATH_IF_URL("zoom-fit", ScriptnodeIcons::zoomFit);
			LOAD_PATH_IF_URL("filter", ColumnIcons::filterIcon);
			LOAD_PATH_IF_URL("tags", ScriptBroadcasterMapIcons::tagIcon);
			LOAD_PATH_IF_URL("dim", ScriptBroadcasterMapIcons::dimIcon);
			LOAD_PATH_IF_URL("and", ScriptBroadcasterMapIcons::andIcon);
			LOAD_PATH_IF_URL("not", ScriptBroadcasterMapIcons::notIcon);
			LOAD_PATH_IF_URL("comment", ScriptBroadcasterMapIcons::commentIcon);
			LOAD_PATH_IF_URL("neighbour", ScriptBroadcasterMapIcons::neighbourIcon);
			LOAD_PATH_IF_URL("zoomwidth", ScriptBroadcasterMapIcons::zoomWidthIcon);
			LOAD_EPATH_IF_URL("active", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);

			return p;
		}
	};

	ScriptBroadcasterMap* map;

	ScriptBroadcasterMapViewport(ScriptBroadcasterMap* c) :
		WrapperWithMenuBarBase(c),
		map(c)
	{
		canvas.setMouseWheelScrollEnabled(true);
		canvas.setScrollOnDragEnabled(true);
		canvas.setMaxZoomFactor(5.0);

		setPostResizeFunction([](Component* c)
		{
			dynamic_cast<ScriptBroadcasterMap*>(c)->zoomToWidth();
		});

		rebuildAfterContentChange();
	};

	void rebuildAfterContentChange() override
	{

		addButton("active");

		addSpacer(10);

		addButton("tags");
		addButton("dim");
		addButton("neighbour");

		addSpacer(10);

		addButton("watch");
		addButton("clear");

		addSpacer(10);

		//addButton("filter");
		addButton("comment");
		addButton("showall");
		addButton("zoomwidth");
		addButton("error");
	}

	struct Actions
	{
		static bool toggleActive(ScriptBroadcasterMap& m)
		{
			m.active = !m.active;
			auto sp = m.findParentComponentOfClass<ScriptBroadcasterPanel>();
			jassert(sp != nullptr);
			sp->active = m.active;
			m.rebuild();
			return true;
		}

		static bool enableWatch(ScriptBroadcasterMap& m) { return true; }
		static bool gotoError(ScriptBroadcasterMap& m) 
		{ 
			auto vp = m.findParentComponentOfClass<ZoomableViewport>();
			
			Component* errorComponent = nullptr;

			Component::callRecursive<ScriptBroadcasterMap::EntryBase>(&m, [&](ScriptBroadcasterMap::EntryBase* e)
			{
				if (e->currentError.isNotEmpty())
				{
					errorComponent = e;
					return true;
				}

				return false;
			});

			if (errorComponent != nullptr)
			{
				auto zoomBounds = m.getLocalArea(errorComponent, errorComponent->getLocalBounds().expanded(50));

				vp->zoomToRectangle(zoomBounds);

			}

			return true; 
		}

		static bool isError(ScriptBroadcasterMap& m) { return !m.ok; }

		static bool toggleComments(ScriptBroadcasterMap& m) 
		{
			m.setShowComments(!m.showComments());
			m.findParentComponentOfClass<ZoomableViewport>()->zoomToRectangle(m.getLocalBounds());
			return false;
		}

		static bool showComments(ScriptBroadcasterMap& m) { return m.showComments(); }

		static bool hasTags(ScriptBroadcasterMap& m) { return !m.availableTags.isEmpty(); }

		static bool showAll(ScriptBroadcasterMap& m)
		{
			m.showAll();
			return false;
		}

		static bool zoomToWidth(ScriptBroadcasterMap& m)
		{
			m.zoomToWidth();
			return false;
		}
	};

	bool keyPressed(const KeyPress& k) override
	{
		if (TopLevelWindowWithKeyMappings::matches(this, k, TextEditorShortcuts::show_search))
		{
			using ActionButton = WrapperWithMenuBarBase::ActionButtonBase<ScriptBroadcasterMap, Factory>;

			getComponentWithName<ActionButton>("tags")->triggerClick(sendNotificationSync);
			return true;
		}

		return false;
	}

	void addButton(const juce::String& name) override
	{
		using ActionButton = WrapperWithMenuBarBase::ActionButtonBase<ScriptBroadcasterMap, Factory>;
		auto b = new ActionButton(map, name);

		if (name == "watch")
		{
			b->actionFunction = [](ScriptBroadcasterMap& m) 
			{ 
				if (m.currentMessageWatcher != nullptr)
					m.currentMessageWatcher = nullptr;
				else
					m.currentMessageWatcher = new ScriptBroadcasterMap::MessageWatcher(m);

				return false;
			};

			b->stateFunction = [](ScriptBroadcasterMap& e) { return e.currentMessageWatcher != nullptr; };
			b->setTooltip("Hide broadcasters until they are used");
		}
		if(name == "active")
		{
			b->actionFunction = Actions::toggleActive;
			b->stateFunction = [](ScriptBroadcasterMap& m) { return m.active; };
			b->setTooltip("Deactivate the map (saves performance when many broadcasters are used");
		}
		if (name == "comment")
		{
			b->actionFunction = Actions::toggleComments;
			b->stateFunction = Actions::showComments;
			b->setTooltip("Show comments");
		}

		if (name == "clear")
		{
			b->actionFunction = [](ScriptBroadcasterMap& m)
			{
				m.filteredBroadcasters.clear();
				m.rebuild();

				Actions::showAll(m);

				return false;
			};
			
			b->setTooltip("Refresh Map and show all broadcasters");
		}

		if (name == "tags")
		{
			b->setControlsPopup([this]() { return new TagEditor(this->map); });
			b->setTooltip("Show all tag filters in a popup");
			b->enabledFunction = Actions::hasTags;
		}

		if (name == "neighbour")
		{
			b->stateFunction = [](ScriptBroadcasterMap& m) { return m.tagFilterOptions.showNextSibling; };
			b->actionFunction = [](ScriptBroadcasterMap& m) { m.tagFilterOptions.showNextSibling = !m.tagFilterOptions.showNextSibling; m.updateTagFilter(); return false; };
			b->setTooltip("Show immediate neighbours of filtered items");
			b->enabledFunction = Actions::hasTags;
		}

		if (name == "and")
		{
			b->stateFunction = [](ScriptBroadcasterMap& m) { return m.tagFilterOptions.useAnd; };
			b->actionFunction = [](ScriptBroadcasterMap& m) { m.tagFilterOptions.useAnd = !m.tagFilterOptions.useAnd; m.updateTagFilter(); return false; };

			b->setTooltip("Use AND logic for tag filter");
		}

		if (name == "not")
		{
			b->stateFunction = [](ScriptBroadcasterMap& m) { return m.tagFilterOptions.useNot; };
			b->actionFunction = [](ScriptBroadcasterMap& m) { m.tagFilterOptions.useNot = !m.tagFilterOptions.useNot; m.updateTagFilter(); return false; };
			b->setTooltip("Invert tag selection (hide all selected tags)");
		}

		if (name == "dim")
		{
			b->stateFunction = [](ScriptBroadcasterMap& m) { return m.tagFilterOptions.dimOpacity; };
			b->actionFunction = [](ScriptBroadcasterMap& m) { m.tagFilterOptions.dimOpacity = !m.tagFilterOptions.dimOpacity; m.updateTagFilter(); return false; };
			b->setTooltip("Dim unselected items instead of hiding them");
			b->enabledFunction = Actions::hasTags;
		}

		if (name == "zoom-fit")
		{
			b->actionFunction = Actions::showAll;
			b->setTooltip("Zoom to fit");
		}
		if (name == "zoomwidth")
		{
			b->actionFunction = Actions::zoomToWidth;
			b->setTooltip("Zoom to width");
		}

		if (name == "error")
		{
			b->actionFunction = Actions::gotoError;
			b->stateFunction = Actions::isError;
			b->setColour(TextButton::ColourIds::buttonOnColourId, Colour(0xFFAA4444));
			
			b->setTooltip("Goto error item");


		}

		addAndMakeVisible(b);
		actionButtons.add(b);
	}

	void bookmarkUpdated(const juce::StringArray& list) override
	{
		
	}

	juce::ValueTree getBookmarkValueTree() override
	{
		return {};
	}

	
};

void ScriptBroadcasterPanel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<JavascriptProcessor>(moduleList);
}

Component* ScriptBroadcasterPanel::createContentComponent(int)
{
	if (auto jp = dynamic_cast<JavascriptProcessor*>(getConnectedProcessor()))
	{
		return new ScriptBroadcasterMapViewport(new ScriptBroadcasterMap(jp, active));
	}
		

	return nullptr;
}

ScriptBroadcasterPanel::ScriptBroadcasterPanel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{

}

juce::Identifier ScriptBroadcasterPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

juce::Path ScriptBroadcasterMap::ListenerEntry::createPath(const String& url) const
{
	Path p;

	LOAD_PATH_IF_URL("mouseevents", ScriptBroadcasterMapIcons::mouseIcon);
	LOAD_PATH_IF_URL("componentproperties", ScriptBroadcasterMapIcons::propertyIcon);
	LOAD_PATH_IF_URL("moduleparameter", ScriptBroadcasterMapIcons::moduleIcon);
	LOAD_PATH_IF_URL("radiogroup", ScriptBroadcasterMapIcons::radioGroupIcon);
	LOAD_PATH_IF_URL("broadcastersource", ScriptBroadcasterMapIcons::otherBroadcasterIcon);
	LOAD_PATH_IF_URL("componentvalue", ScriptBroadcasterMapIcons::valueIcon);
	LOAD_PATH_IF_URL("componentproperties", ScriptBroadcasterMapIcons::propertyIcon);
	LOAD_EPATH_IF_URL("scriptfunctioncalls", HiBinaryData::SpecialSymbols::scriptProcessor);

	if (p.isEmpty())
		p.loadPathFromData(ScriptBroadcasterMapIcons::complexDataIcon, sizeof(ScriptBroadcasterMapIcons::complexDataIcon));

	return p;
}

juce::Path ScriptBroadcasterMapFactory::createPath(const String& url) const
{
    Path p;

	LOAD_EPATH_IF_URL("bypass", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
    LOAD_PATH_IF_URL("goto", ColumnIcons::openWorkspaceIcon);
    LOAD_PATH_IF_URL("queue", ScriptBroadcasterMapIcons::queueIcon);
	LOAD_PATH_IF_URL("error", ColumnIcons::errorIcon);
    LOAD_EPATH_IF_URL("realtime", HnodeIcons::jit);
	LOAD_PATH_IF_URL("comment", ScriptBroadcasterMapIcons::commentIcon);
    return p;
}

int ScriptBroadcasterMap::EntryBase::addPinWidth(int innerWidth) const
{
	if (!inputPins.isEmpty())
		innerWidth += PinWidth;

	if (!outputPins.isEmpty())
		innerWidth += PinWidth;

	return innerWidth;
}

void ScriptBroadcasterMap::EntryBase::paintBackground(Graphics& g, Colour c, bool fill /*= true*/)
{
	auto b = getLocalBounds().toFloat().reduced(3.0f);

	if (currentError.isNotEmpty())
		c = Colour(HISE_ERROR_COLOUR);

	if (fill)
	{
		auto fillRect = getContentBounds(true).toFloat();

		if (!menubar.isEmpty())
			fillRect.removeFromTop(24);

		g.setGradientFill(ColourGradient(c.withMultipliedBrightness(1.1f), 0.0f, 0.0f, c.withMultipliedBrightness(0.9f), 0.0f, (float)getHeight(), false));
		g.fillRoundedRectangle(fillRect.reduced(5.0f), 2.0f);
	}
	else
	{
		g.setColour(Colour(0xFF333333));
		g.fillRect(b);

		if (currentError.isNotEmpty())
		{
			g.setColour(c.withAlpha(0.15f));
			g.fillRect(b);
		}
		

	}

	g.setColour(c.withAlpha(0.5f));

	g.drawRoundedRectangle(b, 3.0, 1.0);
}

void ScriptBroadcasterMap::EntryBase::connectToOutput(EntryBase* source)
{
	inputPins.addIfNotAlreadyThere(source);
	source->outputPins.addIfNotAlreadyThere(this);
	resetSize();
}



void ScriptBroadcasterMap::EntryBase::setCurrentError(const String& e)
{
	currentError = e;
	
	if (!hasErrorButton)
	{
		auto errorLocation = e.fromFirstOccurrenceOf("{", false, false);

		menubar.addButton("error", Justification::right, [errorLocation](Button* b, bool value)
		{
			auto synthChain = b->findParentComponentOfClass<ControlledObject>()->getMainController()->getMainSynthChain();
			DebugableObject::Helpers::gotoLocation(synthChain, errorLocation);
		});

		auto b = menubar.buttons.getLast();
		b->margin = 0;
		b->setColour(Colour(HISE_ERROR_COLOUR));

		menubar.resized();

		hasErrorButton = true;
	}

	repaint();
}

ScriptBroadcasterMap::TagItem::TagItem(const ScriptBroadcaster::Metadata& m):
	ComponentWithMetadata(m)
{
	tagIcon.loadPathFromData(ScriptBroadcasterMapIcons::tagIcon, sizeof(ScriptBroadcasterMapIcons::tagIcon));

	for (auto t : m.tags)
	{
		auto nt = new TagButton(t, m.c);
		nt->setParentList(&tags);
		addAndMakeVisible(nt);
		tags.add(nt);
	}
}

void ScriptBroadcasterMap::TagItem::paint(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.2f));
	g.fillPath(tagIcon);
}

void ScriptBroadcasterMap::TagItem::resized()
{
	auto b = getLocalBounds();

	auto tagArea = b.removeFromLeft(25).toFloat();

	PathFactory::scalePath(tagIcon, tagArea.reduced(5.0f));

	auto row = b.removeFromTop(24);

	for (auto t : tags)
	{
		if (row.getWidth() < t->getTagWidth())
			row = b.removeFromTop(24);

		t->setBounds(row.removeFromLeft(t->getTagWidth()));
	}
}

} // namespace ScriptingObjects
} // namespace hise
