/** Template C File for DSP modules. 
*
*   Use this file to implement your DSP routines and load it with the embedded TCC compiler
*
*	If you put the code in the correct sections, it can be automatically converted to a CPP class.
*/


#include <tcclib.h>
#include <TccLibrary.h>

// Will be replaced by the correct CPP keyword when converting
#define CPP_CONST
#define CPP_OVERRIDE

// [INCLUDE SECTION] ****************************************************************************************

// [/INCLUDE SECTION]

// [PRIVATE MEMBER SECTION] *********************************************************************************
double uptime;
double uptimeDelta;
float gain;
// [/PRIVATE MEMBER SECTION]


// [CALLBACK SECTION] ***************************************************************************************
void initialise()
{
	uptime = 0.0;
    gain = 0.5f;
}

void release() {}

int getNumParameters() CPP_CONST CPP_OVERRIDE
{
    return 1;
}

float getParameter(int index) CPP_CONST CPP_OVERRIDE
{    
	return 1.0f;
}

void setParameter(int index, float newValue) CPP_OVERRIDE
{
    gain = newValue;
}

void prepareToPlay(double sampleRate, int samplesPerBlock) CPP_OVERRIDE
{
    uptimeDelta = 0.08;
}

void processBlock(float** data, int numChannels, int numSamples) CPP_OVERRIDE 
{
    float* d = data[0];

    for(int i = 0; i < numSamples; i++)
    {
        *d++ = sin(uptime);
        uptime += uptimeDelta;
    }

    vMultiplyScalar(data[0], gain, numSamples);

    if(numChannels == 2)
    {
        vCopy(data[1], data[0], numSamples);
    }
}
// [/CALLBACK SECTION]

// [MAIN SECTION] *******************************************************************************************
int main()
{
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