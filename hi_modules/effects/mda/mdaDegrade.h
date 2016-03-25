#ifndef __mdaDegrade_H
#define __mdaDegrade_H


class mdaDegrade : public MdaEffect
{
public:

	

	mdaDegrade();
	~mdaDegrade();

	String getEffectName() const override { return "Degrade";};
	int getNumParameters() const override { return 6; };

	void process(float **, float **, int ) override {};
	void processReplacing(float **inputs, float **outputs, int sampleFrames) override;

	void setParameter(int index, float value) override;
	float getParameter(int index)  const override;
	void getParameterLabel(int index, char *label)  const override;
	void getParameterDisplay(int index, char *text) const override;
	void getParameterName(int index, char *text) const override;
	float filterFreq(float hz);
	void suspend();

protected:
	float fParam1;
	float fParam2;
	float fParam3;
	float fParam4;
	float fParam5;
	float fParam6;
	float fi2, fo2, clp, lin, lin2, g1, g2, g3, mode;
	float buf0, buf1, buf2, buf3, buf4, buf5, buf6, buf7, buf8, buf9;

	float buf0R, buf1R, buf2R, buf3R, buf4R, buf5R, buf6R, buf7R, buf8R, buf9R;

	int tn, tcount;

};

#endif
