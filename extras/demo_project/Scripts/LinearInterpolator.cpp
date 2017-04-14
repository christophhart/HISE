class LinearInterpolator
{
public:

	const Buffer b(2000);

	double counter;
	const double delta = 0.5;
	
	void init()
	{
		b[200] = 1.0f;
		b[201] = 1.0f;
		b[202] = 1.0f;
		
		b[1000] = 1.0f;
		b[1001] = 1.0f;
		b[1002] = 1.0f;

	};

	void prepareToPlay(double sampleRate, int blockSize)
	{
		// Setup the playback configuration here
	};

	float process(float input)
	{
		int index = (int)counter;

		int nextIndex = (index+1);

		const float thisValue = b[index];
		const float nextValue = b[nextIndex];

		const float a = (float)counter - (float)index;
		const float invA = 1.0f - a;

		counter = counter > 2000.0 ? 0.0 : counter + delta;

		return (a * thisValue + invA * nextValue);
	};

private:

	// Define private variables here

};