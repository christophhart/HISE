#define SAFE 

class HiseJitClass
{
public:


	const Buffer b(6);


	void init()
	{
		b[0] = 1.0f;
	};

	void prepareToPlay(double sampleRate, int blockSize)
	{
		// Setup the playback configuration here
	};

	/** The NativeJIT code for the additive synthesiser. */


double uptime = 0.0;
double uptimeDelta = 0.03;

// Buffer is a custom type which correlates to the Buffer type in Javascript
// Treat them like a float array (there is a buffer overrun protection)

const Buffer lastValues(6);

const float a = 0.99f;
const float invA = 0.01f;

float process(float input)
{
    const float uptimeFloat = (float)uptime;

    const float a0 = (lastValues[0]*0.99f + b[0]*0.01f);
    const float a1 = (lastValues[1]*0.99f + b[1]*0.01f);
    const float a2 = (lastValues[2]*0.99f + b[2]*0.01f);
    const float a3 = (lastValues[3]*0.99f + b[3]*0.01f);
    const float a4 = (lastValues[4]*0.99f + b[4]*0.01f);
    const float a5 = (lastValues[5]*0.99f + b[5]*0.01f);

    const float v0 = a0 * sinf(uptimeFloat);
    const float v1 = a1 * sinf(2.0f*uptimeFloat);
    const float v2 = a2 * sinf(3.0f*uptimeFloat);
    const float v3 = a3 * sinf(4.0f*uptimeFloat);
    const float v4 = a4 * sinf(5.0f*uptimeFloat);
    const float v5 = a5 * sinf(6.0f*uptimeFloat);
    
    lastValues[0] = a0;
    lastValues[1] = a1;
    lastValues[2] = a2;
    lastValues[3] = a3;
    lastValues[4] = a4;
    lastValues[5] = a5;

    uptime += uptimeDelta;

    return v0+v1+v2+v3+v4+v5;
};


private:

	// Define private variables here

};