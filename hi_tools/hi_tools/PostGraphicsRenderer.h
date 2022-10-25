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

#pragma once

namespace hise {
using namespace juce;

/** This is a class that will apply some post effects on a image.

	There are some graphic operations which are not supported by the JUCE Graphics
	class so this is meant to be an addition.

	Currently included are:

	- masking with a path
	- adding noise
	- desaturating
	- adding blur (either box blur or gaussian blur)

	Since some of these operations will involve using buffers, it uses an internal
	stack system that fetches the correct internal data for each required operation
	to avoid reallocating.
*/
struct PostGraphicsRenderer
{
	/** This object will hold all internal buffers required for an operation. */
	struct Data
	{
		Data()
		{};

		~Data();

	private:

		void increaseIfNecessary(int minSize);

		void createPathImage(int width, int height);

		bool initGaussianBlur(int kernelSize, float sigma, int width, int height);

		struct WithoutAlphaConverter
		{
			WithoutAlphaConverter(Data& bf_, Image::BitmapData& bd_);

			uint8* getWithoutAlpha() { return bf.withoutAlpha; };

			~WithoutAlphaConverter();

			Data& bf;
			Image::BitmapData& bd;
		};


		friend struct PostGraphicsRenderer;

		Image pathImage;

		HeapBlock<uint8> withoutAlpha;
		int numPixels;
		void* pSpec = nullptr;

		uint8* pBuffer = nullptr;
		int bufSize = 0;
		int lastKernelSize = 0;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Data);
	};

	using DataStack = OwnedArray<Data>;

	PostGraphicsRenderer(DataStack& stackTouse, Image& image, float scaleFactor=1.0f);

	void reserveStackOperations(int numOperationsToAllocate);

	struct Pixel
	{
		Pixel() {};

		Pixel(uint8* ptr);

		uint8 unused = 0;
		uint8* a = &unused;
		uint8* r = &unused;
		uint8* g = &unused;
		uint8* b = &unused;
	};

	void desaturate();

	void applyMask(const Path& path, bool invert = false, bool scale=false);

	void addNoise(float noiseAmount);

	void gaussianBlur(int blur);

	void boxBlur(int blur);

	void stackBlur(int blur);

	void applyHSL(float h, float s, float l);

	void applyGamma(float g);

	void applyGradientMap(ColourGradient g);

	void applySharpness(int delta);

	void applySepia();

	void applyVignette(float amount, float radius, float falloff);

private:

	Data& getNextData();

	DataStack& stack;
	int stackIndex = 0;
	Image::BitmapData bd;
	Image img;
	float scaleFactor = 1.0f;
};


/** A component that can uses additional post rendering effects.

	This is a simple way of using the PostGraphics renderer.
*/
struct ComponentWithPostGraphicsRenderer : public Component
{
	void paint(Graphics& g) final override;

	/** By default, the effects will just applied to the things you'll render in this
		Component, but you can change this behaviour to include the parent's graphics
		by calling this method.
	*/
	void setDrawOverParent(bool shouldDrawOverParent);

protected:

	bool recursive = false;
	bool drawOverParent = true;

	/** Override this with your regular paint routine. It will draw on an internal
		image that will be passed to the applyPostEffect function. */
	virtual void paintBeforeEffect(Graphics& g) = 0;

	/** Override this and apply the post effects you want to use. */
	virtual void applyPostEffect(PostGraphicsRenderer& r) = 0;

	void setNumOperations(int numOperations);

private:

	int numOps = 0;
	Image img;
	PostGraphicsRenderer::DataStack stack;
};

}