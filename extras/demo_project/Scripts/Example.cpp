/** The NativeJIT code for the additive synthesiser. */


double uptime = 0.0;
double uptimeDelta = 0.03;

// Buffer is a custom type which correlates to the Buffer type in Javascript
// Treat them like a float array (there is a buffer overrun protection)
Buffer b(6);
Buffer lastValues(6);

float a = 0.999f;
float invA = 0.001f;

float process(float input)
{
    const float uptimeFloat = (float)uptime;

    const float a0 = (lastValues[0]*a + b[0]*invA);
    const float a1 = (lastValues[1]*a + b[1]*invA);
    const float a2 = (lastValues[2]*a + b[2]*invA);
    const float a3 = (lastValues[3]*a + b[3]*invA);
    const float a4 = (lastValues[4]*a + b[4]*invA);
    const float a5 = (lastValues[5]*a + b[5]*invA);

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
