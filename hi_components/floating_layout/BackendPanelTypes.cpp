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

namespace hise
{
using namespace juce;


juce::Image PoolTableHelpers::getPreviewImage(const AudioSampleBuffer* buffer, float width)
{
	if (buffer == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	return HiseAudioThumbnail::createPreview(buffer, (int)width);
}

juce::Image PoolTableHelpers::getPreviewImage(const Image* img, float width)
{
	if (img == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	auto ratio = (float)img->getWidth() / (float)img->getHeight();

	if (img->getWidth() > width)
	{
		return img->rescaled((int)width, (int)(width / ratio));
	}
	else
	{
		if (img->getHeight() < 1600)
		{
			int heightToUse = jmin<int>(500, img->getHeight());

			return img->rescaled((int)((float)heightToUse * ratio), heightToUse);
		}
		else
		{
			// most likely a filmstrip, so crop it to show the first two strips...
			return img->getClippedImage({ 0, 0, img->getWidth(), img->getWidth() * 2 });

		}
	}
}

juce::Image PoolTableHelpers::getPreviewImage(const ValueTree* v, float width)
{
	if (v == nullptr)
		return PoolHelpers::getEmptyImage((int)width, 150);

	Array<Rectangle<int>> zones;

	auto totalArea = Rectangle<int>(0, 0, (int)width, 128);

	for (const auto& data : *v)
	{
		auto d = StreamingHelpers::getBasicMappingDataFromSample(data);

		int x = jmap((int)d.lowKey, 0, 128, 0, totalArea.getWidth());
		int w = jmap((int)(1 + d.highKey - d.lowKey), 0, 128, 0, totalArea.getWidth());
		int y = jmap((int)d.highVelocity, 128, 0, 0, totalArea.getHeight());
		int h = jmap((int)(1 + d.highVelocity - d.lowVelocity), 0, 128, 0, totalArea.getHeight() - 1);

		zones.add({ x, y, w, h });
	}

	Image img(Image::ARGB, (int)width, 128, true);

	Graphics g(img);

	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(totalArea, 1);

	for (auto z : zones)
	{
		g.fillRect(z);
		g.drawRect(z);
	}

	return img;
}

juce::Image PoolTableHelpers::getPreviewImage(const MidiFileReference* v, float width)
{
	auto f = v->getFile();

#if 0

	MemoryOutputStream mos;
	f.writeTo(mos);

	MemoryBlock mb = mos.getMemoryBlock();
	MemoryInputStream mis(mb, true);
#endif

	HiseMidiSequence seq;
	seq.loadFrom(f);

	auto l = seq.getRectangleList({ 0.0f, 0.0f, width, 200.0f });

	Image img(Image::PixelFormat::ARGB, (int)width, 200, true);
	Graphics g(img);

	g.setColour(Colours::white);

	for (auto note : l)
		g.fillRect(note);

	return img;
}

juce::Path PoolTableHelpers::Factory::createPath(const String& name) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(name);

	Path p;

	LOAD_PATH_IF_URL("preview", HiBinaryData::FrontendBinaryData::infoButtonShape);
	LOAD_PATH_IF_URL("reload", ColumnIcons::moveIcon);

	return p;
}


}