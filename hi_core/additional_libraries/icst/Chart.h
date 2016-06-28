// Chart.h
// XY chart class with optional Z color
// special features: mouse-controlled selection, highlighting 
// send message on mouse event inside diagram: msg = msgbase, wParam = msgtag, 
// lParam = [x(31..18) y(17..4) release(3) press(2) left(1) right(0)], get value
// pairs corresponding to mouse position from lParam with MsgToX and MsgToY
// *** begin notes ***
//		Windows only: uses MFC for windows + graphics + mouse
// *** end notes ***
//
// This code is part of the ICST DSP Library version 1.2. It is released
// under the 2-clause BSD license. Copyright (c) 2008-2010, Zurich
// University of the Arts, Beat Frei. All rights reserved. 

#if (defined(_WIN32) && defined(ICSTLIB_ENABLE_MFC))

//*************************************************************************
//***					begin conditional compilation					***
//*************************************************************************

#ifndef _ICST_DSPLIB_CHART_INCLUDED
#define _ICST_DSPLIB_CHART_INCLUDED

namespace icstdsp {		// begin library specific namespace
										
class Chart : public CWnd
{
public:
	Chart();
	~Chart();
											//*** call once after construction ***
	int Init(								// returns 0 on success
		CWnd* pwnd=NULL,					// parent window
		CRect wframe=CRect(0,0,500,300),	// chart window frame   
		int labelsize=10,					// label font size
		int titlesize=14,					// title font size
		CString font="Arial",				// font type
		bool title=true,					// t: chart has title
		bool xlabel=true,					// t: .. x axis label
		bool xtickl=true,					// t: .. x axis tick labels
		bool ylabel=true,					// t: .. y axis label
		bool ytickl=true,					// t: .. y axis tick labels
		bool zlabel=false,					// t: .. z color label
		bool frame=true,					// t: diagram has frame
		bool dcenter=false,					// t: center labels at diagram frame
		int border=0,						// minimum chart border (pixels)
		int ytadj=0,						// adjust y tick space (+/- pixels)
		int wndstyle=0,						// window: 0-3 child, 4 free, 5 popup 
		unsigned int msgtag=0,				// message tag to identify chart
		unsigned int msgbase=WM_APP	);		// mouse event message
									
	void DrawChart(							//*** draw chart *** 
		CString title="Untitled",			// chart title
		CString xlabel="x",					// x axis label
		CString ylabel="y",					// y axis label
		CString zlabel="z",					// z color label, "": no rainbow
		int xdivs=10,						// nof x axis divisions
		float xmin=-1.0f,					// x axis minimum value
		float xmax=1.0f,					// x axis maximum value
		bool xlog=false,					// t: x axis has log scale
		bool xgrid=true,					// t: x axis has grid
		int ydivs=10,						// nof y axis divisions
		float ymin=-1.0f,					// y axis minimum value
		float ymax=1.0f,					// y axis maximum value
		bool ylog=false,					// t: y axis has log scale
		bool ygrid=true,					// t: y axis has grid
		float zmin=0,						// z color minimum value
		float zmax=1.0f,					// z color maximum value
		bool zlog=false,					// t: z color has log scale
		COLORREF bgcolor=RGB(80,80,80),		// chart background color
		COLORREF txtcolor=RGB(200,200,200),	// label and title text color
		COLORREF frmcolor=RGB(140,140,140),	// diagram frame color 	
		COLORREF dgcolor=RGB(180,180,180),	// diagram background color
		COLORREF gdcolor=RGB(160,160,160),	// diagram grid color
		COLORREF selmask=RGB(64,64,64),		// selection XOR color mask (0:invert)
		int dxborder=0,						// diagram x passive border (pixels)
		int dyborder=0	);					// diagram y passive border (pixels)
													
