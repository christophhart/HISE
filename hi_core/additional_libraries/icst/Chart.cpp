// Chart.cpp
// 2D chart class
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved.

#if DONT_INCLUDE_HEADERS_IN_CPP
#else
#include "common.h"
#endif

#if (defined(_WIN32) && defined(ICSTLIB_ENABLE_MFC))

//*************************************************************************
//***					begin conditional compilation					***
//*************************************************************************

#if DONT_INCLUDE_HEADERS_IN_CPP
#else
#include "Chart.h"
#include "MathDefs.h"
#endif

#define UNINITIALIZED 0			// chart state
#define INITIALIZED 1
#define DRAWN 2
#define ZPENS 13				// number of z color pens

namespace icstdsp {		// begin library specific namespace

BEGIN_MESSAGE_MAP(Chart, CWnd)
	ON_WM_PAINT()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

//******************************
//* construction / destruction
//*
Chart::Chart() {
	cstate = UNINITIALIZED; 
	zpen = NULL; 
	zrgb = NULL;
	dframe.SetRect(0,0,0,0);	// ensure valid GetDiagramFrame result anytime
}

Chart::~Chart()
{
	if (mdc.GetSafeHdc() != NULL) mdc.SelectObject(oldbitmap);
	if (zpen != NULL) {delete[] zpen;}
	if (zrgb != NULL) {delete[] zrgb;}
}

//********************
//* message handling
//*
// request to draw window
void Chart::OnPaint() 
{					
	if (cstate == UNINITIALIZED) return;
	CPaintDC dc(this); 
	dc.BitBlt(0,0,cwframe.Width(),cwframe.Height(),&mdc,0,0,SRCCOPY);
}

// process mouse events inside diagram: send message wParam = chart tag, 
// lParam = [x(31..18) y(17..4) release(3) press(2) left(1) right(0)]
void Chart::ProcessMouseEvent(UINT nFlags, CPoint point, int type)
{
	unsigned int d=0;
	if ((cstate != DRAWN) || (point.x < aframe.left) || (point.x > aframe.right)
		|| (point.y < aframe.top) || (point.y > aframe.bottom)) return;
	if (aframe.Width() > 0) {
		d = (16383*(point.x - aframe.left))/aframe.Width();}
	d <<= 14;
	if (aframe.Height() > 0) {
		d += ((16383*(point.y - aframe.top))/aframe.Height());}
	d <<= 4;
	switch(type)
	{
		case 1:		d += 6;								// left button down
					if (mouseselx || mousesely) {		// update selection
						Unselect();
						selxorigin = dxmin + 
								(dxmax-dxmin)*(float)(d>>18)/16383.0f;
						selyorigin = dymax +
								(dymin-dymax)*(float)((d>>4) & 0x3fff)/16383.0f;
					}
					break;						
		case 2:		d += 10; break;						// left button up
		case 3:		d += 5; break;						// right button down
		case 4:		d += 9; break;						// right button up	
		default:	if (nFlags & MK_RBUTTON) {d += 1;}	// move
					if (nFlags & MK_LBUTTON) {			
						d += 2;
						if (mouseselx || mousesely) {	// update selection
							selxorigin = __min(dxmax,__max(dxmin,selxorigin));
							selyorigin = __min(dymax,__max(dymin,selyorigin));
							if (mouseselx && (!mousesely)) {
								Select(selxorigin,MsgToX(d),dymin,dymax);
							}
							else if ((!mouseselx) && mousesely) {
								Select(dxmin,dxmax,selyorigin,MsgToY(d));
							}
							else {
								Select(selxorigin,MsgToX(d),selyorigin,MsgToY(d));
							}
						}
					}	
					break;
	}
	::PostMessage(pwhnd,msg,mtag,d);
}

// mouse moved
void Chart::OnMouseMove(UINT nFlags, CPoint point) {
	ProcessMouseEvent(nFlags, point, 0);}

// left mouse button pressed
void Chart::OnLButtonDown(UINT nFlags, CPoint point) {
	ProcessMouseEvent(nFlags, point, 1);}

// left mouse button released
void Chart::OnLButtonUp(UINT nFlags, CPoint point) {
	ProcessMouseEvent(nFlags, point, 2);}

// right mouse button pressed
void Chart::OnRButtonDown(UINT nFlags, CPoint point) {
	ProcessMouseEvent(nFlags, point, 3);}

// right mouse button released
void Chart::OnRButtonUp(UINT nFlags, CPoint point) {
	ProcessMouseEvent(nFlags, point, 4);}

// enable mouse response of chart window
void Chart::EnableMouseResponse() {
	if (cstate != UNINITIALIZED) CWnd::EnableWindow(true);}

// disable mouse response of chart window
void Chart::DisableMouseResponse() {
	if (cstate != UNINITIALIZED) CWnd::EnableWindow(false);}

// enable selection by mouse
// get: selected part on axis (true) or max range (false) 
void Chart::EnableMouseSelection(bool x, bool y) {mouseselx = x; mousesely = y;}

// disable selection by mouse
void Chart::DisableMouseSelection() {mouseselx = mousesely = false;}

// get x value from mouse event message data
float Chart::MsgToX(unsigned int lParam)
{
	if (cstate != DRAWN) return 0;
	float x = dxmin + (dxmax-dxmin)*(float)(lParam>>18)/16383.0f;
	if (xlg) {x = expf(x);}
	return x;
}

// get y value from mouse event message data
float Chart::MsgToY(unsigned int lParam) 
{
	if (cstate != DRAWN) return 0;
	float y = dymax + (dymin-dymax)*(float)((lParam>>4) & 0x3fff)/16383.0f;
	if (ylg) {y = expf(y);}
	return y;
}	
	
// true if right button is/was active
bool Chart::RightButton(unsigned int lParam) {return ((lParam & 0x1) != 0);}

// true if left button is/was active
bool Chart::LeftButton(unsigned int lParam) {return ((lParam & 0x2) != 0);}

// true if button has just been pressed
bool Chart::Pressed(unsigned int lParam) {return ((lParam & 0x4) != 0);}		

// true if button has just been released
bool Chart::Released(unsigned int lParam) {return ((lParam & 0x8) != 0);}

//****************
//* create chart
//*
// initialize chart, set general properties
int Chart::Init(CWnd *pwnd, CRect wframe, int labelsize, int titlesize, CString font, 
				bool title, bool xlabel, bool xtickl, bool ylabel, bool ytickl, 
				bool zlabel, bool frame, bool dcenter, int border, int ytadj, 
				int wndstyle, unsigned int msgtag, unsigned int msgbase)
{
	if (cstate != UNINITIALIZED) return 0;

	// init chart properties 
	titleon = title; xlabelon = xlabel;	xticklon = xtickl; ylabelon = ylabel;
	yticklon = ytickl; zlabelon = zlabel; frameon = frame; dcnt = dcenter; 

	// init mouse event messaging properties	
	pwhnd = pwnd->GetSafeHwnd(); msg = msgbase; mtag = msgtag;
	mouseselx = mousesely = false;
	
	// create chart window
	DWORD wstyle = WS_CHILD|WS_VISIBLE|WS_DISABLED, wexstyle = 0;
	if (pwnd == NULL) {wndstyle = 4;}
	switch(wndstyle) {
		case 1:		wexstyle = WS_EX_STATICEDGE; break;
		case 2:		wexstyle = WS_EX_CLIENTEDGE; break;
		case 3:		wexstyle = WS_EX_CLIENTEDGE|WS_EX_STATICEDGE; break;
		case 4:		wstyle = WS_OVERLAPPED|WS_VISIBLE|WS_DISABLED; break;
		case 5:		wstyle = WS_POPUP|WS_VISIBLE|WS_DISABLED; break;
		default:	break;
		}	
	wframe.NormalizeRect();
	BOOL result = CWnd::CreateEx(	wexstyle, 
									AfxRegisterWndClass(NULL),
									"",
									wstyle, 
									wframe,	
									pwnd,
									0		);	
	if (!result) return -1;

	// get chart window frame
	CWnd::GetClientRect(&cwframe);

	// create memory device context
	CClientDC dc(this);					
	if (!mdc.CreateCompatibleDC(&dc)) {CWnd::DestroyWindow(); return -1;}
	if (!mbitmap.CreateCompatibleBitmap(&dc,cwframe.Width(),cwframe.Height()))
		{CWnd::DestroyWindow(); return -1;}
	oldbitmap = (CBitmap*)mdc.SelectObject(&mbitmap);
	if (oldbitmap == NULL) {CWnd::DestroyWindow(); return -1;}

	// create label fonts
	if (labelsize <= 0) {
		lsize = 0; xlabelon = xticklon = ylabelon = yticklon = zlabelon = false;}
	else {lsize = labelsize;}
	if (xlabelon || xticklon || yticklon) {
		if (!hfont.CreateFont(-labelsize,0,0,0,0,0,0,0,ANSI_CHARSET,
		OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,font))
			{xlabelon = xticklon = yticklon = false;}
		}
	if (ylabelon || zlabelon) {
		if (!vfont.CreateFont(-labelsize,0,900,900,0,0,0,0,ANSI_CHARSET,
		OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,font))
			{ylabelon = zlabelon = false;}
		}
	if (titlesize <= 0) {titlesize = 0; titleon = false;}
	if (titleon) {
		if (!tfont.CreateFont(-titlesize,0,0,0,0,0,0,0,ANSI_CHARSET,
		OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,font))
			{titleon = false;}
		}

	// set diagram dimensions (drawing region includes right bottom edge!)
	long offset = 0;
	ytadj = __max(ytadj,-2*lsize);
	if (ylabelon && yticklon) {offset = (int)(4.5f*(float)(lsize + ytadj) + 0.5f);}
	else if (yticklon) {offset = (3*lsize + ytadj);}
	else if (ylabelon) {offset = (2*lsize);}
	else if (xticklon) {offset = (int)(1.5f*(float)lsize + 0.5f);}
	else if (frameon) {offset = 1;}
	dframe.left = cwframe.left + __max(offset,border);
	
	offset = 0;
	if (zlabelon) {offset = (2*lsize);}
	else if (xticklon) {offset = (int)(1.5f*(float)lsize + 0.5f);}
	else if (frameon) {offset = 1;}
	dframe.right = cwframe.right - 1 - __max(offset,border);

	offset = 0;
	if (titleon) {offset = titlesize + lsize;}
	else if (yticklon) {offset = lsize;}
	else if (frameon) {offset = 1;}
	dframe.top = cwframe.top + __max(offset,border);

	offset = 0;
	if (xlabelon && xticklon) {offset = (int)(3.5f*(float)lsize + 0.5f);}
	else if (xticklon || xlabelon) {offset = (2*lsize);}
	else if (yticklon) {offset = lsize;}
	else if (frameon) {offset = 1;}
	dframe.bottom = cwframe.bottom - 1 - __max(offset,border);

	if (dframe.Width() < 0) {dframe.left = dframe.right = cwframe.left;}
	if (dframe.Height() < 0) {dframe.top = dframe.bottom = cwframe.top;}

	// create z color pen palette
	CreatePalette();

	cstate = INITIALIZED;
	return 0;
}

// set chart properties, draw chart (except diagram grid and background) 
void Chart::DrawChart(	CString title, CString xlabel, CString ylabel, 
						CString zlabel, int xdivs, float xmin, float xmax,
						bool xlog, bool xgrid, int ydivs, float ymin, float ymax,
						bool ylog, bool ygrid, float zmin, float zmax, bool zlog,				
						COLORREF bgcolor, COLORREF txtcolor, COLORREF frmcolor,
						COLORREF dgcolor, COLORREF gdcolor, COLORREF selmask, 
						int dxborder, int dyborder)					
{
	int i,n,m;
	float a,b,step;
	bool fticks;
	CPoint anchor;
	CSize txtSize;
	CString str;
	
	if (cstate == UNINITIALIZED) return;
	cstate = INITIALIZED;

	// set diagram active region (includes right bottom edge):
	// where data is mapped to and mouse events are processed
	aframe = dframe;
	aframe.DeflateRect(dxborder,dyborder);
	if ((aframe.Width() < 0) || (aframe.Height() < 0)) {aframe = dframe;} 

	// set diagram properties, correct nonsense settings
	xgridon = xgrid; ygridon = ygrid;
	gcolor=gdcolor; dcolor=dgcolor; smask=selmask;
	xdiv = __max(xdivs,1); ydiv = __max(ydivs,1);
	if (xmax > xmin) {dxmin=xmin; dxmax=xmax;}
	else {dxmin=xmax; dxmax=xmin;}
	a = __max(fabsf(dxmax),fabsf(dxmin))*0.001f;
	if (a < FLT_MIN) {a = 1.0f;}
	if (a > (dxmax-dxmin)) {dxmax += (0.5f*a); dxmin -= (0.5f*a);}
	if (ymax > ymin) {dymin=ymin; dymax=ymax;}
	else {dymin=ymax; dymax=ymin;}
	a = __max(fabsf(dymax),fabsf(dymin))*0.001f;
	if (a < FLT_MIN) {a = 1.0f;}
	if (a > (dymax-dymin)) {dymax += (0.5f*a); dymin -= (0.5f*a);}
	if (zmax > zmin) {dzmin=zmin; dzmax=zmax;}
	else {dzmin=zmax; dzmax=zmin;}
	a = __max(fabsf(dzmax),fabsf(dzmin))*0.001f;
	if (a < FLT_MIN) {a = 1.0f;}
	if (a > (dzmax-dzmin)) {dzmax += (0.5f*a); dzmin -= (0.5f*a);}

	// handle logarithmic scales
	// principle: operate on a linear scale of log(data)
	xlg = ylg = zlg = false;
	if (xlog && (dxmax>0) && (dxmin>0)) {
		dxmax = logf(dxmax); dxmin = logf(dxmin); xlg = true;}
	if (ylog && (dymax>0) && (dymin>0)) {
		dymax = logf(dymax); dymin = logf(dymin); ylg = true;}
	if (zlog && (dzmax>0) && (dzmin>0)) {
		dzmax = logf(dzmax); dzmin = logf(dzmin); zlg = true;}

	// calculate data to diagram range mapping factor
	xmap = (float)aframe.Width()/(dxmax-dxmin);
	ymap = (float)aframe.Height()/(dymax-dymin);
	zmap = (ZPENS-1)/(dzmax-dzmin);

	// draw chart background, erase old content
	mdc.FillSolidRect(cwframe,bgcolor);

	// select text attributes 
	mdc.SelectObject(&hfont);
	mdc.SetTextAlign(TA_RIGHT);					// align text to upper right point
	mdc.SetBkMode(TRANSPARENT);					// of anchor
	mdc.SetTextColor(txtcolor);

	// draw x axis tick labels
	if (xticklon) {
		step = (dxmax-dxmin)/(float)xdiv;
		if (xlg) {fticks = UseFracTicks(expf(dxmin));}
		else {fticks = UseFracTicks(step);}
		a = dxmin;
		for (i=0; i<=xdiv; i++) {		
			Map(anchor,a,dymin,false);
			if (xlg) {b = expf(a);} else {b = a;}
			if (fticks) {str.Format("%.1f",b);} else {str.Format("%.0f",b);}
			txtSize = mdc.GetTextExtent(str);
			anchor.x += (txtSize.cx/2 + 1);
			anchor.y = dframe.bottom + lsize/2;
			mdc.TextOut(anchor.x,anchor.y,str);
			a += step;
		}
	}
			
	// draw y axis tick labels
	if (yticklon) {
		step = (dymax-dymin)/(float)ydiv;
		if (xlg) {fticks = UseFracTicks(expf(dymin));}
		else {fticks = UseFracTicks(step);}
		a = dymin;
		for (i=0; i<=ydiv; i++) {
			Map(anchor,dxmin,a,false);
			if (ylg) {b = expf(a);} else {b = a;}
			if (fticks) {str.Format("%.1f",b);} else {str.Format("%.0f",b);}
			anchor.x = dframe.left - lsize/2;
			anchor.y -= lsize/2;
			mdc.TextOut(anchor.x,anchor.y,str);
			a += step;
		}
	}
	
	// set diagram frame
	DrawFrame(frmcolor,false);

	// draw x label
	if (xlabelon) {
		txtSize = mdc.GetTextExtent(xlabel);	
		if (dcnt) {anchor.x = dframe.CenterPoint().x + txtSize.cx/2;}
		else {anchor.x = cwframe.CenterPoint().x + txtSize.cx/2;}
		anchor.y = cwframe.bottom - txtSize.cy - lsize/2;
		mdc.TextOut(anchor.x,anchor.y,xlabel);
	}

	// draw y label
	if (ylabelon) {
		mdc.SelectObject(&vfont);
		txtSize = mdc.GetTextExtent(ylabel);
		anchor.x = cwframe.left + (lsize-1)/2;								
		if (dcnt) {anchor.y = dframe.CenterPoint().y - txtSize.cx/2;}
		else {anchor.y = cwframe.CenterPoint().y - txtSize.cx/2;}
		mdc.TextOut(anchor.x,anchor.y,ylabel);
	}

	// draw z label and rainbow
	if (zlabelon && (zlabel != "")) {
		mdc.SelectObject(&vfont);
		txtSize = mdc.GetTextExtent(zlabel);
		n = lsize/2;
		anchor.x = cwframe.right - lsize - n - 2;
		m = (lsize + n*ZPENS + txtSize.cx)/2;
		if (dcnt) {anchor.y = dframe.CenterPoint().y + m - txtSize.cx;}
		else {anchor.y = cwframe.CenterPoint().y + m - txtSize.cx;}
		mdc.TextOut(anchor.x,anchor.y,zlabel);
		if (zrgb != NULL) {
			anchor.x += 2;
			anchor.y -= lsize; 
			for (i=0; i<ZPENS; i++) {	
				mdc.FillSolidRect(anchor.x,anchor.y,lsize,n,zrgb[i]);
				anchor.y -= n;
			}
		}
	}

	// draw title
	if (titleon) {
		mdc.SelectObject(&tfont);						
		txtSize = mdc.GetTextExtent(title);
		anchor.y = cwframe.top + (lsize-1)/2;
		if (dcnt) {anchor.x = dframe.CenterPoint().x + txtSize.cx/2;}
		else {anchor.x = cwframe.CenterPoint().x + txtSize.cx/2;}
		mdc.TextOut(anchor.x,anchor.y,title);
	}

	// set diagram background, draw grids
	DrawDiagram();

	RedrawWindow();
	cstate = DRAWN;
}

//  draw diagram grid and background
void Chart::DrawDiagram()
{
	int i;
	float step,y;
	CPoint start,stop;
	CPen pen(PS_SOLID,0,gcolor);
	CPen* oldpen = mdc.SelectObject(&pen);

	// draw diagram background erasing old content
	CRect df = dframe;
	df.InflateRect(0,0,1,1);				// make FillSolidRect draw right 
	mdc.FillSolidRect(df,dcolor);			// and bottom line too

	// draw grid frame
	if ((xgridon) || (ygridon)) {
		mdc.MoveTo(aframe.left,aframe.top);
		mdc.LineTo(aframe.right,aframe.top);
		mdc.LineTo(aframe.right,aframe.bottom);
		mdc.LineTo(aframe.left,aframe.bottom);
		mdc.LineTo(aframe.left,aframe.top);
		}

	// draw x axis grid
	if (xgridon) {
		step = (dxmax-dxmin)/(float)xdiv;
		y = dxmin;
		for (i=1; i<xdiv; i++) {
			y += step;
			Map(start,y,dymin,false);						
			Map(stop,y,dymax,false); 
			stop.y--;						// as LineTo does not draw end point
			mdc.MoveTo(start);			
			mdc.LineTo(stop);
			}
		}
	
	// draw y axis grid
	if (ygridon) {
		step = (dymax-dymin)/(float)ydiv;
		y = dymin;
		for (i=1; i<ydiv; i++) {
			y += step;
			Map(start,dxmin,y,false);					
			Map(stop,dxmax,y,false);
			stop.x++;						// as LineTo does not draw end point
			mdc.MoveTo(start);
			mdc.LineTo(stop);
			}
		}

	mdc.SelectObject(oldpen);
	sframe.SetRectEmpty();					// discard selection if any
}

// draw diagram frame	
void Chart::DrawFrame(COLORREF color, bool hidden)
{
	if ((cstate == UNINITIALIZED) || (!frameon)) return;
	CBrush brush(color);
	CRect df = dframe;
	df.InflateRect(1,1,2,2);		// make FrameRect draw right and bottom 
	mdc.FrameRect(df,&brush);		// line too
	if (!hidden) RedrawWindow();
}

// highlight diagram zone
// unlight by calling again
void Chart::Highlight(float x1, float x2, float y1, float y2,
					  COLORREF mask, bool hidden)
{
	if (cstate != DRAWN) return;
	CPoint pt;
	CRect r;
	Map(pt,__min(x1,x2),__min(y1,y2),true);
	r.left = pt.x;
	r.bottom = pt.y + 1;
	Map(pt,__max(x1,x2),__max(y1,y2),true);
	r.right = pt.x + 1;
	r.top = pt.y;
	DrawHighlight(r,mask);
	if (!hidden) RedrawWindow();
}

// select diagram segment
void Chart::Select(float x1, float x2, float y1, float y2, bool hidden)
{
	if (cstate != DRAWN) return;
	CPoint pt;
	CRect r,rtmp;
	selxmin = __min(x1,x2);
	selxmax = __max(x1,x2);
	selymin = __min(y1,y2);
	selymax = __max(y1,y2); 
	Map(pt,selxmin,selymin,true);
	r.left = pt.x;
	r.bottom = pt.y + 1;
	Map(pt,selxmax,selymax,true);
	r.right = pt.x + 1;
	r.top = pt.y;

	if (sframe.IsRectEmpty()) {					// no old zone	
		DrawHighlight(r,smask);
	}
	else if ((r.right <= sframe.left) || (r.left >= sframe.right)) 
	{
		DrawHighlight(sframe,smask);			// no zone intersection
		DrawHighlight(r,smask);	
	}
	else {										// intersecting zones
		if (r.top < sframe.top) {				// draw new top segment	
			rtmp = r;	
			rtmp.bottom = sframe.top;
			DrawHighlight(rtmp,smask);	
		}
		if (r.bottom > sframe.bottom) {			// draw new bottom segment
			rtmp = r;	
			rtmp.top = sframe.bottom;
			DrawHighlight(rtmp,smask);	
		}
		if (r.left < sframe.left) {				// draw new left segment
			rtmp.left = r.left;
			rtmp.right = sframe.left;
			rtmp.top = __max(r.top,sframe.top);
			rtmp.bottom = __min(r.bottom,sframe.bottom);
			DrawHighlight(rtmp,smask);	
		}
		if (r.right > sframe.right) {			// draw new right segment
			rtmp.left = sframe.right;
			rtmp.right = r.right;
			rtmp.top = __max(r.top,sframe.top);
			rtmp.bottom = __min(r.bottom,sframe.bottom);
			DrawHighlight(rtmp,smask);	
		}
		if (sframe.top < r.top) {				// erase old top segment
			rtmp = sframe;	
			rtmp.bottom = r.top;
			DrawHighlight(rtmp,smask);	
		}
		if (sframe.bottom > r.bottom) {			// erase old bottom segment
			rtmp = sframe;	
			rtmp.top = r.bottom;
			DrawHighlight(rtmp,smask);	
		}
		if (sframe.left < r.left) {				// erase old left segment
			rtmp.left = sframe.left;
			rtmp.right = r.left;
			rtmp.top = __max(sframe.top,r.top);
			rtmp.bottom = __min(sframe.bottom,r.bottom);
			DrawHighlight(rtmp,smask);	
		}
		if (sframe.right > r.right) {			// erase old right segment
			rtmp.left = r.right;
			rtmp.right = sframe.right;
			rtmp.top = __max(sframe.top,r.top);
			rtmp.bottom = __min(sframe.bottom,r.bottom);
			DrawHighlight(rtmp,smask);	
		}
	}
	sframe = r;
	if (!hidden) RedrawWindow();
}

// unselect diagram segment
void Chart::Unselect(bool hidden)
{
	if (cstate != DRAWN) return;
	if (!sframe.IsRectEmpty()) {
		DrawHighlight(sframe,smask);
		sframe.SetRectEmpty();
	}
	if (!hidden) RedrawWindow();
}

// get selection: r[0..3] = {xmin,xmax,ymin,ymax}, return false if unselected
bool Chart::GetSelection(float* r)
{
	if (cstate != DRAWN) {return false;}
	if (sframe.IsRectEmpty()) {return false;}
	r[0] = selxmin; r[1] = selxmax; r[2] = selymin; r[3] = selymax;
	return true;
}

// draw highlighted rectangular segment by a self-complementary operation
void Chart::DrawHighlight(CRect r, COLORREF hlmask)
{
	if (hlmask == 0) {mdc.InvertRect(r); return;}
	CPen pen(PS_SOLID,0,hlmask);
	CPen* oldpen = mdc.SelectObject(&pen);
	int i, dmode = mdc.GetROP2();
	mdc.SetROP2(R2_XORPEN);
	if (r.Width() > r.Height()) {
		for (i=r.top; i<r.bottom; i++) {
			mdc.MoveTo(r.left,i);
			mdc.LineTo(r.right,i);
		}
	}
	else {
		for (i=r.left; i<r.right; i++) {
			mdc.MoveTo(i,r.top);
			mdc.LineTo(i,r.bottom);
		}
	}
	mdc.SetROP2(dmode);
	mdc.SelectObject(oldpen);
}

//*******************
//* plot operations
//*
// plot line diagram
void Chart::PlotLine(float *xdata, float *ydata, int size, COLORREF pltcolor,
						bool hidden)
{
	if ((cstate != DRAWN) || (size<2)) return;
	CPoint pt;
	CPen pen(PS_SOLID,0,pltcolor);
	CPen* oldpen = mdc.SelectObject(&pen);
	Map(pt,xdata[0],ydata[0],true);
	mdc.MoveTo(pt);
	for (int i=1; i<size; i++) {
		Map(pt,xdata[i],ydata[i],true);
		mdc.LineTo(pt);
	}
	Map(pt,xdata[size-2],ydata[size-2],true);		// draw end point	
	mdc.LineTo(pt);		
	mdc.SelectObject(oldpen);
	if (!hidden) RedrawWindow();
}

void Chart::PlotLine(float *xdata, float *ydata, float *zdata, int size, 
						bool hidden)
{
	if ((cstate != DRAWN) || (size<2) || (zpen == NULL)) return;
	CPoint pt;
	int idx,previdx;
	idx = ZMap(zdata[0]);
	CPen* oldpen = mdc.SelectObject(zpen + idx);
	Map(pt,xdata[0],ydata[0],true);
	mdc.MoveTo(pt);
	for (int i=1; i<size; i++) {
		Map(pt,xdata[i],ydata[i],true);
		mdc.LineTo(pt);
		previdx = idx;
		idx = ZMap(zdata[i]);
		if (idx != previdx) {mdc.SelectObject(zpen + idx);}
	}
	mdc.SelectObject(zpen + previdx);				// draw end point
	Map(pt,xdata[size-2],ydata[size-2],true);			
	mdc.LineTo(pt);		
	mdc.SelectObject(oldpen);
	if (!hidden) RedrawWindow();
}

// plot point diagram
void Chart::PlotPoint(float *xdata, float *ydata, int size, COLORREF pltcolor,
						bool hidden)
{
	if (cstate != DRAWN) return;
	CPoint pt;
	for (int i=0; i<size; i++) {
		if (Map(pt,xdata[i],ydata[i],true)) {mdc.SetPixelV(pt,pltcolor);}
	}	
	if (!hidden) RedrawWindow();
}

void Chart::PlotPoint(float *xdata, float *ydata, float *zdata, int size, 
						 bool hidden)
{
	if ((cstate != DRAWN) || (zrgb == NULL)) return;
	CPoint pt;
	for (int i=0; i<size; i++) {
		if (Map(pt,xdata[i],ydata[i],true)) {mdc.SetPixelV(pt,zrgb[ZMap(zdata[i])]);}
	}	
	if (!hidden) RedrawWindow();
}

// plot circle diagram
void Chart::PlotCircle(float *xdata, float *ydata, int size, COLORREF pltcolor,
						unsigned int diameter, bool hidden)
{
	if (cstate != DRAWN) return;
	if (diameter < 2) {PlotPoint(xdata, ydata, size, pltcolor, hidden); return;}
	CPoint pt;
	CPen pen(PS_SOLID,diameter,pltcolor);
	CPen* oldpen = mdc.SelectObject(&pen);
	for (int i=0; i<size; i++) {
		if (Map(pt,xdata[i],ydata[i],true)) {
			mdc.MoveTo(pt);
			pt.x++;
			mdc.LineTo(pt);
		}
	}		
	mdc.SelectObject(oldpen);
	if (!hidden) RedrawWindow();
}

void Chart::PlotCircle(float *xdata, float *ydata, float *zdata, int size, 
						unsigned int diameter, bool hidden)
{
	if ((cstate != DRAWN) || (size<1)) return;
	if (diameter < 2) {PlotPoint(xdata, ydata, zdata, size, hidden); return;}
	CPen lzpen[ZPENS];
	CPoint pt;
	int i,idx,previdx;
	bool success = true;
	
	// create local pens
	for (i=0; i<ZPENS; i++) {
		if (lzpen[i].CreatePen(PS_SOLID,diameter,zrgb[i]) == 0) {success=false;}
	}
	if (!success) {PlotPoint(xdata, ydata, zdata, size, hidden); return;}
	
	// plot
	idx = ZMap(zdata[0]);
	CPen* oldpen = mdc.SelectObject(lzpen + idx);
	for (i=0; i<size; i++) {
		previdx = idx;
		idx = ZMap(zdata[i]);
		if (idx != previdx) {mdc.SelectObject(lzpen + idx);}
		if (Map(pt,xdata[i],ydata[i],true)) {
			mdc.MoveTo(pt);
			pt.x++;
			mdc.LineTo(pt);
		}
	}
	mdc.SelectObject(oldpen);
	if (!hidden) RedrawWindow();
}

// plot bulk diagram
void Chart::PlotVBulk(float* xdata, float* ydata, int size, COLORREF pltcolor,
						int thickness, bool hidden)
{
	if (cstate != DRAWN) return;
	CPoint pt;
	CRect r;
	r.bottom = aframe.bottom + 1;
	for (int i=0; i<size; i++) {
		Map(pt,xdata[i],ydata[i],true);
		r.left = pt.x - thickness;
		r.right = pt.x + thickness + 1;	
		r.top = pt.y;
		mdc.FillSolidRect(r,pltcolor);		
	}	
	if (!hidden) RedrawWindow();
}			

void Chart::PlotVBulk(float *xdata, float *ydata, float *zdata, int size, 
						int thickness, bool hidden)
{
	if ((cstate != DRAWN) || (zrgb == NULL)) return;
	CPoint pt;
	CRect r;
	r.bottom = aframe.bottom + 1;
	for (int i=0; i<size; i++) {
		Map(pt,xdata[i],ydata[i],true);
		r.left = pt.x - thickness;
		r.right = pt.x + thickness + 1;	
		r.top = pt.y;
		mdc.FillSolidRect(r,zrgb[ZMap(zdata[i])]);		
	}	
	if (!hidden) RedrawWindow();
}

void Chart::PlotVBulkInterval(float* xdata, float* y1data, float* y2data,
							  int size,	COLORREF pltcolor, int thickness,
							  bool hidden)
{
	if (cstate != DRAWN) return;
	float y1,y2;
	CPoint pt1,pt2;
	CRect r;
	for (int i=0; i<size; i++) {
		y1 = y1data[i]; y2 = y2data[i];
		if (y2 < y1) {y2 = y1; y1 = y2data[i];}
		Map(pt1,xdata[i],y1,true);
		Map(pt2,xdata[i],y2,true);
		r.left = pt1.x - thickness;
		r.right = pt1.x + thickness + 1;	
		r.bottom = pt1.y + 1;
		r.top = pt2.y;
		mdc.FillSolidRect(r,pltcolor);		
	}	
	if (!hidden) RedrawWindow();
}

void Chart::PlotVBulkInterval(float *xdata, float *y1data, float *y2data,
							  float *zdata, int size, int thickness,
							  bool hidden)
{
	if ((cstate != DRAWN) || (zrgb == NULL)) return;
	float y1,y2;
	CPoint pt1,pt2;
	CRect r;
	for (int i=0; i<size; i++) {
		y1 = y1data[i]; y2 = y2data[i];
		if (y2 < y1) {y2 = y1; y1 = y2data[i];}
		Map(pt1,xdata[i],y1,true);
		Map(pt2,xdata[i],y2,true);
		r.left = pt1.x - thickness;
		r.right = pt1.x + thickness + 1;	
		r.bottom = pt1.y + 1;
		r.top = pt2.y;
		mdc.FillSolidRect(r,zrgb[ZMap(zdata[i])]);		
	}	
	if (!hidden) RedrawWindow();
}

void Chart::PlotHBulkInterval(float* x1data, float* x2data, float* ydata,
							  int size,	COLORREF pltcolor, int thickness,
							  bool hidden)
{
	if (cstate != DRAWN) return;
	float x1,x2;
	CPoint pt1,pt2;
	CRect r;
	for (int i=0; i<size; i++) {
		x1 = x1data[i]; x2 = x2data[i];
		if (x2 < x1) {x2 = x1; x1 = x2data[i];}
		Map(pt1,x1,ydata[i],true);
		Map(pt2,x2,ydata[i],true);
		r.left = pt1.x;
		r.right = pt2.x + 1;
		r.bottom = pt1.y + thickness + 1;
		r.top = pt1.y - thickness;
		mdc.FillSolidRect(r,pltcolor);		
	}	
	if (!hidden) RedrawWindow();
}

void Chart::PlotHBulkInterval(float* x1data, float* x2data, float* ydata,
							  float* zdata, int size, int thickness,
							  bool hidden)
{
	if ((cstate != DRAWN) || (zrgb == NULL)) return;
	float x1,x2;
	CPoint pt1,pt2;
	CRect r;
	for (int i=0; i<size; i++) {
		x1 = x1data[i]; x2 = x2data[i];
		if (x2 < x1) {x2 = x1; x1 = x2data[i];}
		Map(pt1,x1,ydata[i],true);
		Map(pt2,x2,ydata[i],true);
		r.left = pt1.x;
		r.right = pt2.x + 1;
		r.bottom = pt1.y + thickness + 1;
		r.top = pt1.y - thickness;
		mdc.FillSolidRect(r,zrgb[ZMap(zdata[i])]);		
	}	
	if (!hidden) RedrawWindow();
}

// clear diagram
void Chart::Clear(bool hidden)
{
	if (cstate != DRAWN) return;
	DrawDiagram();
	if (!hidden) RedrawWindow();
}

//*************
//* auxiliary
//*
// map xy data to diagram point, clip at boundaries, false if point outside the diagram
bool Chart::Map(CPoint &p, float x, float y, bool conv)
{
	bool valid = true;
	if (conv) {
		if (xlg) {if (x>0) {x = logf(x);} else {x = dxmin; valid = false;}}
		if (ylg) {if (y>0) {y = logf(y);} else {y = dymin; valid = false;}}
		}
	if (x > dxmax) {x = dxmax; valid = false;}
	if (x < dxmin) {x = dxmin; valid = false;}
	if (y > dymax) {y = dymax; valid = false;}
	if (y < dymin) {y = dymin; valid = false;}
	p.x = aframe.left + (int)(0.5f + xmap*(x - dxmin)); 
	p.y = aframe.bottom - (int)(0.5f + ymap*(y - dymin));
	return valid;
}

// map z data to pen number
int Chart::ZMap(float z)
{
	if (zlg) {if (z>0) {z = logf(z);} else {z = dzmin;}}
	if (z > dzmax) z = dzmax;
	if (z < dzmin) z = dzmin;
	return (int)(0.5f + zmap*(z - dzmin)); 
}

// true if fractional tick labels should be used, x = tick interval
bool Chart::UseFracTicks(float x)				
{
	x = fabsf(x);
	if (fabsf(floor(x + 0.5f) - x) > 0.01f*x) return true;
	return false;
}

// create pen palette for z color
void Chart::CreatePalette()
{
	COLORREF color[ZPENS]= {RGB(0x00,0x00,0x66),RGB(0x40,0x00,0x88),
							RGB(0x66,0x00,0x99),RGB(0x88,0x00,0x99),
							RGB(0xaa,0x00,0x99),RGB(0xcc,0x00,0x80),
							RGB(0xdd,0x00,0x66),RGB(0xff,0x00,0x00),
							RGB(0xff,0x66,0x00),RGB(0xff,0x99,0x00),
							RGB(0xff,0xbb,0x00),RGB(0xff,0xd0,0x33),
							RGB(0xff,0xff,0x66)	};


	bool success = true;
	if (zpen != NULL) {delete[] zpen;}
	zpen = new CPen[ZPENS];
	if (zrgb == NULL) {zrgb = new COLORREF[ZPENS];}
	for (int i=0; i<ZPENS; i++) {
		zrgb[i] = color[i];
		if (zpen[i].CreatePen(PS_SOLID,0,color[i]) == 0) {success=false;}
	}
	if (!success) {delete[] zpen; zpen = NULL;}
}

// get device context for external graphic operations
CDC* Chart::GetSafeDeviceContext() {
	if (cstate != UNINITIALIZED) {return &mdc;} else {return NULL;}}
	
// get diagram frame for external graphic operations
CRect Chart::GetDiagramFrame() {return dframe;}

}	// end library specific namespace

//*************************************************************************
//***					end conditional compilation						***
//*************************************************************************

#endif

