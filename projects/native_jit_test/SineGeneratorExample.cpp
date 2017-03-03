class SineGenerator
{
public:

	void setFrequency(float newFreq)
	{
		freq = newFreq;
		uptimeDelta = (double)freq / sr * 2.0 * M_PI;
	}

	float getFrequency() const
	{
		return freq;

	}

	void prepareToPlay(double sampleRate, int samplesPerBlock) override
	{
		sr = sampleRate;
	}

	float process(float input)
	{
		const float v = sinf(float)uptime);

		uptime += uptimeDelta;

		return v;
	};

private:
	
	float freq = 440.0f;
	
	double sr = 44100.0;
	double uptimeDelta = 0.01;
	double uptime = 0.0;
};

const var voices = [];

for(var i = 0; i < numVoices; i++)
{
	voices.insert(new SineGenerator(8192, 1024));
};

function renderVoice(channels, voiceIndex)
{
	voices[voiceIndex] >> channels;

	return 1;
}