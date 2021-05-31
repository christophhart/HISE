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

namespace ScriptingObjects
{
	class ScriptShader : public ConstScriptingObject
	{
	public:

		enum class BlendMode
		{
			_GL_ZERO = GL_ZERO, //< (0, 0, 0, 0)
			_GL_ONE = GL_ONE, //< (1, 1, 1, 1)
			_GL_SRC_COLOR = GL_SRC_COLOR, //< (Rs / kR, Gs / kG, Bs / kB, As / kA)
			_GL_ONE_MINUS_SRC_COLOR = GL_ONE_MINUS_SRC_COLOR, //< (1, 1, 1, 1) - (Rs / kR, Gs / kG, Bs / kB, As / kA)
			_GL_DST_COLOR = GL_DST_COLOR, //< (Rd / kR, Gd / kG, Bd / kB, Ad / kA)
			_GL_ONE_MINUS_DST_COLOR = GL_ONE_MINUS_DST_COLOR,  //< (1, 1, 1, 1) - (Rd / kR, Gd / kG, Bd / kB, Ad / kA)
			_GL_SRC_ALPHA = GL_SRC_ALPHA, //< (As / kA, As / kA, As / kA, As / kA)
			_GL_ONE_MINUS_SRC_ALPHA = GL_ONE_MINUS_SRC_ALPHA, //< (1, 1, 1, 1) - (As / kA, As / kA, As / kA, As / kA)
			_GL_DST_ALPHA = GL_DST_ALPHA, //< (Ad / kA, Ad / kA, Ad / kA, Ad / kA)
			_GL_ONE_MINUS_DST_ALPHA = GL_ONE_MINUS_DST_ALPHA, //< (1, 1, 1, 1) - (Ad / kA, Ad / kA, Ad / kA, Ad / kA)
			_GL_SRC_ALPHA_SATURATE = GL_SRC_ALPHA_SATURATE,
			numBlendModes = 11 // needs to be set manually...
		};

		ScriptShader(ProcessorWithScriptingContent* sp);;

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("ScriptShader"); }

		// =============================================================== API Methods

		/** Loads a .glsl file from the script folder. */
		void setFragmentShader(String shaderFile);

		/** Sets an uniform variable to be used in the shader code. */
		void setUniformData(const String& id, var data);

		/** Sets the blend mode for the shader. */
		void setBlendFunc(bool enabled, int sFactor, int dFactor);

		// ===========================================================================

		bool compiledOk() const { return r.wasOk(); }

		String getErrorMessage() { return r.getErrorMessage(); }

		void setCompileResult(Result compileResult)
		{
			r = processErrorMessage(compileResult);

			if (currentShaderFile != nullptr)
				currentShaderFile->setRuntimeErrors(r);
		}

		void setGlobalBounds(Rectangle<int> b, float sf)
		{
			globalRect = b.toFloat();
			scaleFactor = sf;
		}

		struct Wrapper;

		float scaleFactor = 1.0f;
		String shaderCode;
		NamedValueSet uniformData;

		ScopedPointer<juce::OpenGLGraphicsContextCustomShader> shader;
		bool dirty = false;

		double iTime = 0;

		Rectangle<float> globalRect;
		Rectangle<float> localRect;

		WeakReference<ExternalScriptFile> currentShaderFile;
		
		bool enableBlending = false;
		BlendMode src = BlendMode::_GL_SRC_ALPHA;
		BlendMode dst = BlendMode::_GL_ONE_MINUS_SRC_ALPHA;

	private:

		struct Result processErrorMessage(const Result& r);

