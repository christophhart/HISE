#include "HintedFeel.h"



#if 0 // JUCE_WINDOWS
#include "juce_core/native/juce_BasicNativeHeaders.h"

// adapted from Wouter Huysentruit's example
bool GetFontDataFromSystem(String faceName_in, std::vector<char>& data_out)
{
	bool result = false;
	HDC hdc = CreateCompatibleDC(NULL);
	if (hdc == NULL)
		return result;
	// give random height, we just want a pointer to the font file's data
	HFONT hFont = CreateFont(12,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_OUTLINE_PRECIS,
		CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY, VARIABLE_PITCH,faceName_in.toWideCharPointer());
	if (hFont == NULL) {
		DeleteDC(hdc);
		return result;
	}
	SelectObject(hdc, hFont);
	const size_t size = GetFontData(hdc, 0, 0, NULL, 0);
	if (size > 0)
	{
		char* buffer = new char[size];
		if (GetFontData(hdc, 0, 0, buffer, size) == size)
		{
			data_out.resize(size);
			memcpy(&data_out[0], buffer, size);
			result = true;
		}
		delete[] buffer;
	}
	DeleteObject(hFont);
	DeleteDC(hdc);
	return result;
}

#else

bool GetFontDataFromSystem(String faceName_in, std::vector<char>& data_out)
{
	// some day over the
	return false;
}

#endif

bool GetFontData(const String faceName_in, std::vector<char>& data_out)
{
	for (unsigned int i = 0; i < sizeof (protoFonts) / sizeof (protoFonts[0]); ++i)
		if (faceName_in==protoFonts[i].name)
		{
			data_out.resize(protoFonts[i].size);
			memcpy(&data_out[0], protoFonts[i].data, protoFonts[i].size);
			return true;
		}
	return GetFontDataFromSystem(faceName_in, data_out);
}

FontDataMap HintedFeel::faces = FontDataMap();

Typeface::Ptr HintedFeel::getTypefaceForFont (Font const& font)
{
	Typeface::Ptr tf;
	String faceName (font.getTypefaceName());

	if (faceName.endsWith("_hinted_"))
	{
		faceName = faceName.dropLastCharacters(8);
		// add typeface if not yet added
		if (faces.count(faceName)==0) {
			// for now each font is kept once in memory until program exit
			std::vector<char> *data = new std::vector<char>;
			faces[faceName] = data;
			if (GetFontData(faceName, *data))
				FreeTypeFaces::addFaceFromMemory(9.f, 18.f,true,&((*data)[0]),data->size());
		}
		// use freetype if font reading hasn't failed
		if (faces[faceName]->size()>0)
		{
			Font f (font);
			f.setTypefaceName (faceName);
			tf = FreeTypeFaces::createTypefaceForFont (f);
		}
	}
	// use JUCE rendering if not _hinted_ or freetype failed
	if (!tf) {
		Font f (font);
		f.setTypefaceName (faceName);
		tf = LookAndFeel::getTypefaceForFont (f);
	}

	return tf;
}

HintedFeel::~HintedFeel()
{
	// possibly use a static ReferenceCountedObjectPtr<FontDataMap> ?
	// the current implementation may be more efficient
}