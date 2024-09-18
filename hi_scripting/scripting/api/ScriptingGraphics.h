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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;

struct ScreenshotListener
{
	struct CachedImageBuffer : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<CachedImageBuffer>;

		CachedImageBuffer(Rectangle<int> sb) :
			data(Image::RGB, sb.getWidth(), sb.getHeight(), true)
		{}

		Image data;
	};

	virtual ~ScreenshotListener() {};

	virtual void makeScreenshot(const File& targetFile, Rectangle<float> area) {};

	/** This will be called on the scripting thread and can be used by listeners to prepare the screenshot. */
	virtual void prepareScreenshot() {};

	virtual int blockWhileWaiting() { return 0; };

	virtual void visualGuidesChanged() {};

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(ScreenshotListener);
};

namespace ScriptingObjects
{
	class ScriptShader : public ConstScriptingObject,
						 public ScreenshotListener
	{
	public:

		struct PreviewComponent;

		struct FileParser: public ControlledObject
		{
			using FileList = ReferenceCountedArray<ExternalScriptFile>;

			FileParser(ProcessorWithScriptingContent* p, bool addLineNumbers_, String& fileNameWithoutExtension, FileList& listToUse);

			StringArray getLines();

		private:

			const bool addLineNumbers = false;
			String createLinePointer(int i) const;

			ProcessorWithScriptingContent* sp;

			String loadFileContent();;

			ReferenceCountedArray<ExternalScriptFile>& includedFiles;

			String s;
			String fileNameWithoutExtension;
		};

		enum class BlendMode
		{
			_GL_ZERO = juce::gl::GL_ZERO, //< (0, 0, 0, 0)
			_GL_ONE = juce::gl::GL_ONE, //< (1, 1, 1, 1)
			_GL_SRC_COLOR = juce::gl::GL_SRC_COLOR, //< (Rs / kR, Gs / kG, Bs / kB, As / kA)
			_GL_ONE_MINUS_SRC_COLOR = juce::gl::GL_ONE_MINUS_SRC_COLOR, //< (1, 1, 1, 1) - (Rs / kR, Gs / kG, Bs / kB, As / kA)
			_GL_DST_COLOR = juce::gl::GL_DST_COLOR, //< (Rd / kR, Gd / kG, Bd / kB, Ad / kA)
			_GL_ONE_MINUS_DST_COLOR = juce::gl::GL_ONE_MINUS_DST_COLOR,  //< (1, 1, 1, 1) - (Rd / kR, Gd / kG, Bd / kB, Ad / kA)
			_GL_SRC_ALPHA = juce::gl::GL_SRC_ALPHA, //< (As / kA, As / kA, As / kA, As / kA)
			_GL_ONE_MINUS_SRC_ALPHA = juce::gl::GL_ONE_MINUS_SRC_ALPHA, //< (1, 1, 1, 1) - (As / kA, As / kA, As / kA, As / kA)
			_GL_DST_ALPHA = juce::gl::GL_DST_ALPHA, //< (Ad / kA, Ad / kA, Ad / kA, Ad / kA)
			_GL_ONE_MINUS_DST_ALPHA = juce::gl::GL_ONE_MINUS_DST_ALPHA, //< (1, 1, 1, 1) - (Ad / kA, Ad / kA, Ad / kA, Ad / kA)
			_GL_SRC_ALPHA_SATURATE = juce::gl::GL_SRC_ALPHA_SATURATE,
			numBlendModes = 11 // needs to be set manually...
		};

		ScriptShader(ProcessorWithScriptingContent* sp);;

		Identifier getObjectName() const override;

		// =============================================================== API Methods

		/** Loads a .glsl file from the script folder. */
		void setFragmentShader(String shaderFile);

		/** Sets an uniform variable to be used in the shader code. */
		void setUniformData(const String& id, var data);

		/** Sets the blend mode for the shader. */
		void setBlendFunc(bool enabled, int sFactor, int dFactor);

		/** Compresses the GLSL code and returns a encoded string snippet. */
		String toBase64();

		/** Compiles the code from the given base64 string. */
		void fromBase64(String b64);

		/** Returns a JSON object with the current OpenGL statistics. */
		var getOpenGLStatistics();

		/** If this is enabled, the shader will create a buffered image of the last rendering result. */
		void setEnableCachedBuffer(bool shouldEnableBuffer);

		/** Adds a preprocessor definition before the code and recompiles the shader (Empty string removes all preprocessors). */
		void setPreprocessor(String preprocessorString, var value);

		// ===========================================================================

		int blockWhileWaiting() override;

		void prepareScreenshot() override;

		void makeStatistics();

		void setEnableLineNumbers(bool shouldUseLineNumbers);

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		bool compiledOk() const;

		String getErrorMessage(bool verbose) const;

		void setCompileResult(Result compileResult);

		void setGlobalBounds(Rectangle<int> b, float sf);

		bool shouldWriteToBuffer() const;

		void renderWasFinished(ScreenshotListener::CachedImageBuffer::Ptr newData);

		ScreenshotListener::CachedImageBuffer::Ptr getScreenshotBuffer();

		struct Wrapper;


		ScreenshotListener::CachedImageBuffer::Ptr lastScreenshot;
		float scaleFactor = 1.0f;
		String shaderCode;
		NamedValueSet uniformData;
		var openGLStats;
		ScopedPointer<juce::OpenGLGraphicsContextCustomShader> shader;
		bool dirty = false;
		bool useLineNumbers = false;

		double iTime = 0;

		Rectangle<float> globalRect;
		Rectangle<float> localRect;

		ReferenceCountedArray<ExternalScriptFile> includedFiles;
		
		bool enableBlending = false;
		bool enableCache = false;

		BlendMode src = BlendMode::_GL_SRC_ALPHA;
		BlendMode dst = BlendMode::_GL_ONE_MINUS_SRC_ALPHA;

		static bool isRenderingScreenshot();

		struct ScopedScreenshotRenderer
		{
			ScopedScreenshotRenderer();

			~ScopedScreenshotRenderer();
		};

	private:

		NamedValueSet preprocessors;

		bool screenshotPending = false;
		static bool renderingScreenShot;

		String compiledCode;

		String shaderName;

		String getHeader();

		void compileRawCode(const String& code);

		Result processErrorMessage(const Result& r);

		Result r;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptShader);
	};

    class SVGObject: public ConstScriptingObject
    {
    public:
        
        SVGObject(ProcessorWithScriptingContent* p, const String& base64);
    
        Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("SVG"); }
        
        String getDebugName() const override { return "SVG"; };
        
        Component* createPopupComponent(const MouseEvent& e, Component* c) override { return nullptr; }
        
        bool isValid() const { return svg != nullptr; }
        
        void draw(Graphics& g, Rectangle<float> r, float opacity=1.0f)
        {
            if(isValid() && currentBounds != r)
            {
                svg->setTransformToFit(r, RectanglePlacement::centred);
                currentBounds = r;
            }
            
            if(isValid())
                svg->draw(g, opacity);
        }
        
    private:
        
        Rectangle<float> currentBounds;
        std::unique_ptr<Drawable> svg;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SVGObject);
    };

	class PathObject : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		PathObject(ProcessorWithScriptingContent* p);
		~PathObject();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Path"); }

		String getDebugName() const override { return "Path"; }

		String getDebugValue() const override {
			return p.getBounds().toString();
		}
		

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;



		// ============================================================================================================ API Methods

		/** Loads a path from a data array. */
		void loadFromData(var data);

		/** Clears the Path. */
		void clear();

		/** Starts a new Path. It does not clear the path, so use 'clear()' if you want to start all over again. */
		void startNewSubPath(var x, var y);

		/** Closes the Path. */
		void closeSubPath();

		/** Adds a line to [x,y]. */
		void lineTo(var x, var y);

		/** Adds a quadratic bezier curve with the control point [cx,cy] and the end point [x,y]. */
		void quadraticTo(var cx, var cy, var x, var y);

		/** Adds a cubic bezier curve with two sets of control point arrays [cx1,cy1] and [cx2,cy2], and the end point [x,y]. */
		void cubicTo(var cxy1, var cxy2, var x, var y);

		/** Adds a addQuadrilateral to the path. */
		void addQuadrilateral(var xy1, var xy2, var xy3, var xy4);

		/** Adds an arc to the path. */
		void addArc(var area, var fromRadians, var toRadians);

		/** Adds an ellipse to the path. */
		void addEllipse(var area);

		/** Adds a rectangle to the path. */
		void addRectangle(var area);

		/** Adds a rounded rectangle to the path. */
		void addRoundedRectangle(var area, var cornerSize);

		/** Adds a fully customisable rounded rectangle to the path. area[x,y,w,h], cornerSizeXY[x,y], boolCurves[bool,bool,bool,bool]*/
		void addRoundedRectangleCustomisable(var area, var cornerSizeXY, var boolCurves);

		/** Adds a triangle to the path. */
		void addTriangle(var xy1, var xy2, var xy3);

		/** Adds a polygon to the path from the center [x, y]. */
		void addPolygon(var center, var numSides, var radius, var angle);

		/** Adds an arrow to the path from start [x, y] and end [x, y]. */
		void addArrow(var start, var end, var thickness, var headWidth, var headLength);

		/** Adds a star to the path from the center [x, y]. */
		void addStar(var center, var numPoints, var innerRadius, var outerRadius, var angle);

		/** Rescales the path to make it fit neatly into a given space. preserveProportions keeps the w/h ratio.*/
		void scaleToFit(var x, var y, var width, var height, bool preserveProportions);

		/** Creates a version of this path where all sharp corners have been replaced by curves.*/
		void roundCorners(var radius);

		/** Returns the point where a line ([x1, y1], [x2, y2]) intersects the path when appropriate. Returns false otherwise. */
		var getIntersection(var start, var end, bool keepSectionOutsidePath);

		/** Returns the point at a certain distance along the path. */
		var getPointOnPath(var distanceFromStart);

		/** Checks whether a point lies within the path. This is only relevant for closed paths. */
		var contains(var point);

		/** Returns the area ([x, y, width, height]) that the path is occupying with the scale factor applied. */
		var getBounds(var scaleFactor);

		/** Returns the length of the path. */
		var getLength();

		/** Creates a fillable path using the provided strokeData (with optional dot. */
		var createStrokedPath(var strokeData, var dotData);

		/** Creates a string representation of this path. */
		String toString();

		/** Creates a base64 encoded representation of the path. */
		String toBase64();

		/** Restores a path that has been converted into a string. */
		void fromString(String stringPath);

		// ============================================================================================================

		struct Wrapper;

		Path& getPath() { return p; }

		const Path& getPath() const { return p; }

	private:

		Path p;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PathObject);

		// ============================================================================================================
	};

	class MarkdownObject : public ConstScriptingObject
	{
	public:

		MarkdownObject(ProcessorWithScriptingContent* pwsc);;

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("MarkdownRenderer"); }

		String getDebugName() const override { return "MarkdownRenderer"; }

		Component* createPopupComponent(const MouseEvent& e, Component *c) override;

		// ============================================================================================================ API

		/** Set the markdown text to be displayed. */
		void setText(const String& markdownText);

		/** Parses the text for the specified area and returns the used height (might be more or less than the height of the area passed in). */
		float setTextBounds(var area);

		/** Sets the style data for the markdown renderer. */
		void setStyleData(var styleData);

		/** Returns the current style data. */
		var getStyleData();

		/** Creates an image provider from the given JSON data that resolves image links. */
		void setImageProvider(var data);

		// ============================================================================================================ API End

		hise::DrawActions::MarkdownAction::Ptr obj;

	private:

		class ScriptedImageProvider;

		class Preview;
		struct Wrapper;
	};

	class GraphicsObject : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		GraphicsObject(ProcessorWithScriptingContent *p, ConstScriptingObject* parent);
		~GraphicsObject();

		// ============================================================================================================

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Graphics"); }

		// ============================================================================================================ API Methods

		/** Starts a new Layer. */
		void beginLayer(bool drawOnParent);

		/** Begins a new layer that will use the given blend effect. */
		void beginBlendLayer(String blendMode, float alpha);

		/** flushes the current layer. */
		void endLayer();

		/** Applies gaussian blur to the current layer. */
		void gaussianBlur(var blurAmount);

		/** Applies a box blur to the current layer. */
		void boxBlur(var blurAmount);

		/** Adds noise to the current layer. */
		void addNoise(var noiseAmount);

		/** Removes all colour from the current layer. */
		void desaturate();

		/** Applies a mask to the current layer. */
		void applyMask(var path, var area, bool invert);

		/** Fills the whole area with the given colour. */
		void fillAll(var colour);

		/** Fills a rectangle with the given colour. */
		void fillRect(var area);

		/** Draws a rectangle. */
		void drawRect(var area, float borderSize);

		/** Fills a rounded rectangle. cornerData can be either a float number (for the corner size) or a JSON object for more customization options. */
		void fillRoundedRectangle(var area, var cornerData);

		/** Draws a rounded rectangle. cornerData can be either a float number (for the corner size) or a JSON object for more customization options.*/
		void drawRoundedRectangle(var area, var cornerData, float borderSize);

		/** Draws a (non interpolated) horizontal line. */
		void drawHorizontalLine(int y, float x1, float x2);

		/** Draws a (non interpolated) vertical line. */
		void drawVerticalLine(int x, float y1, float y2);

		/** Sets a global transparency level. */
		void setOpacity(float alphaValue);

		/** Draws a line. */
		void drawLine(float x1, float x2, float y1, float y2, float lineThickness);

		/** Sets the current colour. */
		void setColour(var colour);

		/** Sets the current font. */
		void setFont(String fontName, float fontSize);

		/** Sets the current font with the specified spacing between the characters. */
		void setFontWithSpacing(String fontName, float fontSize, float spacing);

		/** Draws a centered and vertically stretched text. */
		void drawText(String text, var area);

		/** Draws a text with the given alignment (see the Label alignment property). */
		void drawAlignedText(String text, var area, String alignment);

		/** Renders a (blurred) shadow for the text. */
		void drawAlignedTextShadow(String text, var area, String alignment, var shadowData);

		/** Tries to draw a text string inside a given space. */
		void drawFittedText(String text, var area, String alignment, int maxLines, float scale);

		/** Break to new lines when the text becomes wider than maxWidth. */
		void drawMultiLineText(String text, var xy, int maxWidth, String alignment, float leading);

		/** Draws the text of the given markdown renderer to its specified area. */
		void drawMarkdownText(var markdownRenderer);

		/** Draws the spectrum of the FFT object to the panel. */
		void drawFFTSpectrum(var fftObject, var area);

		/** Sets the current gradient via an array [Colour1, x1, y1, Colour2, x2, y2] */
		void setGradientFill(var gradientData);

		/** Draws a ellipse in the given area. */
		void drawEllipse(var area, float lineThickness);

        /** Draws a SVG object within the given bounds and opacity. */
        void drawSVG(var svgObject, var bounds, float opacity);
        
		/** Applies a HSL grading on the current layer. */
		void applyHSL(float hue, float saturation, float lightness);

		/** Applies a gamma correction to the current layer. */
		void applyGamma(float gamma);

		/** Applies a gradient map to the brightness level of the current layer. */
		void applyGradientMap(var darkColour, var brightColour);

		/** Applies a sharpen / soften filter on the current layer. */
		void applySharpness(int delta);

		/** Applies an oldschool sepia filter on the current layer. */
		void applySepia();

		/** Applies a vignette (dark corners on the current layer. */
		void applyVignette(float amount, float radius, float falloff);

		/** Fills a ellipse in the given area. */
		void fillEllipse(var area);

		/** Draws a image into the area. */
		void drawImage(String imageName, var area, int xOffset, int yOffset);

		/** Draws a drop shadow around a rectangle. */
		void drawDropShadow(var area, var colour, int radius);

		/** Draws a drop shadow from a path. */
		void drawDropShadowFromPath(var path, var area, var colour, int radius, var offset);

		/** Draws a triangle rotated by the angle in radians. */
		void drawTriangle(var area, float angle, float lineThickness);

		/** Fills a triangle rotated by the angle in radians. */
		void fillTriangle(var area, float angle);

		/** Adds a drop shadow based on the alpha values of the current image. */
		void addDropShadowFromAlpha(var colour, int radius);

		/** fills the entire component with a random colour to indicate a UI repaint. */
		void drawRepaintMarker(const String& label);

		/** Applies an OpenGL shader to the panel. Returns false if the shader could not be compiled. */
		bool applyShader(var shader, var area);

		/** Returns the width of the string using the current font. */
		float getStringWidth(String text);

		/** Fills a Path. */
		void fillPath(var path, var area);

		/** Draws the given path. */
		void drawPath(var path, var area, var strokeStyle);

		/** Rotates the canvas around center `[x, y]` by the given amount in radian. */
		void rotate(var angleInRadian, var center);

        /** Flips the canvas at the center. */
        void flip(bool horizontally, var totalArea);
        
		// ============================================================================================================

		struct Wrapper;


		DrawActions::Handler& getDrawHandler() { return drawActionHandler; }

	private:

		Point<float> getPointFromVar(const var& data);
		Rectangle<float> getRectangleFromVar(const var &data);
		Rectangle<int> getIntRectangleFromVar(const var &data);

		Font currentFont;
		String currentFontName = "";
		float currentKerningFactor = 0.0f;
		float currentFontHeight = 13.0f;

		Result rectangleResult;

		ConstScriptingObject* parent = nullptr;

		DrawActions::Handler drawActionHandler;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicsObject);

		// ============================================================================================================
	};

	class ScriptedLookAndFeel : public ConstScriptingObject,
								public ControlledObject
	{
	public:

		struct LafBase
		{
			virtual ~LafBase() {};

			virtual ScriptedLookAndFeel* get() = 0;
			
		};

		struct CSSLaf: public simple_css::StyleSheetLookAndFeel,
					   public LafBase
		{
			CSSLaf(ScriptedLookAndFeel* parent_, ScriptContentComponent* content, Component* c, const ValueTree& dataTree, const ValueTree& additionalPropertyTree);;

			ScriptedLookAndFeel* get() override;

			void updateMultipageDialog(multipage::Dialog& mp)
			{
				simple_css::Parser p(parent->currentStyleSheet);
				auto ok = p.parse();
				auto css = p.getCSSValues();
				mp.update(css);
			}

			simple_css::CSSRootComponent& root;
			WeakReference<ScriptedLookAndFeel> parent;

			valuetree::PropertyListener colourUpdater;
			valuetree::PropertyListener additionalPropertyUpdater;
			valuetree::PropertyListener additionalComponentPropertyUpdater;
		};

		struct Laf : public GlobalHiseLookAndFeel,
			public LafBase,
			public PresetBrowserLookAndFeelMethods,
			public TableEditor::LookAndFeelMethods,
            public HiseAudioThumbnail::LookAndFeelMethods,
			public NumberTag::LookAndFeelMethods,
			public MessageWithIcon::LookAndFeelMethods,
			public ControlledObject,
			public FilterGraph::LookAndFeelMethods,
			public FilterDragOverlay::LookAndFeelMethods,
			public RingBufferComponentBase::LookAndFeelMethods,
			public AhdsrGraph::LookAndFeelMethods,
			public MidiFileDragAndDropper::LookAndFeelMethods,
			public SliderPack::LookAndFeelMethods,
			public CustomKeyboardLookAndFeelBase,
			public ScriptTableListModel::LookAndFeelMethods,
            public MatrixPeakMeter::LookAndFeelMethods,
			public WaterfallComponent::LookAndFeelMethods
		{
			Laf(MainController* mc);

			virtual ~Laf();;
			
			Font getFont();

			ScriptedLookAndFeel* get() override;

			void drawAlertBox(Graphics&, AlertWindow&, const Rectangle<int>& textArea, TextLayout&) override;

			Font getAlertWindowMessageFont() override;
			Font getAlertWindowTitleFont() override;
			Font getTextButtonFont(TextButton &, int) override;
			Font getComboBoxFont(ComboBox&) override;
			Font getPopupMenuFont() override;;
			Font getAlertWindowFont() override;;

			MarkdownLayout::StyleData getAlertWindowMarkdownStyleData() override;

			void drawPopupMenuBackground(Graphics& g_, int width, int height) override;

			void drawPopupMenuItem(Graphics& g, const Rectangle<int>& area,
				bool isSeparator, bool isActive,
				bool isHighlighted, bool isTicked,
				bool hasSubMenu, const String& text,
				const String& shortcutKeyText,
				const Drawable* icon, const Colour* textColourToUse);

            void drawPopupMenuSectionHeader (Graphics& g,
                                             const Rectangle<int>& area,
                                             const String& sectionName);
            
			void drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool /*isButtonDown*/) override;

			void drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

			void drawLinearSlider(Graphics &g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider &slider) override;

			void drawButtonText(Graphics &g_, TextButton &button, bool isMouseOverButton, bool isButtonDown) override;

			void drawComboBox(Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb);

			void positionComboBoxText(ComboBox &c, Label &labelToPosition) override;

			void drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label);

			void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/,
				bool isMouseOverButton, bool isButtonDown) override;

			void drawNumberTag(Graphics& g, Component& comp, Colour& c, Rectangle<int> area, int offset, int size, int number) override;

			Path createPresetBrowserIcons(const String& id) override;
			void drawPresetBrowserBackground(Graphics& g, Component* p) override;
			void drawColumnBackground(Graphics& g, int columnIndex, Rectangle<int> listArea, const String& emptyText) override;
			void drawTag(Graphics& g, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position) override;
			void drawModalOverlay(Graphics& g, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command) override;
			void drawListItem(Graphics& g, int columnIndex, int, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode, bool hover) override;
			void drawSearchBar(Graphics& g, Rectangle<int> area) override;

			void drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition) override;
			void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) override;
			void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) override;
			void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) override;

			void drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

			void drawAhdsrBackground(Graphics& g, AhdsrGraph& graph) override;
			void drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive) override;
			void drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p) override;

			void drawMidiDropper(Graphics& g, Rectangle<float> area, const String& text, MidiFileDragAndDropper& d) override;

            void drawThumbnailRange(Graphics& g, HiseAudioThumbnail& te, Rectangle<float> area, int areaIndex, Colour c, bool areaEnabled);
            void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area) override;
            void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path) override;
            void drawHiseThumbnailRectList(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const HiseAudioThumbnail::RectangleListType& rectList) override;

            HiseAudioThumbnail::RenderOptions getThumbnailRenderOptions(HiseAudioThumbnail& th, const HiseAudioThumbnail::RenderOptions& defaultOptions) override;
            
			void drawThumbnailRuler(Graphics& g, HiseAudioThumbnail& te, int xPosition) override;

            void drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area) override;
            
			void drawKeyboardBackground(Graphics &g, Component* c, int width, int height) override;
			void drawWhiteNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &lineColour, const Colour &textColour) override;
			void drawBlackNote(CustomKeyboardState* state, Component* c, int midiNoteNumber, Graphics &g, int x, int y, int w, int h, bool isDown, bool isOver, const Colour &noteFillColour) override;

			void drawSliderPackBackground(Graphics& g, SliderPack& s) override;
			void drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity) override;
			void drawSliderPackRightClickLine(Graphics& g, SliderPack& s, Line<float> lineToDraw) override;
			void drawSliderPackTextPopup(Graphics& g, SliderPack& s, const String& textToDraw) override;

			void drawTableRowBackground(Graphics& g, const ScriptTableListModel::LookAndFeelData& d, int rowNumber, int width, int height, bool rowIsSelected, bool rowIsHovered) override;

			void drawTableCell(Graphics& g, const  ScriptTableListModel::LookAndFeelData& d, const String& text, int rowNumber, int columnId, int width, int height, bool rowIsSelected, bool cellIsClicked, bool cellIsHovered) override;

			void drawTableHeaderBackground(Graphics& g, TableHeaderComponent& h) override;

			void drawTableHeaderColumn(Graphics& g, TableHeaderComponent&, const String& columnName, int columnId, int width, int height, bool isMouseOver, bool isMouseDown, int columnFlags) override;

            void getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight) override;
            
			void drawFilterDragHandle(Graphics& g, FilterDragOverlay& o, int index, Rectangle<float> handleBounds, const FilterDragOverlay::DragData& d) override;

			void drawFilterBackground(Graphics &g, FilterGraph& fg) override;
			void drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p) override;
			void drawFilterGridLines(Graphics &g, FilterGraph& fg, const Path& gridPath) override;
            
			void drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> areaToFill) override;
			void drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p) override;
			void drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index) override;
			void drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p) override;

            void drawMatrixPeakMeter(Graphics& g, float* peakValues, float* maxPeaks, int numChannels, bool isVertical, float segmentSize, float paddingSize, Component* c) override;
            
			void drawWavetableBackground(Graphics& g, WaterfallComponent& wc, bool isEmpty) override;
			void drawWavetablePath(Graphics& g, WaterfallComponent& wc, const Path& p, int tableIndex, bool isStereo, int currentTableIndex, int numTables) override;

			Image createIcon(PresetHandler::IconType type) override;

			bool functionDefined(const String& s);

			static Identifier getIdOfParentFloatingTile(Component& c);

			static bool addParentFloatingTile(Component& c, DynamicObject* obj);

			static void setColourOrBlack(DynamicObject* obj, const Identifier& id, Component& c, int colourId);

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Laf);
			JUCE_DECLARE_WEAK_REFERENCEABLE(Laf);
		};

		struct LocalLaf : public Laf
		{
			LocalLaf(ScriptedLookAndFeel* l);;
			ScriptedLookAndFeel* get() override;
			
			WeakReference<ScriptedLookAndFeel> weakLaf;
		};

		struct Wrapper;

		ScriptedLookAndFeel(ProcessorWithScriptingContent* sp, bool isGlobal);

		~ScriptedLookAndFeel();

		Identifier getObjectName() const override;

		// ========================================================================================

		/** Registers a function that will be used for the custom look and feel. */
		void registerFunction(var functionName, var function);

		/** Set a global font. */
		void setGlobalFont(const String& fontName, float fontSize);

		/** Parses CSS code and switches the look and feel to use the CSS renderer. */
		void setInlineStyleSheet(const String& cssCode);

		/** Parses CSS code from a style sheet file in the scripts folder and switches the look and feel to use the CSS renderer. */
		void setStyleSheet(const String& fileName);

		/** Sets a variable that can be queried from a style sheet. */
		void setStyleSheetProperty(const String& variableId, var value, const String& type);

		/** Loads an image that can be used by the look and feel functions. */
		void loadImage(String imageFile, String prettyName);

		/** Unload all images from the look and feel object. */
		void unloadAllImages();

		/** Checks if the image has been loaded into the look and feel obkect */
		bool isImageLoaded(String prettyName);

		// ========================================================================================

		bool isUsingCSS() const { return !currentStyleSheet.isEmpty(); }

		bool callWithGraphics(Graphics& g_, const Identifier& functionname, var argsObject, Component* c);

		var callDefinedFunction(const Identifier& name, var* args, int numArgs);

		String loadStyleSheetFile(const String& filename);

		void setStyleSheetInternal(const String& cssCode);

		void clearScriptContext();

		int getNumChildElements() const override;

		Location getLocation() const override;

		DebugInformationBase* getChildElement(int index) override;

		static Array<Identifier> getAllFunctionNames();

        
		Font f = GLOBAL_BOLD_FONT();
        
        struct GraphicsWithComponent
        {
            ReferenceCountedObjectPtr<GraphicsObject> g;
            Identifier functionName;
            Component* c = nullptr;
        };
        
        Array<GraphicsWithComponent> graphics;
        
		String currentStyleSheet;
		String currentStyleSheetFile;
		simple_css::StyleSheet::Collection css;

		var functions;

		Image getLoadedImage(const String& prettyName);

		struct NamedImage
		{
			PooledImage image;
			String prettyName;
		};

		const bool wasGlobal;
		Array<NamedImage> loadedImages;

		Result lastResult;

		ValueTree additionalProperties;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptedLookAndFeel);
		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptedLookAndFeel);
	};
}

} // namespace hise
