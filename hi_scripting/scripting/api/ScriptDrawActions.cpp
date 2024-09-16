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

namespace hise { using namespace juce;

struct ScriptedPostDrawActions
{
	struct guassianBlur : public DrawActions::PostActionBase
	{
		guassianBlur(int b) : blurAmount(b) {};

		bool needsStackData() const override { return false; }
		void perform(PostGraphicsRenderer& r) override
		{
			r.gaussianBlur(blurAmount);
		}

		int blurAmount;
	};

	struct boxBlur : public DrawActions::PostActionBase
	{
		boxBlur(int b) : blurAmount(b) {};

		bool needsStackData() const override { return false; }
		void perform(PostGraphicsRenderer& r) override
		{
			r.boxBlur(blurAmount);
		}

		int blurAmount;
	};

	struct desaturate : public DrawActions::PostActionBase
	{
		desaturate() {};

		bool needsStackData() const override { return false; }
		void perform(PostGraphicsRenderer& r) override
		{
			r.desaturate();
		}

		int blurAmount;
	};

	struct addNoise : public DrawActions::ActionBase
	{
		addNoise(DrawActions::NoiseMapManager* manager, float v, Rectangle<int> area_, bool monochrom_=false, float scale_=1.0f) : 
			m(manager),
			noise(v), 
			area(area_), 
			scale(scale_), 
			monochrom(monochrom_) 
		{};

		SET_ACTION_ID(addNoise);

		void perform(Graphics& g) override
		{
			m->drawNoiseMap(g, area, noise, monochrom, scale);
		}

		bool wantsCachedImage() const override { return false; };
		bool wantsToDrawOnParent() const override { return false; }

		DrawActions::NoiseMapManager* m;

		const float noise;
		const float scale;

		
		const Rectangle<int> area;
		const bool monochrom;
	};

	struct applyHSL : public DrawActions::PostActionBase
	{
		applyHSL(float hue, float sat, float light) :
			h(hue),
			s(sat),
			l(light)
		{}

		bool needsStackData() const override { return false; }

		void perform(PostGraphicsRenderer& r) override
		{
			r.applyHSL(h, s, l);

		}

		float h, s, l;
	};

	struct applyGradientMap : public DrawActions::PostActionBase
	{
		applyGradientMap(Colour c1_, Colour c2_) :
			c1(c1_),
			c2(c2_)
		{}

		bool needsStackData() const override { return false; }

		void perform(PostGraphicsRenderer& r) override
		{
			r.applyGradientMap(ColourGradient(c1, {}, c2, {}, false));
		}

		Colour c1, c2;
	};

	struct applyGamma : public DrawActions::PostActionBase
	{
		applyGamma(float gamma_) :
			gamma(gamma_)
		{}

		bool needsStackData() const override { return false; }

		void perform(PostGraphicsRenderer& r) override
		{
			r.applyGamma(gamma);
		}

		float gamma;
	};

	struct applySharpness : public DrawActions::PostActionBase
	{
		applySharpness(int d) :
			delta(d)
		{}

		bool needsStackData() const override { return false; }

		void perform(PostGraphicsRenderer& r) override
		{
			r.applySharpness(delta);
		}

		int delta;
	};

	struct applyVignette : public DrawActions::PostActionBase
	{
		applyVignette(float amount_, float radius_, float falloff_) :
			amount(amount_),
			radius(radius_),
			falloff(falloff_)
		{}

		bool needsStackData() const override { return false; }

		void perform(PostGraphicsRenderer& r) override
		{
			r.applyVignette(amount, radius, falloff);
		}

		float amount, radius, falloff;
	};

	struct applySepia : public DrawActions::PostActionBase
	{
		applySepia() = default;

		bool needsStackData() const override { return false; }

		void perform(PostGraphicsRenderer& r) override
		{
			r.applySepia();
		}
	};

	struct applyMask : public DrawActions::PostActionBase
	{
		applyMask(const Path& p, bool i) : path(p), invert(i) {};

		bool needsStackData() const override { return true; }
		void perform(PostGraphicsRenderer& r) override
		{
			

			r.applyMask(path, invert, false);
		}

		Path path;
		bool invert;
	};
};

namespace ScriptedDrawActions
{
	struct fillAll : public DrawActions::ActionBase
	{
		SET_ACTION_ID(fillAll);

		fillAll(Colour c_) : c(c_) {};
		void perform(Graphics& g) { g.fillAll(c); };
		Colour c;
	};

