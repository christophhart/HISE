/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2022 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 7 End-User License
   Agreement and JUCE Privacy Policy.

   End User License Agreement: www.juce.com/juce-7-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

namespace juce
{

struct DefaultImageFormats
{
    static std::vector<std::unique_ptr<ImageFileFormat>>& get()
    {
        static DefaultImageFormats formats;
        return formats.formats;
    }

private:
    DefaultImageFormats() noexcept
    {
		formats.push_back (std::make_unique<PNGImageFormat>());
		formats.push_back (std::make_unique<JPEGImageFormat>());
		formats.push_back (std::make_unique<GIFImageFormat>());
    }

	std::vector<std::unique_ptr<ImageFileFormat>> formats;
};

void ImageFileFormat::registerFileFormat (std::unique_ptr<ImageFileFormat> format)
{
	DefaultImageFormats::get().push_back (std::move (format));
}

ImageFileFormat* ImageFileFormat::findImageFormatForStream (InputStream& input)
{
    const int64 streamPos = input.getPosition();

    for (auto& format : DefaultImageFormats::get())
    {
        const bool found = format->canUnderstand (input);
        input.setPosition (streamPos);

        if (found)
            return format.get();
    }

    return nullptr;
}

ImageFileFormat* ImageFileFormat::findImageFormatForFileExtension (const File& file)
{
    for (auto& format : DefaultImageFormats::get())
        if (format->usesFileExtension (file))
            return format.get();

    return nullptr;
}

//==============================================================================
Image ImageFileFormat::loadFrom (InputStream& input)
{
    if (ImageFileFormat* format = findImageFormatForStream (input))
        return format->decodeImage (input);

    return Image();
}

Image ImageFileFormat::loadFrom (const File& file)
{
    FileInputStream stream (file);

    if (stream.openedOk())
    {
        BufferedInputStream b (stream, 8192);
        return loadFrom (b);
    }

    return Image();
}

Image ImageFileFormat::loadFrom (const void* rawData, const size_t numBytes)
{
    if (rawData != nullptr && numBytes > 4)
    {
        MemoryInputStream stream (rawData, numBytes, false);
        return loadFrom (stream);
    }

    return Image();
}

} // namespace juce