	void PlotLine(							//*** plot line diagram data *** 
		float* xdata,						// x data array
		float* ydata,						// y data array
		int size,							// data array size (nof elements)
		COLORREF pltcolor=RGB(0,0,255),		// plot color
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotLine(								
		float* xdata,						// x data array
		float* ydata,						// y data array
		float* zdata,						// z data array
		int size,							// data array size (nof elements)
		bool hidden=false );				// t: plot to memory, no chart update

	void PlotPoint(							//*** plot point diagram data *** 
		float* xdata,						// x data array
		float* ydata,						// y data array
		int size,							// data array size (nof elements)
		COLORREF pltcolor=RGB(0,0,255),		// plot color
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotPoint(								
		float* xdata,						// x data array
		float* ydata,						// y data array
		float* zdata,						// z data array
		int size,							// data array size (nof elements)
		bool hidden=false );				// t: plot to memory, no chart update

	void PlotCircle(						//*** plot filled circle diagram data *** 							
		float* xdata,						// x data array
		float* ydata,						// y data array
		int size,							// data array size (nof elements)
		COLORREF pltcolor=RGB(0,0,255),		// plot color
		unsigned int diameter=5,			// circle diameter
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotCircle(								
		float* xdata,						// x data array
		float* ydata,						// y data array
		float* zdata,						// z data array
		int size,							// data array size (nof elements)
		unsigned int diameter=5,			// circle diameter
		bool hidden=false );				// t: plot to memory, no chart update

	void PlotVBulk(							//*** plot bulk diagram data *** 
		float* xdata,						// x data array
		float* ydata,						// y data array
		int size,							// data array size (nof elements)
		COLORREF pltcolor=RGB(0,0,255),		// plot color
		int thickness=0,					// enlarge bulk by 2*thickness pixels
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotVBulk(								
		float* xdata,						// x data array
		float* ydata,						// y data array
		float* zdata,						// z data array
		int size,							// data array size (nof elements)
		int thickness=0,					// enlarge bulk by 2*thickness pixels
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotVBulkInterval(							
		float* xdata,						// x data array
		float* y1data,						// y first data array
		float* y2data,						// y second data array
		int size,							// data array size (nof elements)
		COLORREF pltcolor=RGB(0,0,255),		// plot color
		int thickness=0,					// enlarge bulk by 2*thickness pixels
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotVBulkInterval(								
		float* xdata,						// x data array
		float* y1data,						// y first data array
		float* y2data,						// y second data array
		float* zdata,						// z data array
		int size,							// data array size (nof elements)
		int thickness=0,					// enlarge bulk by 2*thickness pixels
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotHBulkInterval(							
		float* x1data,						// x first data array
		float* x2data,						// x second data array
		float* ydata,						// y data array
		int size,							// data array size (nof elements)
		COLORREF pltcolor=RGB(0,0,255),		// plot color
		int thickness=0,					// enlarge bulk by 2*thickness pixels
		bool hidden=false );				// t: plot to memory, no chart update
	void PlotHBulkInterval(							
		float* x1data,						// x first data array
		float* x2data,						// x second data array
		float* ydata,						// y data array
		float* zdata,						// z data array
		int size,							// data array size (nof elements)
		int thickness=0,					// enlarge bulk by 2*thickness pixels
		bool hidden=false );				// t: plot to memory, no chart update


	void Clear(bool hidden=false);			//*** clear diagram ***
	void DrawFrame(							//*** draw diagram frame ***	
		COLORREF color,						// frame color
		bool hidden=false );				// t: draw to memory, no chart update
	void Highlight(	float x1,				//*** highlight diagram zone ***
					float x2,				// unlight by calling again
					float y1,
					float y2,
					COLORREF mask,			// color XOR mask (0:invert)
					bool hidden=false	);	// t: draw to memory, no chart update
											//*** selection ***
	void Select(	float x1,				// select diagram zone
					float x2,				//
					float y1,				//
					float y2,				//
					bool hidden=false	);	// t: draw to memory, no chart update
	void Unselect(	bool hidden=false	);	// unselect diagram segment
	bool GetSelection(float* r);			// get selection, ret false if unselected
											// r[0..3] = {xmin,xmax,ymin,ymax} 
											//*** mouse ***
	void EnableMouseResponse();				// enable mouse response
	void DisableMouseResponse();			// disable mouse response
	void EnableMouseSelection(				// enable selection by mouse, get..
				bool x=true, bool y=true);	// t:selected part on axis, f:max range			
	void DisableMouseSelection();			// disable selection by mouse
	float MsgToX(unsigned int lParam);		// get x value from mouse message
	float MsgToY(unsigned int lParam);		// ..  y value ..
	bool RightButton(unsigned int lParam);	// .. "right button activity" ..
	bool LeftButton(unsigned int lParam);	// .. "left button activity" ..
	bool Pressed(unsigned int lParam);		// .. "button has just been pressed" ..
	bool Released(unsigned int lParam);		// .. "button has just been released" ..
											//*** graphic interface ***																
	CDC* GetSafeDeviceContext();			// pointer to device context (uninit:NULL)		
	CRect GetDiagramFrame();				// diagram frame (uninit:0)

private:
	int cstate;								// chart state
	CDC mdc;								// memory device context
	CBitmap mbitmap;						// mdc associated bitmap
	CBitmap* oldbitmap;						// old bitmap 
	CRect cwframe;							// chart window frame
	CRect dframe;							// diagram frame incl. passive border
	CRect aframe;							// active diagram part frame
	CRect sframe;							// selection frame
	COLORREF dcolor;						// diagram background color
	COLORREF gcolor;						// diagram grid color
	COLORREF smask;							// selection color mask 
	CFont hfont;							// horizontal label font
	CFont vfont;							// vertical label font
	CFont tfont;							// title font
	int lsize;								// label font size
	float xmap;								// x data to diagram mapping factor
	float ymap;								// y data to diagram mapping factor
	float zmap;								// z data to diagram mapping factor	
	float dxmin;							// x data minimum
	float dxmax;							// x data maximum
	float dymin;							// y data minimum
	float dymax;							// y data maximum
	float dzmin;							// z data minimum
	float dzmax;							// z data maximum
	bool xlg;								// t: x axis has log scale
	bool ylg;								// t: y axis has log scale
	bool zlg;								// t: z color has log scale
	int xdiv;								// nof x axis divisions				
	int ydiv;								// nof y axis divisions
	bool titleon;							// t: chart has title
	bool xlabelon;							// t: .. x axis label
	bool xticklon;							// t: .. x axis tick labels
	bool ylabelon;							// t: .. y axis label
	bool yticklon;							// t: .. y axis tick labels
	bool zlabelon;							// t: .. z color label
	bool frameon;							// t: diagram has frame
	bool xgridon;							// t: .. x axis grid
	bool ygridon;							// t: .. y axis grid
	bool dcnt;								// t: center labels at diagram frame
	bool mouseselx;							// t: x selection by mouse enabled
	bool mousesely;							// t: y selection by mouse enabled
	float selxmin;							// selection minimum x value
	float selxmax;							// selection maximum x value
	float selymin;							// selection minimum y value
	float selymax;							// selection maximum y value
	float selxorigin;						// x origin value of selection by mouse 
	float selyorigin;						// y origin value of selection by mouse
	HWND pwhnd;								// parent/owner window handle
	unsigned int msg;						// to post mousemove message to
	unsigned int mtag;						// chart message tag
	CPen* zpen;								// z color pens
	COLORREF* zrgb;							// z color RGB values
	void DrawDiagram();						// draw diagram grid + background
	void DrawHighlight(	CRect r,			// draw highlighted rectangular segment
						COLORREF hlmask	);	// 
	bool UseFracTicks(float x);				// true: use fractional ticks
	bool Map(CPoint &p, float x,			// map xy data to diagram point,
				float y, bool conv);		// clip at boundaries
	int ZMap(float z);						// map z data to pen number
	void CreatePalette();					// create pen palette for z color plot
	void ProcessMouseEvent(UINT nFlags,		// process mouse events (send
				CPoint point, int type);	// messages on move/click)
	Chart& operator = (const Chart& src);
	Chart(const Chart& src);

protected:									// message map functions
	afx_msg void OnPaint();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
};

}	// end library specific namespace

#endif

//*************************************************************************
//***					end conditional compilation						***
//*************************************************************************

#endif