	struct setColour : public DrawActions::ActionBase
	{
		SET_ACTION_ID(setColour);

		setColour(Colour c_) : c(c_) {};
		void perform(Graphics& g) { g.setColour(c); };
		Colour c;
	};

	struct addTransform : public DrawActions::ActionBase
	{
		SET_ACTION_ID(addTransform);

		addTransform(AffineTransform a_) : a(a_) {};
		void perform(Graphics& g) override { g.addTransform(a); };
		AffineTransform a;
	};

	struct fillPath : public DrawActions::ActionBase
	{
		SET_ACTION_ID(fillPath);

		fillPath(const Path& p_) : p(p_) {};
		void perform(Graphics& g) override { g.fillPath(p); };
		Path p;
	};

	struct drawPath : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawPath);

		drawPath(const Path& p_, PathStrokeType strokeType) : p(p_), s(strokeType) {};
		void perform(Graphics& g) override
		{
			g.strokePath(p, s);
		}
		Path p;
		PathStrokeType s;
	};

	struct drawRepaintMarker: public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawRepaintMarker);

		drawRepaintMarker(const String& l)
		{
			if(l.isEmpty())
				b <<  "anonymous repaint";
			else
				b << l;
		}


		void perform(Graphics& g)
		{
#if PERFETTO
			perfetto::CounterTrack ct(b.get());
			TRACE_COUNTER("drawactions", ct, ++numRepaints);
			TRACE_EVENT("drawactions", DYNAMIC_STRING_BUILDER(b));
#endif
			g.fillAll(Colour::fromHSL(Random::getSystemRandom().nextFloat(), 0.33f, 0.5, 0.3f));
		};

		dispatch::StringBuilder b;
		uint32 numRepaints = 0;
	};

	struct fillRect : public DrawActions::ActionBase
	{
		SET_ACTION_ID(fillRect);

		fillRect(Rectangle<float> area_) : area(area_) {};
		void perform(Graphics& g) { g.fillRect(area); };
		Rectangle<float> area;
	};

	struct fillEllipse : public DrawActions::ActionBase
	{
		SET_ACTION_ID(fillEllipse);

		fillEllipse(Rectangle<float> area_) : area(area_) {};
		void perform(Graphics& g) { g.fillEllipse(area); };
		Rectangle<float> area;
	};

	struct drawRect : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawRect);

		drawRect(Rectangle<float> area_, float borderSize_) : area(area_), borderSize(borderSize_) {};
		void perform(Graphics& g) { g.drawRect(area, borderSize); };
		Rectangle<float> area;
		float borderSize;
	};

	struct drawEllipse : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawEllipse);

		drawEllipse(Rectangle<float> area_, float borderSize_) : area(area_), borderSize(borderSize_) {};
		void perform(Graphics& g) { g.drawEllipse(area, borderSize); };
		Rectangle<float> area;
		float borderSize;
	};

	struct fillRoundedRect : public DrawActions::ActionBase
	{
		SET_ACTION_ID(fillRoundedRect);

		fillRoundedRect(Rectangle<float> area_, float cornerSize_) :
			area(area_), cornerSize(cornerSize_) {};
		void perform(Graphics& g) 
		{ 
			if(allRounded)
				g.fillRoundedRectangle(area, cornerSize); 
			else if (!rounded[0] && !rounded[1] && !rounded[2] && !rounded[3])
				g.fillRect(area);
			else
			{
				Path p;
				p.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(), 
									  cornerSize, cornerSize, 
									  rounded[0], rounded[1], rounded[2], rounded[3]);

				g.fillPath(p);
			}
		};
		Rectangle<float> area;
		float cornerSize;

		bool allRounded = true;
		bool rounded[4] = { true, true, true, true };
	};

	struct drawRoundedRectangle : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawRoundedRectangle);

		drawRoundedRectangle(Rectangle<float> area_, float borderSize_, float cornerSize_) :
			area(area_), borderSize(borderSize_), cornerSize(cornerSize_) {};
		void perform(Graphics& g) 
		{ 
			if(allRounded)
				g.drawRoundedRectangle(area, cornerSize, borderSize); 
			else if (!rounded[0] && !rounded[1] && !rounded[2] && !rounded[3])
				g.drawRect(area, borderSize);
			else
			{
				Path p;
				p.addRoundedRectangle(area.getX(), area.getY(), area.getWidth(), area.getHeight(),
					cornerSize, cornerSize,
					rounded[0], rounded[1], rounded[2], rounded[3]);

				g.strokePath(p, PathStrokeType(borderSize));
			}
		};
		Rectangle<float> area;
		float cornerSize, borderSize;

		bool allRounded = true;
		bool rounded[4] = { true, true, true, true };
	};

	struct drawFFTSpectrum: public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawFFTSpectrum);

		drawFFTSpectrum(const Image& img_, Rectangle<float> r_, Graphics::ResamplingQuality quality_) :
			img(img_), r(r_), quality(quality_) {}

		void perform(Graphics& g) override
		{
			Spectrum2D::draw(g, img, r.toNearestInt(), quality);
		}

		Graphics::ResamplingQuality quality;
		Image img;
		Rectangle<float> r;
	};

	struct drawImageWithin : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawImageWithin);

		drawImageWithin(const Image& img_, Rectangle<float> r_, RectanglePlacement p=RectanglePlacement::centred) :
			img(img_), r(r_), placement(p) {};

		void perform(Graphics& g) override
		{
			g.drawImageWithin(img, (int)r.getX(), (int)r.getY(), (int)r.getWidth(), (int)r.getHeight(), placement);


			//			g.drawImage(img, ri.getX(), ri.getY(), (int)(r.getWidth() / scaleFactor), (int)(r.getHeight() / scaleFactor), 0, yOffset, (int)img.getWidth(), (int)((double)img.getHeight()));
		}

		Image img;
		Rectangle<float> r;
		RectanglePlacement placement = RectanglePlacement::centred;
	};

	struct drawImage : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawImage);

		drawImage(const Image& img_, Rectangle<float> r_, float scaleFactor_, int yOffset_) :
			img(img_), r(r_), scaleFactor(scaleFactor_), yOffset(yOffset_) {};

		void perform(Graphics& g) override
		{
			g.drawImage(img, (int)r.getX(), (int)r.getY(), (int)r.getWidth(), (int)r.getHeight(), 0, yOffset, (int)img.getWidth(), (int)((double)r.getHeight() * scaleFactor));


			//			g.drawImage(img, ri.getX(), ri.getY(), (int)(r.getWidth() / scaleFactor), (int)(r.getHeight() / scaleFactor), 0, yOffset, (int)img.getWidth(), (int)((double)img.getHeight()));
		}

		Image img;
		Rectangle<float> r;
		float scaleFactor;
		int yOffset;
	};

	struct drawHorizontalLine : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawHorizontalLine);

		drawHorizontalLine(int y_, float x1_, float x2_) :
			y(y_), x1(x1_), x2(x2_) {};
		void perform(Graphics& g) { g.drawHorizontalLine(y, x1, x2); };
		int y; float x1; float x2;
	};

	struct drawVerticalLine : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawVerticalLine);

		drawVerticalLine(int x_, float y1_, float y2_) :
			x(x_), y1(y1_), y2(y2_) {};
		void perform(Graphics& g) { g.drawVerticalLine(x, y1, y2); };
		int x; float y1; float y2;
	};

	struct setOpacity : public DrawActions::ActionBase
	{
		SET_ACTION_ID(setOpacity);

		setOpacity(float alpha_) :
			alpha(alpha_) {};
		void perform(Graphics& g) { g.setOpacity(alpha); };
		float alpha;
	};

	struct drawLine : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawLine);

		drawLine(float x1_, float x2_, float y1_, float y2_, float lineThickness_) :
			x1(x1_), x2(x2_), y1(y1_), y2(y2_), lineThickness(lineThickness_) {};
		void perform(Graphics& g) { g.drawLine(x1, x2, y1, y2, lineThickness); };
		float x1, x2, y1, y2, lineThickness;
	};

	struct setFont : public DrawActions::ActionBase
	{
		SET_ACTION_ID(setFont);

		setFont(Font f_) : f(f_) {};
		void perform(Graphics& g) { g.setFont(f); };
		Font f;
	};

	struct setGradientFill : public DrawActions::ActionBase
	{
		SET_ACTION_ID(setGradientFill);

		setGradientFill(ColourGradient grad_) : grad(grad_) {};
		void perform(Graphics& g) { g.setGradientFill(grad); };
		ColourGradient grad;
	};

	struct drawText : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawText);

		drawText(const String& text_, Rectangle<float> area_, Justification j_ = Justification::centred) : text(text_), area(area_), j(j_) {};
		void perform(Graphics& g) override { g.drawText(text, area, j); };
		String text;
		Rectangle<float> area;
		Justification j;
	};

	struct drawTextShadow : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawTextShadow);

		drawTextShadow(const String& text_, Rectangle<float> area_, Justification j_ = Justification::centred, const melatonin::ShadowParameters& sp_={}) : text(text_), area(area_), j(j_), sp(sp_)
		{
			if(sp.inner)
				is.setShadow(sp, 0);
			else
				ds.setShadow(sp, 0);
		};
		void perform(Graphics& g) override
		{
			if(sp.inner)
				is.render(g, text, area, j);
			else
				ds.render(g, text, area, j);
		};

		String text;
		Rectangle<float> area;
		Justification j;

		melatonin::ShadowParameters sp;
		melatonin::DropShadow ds;
		melatonin::InnerShadow is;
	};

	struct drawFittedText : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawFittedText);

		drawFittedText(const String& text_, var area_, Justification j_, int maxLines_, float scale_ = Justification::centred) : text(text_), area(area_), j(j_), maxLines(maxLines_), scale(scale_) {};
		void perform(Graphics& g) override { g.drawFittedText(text, area[0], area[1], area[2], area[3], j, maxLines, scale); };
		String text;
		var area;
		Justification j;
		int maxLines;
		float scale;
	};

	struct drawMultiLineText : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawMultiLineText);

		drawMultiLineText(const String& text_, int startX_, int baseLineY_, int maxWidth_, Justification j_ = Justification::centred, float leading_ = 0.0f) : text(text_), startX(startX_), baseLineY(baseLineY_), maxWidth(maxWidth_), j(j_), leading(leading_) {};
		void perform(Graphics& g) override { g.drawMultiLineText(text, startX, baseLineY, maxWidth, j, leading); };
		String text;
        int startX;
        int baseLineY;
		int maxWidth;
		Justification j;
		float leading;
	};

	struct drawDropShadow : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawDropShadow);

		drawDropShadow(Rectangle<int> r_, DropShadow& shadow_) : r(r_), shadow(shadow_) {};
		void perform(Graphics& g) override { shadow.drawForRectangle(g, r); };
		Rectangle<int> r;
		DropShadow shadow;
	};

	struct addDropShadowFromAlpha : public DrawActions::ActionBase
	{
		SET_ACTION_ID(addDropShadowFromAlpha);

		addDropShadowFromAlpha(const DropShadow& shadow_) : shadow(shadow_) {};

		bool wantsCachedImage() const override { return true; };

		//bool wantsToDrawOnParent() const override { return true; }

		void perform(Graphics& g) override
		{
			if (mainImage.getBounds().isEmpty())
				return;

			auto invT = AffineTransform::scale(1.0f / scaleFactor);

			g.saveState();
			g.addTransform(invT);


			int prevR = shadow.radius;

			shadow.radius *= scaleFactor;

			if (shadow.radius > 0)
			{
				shadow.drawForImage(g, mainImage);
			}

			shadow.radius = prevR;

			g.restoreState();
		}

		DropShadow shadow;
	};

	struct drawDropShadowFromPath : public DrawActions::ActionBase
	{
		SET_ACTION_ID(drawDropShadowFromPath);

		drawDropShadowFromPath(const Path& p_, Rectangle<float> a, Colour c_, int r_) :
			p(p_),
			c(c_),
			area(a),
			radius(r_)
		{
			//shadow.setColor(c);
			//shadow.setRadius(radius);
		}

		void perform(Graphics& g) override
		{
//			PathFactory::scalePath(p, area);
//			shadow.render(g, p);

#if 1
			auto spb = area.withPosition((float)radius, (float)radius).transformed(AffineTransform::scale(scaleFactor));

			auto copy = p;

			copy.scaleToFit(spb.getX(), spb.getY(), spb.getWidth(), spb.getHeight(), false);

			auto drawTargetArea = area.expanded((float)radius).transformed(AffineTransform::scale(scaleFactor));
			Image img(Image::PixelFormat::ARGB, drawTargetArea.getWidth(), drawTargetArea.getHeight(), true);
			Graphics g2(img);
			g2.setColour(c);
			g2.fillPath(copy);
			gin::applyStackBlur(img, radius);
			
			g.drawImageAt(img, drawTargetArea.getX(), drawTargetArea.getY());
#endif
		}

        // Soon...
		//melatonin::DropShadow shadow;

		Rectangle<float> area;
		Path p;
		Colour c;
		int radius;
	};

    struct drawSVG: public DrawActions::ActionBase
    {
		SET_ACTION_ID(drawSVG);

        drawSVG(var svgObject, Rectangle<float> bounds_, float opacity_):
           svg(svgObject),
           bounds(bounds_),
          opacity(opacity_)
        {
            
        };
        
        void perform(Graphics& g) override
        {
            if(auto obj = dynamic_cast<ScriptingObjects::SVGObject*>(svg.getObject()))
            {
                obj->draw(g, bounds, opacity);
            }
        }
        
        const float opacity;
        const Rectangle<float> bounds;
        var svg;
    };

	struct addShader : public DrawActions::ActionBase
	{
		SET_ACTION_ID(addShader);

		addShader(DrawActions::Handler* h, ScriptingObjects::ScriptShader* o, Rectangle<int> b) :
			obj(o),
			handler(h),
			bounds(b)
		{

		}

		void perform(Graphics& g) override
		{
			using namespace juce::gl;

			auto invT = AffineTransform::scale(1.0f / handler->getScaleFactor()).translated(bounds.getX(), bounds.getY());

			

			if (obj != nullptr && obj->shader != nullptr)
			{
				if (auto lastScreenshot = obj->getScreenshotBuffer())
				{
					g.drawImageTransformed(lastScreenshot->data, invT);
					return;
				}

				if (obj->dirty)
				{
					{
						obj->makeStatistics();

					}

					auto r = obj->shader->checkCompilation(g.getInternalContext());
					obj->setCompileResult(r);
					obj->dirty = false;

#if USE_BACKEND
					if (!obj->compiledOk())
					{
						int safeCount = 0;

                        if(OpenGLContext::getCurrentContext() != nullptr)
                        {
                            while (glGetError() != GL_NO_ERROR)
                            {
                                safeCount++;

                                if (safeCount > 10000)
                                    break;
                            };
                            
                            auto s = StringArray::fromLines(obj->getErrorMessage(true));
                            s.removeEmptyStrings();

                            for (auto l : s)
                                handler->logError(l);
                        }
                        else
                        {
                            handler->logError("Open GL is not enabled");
                        }
					}
#endif
				}

				if (obj->compiledOk())
				{
					obj->setGlobalBounds(handler->getGlobalBounds(), handler->getScaleFactor());

					obj->localRect = bounds.toFloat();

					auto enabled = obj->enableBlending;

					using namespace juce::gl;

					auto wasEnabled = glIsEnabled(GL_BLEND);

					int blendSrc;
					int blendDst;

					glGetIntegerv(GL_BLEND_SRC, &blendSrc);
					glGetIntegerv(GL_BLEND_DST, &blendDst);

					if (enabled)
					{
						glEnable(GL_BLEND);
						glBlendFunc((int)obj->src, (int)obj->dst);
					}

					obj->shader->fillRect(g.getInternalContext(), bounds);

					// reset it to default
					if (enabled)
					{
						if (!wasEnabled)
							glDisable(GL_BLEND);

						glBlendFunc(blendSrc, blendDst);
					}

					if (obj->shouldWriteToBuffer())
					{
						auto sb = handler->getScreenshotBounds(bounds);

						cachedOpenGlBuffer = new ScreenshotListener::CachedImageBuffer(sb);

						Image::BitmapData data(cachedOpenGlBuffer->data, Image::BitmapData::writeOnly);

						

						glFlush();
						glReadPixels(sb.getX(), sb.getY(), sb.getWidth(), sb.getHeight(), GL_BGR_EXT, GL_UNSIGNED_BYTE, data.getPixelPointer(0, 0));

						for (int y = 0; y < sb.getHeight() / 2; y++)
						{
							auto srcLine = data.getLinePointer(y);
							auto dstLine = data.getLinePointer(sb.getHeight() - y - 1);

							for (int x = 0; x < data.width * data.pixelStride; x++)
							{
								std::swap(srcLine[x], dstLine[x]);
							}
						}
					}
				}
				
				obj->renderWasFinished(cachedOpenGlBuffer);
			}
		}

		WeakReference<DrawActions::Handler> handler;
		WeakReference<ScriptingObjects::ScriptShader> obj;
		Rectangle<int> bounds;

		ScreenshotListener::CachedImageBuffer::Ptr cachedOpenGlBuffer;
	};
};



}
