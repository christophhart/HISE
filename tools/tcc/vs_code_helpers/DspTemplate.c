/** Template C File for DSP modules. 
*
*   Use this file to implement your DSP routines and load it with the embedded TCC compiler
*
*	If you put the code in the correct sections, it can be automatically converted to a CPP class.
*/

#include <TccLibrary.h>

// Will be replaced by the correct CPP keyword when converting
#define CPP_CONST
#define CPP_OVERRIDE

// [INCLUDE SECTION] ****************************************************************************************

// [/INCLUDE SECTION]

// [PRIVATE MEMBER SECTION] *********************************************************************************

// [/PRIVATE MEMBER SECTION]


// [CALLBACK SECTION] ***************************************************************************************
void initialise()
{

}

void release() {}

int getNumParameters() CPP_CONST CPP_OVERRIDE { return 1; }

float getParameter(int index) CPP_CONST CPP_OVERRIDE { return 1.0f; }

void setParameter(int index, float newValue) CPP_OVERRIDE {}

void prepareToPlay(double sampleRate, int samplesPerBlock) CPP_OVERRIDE {}

void processBlock(float** data, int numChannels, int numSamples) CPP_OVERRIDE 
{
    
}
// [/CALLBACK SECTION]

// [MAIN SECTION] *******************************************************************************************
int main()
{
	// A dummy test routine...
    initialise();

    float l[512];
    float r[512];

    float *channels[2] = {l, r};

    prepareToPlay(44100, 512);
    processBlock(channels, 2, 512);

    printString("Rendered successful");

    return 0;
}
// [/MAIN SECTION]