		Result r;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptShader);
	};

	class PathObject : public ConstScriptingObject
	{
	public:

		// ============================================================================================================

		PathObject(ProcessorWithScriptingContent* p);
		~PathObject();

		Identifier getObjectName() const override { RETURN_STATIC_IDENTIFIER("Path"); }


		String getDebugValue() const override {
			return p.getBounds().toString();
		}
		String getDebugName() const override { return "Path"; }

		void rightClickCallback(const MouseEvent &e, Component* componentToNotify) override;



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

		/** Adds an arc to the path. */
		void addArc(var area, var fromRadians, var toRadians);

		/** Returns the area ([x, y, width, height]) that the path is occupying with the scale factor applied. */
		var getBounds(var scaleFactor);

		// ============================================================================================================

		struct Wrapper;

		Path& getPath() { return p; }

		const Path& getPath() const { return p; }

	private:

		Path p;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PathObject);

		// ============================================================================================================
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

		/** Fills a rounded rectangle. */
		void fillRoundedRectangle(var area, float cornerSize);

		/** Draws a rounded rectangle. */
		void drawRoundedRectangle(var area, float cornerSize, float borderSize);

		/** Draws a (non interpolated) horizontal line. */
		void drawHorizontalLine(int y, float x1, float x2);

		/** Sets a global transparency level. */
		void setOpacity(float alphaValue);

		/** Draws a line. */
		void drawLine(float x1, float x2, float y1, float y2, float lineThickness);

		/** Sets the current colour. */
		void setColour(var colour);

		/** Sets the current font. */
		void setFont(String fontName, float fontSize);

		/** Draws a centered and vertically stretched text. */
		void drawText(String text, var area);

		/** Draws a text with the given alignment (see the Label alignment property). */
		void drawAlignedText(String text, var area, String alignment);

		/** Sets the current gradient via an array [Colour1, x1, y1, Colour2, x2, y2] */
		void setGradientFill(var gradientData);

		/** Draws a ellipse in the given area. */
		void drawEllipse(var area, float lineThickness);

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

		/** Applies Perlin Noise to the current layer. */
		void addPerlinNoise(double freq, double octave, double z, float amount);

		/** Applies a vignette (dark corners on the current layer. */
		void applyVignette(float amount, float radius, float falloff);

		/** Fills a ellipse in the given area. */
		void fillEllipse(var area);

		/** Draws a image into the area. */
		void drawImage(String imageName, var area, int xOffset, int yOffset);

		/** Draws a drop shadow around a rectangle. */
		void drawDropShadow(var area, var colour, int radius);

		/** Draws a triangle rotated by the angle in radians. */
		void drawTriangle(var area, float angle, float lineThickness);

		/** Fills a triangle rotated by the angle in radians. */
		void fillTriangle(var area, float angle);

		/** Adds a drop shadow based on the alpha values of the current image. */
		void addDropShadowFromAlpha(var colour, int radius);

		/** Applies an OpenGL shader to the panel. */
		void applyShader(var shader, var area);


		/** Fills a Path. */
		void fillPath(var path, var area);

		/** Draws the given path. */
		void drawPath(var path, var area, var thickNess);

		/** Rotates the canvas around center `[x, y]` by the given amount in radian. */
		void rotate(var angleInRadian, var center);

		// ============================================================================================================

		struct Wrapper;


		DrawActions::Handler& getDrawHandler() { return drawActionHandler; }

	private:

		Point<float> getPointFromVar(const var& data);
		Rectangle<float> getRectangleFromVar(const var &data);
		Rectangle<int> getIntRectangleFromVar(const var &data);

		Result rectangleResult;

		ConstScriptingObject* parent = nullptr;

		DrawActions::Handler drawActionHandler;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GraphicsObject);

		// ============================================================================================================
	};

	class ScriptedLookAndFeel : public ConstScriptingObject
	{
	public:

		struct Laf : public GlobalHiseLookAndFeel,
			public PresetBrowserLookAndFeelMethods,
			public TableEditor::LookAndFeelMethods,
			public NumberTag::LookAndFeelMethods,
			public MessageWithIcon::LookAndFeelMethods,
			public ControlledObject
		{
			Laf(MainController* mc) :
				ControlledObject(mc)
			{}

			ScriptedLookAndFeel* get()
			{
				return dynamic_cast<ScriptedLookAndFeel*>(getMainController()->getCurrentScriptLookAndFeel());
			}

			Font getFont()
			{
				if (auto l = get())
					return l->f;
				else
					return GLOBAL_BOLD_FONT();
			}

			void drawAlertBox(Graphics&, AlertWindow&, const Rectangle<int>& textArea, TextLayout&) override;

			Font getAlertWindowMessageFont() override { return getFont(); }
			Font getAlertWindowTitleFont() override { return getFont(); }
			Font getTextButtonFont(TextButton &, int) override { return getFont(); }
			Font getComboBoxFont(ComboBox&) override { return getFont(); }
			Font getPopupMenuFont() override { return getFont(); };
			Font getAlertWindowFont() override { return getFont(); };

			MarkdownLayout::StyleData getAlertWindowMarkdownStyleData() override;

			void drawPopupMenuBackground(Graphics& g_, int width, int height) override;

			void drawPopupMenuItem(Graphics& g, const Rectangle<int>& area,
				bool isSeparator, bool isActive,
				bool isHighlighted, bool isTicked,
				bool hasSubMenu, const String& text,
				const String& shortcutKeyText,
				const Drawable* icon, const Colour* textColourToUse);

			void drawToggleButton(Graphics &g, ToggleButton &b, bool isMouseOverButton, bool /*isButtonDown*/) override;

			void drawRotarySlider(Graphics &g, int /*x*/, int /*y*/, int width, int height, float /*sliderPosProportional*/, float /*rotaryStartAngle*/, float /*rotaryEndAngle*/, Slider &s) override;

			void drawLinearSlider(Graphics &g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider &slider) override;

			void drawButtonText(Graphics &g_, TextButton &button, bool isMouseOverButton, bool isButtonDown) override;

			void drawComboBox(Graphics&, int width, int height, bool isButtonDown, int buttonX, int buttonY, int buttonW, int buttonH, ComboBox& cb);

			void positionComboBoxText(ComboBox &c, Label &labelToPosition) override;

			void drawComboBoxTextWhenNothingSelected(Graphics& g, ComboBox& box, Label& label);

			void drawButtonBackground(Graphics& g, Button& button, const Colour& /*backgroundColour*/,
				bool isMouseOverButton, bool isButtonDown) override;

			void drawNumberTag(Graphics& g, Colour& c, Rectangle<int> area, int offset, int size, int number) override;

			void drawPresetBrowserBackground(Graphics& g, PresetBrowser* p) override;
			void drawColumnBackground(Graphics& g, Rectangle<int> listArea, const String& emptyText) override;
			void drawTag(Graphics& g, bool blinking, bool active, bool selected, const String& name, Rectangle<int> position) override;
			void drawModalOverlay(Graphics& g, Rectangle<int> area, Rectangle<int> labelArea, const String& title, const String& command) override;
			void drawListItem(Graphics& g, int columnIndex, int, const String& itemName, Rectangle<int> position, bool rowIsSelected, bool deleteMode) override;
			void drawSearchBar(Graphics& g, Rectangle<int> area) override;

			void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) override;
			void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) override;
			void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) override;

			void drawScrollbar(Graphics& g, ScrollBar& scrollbar, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

			Image createIcon(PresetHandler::IconType type) override;

			bool functionDefined(const String& s);

			static Identifier getIdOfParentFloatingTile(Component& c);

			static bool addParentFloatingTile(Component& c, DynamicObject* obj);
		};

		struct Wrapper;

		ScriptedLookAndFeel(ProcessorWithScriptingContent* sp);

		~ScriptedLookAndFeel();

		Identifier getObjectName() const override { return "ScriptLookAndFeel"; }

		/** Registers a function that will be used for the custom look and feel. */
		void registerFunction(var functionName, var function);

		/** Set a global font. */
		void setGlobalFont(const String& fontName, float fontSize);

		bool callWithGraphics(Graphics& g_, const Identifier& functionname, var argsObject);

		var callDefinedFunction(const Identifier& name, var* args, int numArgs);

		Font f = GLOBAL_BOLD_FONT();
		ReferenceCountedObjectPtr<GraphicsObject> g;

		var functions;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ScriptedLookAndFeel);
	};
}

} // namespace hise