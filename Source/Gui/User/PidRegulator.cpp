#include "PidRegulator.h"
#ifdef ENABLE_PIDREGULATOR

#include <Source/Core/Settings.h>
#include <Source/Core/Utils.h>
#include <Source/Core/Bitmap.h>
#include <Source/Gui/MainWnd.h>
#include <Source/Gui/Oscilloscope/Meas/Statistics.h>

// http://www.embeddedheaven.com/pid-control-algorithm-c-language.htm
// http://radhesh.wordpress.com/2008/05/11/pid-controller-simplified/

int m_nFocus = 0;

float fVoltPerDiv = 0.2f;
float fUpdate = 0.020f;

int m_nGraphX = 0;
int m_nTarget = -1;
int m_nInput = -1;
int m_nOutput = -1;
//int m_nCurrent = -1;

bool m_bRealModel = false;


/*
http://www.kvraudio.com/forum/viewtopic.php?p=4999198

transfer function :


				b0 + b1*z^-1 + b2*z^-2 + b3*z^-3 + ...
		H(z) = ----------------------------------------
				a0 + a1*z^-1 + a2*z^-2 + a3*z^-3 + ... 

code implementation :		

		a0*y[n] = b0*x[n] + b1*x[n-1] + b2*x[n-2] + b3*x[n-3] + ...
						  - a1*y[n-1] - a2*y[n-2] - a3*y[n-3] - ... 

1/(s^2+s) -> b0/(

1/(s+1) - > b0/(a0+a1*z^-1) -> y[n] = x[n]-y[n-1]
p/(qs+r) -> y[n] = (r*x[n]-q*y[n-1])/p

*/

void CWndPidRegulator::Simulation()
{
	// simple transfer function 1/(s+1)
	m_Pid.m_Current = m_Pid.m_Current*0.9f + m_Pid.m_Output*0.1f;
}

void CWndPidRegulator::Process()
{
	CMeasStatistics Meas;
	Meas.Process( CSettings::Measure::_CH1, CSettings::Measure::_All );
	m_Pid.m_Current = Meas.GetAvg();

	ui16 nValue = Settings.DacCalib.Get( m_Pid.m_Output );
	BIOS::GEN::ConfigureDc( nValue );
}

void CWndPidRegulator::OnTimer()
{
	if ( MainWnd.HasOverlay() || !BIOS::ADC::Enabled() )
		return;

	long lTick = BIOS::SYS::GetTick();

	m_Pid();

	if ( m_bRealModel )
		Process();
	else
		Simulation();

	if ( m_nFocus == 2 ) // graph
		OnTimerGraph();
	if ( m_nFocus == 1 )
		ShowValues( true );
	if ( m_nFocus == 0 ) // diagram
		ShowDiagramValues( CPoint(5, 55) );

	long lPassed = BIOS::SYS::GetTick() - lTick;
	if ( HasFocus() )
		BIOS::LCD::Printf(300, 20, RGB565(404040), RGB565(000000), "upd: %dms  ", lPassed);
}

void CWndPidRegulator::Create(CWnd *pParent, ui16 dwFlags)
{
	CWnd::Create("CWndPidRegulator", dwFlags, CRect(0, 16, 400, 240), pParent);
}

void CWndPidRegulator::_Highlight( CRect rcRect, int nR, int nG, int nB )
{
	for ( int y = rcRect.top; y < rcRect.bottom; y++ )
		for ( int x = rcRect.left; x < rcRect.right; x++ )
		{
			ui16 clrPixel = BIOS::LCD::GetPixel( x, y );
			int r = Get565R( clrPixel ) + nR;
			int g = Get565G( clrPixel ) + nG;
			int b = Get565B( clrPixel ) + nB;
			UTILS.Clamp<int>( r, 0, 255 );
			UTILS.Clamp<int>( g, 0, 255 );
			UTILS.Clamp<int>( b, 0, 255 );
			clrPixel = RGB565RGB(r, g, b);
			BIOS::LCD::PutPixel( x, y, clrPixel );
		}
}

void CWndPidRegulator::_Highlight( CPoint cpPoint, int nRadius, int nR, int nG, int nB )
{
	int nMax = nRadius*nRadius; 
	for ( int y = -nRadius; y < nRadius; y++ )
		for ( int x = -nRadius; x < nRadius; x++ )
			if ( x*x + y*y < nMax )
			{
				ui16 clrPixel = BIOS::LCD::GetPixel( x + cpPoint.x, y + cpPoint.y );
				int r = Get565R( clrPixel ) + nR;
				int g = Get565G( clrPixel ) + nG;
				int b = Get565B( clrPixel ) + nB;
				UTILS.Clamp<int>( r, 0, 255 );
				UTILS.Clamp<int>( g, 0, 255 );
				UTILS.Clamp<int>( b, 0, 255 );
				clrPixel = RGB565RGB(r, g, b);
				BIOS::LCD::PutPixel( x + cpPoint.x, y + cpPoint.y, clrPixel );
			}
}

void CWndPidRegulator::ShowLocalMenu(bool bFocus)
{
	/*static*/ const char* const ppszLocalMenu[] =
	{"Diagram", "Values", "Graph", NULL};

	int x = 4;
	for ( int i=0; ppszLocalMenu[i]; i++ )
	{
		if ( bFocus )
		{
			ui16 clrTab = (i==m_nFocus) ? RGB565(ffffff) : RGB565(b0b0b0);
			x += BIOS::LCD::Draw(x, 20, clrTab, RGB565(000000), CShapes::corner_left);
			BIOS::LCD::Bar( x, 20, x + strlen(ppszLocalMenu[i])*8, 36, clrTab);
			x += BIOS::LCD::Print(x, 20, RGB565(000000), RGBTRANS, ppszLocalMenu[i]);
			x += BIOS::LCD::Draw(x, 20, clrTab, RGB565(000000), CShapes::corner_right);
		} else
		{
			ui16 clrTab = (i==m_nFocus) ? RGB565(ffffff) : RGB565(808080);
			x += BIOS::LCD::Draw(x, 20, RGB565(b0b0b0), RGBTRANS, CShapes::sel_left);
			x += BIOS::LCD::Print(x, 20, clrTab, RGB565(b0b0b0), ppszLocalMenu[i]);
			x += BIOS::LCD::Draw(x, 20, RGB565(b0b0b0), RGBTRANS, CShapes::sel_right);
			x += 2;
		}
		x += 16;
	}
}

void CWndPidRegulator::ShowDiagram( CPoint& cpBase )
{
	CBitmap bmpDiagram;
	bmpDiagram.Load( bitmapRegulator );
	bmpDiagram.m_arrPalette[0] = RGB565(ff00ff); // make white transparent, drawing will be faster
	cpBase = CPoint( (400-bmpDiagram.m_width)/2, (240-bmpDiagram.m_height)/2 + 20 );
	bmpDiagram.Blit ( cpBase.x, cpBase.y );
	
	// highlight blocks
	_Highlight( CRect(131, 1, 228, 38) + cpBase, 0, -60, -60 );		// Proportional - red
	_Highlight( CRect(131, 63, 228, 100) + cpBase, -60, 0, -60 );	// Integral - green
	_Highlight( CRect(131, 125, 228, 162) + cpBase, -60, -60, -0 );	// Derivative - blue
	
	_Highlight( CRect(283, 70, 335, 93) + cpBase, 0, 0, -80 );		// Process - yellow
	_Highlight( cpBase + CPoint(67, 81), 12, -30, -30, -10 );
	_Highlight( cpBase + CPoint(254, 81), 12, -30, -30, -10 );
}

void CWndPidRegulator::ShowDiagramValues( CPoint cpBase )
{
	// values
	BIOS::LCD::Printf( cpBase.x + 140, cpBase.y - 14, RGB565(b00000), RGB565(ffffff), "Kp =%s",
		CUtils::FormatFloat5(m_Pid.m_Kp) );
	BIOS::LCD::Printf( cpBase.x + 140, cpBase.y + 48, RGB565(00b000), RGB565(ffffff), "Ki =%s",
		CUtils::FormatFloat5(m_Pid.m_Ki) );
	BIOS::LCD::Printf( cpBase.x + 140, cpBase.y + 110, RGB565(0000b0), RGB565(ffffff), "Kd =%s",
		CUtils::FormatFloat5(m_Pid.m_Kd) );

	BIOS::LCD::Printf( cpBase.x + 4, cpBase.y + 52, RGB565(404040), RGB565(ffffff), "S =%s",
		CUtils::FormatFloat5(m_Pid.m_Target) );
	BIOS::LCD::Printf( cpBase.x + 280, cpBase.y + 52, RGB565(404040), RGB565(ffffff), "O =%s",
		CUtils::FormatFloat5(m_Pid.m_Output) );
	BIOS::LCD::Printf( cpBase.x + 260, cpBase.y + 154, RGB565(404040), RGB565(ffffff), "I =%s",
		CUtils::FormatFloat5(m_Pid.m_Current) );
}

void CWndPidRegulator::ShowValues(bool bValues)
{
	struct
	{
		ui16 clr;
		const char* strDesc;
		const char* strVar;
		const char* strUnits;
		float *pValue;
	} const arrAttrs[] = 
	{ 
		{ RGB565(000000), "Set point", "S ", "V", &m_Pid.m_Target },
		{ RGB565(000000), "Refresh time", "\x7ft", "s", &m_Pid.m_dt },
		{ RGB565(ff0000), "Proportional", "Kp", "", &m_Pid.m_Kp },
		{ RGB565(00ff00), "Integral", "Ki", "", &m_Pid.m_Ki },
		{ RGB565(0000ff), "Derivative", "Kd", "", &m_Pid.m_Kd },
		{ RGB565(b0b0b0), NULL, NULL, NULL, NULL },
		{ RGB565(808080), "Output (Wave)", "O ", "V", &m_Pid.m_Output },
		{ RGB565(808080), "Input (CH1)", "I ", "V", &m_Pid.m_Current },
		{ RGB565(808080), "error", "e ", "", &m_Pid.m_error },
		{ RGB565(808080), "Out.Proportional", "Op", "", NULL },
		{ RGB565(808080), "Out.Integral", "Oi", "", NULL },
		{ RGB565(808080), "Out.Derivative", "Od", "", NULL },

		{ RGB565(808080), "epsilon", "\xEE ", "", (float*)&m_Pid.m_fEpsilon }
	};

	int y = 42;
	for ( int i = 0; i < (int)COUNT(arrAttrs); i++, y += 16 )
	{
		if ( arrAttrs[i].strDesc == NULL )
		{
			if ( !bValues )
				BIOS::LCD::Bar( 8, y+2, 300, y+3, arrAttrs[i].clr );
			y -= 10;
			continue;
		}

		float fValue = 0.0f;
		
		if ( arrAttrs[i].pValue )
			fValue = *arrAttrs[i].pValue;
		else
		{
			if ( strcmp(arrAttrs[i].strVar, "Op") == 0 )
				fValue = m_Pid.m_error * m_Pid.m_Kp;
			if ( strcmp(arrAttrs[i].strVar, "Oi") == 0 )
				fValue = m_Pid.m_integral * m_Pid.m_Ki;
			if ( strcmp(arrAttrs[i].strVar, "Od") == 0 )
				fValue = m_Pid.m_derivative * m_Pid.m_Kd;
		}

		if ( !bValues )
		{
			BIOS::LCD::Print(   8, y, RGB565(808080), RGB565(ffffff), arrAttrs[i].strDesc );
			BIOS::LCD::Printf( 208, y, arrAttrs[i].clr, RGB565(ffffff), "%s =%s %s", 
				arrAttrs[i].strVar, CUtils::FormatFloat5(fValue), arrAttrs[i].strUnits );
		} else {
			BIOS::LCD::Print( 208+4*8, y, arrAttrs[i].clr, RGB565(ffffff), CUtils::FormatFloat5(fValue) );
		}
	}
}

void CWndPidRegulator::ShowGraph()
{
	CRect rcGraph(60, 46, 400-20, 220);
	rcGraph.Inflate(1, 1, 1, 1);
	BIOS::LCD::Rectangle( rcGraph, RGB565(b0b0b0) );
	rcGraph.Deflate(1, 1, 1, 1);

	const ui16 pattern[] = { RGB565(b0b0b0), RGB565(ffffff), RGB565(ffffff), RGB565(ffffff) };


	for (int i=1; i<=4; i++)
	{
		int y = rcGraph.bottom - rcGraph.Height()*i/4;
		BIOS::LCD::Print( rcGraph.left-44, y-6, RGB565(000000), RGB565(ffffff), 
			CUtils::FormatVoltage(fVoltPerDiv*i*2.0f, 4) );
		if ( i==4 )
			continue;
		BIOS::LCD::Pattern( rcGraph.left+1, y, rcGraph.right-1, y+1, pattern, COUNT(pattern) );
	}

	BIOS::LCD::Print( rcGraph.left-10, rcGraph.bottom+4, RGB565(000000), RGB565(ffffff), "0" );

	ui8 c = 0;
	for (int i=0; i<rcGraph.Width(); i+=50, c++)
	{
		if ( i == 0 )
			continue;

		int x = rcGraph.left + i;
		BIOS::LCD::Pattern( x, rcGraph.top+1, x+1, rcGraph.bottom-1, pattern, COUNT(pattern) );
		
		if ( c & 1 )
			continue;

		BIOS::LCD::Print( x-12, rcGraph.bottom+4, RGB565(000000), RGB565(ffffff), 
			CUtils::FormatTime( fUpdate * i, 5 ) );
	}
	BIOS::LCD::Print( rcGraph.right - 90, rcGraph.top +  2, RGB565(ff0000), RGBTRANS, "--- Input");
	BIOS::LCD::Print( rcGraph.right - 90, rcGraph.top + 18, RGB565(00ff00), RGBTRANS, "--- Output");
	BIOS::LCD::Print( rcGraph.right - 90, rcGraph.top + 34, RGB565(000000), RGBTRANS, "--- Target");
}

void CWndPidRegulator::OnTimerGraph()
{
	CRect rcGraph(60, 46, 400-20, 220);

	int nPrevInput = m_nInput;
	int nPrevOutput = m_nOutput;
	int nPrevTarget = m_nTarget;

	m_nTarget = (int)( m_Pid.m_Target / fVoltPerDiv * rcGraph.Height() / 8.0f  );
	m_nInput = (int)( m_Pid.m_Current / fVoltPerDiv * rcGraph.Height() / 8.0f );
	m_nOutput = (int)( m_Pid.m_Output / fVoltPerDiv * rcGraph.Height() / 8.0f );

	UTILS.Clamp<int>( m_nInput, 0, rcGraph.Height() );
	UTILS.Clamp<int>( m_nTarget, 0, rcGraph.Height() );
	UTILS.Clamp<int>( m_nOutput, 0, rcGraph.Height() );
	
	if ( nPrevInput == -1 )
		return;

	if ( abs(m_nOutput - nPrevOutput) > 40 )
		BIOS::LCD::PutPixel( rcGraph.left + m_nGraphX, rcGraph.bottom - m_nOutput, RGB565(00ff00) );
	else
		BIOS::LCD::Line( rcGraph.left + m_nGraphX, rcGraph.bottom - nPrevOutput, 
			rcGraph.left + m_nGraphX, rcGraph.bottom - m_nOutput, RGB565(00ff00) );
	BIOS::LCD::Line( rcGraph.left + m_nGraphX, rcGraph.bottom - nPrevInput, 
		rcGraph.left + m_nGraphX, rcGraph.bottom - m_nInput, RGB565(ff0000) );
	BIOS::LCD::Line( rcGraph.left + m_nGraphX, rcGraph.bottom - nPrevTarget, 
		rcGraph.left + m_nGraphX, rcGraph.bottom - m_nTarget, RGB565(000000) );

	if ( ++m_nGraphX >= rcGraph.Width() )
	{
		Invalidate();
		m_nGraphX = 0;
	}
}

void CWndPidRegulator::OnPaint()
{
	if ( HasFocus() )
	{
		BIOS::LCD::Bar( m_rcClient.left, m_rcClient.top, m_rcClient.right, m_rcClient.top+20, RGB565(000000) );
		BIOS::LCD::Bar( m_rcClient.left, m_rcClient.top+20, m_rcClient.right, m_rcClient.bottom, RGB565(ffffff) );
		ShowLocalMenu(true);
	} else {
		BIOS::LCD::Bar( m_rcClient, RGB565(ffffff) );
		ShowLocalMenu(false);
	}

	switch ( m_nFocus )
	{
		case 0:
		{
			CPoint cpBase;
			ShowDiagram( cpBase );
			ShowDiagramValues( cpBase );
		};
		break;
		case 1:
			ShowValues( false );
		break;
		case 2:
			ShowGraph();
		break;
	}
}

void CWndPidRegulator::OnKey(ui16 nKey)
{
	if ( nKey == BIOS::KEY::KeyLeft )
	{
		if ( m_nFocus > 0 )
		{
			m_nFocus--;
			Invalidate();
		}
		return;
	}
	if ( nKey == BIOS::KEY::KeyRight )
	{
		if ( m_nFocus < 2 )
		{
			m_nFocus++;
			Invalidate();
		}
		return;
	}
	if ( nKey == BIOS::KEY::KeyEnter )
	{
		m_Pid.Reset();
		m_Pid.m_Output = 0;
		m_Pid.m_Current = 0;
		return;
	}
	CWnd::OnKey( nKey );
}

void CWndPidRegulator::OnMessage(CWnd* pSender, ui16 code, ui32 data)
{
	// LAYOUT ENABLE/DISABLE FROM TOP MENU BAR
	if (code == ToWord('L', 'D') )
	{
		KillTimer();
		return;
	}

	if (code == ToWord('L', 'E') )
	{
		SetTimer( (int)(fUpdate * 1000) );
		return;
	}
}

LINKERSECTION(".extra")
/*static*/ const unsigned char CWndPidRegulator::bitmapRegulator[] = {
	0x47, 0x42, 0x85, 0x03, 0xaa, 0x01, 0xff, 0xff, 0xff, 0xdf, 0xf7, 0xbe, 0xef, 0x7d, 0xde, 0xfb, 0xd6, 0x9a, 0xc6, 0x18, 0xad, 0x75, 0x9c, 0xd3, 0x7b, 0xef, 0x6b, 0x6d, 0x5a, 0xcb, 0x42, 0x28, 
	0x31, 0x86, 0x21, 0x24, 0x00, 0x20, 0xa8, 0x20, 0x03, 0xeb, 0x14, 0x01, 0x98, 0x50, 0x29, 0xfd, 0xcb, 0x1e, 0x0d, 0xd8, 0x40, 0x2d, 0x50, 0x31, 0x02, 0xf4, 0x10, 0x2b, 0x31, 0x02, 0xf1, 0x10, 
	0x01, 0xa0, 0x08, 0x85, 0x01, 0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x84, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa9, 0x85, 0x01, 0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x84, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa9, 0x85, 
	0x01, 0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x84, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa9, 0x85, 0x01, 0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x84, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa9, 0x85, 0x01, 0xaa, 0xf8, 0x10, 
	0x11, 0x11, 0x01, 0x21, 0xe1, 0x01, 0x5d, 0xd8, 0x40, 0x1d, 0x5e, 0x10, 0x13, 0x91, 0xe4, 0xcc, 0xe9, 0x1c, 0x81, 0x01, 0xaa, 0x98, 0x50, 0x1a, 0xac, 0x81, 0x04, 0xcf, 0xe0, 0x91, 0xf0, 0x2f, 
	0x10, 0x15, 0xdd, 0x84, 0x01, 0xd5, 0x82, 0x04, 0xff, 0x90, 0x81, 0xf0, 0x4a, 0x40, 0x03, 0x18, 0x33, 0x38, 0x7b, 0x10, 0x15, 0x33, 0x01, 0x53, 0x50, 0x1a, 0xa9, 0x85, 0x01, 0xaa, 0x40, 0x43, 
	0xa4, 0x01, 0x20, 0x18, 0x7a, 0x10, 0x55, 0xd8, 0x0b, 0xfb, 0x40, 0x04, 0x1f, 0x28, 0x09, 0x1f, 0x00, 0xf1, 0x01, 0x5d, 0xff, 0x30, 0x25, 0x92, 0x20, 0x1d, 0x58, 0x20, 0x6f, 0xf9, 0x0e, 0xfc, 
	0xb4, 0x04, 0x2f, 0x57, 0x88, 0x10, 0x51, 0x20, 0x4c, 0x12, 0x04, 0x84, 0x08, 0xb4, 0x01, 0xaa, 0x98, 0x50, 0x1a, 0xa3, 0x06, 0x4e, 0x20, 0x4f, 0xa2, 0x05, 0xb9, 0x79, 0x85, 0x81, 0x03, 0x78, 
	0xe6, 0xb4, 0x04, 0x18, 0xec, 0xd1, 0xf0, 0x0f, 0x10, 0x45, 0xd0, 0x19, 0x1f, 0x05, 0x8d, 0x30, 0x06, 0xf1, 0x70, 0x93, 0xf2, 0x9e, 0x48, 0x20, 0x2f, 0xf9, 0xf4, 0x02, 0x7f, 0xc8, 0x10, 0x64, 
	0xc1, 0x8b, 0xd9, 0x20, 0x1c, 0xa2, 0x01, 0xe8, 0x30, 0x2a, 0xc6, 0x83, 0x70, 0x4b, 0x94, 0x01, 0xdd, 0xf2, 0xc2, 0xbf, 0x93, 0x01, 0x9e, 0x20, 0x16, 0xe2, 0x06, 0x8e, 0x2a, 0x7a, 0xc7, 0x03, 
	0x7e, 0x9b, 0xf4, 0x00, 0x91, 0xf0, 0x0f, 0x10, 0x14, 0xe4, 0xf0, 0xdf, 0x1c, 0x1f, 0x4d, 0xa3, 0x01, 0x5d, 0xf1, 0x00, 0x52, 0xf3, 0x80, 0xd5, 0x82, 0x02, 0xff, 0x9f, 0x40, 0x4d, 0x37, 0xe1, 
	0x60, 0x1f, 0x81, 0x22, 0x0e, 0x81, 0x01, 0x6e, 0x30, 0x1e, 0x83, 0x01, 0xaa, 0x83, 0x01, 0x5d, 0xb9, 0x40, 0x1d, 0x68, 0x30, 0x1a, 0xa3, 0x01, 0x5e, 0x20, 0x24, 0xc9, 0x10, 0x8a, 0xb0, 0x98, 
	0xde, 0x01, 0x40, 0x1c, 0xe1, 0x01, 0xf7, 0xe4, 0x00, 0xb1, 0xf0, 0x4f, 0x10, 0x15, 0xd2, 0x02, 0x8e, 0x6f, 0x10, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf1, 0x01, 0x11, 0x30, 0x1d, 0x5e, 0x10, 0x13, 
	0x81, 0xb1, 0xa6, 0xc4, 0x07, 0x29, 0xb4, 0x2a, 0xc7, 0x10, 0xa9, 0x89, 0x34, 0xc9, 0x30, 0x5c, 0x10, 0x17, 0x92, 0x01, 0x6c, 0x40, 0x1a, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x01, 
	0xaa, 0x40, 0x15, 0xa5, 0x01, 0x89, 0x50, 0x35, 0xa4, 0xab, 0x81, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xda, 0x30, 0x15, 0xdd, 0x20, 0x1d, 0x5a, 0x81, 0x03, 0x3b, 0x69, 0x70, 0x17, 0x43, 0x01, 0x65, 
	0x50, 0x1a, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0xd2, 0x03, 0x15, 0xa6, 0xa8, 0x10, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xa8, 0x10, 0x19, 
	0x7f, 0x20, 0x1a, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0xf2, 0x01, 0x11, 0xa8, 0x10, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 
	0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 
	0xd6, 0x83, 0x01, 0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0xcb, 0x10, 
	0x15, 0xdd, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x01, 0xa9, 0xcb, 0x10, 0x14, 0xdd, 0x20, 0x1d, 0x5d, 0xa3, 
	0x01, 0x5d, 0xd2, 0x01, 0xeb, 0xcb, 0x1a, 0x1e, 0xa8, 0x30, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0x83, 0x00, 0x5e, 0xb1, 0xa0, 0x8d, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 
	0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 
	0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 
	0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 
	0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 
	0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xdb, 0x94, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 
	0xa3, 0x01, 0x5d, 0xf9, 0x20, 0x15, 0xd8, 0x94, 0x00, 0x31, 0xc3, 0xfd, 0xc7, 0xe2, 0x01, 0x8e, 0xcb, 0x1d, 0x1e, 0xbd, 0x20, 0x1d, 0x5d, 0xa3, 0x01, 0x5d, 0xd2, 0x01, 0xe8, 0xbb, 0x16, 0x25, 
	0xba, 0xe2, 0x00, 0x53, 0xf0, 0xef, 0x84, 0x00, 0x62, 0xf0, 0xbf, 0x20, 0x1a, 0xac, 0xb1, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xda, 0x30, 0x15, 0xdd, 0x20, 0x1d, 0x5c, 0xb1, 0x01, 0xaa, 0xf2, 0x00, 
	0x41, 0xf0, 0xd9, 0x94, 0x02, 0x6f, 0xb8, 0x30, 0x1a, 0xac, 0xb1, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xad, 0x20, 0x13, 0x62, 0x81, 0x63, 0xb4, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0x96, 0x01, 0x43, 0x85, 
	0x01, 0xaa, 0xe2, 0x06, 0x36, 0x9e, 0x75, 0x28, 0xf2, 0x00, 0x29, 0x62, 0x00, 0xe2, 0x04, 0x7c, 0xed, 0xc1, 0xb4, 0xde, 0xd8, 0x2b, 0x20, 0x1a, 0xa8, 0x50, 0x25, 0xc3, 0x86, 0x01, 0x5d, 0xd2, 
	0x01, 0xd5, 0x94, 0x0a, 0x8c, 0xed, 0xcb, 0xcd, 0xec, 0x79, 0xf1, 0x04, 0x6e, 0xd8, 0x34, 0x04, 0x38, 0xde, 0x7f, 0x30, 0x15, 0xdd, 0x20, 0x1d, 0x58, 0x60, 0x16, 0xa9, 0x50, 0x1a, 0xaa, 0x20, 
	0x48, 0xec, 0x72, 0x40, 0x14, 0x81, 0xd0, 0x5c, 0x20, 0x18, 0xf8, 0x6e, 0x1f, 0x79, 0xd1, 0x02, 0x7c, 0x0e, 0x53, 0x20, 0xa9, 0xb2, 0x02, 0x8e, 0x8a, 0x10, 0x26, 0xeb, 0x92, 0x01, 0xaa, 0x95, 
	0x01, 0x99, 0x86, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xe3, 0x02, 0x9e, 0x7a, 0x10, 0x27, 0xe9, 0xa1, 0x01, 0x28, 0xbb, 0x10, 0x1d, 0x39, 0x10, 0x2b, 0xd3, 0xc1, 0x02, 0x3d, 0xbd, 0x30, 0x15, 0xdd, 
	0x20, 0x1d, 0x58, 0x60, 0x17, 0x69, 0x50, 0x1a, 0xa8, 0x20, 0x2c, 0xc1, 0xc1, 0x02, 0x5e, 0x9a, 0x20, 0x19, 0x98, 0x60, 0x1b, 0x79, 0xd1, 0x01, 0x7b, 0x86, 0x01, 0x99, 0x92, 0x02, 0x7e, 0x3f, 
	0x10, 0x1c, 0xbf, 0x10, 0x1a, 0xaf, 0x40, 0x21, 0x31, 0x96, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xc3, 0x01, 0x9d, 0xf1, 0x02, 0x1d, 0x98, 0x10, 0x13, 0xd8, 0xb1, 0x00, 0x81, 0xe0, 0xf1, 0xe0, 0x95, 
	0x02, 0x6e, 0x48, 0x20, 0x23, 0xe7, 0xb3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xf1, 0x01, 0x17, 0x2b, 0x19, 0x3a, 0x30, 0x2b, 0xfa, 0xf4, 0x01, 0xaa, 0xe1, 0x01, 0x8e, 0x92, 0x02, 0x6e, 0x48, 0x20, 
	0x19, 0x98, 0x60, 0x1b, 0x7b, 0xb1, 0x00, 0x11, 0x50, 0x09, 0x10, 0x17, 0xb8, 0x60, 0x19, 0x98, 0x20, 0x1c, 0x9a, 0x20, 0x25, 0xe2, 0xd1, 0x01, 0xaa, 0xf4, 0x03, 0xad, 0xc4, 0xa3, 0x00, 0x51, 
	0xf0, 0xd9, 0x20, 0x15, 0xdd, 0x20, 0x18, 0x3b, 0x30, 0x1e, 0x7a, 0x20, 0x18, 0xd5, 0x03, 0x45, 0x6e, 0x15, 0x03, 0x8b, 0x10, 0x1d, 0x36, 0x01, 0x7d, 0xc2, 0x01, 0xd8, 0xa6, 0x01, 0xd5, 0x92, 
	0x00, 0xb1, 0xf0, 0x2a, 0x30, 0x18, 0xb9, 0x50, 0x1a, 0xad, 0x10, 0x19, 0xbb, 0x20, 0x21, 0xe5, 0xf1, 0x01, 0x99, 0x50, 0x32, 0x10, 0x1e, 0x40, 0x1b, 0x78, 0x10, 0x03, 0x1b, 0x2a, 0xc4, 0x60, 
	0x01, 0xc1, 0x00, 0x1f, 0x30, 0x17, 0x7b, 0x10, 0x19, 0x24, 0x02, 0x1e, 0x81, 0x01, 0xad, 0x81, 0x01, 0x7b, 0xc4, 0x05, 0x8e, 0xce, 0xd6, 0x50, 0x19, 0x9f, 0x10, 0x1a, 0xa6, 0x02, 0x78, 0x93, 
	0x80, 0x75, 0x01, 0x6d, 0xd1, 0x01, 0xaa, 0xc1, 0x02, 0x75, 0x15, 0x01, 0x24, 0x50, 0x14, 0x19, 0x10, 0x1d, 0x93, 0x03, 0x45, 0x30, 0x24, 0x00, 0xe1, 0x00, 0x21, 0xf0, 0xb9, 0x20, 0x15, 0xdb, 
	0x30, 0x06, 0x38, 0x05, 0x82, 0x01, 0xc8, 0x60, 0x27, 0x89, 0x38, 0x07, 0x50, 0x18, 0xc6, 0x01, 0x2b, 0xc1, 0x01, 0x27, 0xa2, 0x04, 0x12, 0x68, 0x3c, 0x10, 0x53, 0xc8, 0x9d, 0x14, 0x01, 0x23, 
	0xa1, 0x02, 0x1c, 0x35, 0x01, 0x32, 0x70, 0x15, 0x32, 0x02, 0x1e, 0x45, 0x00, 0xc1, 0xf2, 0xcc, 0xb1, 0xf0, 0x35, 0x02, 0x4e, 0x2f, 0x10, 0x53, 0xfb, 0x89, 0xad, 0x20, 0x13, 0x53, 0x01, 0xd5, 
	0x92, 0x00, 0xb1, 0xf0, 0x2f, 0x10, 0x69, 0xf8, 0x0b, 0xe5, 0x30, 0x19, 0xe9, 0x10, 0x18, 0x65, 0x02, 0x5a, 0x35, 0x01, 0xae, 0xc1, 0x01, 0xaa, 0x70, 0x16, 0x32, 0x01, 0x5d, 0x60, 0x0c, 0x1f, 
	0x2c, 0xcb, 0x1f, 0x03, 0x50, 0x16, 0xda, 0x10, 0x04, 0x30, 0x19, 0x96, 0x05, 0xef, 0x2d, 0xf7, 0xb4, 0x01, 0xb7, 0x70, 0x26, 0xf2, 0x20, 0x0e, 0x19, 0xf7, 0x3a, 0x4a, 0xf6, 0x9a, 0x8d, 0x56, 
	0xa1, 0x80, 0x93, 0x3f, 0xa3, 0x70, 0x12, 0x57, 0x03, 0x6d, 0xf3, 0x81, 0x0f, 0x8d, 0x0e, 0x61, 0xe3, 0xd9, 0x0e, 0x80, 0xe5, 0x07, 0x1d, 0x02, 0xa0, 0xd3, 0x01, 0xf8, 0x70, 0x17, 0xb6, 0x0b, 
	0x58, 0x70, 0x36, 0x83, 0x03, 0x86, 0x10, 0x24, 0x86, 0x20, 0xe6, 0x84, 0x05, 0x61, 0x42, 0x9f, 0xa0, 0xfd, 0x60, 0x19, 0x91, 0x00, 0x61, 0xe0, 0x08, 0x10, 0x23, 0xd3, 0x50, 0x25, 0xf3, 0x10, 
	0x3a, 0xfe, 0x26, 0x06, 0xc7, 0x01, 0x8f, 0xe7, 0x01, 0xaa, 0xc1, 0x01, 0xab, 0x50, 0x1b, 0x87, 0x01, 0x97, 0x81, 0x01, 0xf9, 0x40, 0x52, 0x98, 0x0f, 0x9f, 0x10, 0x02, 0x1f, 0x0b, 0x92, 0x01, 
	0x5d, 0x10, 0x35, 0xcf, 0x64, 0x01, 0x37, 0x15, 0xf0, 0x28, 0x70, 0x56, 0x54, 0x75, 0x62, 0x32, 0x00, 0x70, 0xe8, 0x20, 0x15, 0xd6, 0x02, 0x5f, 0x31, 0x03, 0xaf, 0xe2, 0x60, 0x6d, 0x50, 0x29, 
	0xfc, 0x70, 0x65, 0xc2, 0x04, 0x32, 0x40, 0x21, 0x53, 0x10, 0x12, 0x51, 0x22, 0x08, 0xa1, 0x04, 0x53, 0x03, 0x81, 0x01, 0xc8, 0xb1, 0x03, 0x4f, 0xc2, 0x10, 0xd5, 0xac, 0xb3, 0xfa, 0x6e, 0x9b, 
	0xd0, 0x41, 0xbd, 0xd1, 0x6c, 0x29, 0xd7, 0xd7, 0x6f, 0x82, 0x53, 0x10, 0xc2, 0xf2, 0x98, 0xc7, 0x00, 0x51, 0xf0, 0x81, 0x01, 0x84, 0x60, 0x2c, 0x83, 0xb1, 0x40, 0x22, 0x03, 0xeb, 0x7a, 0x13, 
	0x7e, 0xcd, 0xbe, 0xc9, 0x81, 0xa2, 0xd2, 0xc1, 0xd0, 0x71, 0x03, 0x33, 0x26, 0x2f, 0x3c, 0x5d, 0x49, 0x20, 0x0b, 0x1f, 0x02, 0xf1, 0x01, 0xdb, 0x18, 0x00, 0x50, 0x1a, 0xf4, 0x05, 0x47, 0x93, 
	0xb6, 0x10, 0x45, 0xbc, 0xd8, 0x10, 0x1e, 0x61, 0x00, 0x41, 0x73, 0xe6, 0x07, 0x2c, 0x05, 0x60, 0x2a, 0xa1, 0x43, 0x24, 0x3e, 0x2f, 0x28, 0x9b, 0x70, 0x05, 0x1f, 0x08, 0x10, 0x18, 0x46, 0x02, 
	0xd6, 0x35, 0x40, 0x21, 0x00, 0xe1, 0xf1, 0xd5, 0x19, 0x00, 0x50, 0x2d, 0xf6, 0x1f, 0xf4, 0x9f, 0xcf, 0xc5, 0xf9, 0xfa, 0x09, 0xec, 0xf0, 0x4c, 0x9d, 0xae, 0x2e, 0xac, 0x78, 0xe9, 0xd6, 0x01, 
	0xb7, 0x10, 0x02, 0x33, 0x20, 0x7f, 0x30, 0x0e, 0x19, 0xfa, 0x0c, 0x75, 0xe0, 0x6c, 0x04, 0xe4, 0xe1, 0x50, 0xd2, 0x0c, 0x81, 0x00, 0x36, 0x30, 0xf1, 0xf3, 0xd6, 0x08, 0x4f, 0x0d, 0x6d, 0xf4, 
	0x08, 0xc0, 0xd6, 0x1e, 0x3e, 0x40, 0xc6, 0x00, 0xd8, 0x57, 0xd0, 0xa9, 0x4f, 0x31, 0x04, 0x6f, 0x30, 0xb5, 0xd1, 0xeb, 0x50, 0xf1, 0x58, 0xfb, 0x25, 0xef, 0x9f, 0x8c, 0xf5, 0xb0, 0x37, 0xaf, 
	0x4f, 0xc0, 0xfe, 0x14, 0x8b, 0xf8, 0x04, 0xab, 0xfd, 0x60, 0x19, 0xe4, 0xf1, 0xdd, 0x91, 0xe0, 0x08, 0x10, 0x06, 0x1f, 0x07, 0x81, 0x00, 0x95, 0xf0, 0xd6, 0xd1, 0xf9, 0x60, 0xe4, 0xcd, 0xfb, 
	0x50, 0xf9, 0x0b, 0x70, 0xad, 0x10, 0x17, 0xc1, 0xe9, 0xa3, 0x03, 0xea, 0xc1, 0xc6, 0x20, 0x21, 0xfa, 0x70, 0x08, 0x1f, 0x02, 0xe1, 0x00, 0x21, 0xf0, 0xb9, 0x20, 0x04, 0x5f, 0x0e, 0x1d, 0x0b, 
	0x30, 0x0c, 0x18, 0xfe, 0x02, 0xe3, 0x0e, 0x50, 0x8c, 0x03, 0xa7, 0xb0, 0xe0, 0x01, 0x00, 0x6b, 0x1e, 0x2d, 0xfc, 0x91, 0x00, 0x61, 0xf0, 0x78, 0x10, 0x0b, 0x5f, 0x0d, 0x5d, 0xf5, 0x3e, 0x0b, 
	0x90, 0xe5, 0x3f, 0x2b, 0xc0, 0x80, 0xbe, 0x8e, 0x08, 0xd0, 0x8c, 0x3f, 0x68, 0xb0, 0x81, 0xf2, 0x61, 0xc3, 0xd1, 0x72, 0x44, 0xf3, 0x10, 0xaf, 0x6d, 0x54, 0x30, 0xc8, 0x0c, 0x60, 0x0f, 0xba, 
	0xe6, 0x08, 0xd0, 0xe3, 0x5d, 0x09, 0xc0, 0x0e, 0x14, 0x05, 0x21, 0x0c, 0x2f, 0x28, 0x7c, 0x91, 0x02, 0x9f, 0xb9, 0x10, 0x2c, 0x70, 0xb1, 0x10, 0x02, 0x02, 0xe7, 0x01, 0x2e, 0x0c, 0x80, 0x5d, 
	0x05, 0xf1, 0x0c, 0x98, 0xb3, 0x03, 0x12, 0x06, 0x2f, 0x3c, 0x5d, 0x49, 0x20, 0x0b, 0x1f, 0x02, 0xe1, 0x03, 0x7f, 0xbe, 0x70, 0x2a, 0xf2, 0x20, 0xbe, 0x87, 0xb6, 0xf2, 0x02, 0x0a, 0x53, 0x0c, 
	0xcc, 0x2e, 0x40, 0xcb, 0x02, 0x3b, 0x28, 0x10, 0x2a, 0xa0, 0x52, 0x10, 0xe1, 0xf3, 0xe6, 0x9b, 0x91, 0x02, 0xaf, 0xb9, 0x10, 0x2d, 0x50, 0x51, 0x24, 0x43, 0x3f, 0x28, 0xa9, 0x60, 0x2d, 0xf3, 
	0x20, 0x28, 0xf8, 0x10, 0x7f, 0xf0, 0xcf, 0x8f, 0xa1, 0x06, 0x5f, 0xc7, 0x83, 0x91, 0xf1, 0x63, 0x1f, 0x0c, 0x60, 0x2b, 0x83, 0x14, 0x02, 0x22, 0xf0, 0x08, 0xe6, 0x5c, 0xa0, 0x8d, 0x4e, 0xa4, 
	0xe1, 0x6d, 0xe5, 0xba, 0x0e, 0x78, 0xf2, 0xbc, 0x40, 0x52, 0x11, 0x42, 0xf2, 0xe7, 0x12, 0x03, 0x7e, 0xf3, 0x70, 0x11, 0x71, 0x33, 0x96, 0xa6, 0x10, 0xb9, 0x9e, 0x64, 0x60, 0x78, 0x7a, 0x11, 
	0x00, 0x71, 0xa0, 0x69, 0x10, 0x17, 0xb5, 0x07, 0x5f, 0xc2, 0x4c, 0xf6, 0x14, 0xe1, 0x0e, 0xf5, 0x10, 0xef, 0x4f, 0xc0, 0xfe, 0x10, 0x28, 0xf8, 0x20, 0x23, 0xfd, 0x60, 0x69, 0x90, 0x19, 0xfe, 
	0x91, 0x02, 0x2d, 0x38, 0x10, 0x23, 0xfd, 0x91, 0x01, 0xc7, 0x10, 0x07, 0x1e, 0x00, 0x60, 0x1a, 0xa9, 0x10, 0x19, 0x81, 0x05, 0x7f, 0x50, 0xc9, 0x1c, 0x00, 0x30, 0x1b, 0x51, 0x07, 0x3f, 0x50, 
	0x67, 0xf6, 0x10, 0x21, 0xfa, 0x60, 0x4b, 0xe0, 0xdb, 0xe1, 0x00, 0x31, 0xf0, 0xc9, 0x20, 0x15, 0xd1, 0x03, 0x5c, 0xf6, 0x60, 0x6b, 0x86, 0xc0, 0x7f, 0x10, 0x6e, 0x60, 0x8c, 0x38, 0x10, 0x18, 
	0xe8, 0x20, 0x15, 0xd9, 0x10, 0x23, 0xfc, 0x91, 0x06, 0xd5, 0x01, 0x8f, 0xc7, 0x0f, 0x4e, 0x0c, 0x90, 0xd5, 0x3e, 0x0b, 0x90, 0x9d, 0x08, 0xdc, 0x07, 0xd0, 0x8c, 0x36, 0x09, 0xd8, 0xd1, 0x01, 
	0x77, 0xb1, 0x06, 0x7d, 0x89, 0xc0, 0x71, 0xfd, 0xb0, 0xac, 0x3d, 0xc9, 0xc0, 0x7e, 0x8b, 0x14, 0xaf, 0x68, 0xe3, 0xad, 0x0c, 0xa2, 0x60, 0x15, 0x33, 0x01, 0xd4, 0x70, 0x2b, 0xf5, 0x10, 0x14, 
	0x65, 0x02, 0x4d, 0x1f, 0x10, 0x26, 0xec, 0x2b, 0xf3, 0xca, 0x08, 0xd5, 0x08, 0xea, 0x90, 0xac, 0x35, 0x01, 0x35, 0x30, 0x1d, 0x5f, 0x10, 0x61, 0x7d, 0xfe, 0x93, 0xd1, 0x05, 0xf9, 0x06, 0xf7, 
	0x10, 0x15, 0x61, 0x01, 0xaf, 0x20, 0x76, 0xf7, 0x58, 0x3e, 0x41, 0x02, 0x9e, 0x73, 0x07, 0xd9, 0xbd, 0x6b, 0xe4, 0x10, 0x2c, 0xc6, 0x81, 0x01, 0xaa, 0x70, 0x15, 0x12, 0x02, 0x4d, 0x17, 0x02, 
	0xbf, 0x51, 0x01, 0x46, 0x50, 0x17, 0xda, 0x10, 0x17, 0x32, 0x01, 0x99, 0x50, 0x35, 0xef, 0x72, 0x02, 0x9f, 0xb1, 0x0f, 0x7f, 0x9f, 0xa0, 0xaf, 0xea, 0x2a, 0xfd, 0xa6, 0x0e, 0x78, 0xe8, 0xab, 
	0xae, 0x15, 0x01, 0xb7, 0xf2, 0x01, 0x7d, 0xb2, 0x01, 0x26, 0xe3, 0x02, 0x4b, 0x8f, 0x20, 0x17, 0xb6, 0x0b, 0x38, 0x73, 0x06, 0x85, 0x04, 0x86, 0x10, 0x24, 0x86, 0x20, 0x26, 0x85, 0x10, 0x01, 
	0x24, 0x00, 0x10, 0x04, 0x24, 0x00, 0x40, 0x19, 0x9f, 0x10, 0x1a, 0xa5, 0x01, 0xcc, 0x20, 0x27, 0xf8, 0x60, 0x17, 0xdd, 0x10, 0x1a, 0xa8, 0x10, 0x23, 0xea, 0x10, 0x83, 0xc6, 0xae, 0x54, 0xe2, 
	0x20, 0x26, 0xe7, 0x10, 0x67, 0xa0, 0x4a, 0xea, 0x30, 0x1f, 0x91, 0x0a, 0x48, 0x06, 0xde, 0x60, 0x9e, 0x8c, 0x10, 0x11, 0x22, 0x00, 0x28, 0x20, 0x15, 0xdb, 0x10, 0x01, 0x11, 0x30, 0x05, 0x32, 
	0x02, 0x73, 0x02, 0x10, 0x01, 0x12, 0x00, 0x28, 0x20, 0x1c, 0x85, 0x01, 0xcc, 0x20, 0x27, 0xf8, 0x60, 0x19, 0xbd, 0x10, 0x04, 0x12, 0x24, 0x43, 0x34, 0x43, 0x04, 0x85, 0x10, 0xa3, 0x89, 0xc0, 
	0x41, 0x02, 0x85, 0x10, 0x43, 0x87, 0x32, 0x93, 0x01, 0xd7, 0x84, 0x01, 0x6d, 0x50, 0x07, 0x1f, 0x0b, 0x1c, 0x2e, 0xf9, 0x50, 0x1d, 0x7a, 0x30, 0x15, 0xdd, 0x20, 0x1d, 0x5b, 0x40, 0x42, 0x01, 
	0x01, 0x10, 0x31, 0x3c, 0x31, 0x01, 0x9f, 0x81, 0x02, 0x5a, 0x26, 0x01, 0x89, 0xc2, 0x01, 0xaa, 0xd1, 0x01, 0x9c, 0x50, 0x07, 0x1f, 0x0b, 0x1c, 0x2e, 0xf9, 0x40, 0x23, 0xe4, 0xf1, 0x01, 0x99, 
	0x86, 0x01, 0xb7, 0x9d, 0x10, 0x17, 0xb8, 0x60, 0x19, 0x98, 0x20, 0x1a, 0xc4, 0x01, 0x5e, 0x3f, 0x2e, 0xfc, 0x40, 0x18, 0xde, 0x10, 0x1a, 0xad, 0x20, 0x16, 0x65, 0x01, 0x75, 0x91, 0x01, 0xf8, 
	0x20, 0x1b, 0x7b, 0x50, 0x15, 0xdd, 0x20, 0x1d, 0x5b, 0x30, 0x1c, 0xa4, 0x01, 0x5e, 0x3f, 0x2e, 0xfc, 0x40, 0x1a, 0xb9, 0x40, 0x23, 0x42, 0xa9, 0x10, 0x23, 0xe7, 0x82, 0x02, 0x7e, 0x3b, 0x30, 
	0x15, 0xdd, 0x20, 0x1d, 0x5b, 0x50, 0x14, 0x62, 0x01, 0x6d, 0x95, 0x01, 0xaa, 0xe1, 0x02, 0x5e, 0x58, 0x20, 0x19, 0xd9, 0x20, 0x19, 0x98, 0x60, 0x1b, 0x7d, 0x20, 0x17, 0x59, 0x70, 0x18, 0xcd, 
	0x20, 0x17, 0xb8, 0x60, 0x19, 0x99, 0x20, 0x24, 0xe8, 0xe1, 0x02, 0x4e, 0x8f, 0x10, 0x1a, 0xa9, 0x50, 0x0a, 0x19, 0x01, 0xe5, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xc3, 0x02, 0x6e, 0x5e, 0x10, 0x26, 
	0xe6, 0xae, 0x10, 0x27, 0xe8, 0xc1, 0x02, 0x8e, 0x7d, 0x30, 0x15, 0xdd, 0x20, 0x1d, 0x5e, 0x50, 0x03, 0x1f, 0x06, 0x95, 0x01, 0xaa, 0x82, 0x02, 0x8e, 0x6c, 0x10, 0x29, 0xe5, 0xa2, 0x01, 0x99, 
	0x86, 0x01, 0xb7, 0xd2, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xd2, 0x02, 0x7c, 0x3e, 0x54, 0x23, 0xa9, 0xb2, 0x03, 0x3d, 0xc5, 0x81, 0x03, 0x3a, 0xe7, 0x92, 0x01, 0xaa, 0xa5, 0x01, 0x66, 0xf5, 0x01, 
	0x5d, 0xd2, 0x01, 0xd5, 0xe3, 0x03, 0x5e, 0xb4, 0x81, 0x03, 0x4c, 0xe5, 0xee, 0x10, 0xe1, 0x9e, 0xc8, 0x53, 0x23, 0x58, 0xce, 0xa2, 0xf3, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xaa, 
	0x20, 0xd3, 0xbe, 0xb8, 0x53, 0x23, 0x69, 0xce, 0x9d, 0x20, 0x18, 0xe8, 0x6d, 0x1e, 0x6d, 0x20, 0x1c, 0x89, 0x70, 0x18, 0xc9, 0xc1, 0x02, 0x17, 0xb1, 0xd0, 0xe1, 0xd2, 0xc9, 0x3c, 0x20, 0x1a, 
	0xa8, 0x50, 0x24, 0x85, 0x86, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0x94, 0x02, 0x38, 0xc1, 0xd0, 0xe1, 0xd2, 0xc8, 0x2f, 0xf1, 0x02, 0x5f, 0x5d, 0x40, 0x15, 0xdd, 0x20, 0x1d, 0x58, 0x60, 0x2b, 0x6a, 
	0x85, 0x01, 0xaa, 0x83, 0x02, 0x7f, 0x2d, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xdc, 0x10, 0x29, 0xfd, 0x83, 0x01, 0xaa, 0xf4, 0x04, 0x4b, 0x0c, 0x4f, 0x50, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0x40, 
	0x2b, 0xfb, 0xa8, 0x20, 0x04, 0x2f, 0x05, 0xc4, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xf5, 0x04, 0x3c, 0x1c, 0x3f, 0x40, 0x1a, 0xaf, 0x20, 0x06, 0x2f, 0x01, 0xcc, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xcc, 
	0xc1, 0x00, 0x92, 0xf0, 0xdf, 0x20, 0x1a, 0xa8, 0x50, 0x07, 0x18, 0x00, 0xf5, 0x01, 0x4d, 0xd2, 0x01, 0xd5, 0xc4, 0x00, 0xb2, 0xf0, 0xb8, 0x82, 0x00, 0x44, 0xf0, 0x5b, 0x40, 0x15, 0xdd, 0x20, 
	0x1e, 0xa9, 0x69, 0x0b, 0x95, 0x91, 0xda, 0xe2, 0x00, 0x64, 0xf0, 0x1b, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xbc, 0x10, 0x63, 0x76, 0xe9, 0x74, 0xe2, 0x00, 0x6e, 0xb1, 0xa0, 0x8d, 0x20, 0x1d, 
	0x5b, 0x40, 0x04, 0x17, 0x0e, 0x17, 0x04, 0x98, 0x20, 0x22, 0xd3, 0xd4, 0x01, 0x5d, 0xf9, 0x20, 0x15, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x6f, 0x92, 0x01, 0xd5, 0xd4, 
	0x02, 0x3d, 0x2b, 0x82, 0x02, 0x3d, 0x3d, 0x40, 0x15, 0xdf, 0x92, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 
	0x22, 0xd3, 0xd4, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x6f, 0x92, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x2b, 0x82, 0x02, 0x3d, 0x3d, 0x40, 0x15, 
	0xdf, 0x92, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd2, 0x9f, 0x10, 0x08, 0x4c, 0x06, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 
	0xf9, 0x20, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x6f, 0x92, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x32, 0x01, 0x59, 0x28, 0x19, 0x69, 0xf1, 0x02, 0x3d, 0x3d, 0x40, 0x15, 
	0xdf, 0x92, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xde, 
	0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x6f, 0x92, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3d, 0x40, 0x15, 0xdf, 0x92, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 
	0x18, 0xce, 0xc1, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 
	0x6f, 0x92, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3d, 0x40, 0x15, 0xdf, 0x92, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 
	0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xf9, 0x20, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x6f, 0x92, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x3b, 0x82, 
	0x02, 0x3d, 0x3d, 0x40, 0x15, 0xdf, 0x92, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0xf9, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 
	0x5d, 0xf9, 0x20, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x68, 0x30, 0x17, 0xdc, 0xb1, 0xc1, 0xda, 0xd2, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x2b, 0x82, 0x02, 0x2d, 0x3d, 
	0x40, 0x15, 0xdd, 0x20, 0x1e, 0x8b, 0xb1, 0x62, 0x5b, 0xa8, 0x30, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x68, 0x30, 0x1a, 0xac, 0xb1, 0x01, 0x4d, 0xd2, 0x01, 0xd5, 
	0xd4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x2d, 0x3d, 0x40, 0x15, 0xdd, 0x20, 0x1d, 0x5c, 0xb1, 0x01, 0xaa, 0x83, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0x83, 0x01, 
	0xaa, 0xcb, 0x10, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xcb, 0x10, 0x1a, 0xa8, 0x30, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 
	0x01, 0x8c, 0xec, 0x10, 0x1d, 0x68, 0x30, 0x1a, 0xac, 0xb1, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x2b, 0x82, 0x02, 0x3d, 0x3d, 0x40, 0x15, 0xdd, 0x20, 0x1d, 0x5a, 0x81, 0x02, 0x6d, 
	0x54, 0x01, 0x47, 0x30, 0x21, 0x74, 0x81, 0x01, 0xaa, 0x83, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0x70, 0x43, 0xb5, 0x04, 0x20, 0x23, 0xa4, 
	0x30, 0x22, 0xe5, 0xa8, 0x10, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0x88, 0x10, 0x33, 0x39, 0xc1, 0x04, 0x44, 0x0b, 0x92, 
	0x01, 0x8a, 0x10, 0x1a, 0xa7, 0x01, 0xaa, 0x83, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0x60, 0x21, 0xf6, 0x10, 0x2a, 0xf5, 0x10, 0xb4, 0xe7, 
	0xc7, 0x90, 0x8f, 0x78, 0x7f, 0x70, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0xf1, 0x01, 0x49, 0x1b, 0x3a, 0xab, 0x6e, 0x40, 
	0xb8, 0xb0, 0x5f, 0x2c, 0x83, 0xd9, 0xf2, 0x02, 0x5e, 0x21, 0x02, 0x3f, 0x66, 0x01, 0xaa, 0x83, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0x60, 
	0x27, 0xf2, 0x20, 0x1c, 0x92, 0x0c, 0xf8, 0x49, 0x7f, 0x5d, 0xb0, 0x5e, 0x2b, 0x40, 0x54, 0xef, 0xc4, 0xa1, 0xf0, 0x68, 0x20, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 
	0xd3, 0xd4, 0x01, 0x5d, 0xd2, 0x01, 0xd5, 0x82, 0x00, 0x41, 0xf0, 0x91, 0x02, 0xcf, 0xeb, 0x40, 0x77, 0xe0, 0x6f, 0x98, 0xf1, 0x03, 0x46, 0xf2, 0x10, 0x1d, 0x82, 0x02, 0x4f, 0x56, 0x01, 0xaa, 
	0x83, 0x01, 0x6d, 0xec, 0x10, 0x1c, 0x89, 0x70, 0x18, 0xce, 0xc1, 0x01, 0xd6, 0x83, 0x01, 0xaa, 0x70, 0x1e, 0x71, 0x0f, 0x29, 0xe2, 0x05, 0xe0, 0x7b, 0xdf, 0x48, 0xe8, 0x0c, 0x1f, 0x55, 0x07, 
	0x48, 0x70, 0x68, 0x72, 0xc2, 0x00, 0x81, 0xf0, 0x71, 0x00, 0x91, 0xf0, 0x48, 0x20, 0x15, 0xdd, 0x20, 0x1d, 0x5d, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd4, 0x01, 0x5d, 0xf1, 0x01, 0x35, 
	0x30, 0x1d, 0x58, 0x20, 0x04, 0x1f, 0x09, 0x10, 0x04, 0x1f, 0x0c, 0xd2, 0x06, 0xbf, 0x40, 0xda, 0x36, 0x08, 0x67, 0x06, 0x30, 0x59, 0x41, 0x01, 0xaa, 0x10, 0x17, 0x42, 0x01, 0xa9, 0x70, 0x1a, 
	0xa8, 0x30, 0x16, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0xec, 0x10, 0x1d, 0x68, 0x30, 0x1a, 0xa8, 0x10, 0x19, 0x64, 0x02, 0x6a, 0x1b, 0x20, 0x26, 0xa2, 0x1c, 0x00, 0xc2, 0x00, 0xe1, 0xf0, 
	0x41, 0x00, 0x91, 0xf0, 0x48, 0x20, 0x15, 0xd1, 0x03, 0x5c, 0xf6, 0xf1, 0x01, 0xd5, 0xd4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3d, 0x40, 0x15, 0xee, 0x13, 0x12, 0x62, 0xf3, 0xc5, 0xd4, 0x82, 
	0x00, 0x41, 0xf0, 0x91, 0x00, 0x41, 0xf0, 0xdc, 0x20, 0x41, 0xfa, 0x94, 0xd2, 0x02, 0x26, 0x12, 0x02, 0x16, 0x28, 0x10, 0x2a, 0xa0, 0xe2, 0x32, 0x26, 0xde, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 
	0xec, 0x10, 0x0c, 0x83, 0xe2, 0xdf, 0x96, 0x00, 0x11, 0x10, 0x04, 0x10, 0x0d, 0x11, 0x00, 0x81, 0x00, 0x71, 0xf0, 0x6c, 0x20, 0x0a, 0x1f, 0x06, 0x10, 0x09, 0x1f, 0x04, 0x82, 0x00, 0x45, 0xf0, 
	0xe8, 0x2e, 0x03, 0xd4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3e, 0x40, 0x02, 0xe1, 0x31, 0x26, 0x2f, 0x3c, 0x5d, 0x48, 0x20, 0x04, 0x1f, 0x09, 0x10, 0x0a, 0x1f, 0x03, 0xc2, 0x04, 0xad, 0xae, 
	0x17, 0x00, 0x4f, 0x2b, 0x05, 0x50, 0x2a, 0xa0, 0x83, 0x30, 0x1e, 0xc1, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0x98, 0x20, 0x1a, 0xa6, 0x00, 0x1f, 0x21, 0x20, 0x58, 0x40, 0x4b, 0xe0, 0xae, 0xd2, 0x00, 
	0x71, 0xf2, 0x70, 0xa1, 0xf0, 0x58, 0x20, 0x15, 0xd1, 0x03, 0x5c, 0xf6, 0xf6, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3f, 0x60, 0x13, 0x53, 0x01, 0xd5, 0xf1, 0x01, 0x4a, 0x1f, 0x4d, 0xbd, 0xa3, 
	0xd2, 0x06, 0x6f, 0x60, 0x8f, 0x83, 0x01, 0xb5, 0x84, 0x01, 0xaa, 0x98, 0x20, 0x1c, 0x89, 0x70, 0x18, 0xc9, 0x82, 0x01, 0xaa, 0x94, 0x0c, 0xb8, 0x70, 0x5b, 0xc7, 0x07, 0xb9, 0x2f, 0x20, 0x01, 
	0x20, 0x12, 0x1f, 0x10, 0x15, 0xdd, 0x70, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd7, 0x01, 0xd5, 0xe6, 0x03, 0x77, 0x89, 0xd1, 0x00, 0x21, 0x80, 0x0f, 0x10, 0x1a, 0xa9, 0x82, 0x01, 0xc8, 0x97, 
	0x01, 0x8c, 0x98, 0x20, 0x1a, 0xad, 0x10, 0x41, 0x20, 0xbc, 0xe1, 0x03, 0x6c, 0x9a, 0xe6, 0x01, 0x5d, 0xd7, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3d, 0x70, 0x1d, 0x5e, 0x60, 0x34, 0x75, 0x3c, 
	0x10, 0x62, 0x0e, 0x60, 0x92, 0xd1, 0x01, 0xaa, 0x98, 0x20, 0x1c, 0x89, 0x70, 0x18, 0xc9, 0x82, 0x01, 0xaa, 0xd1, 0x05, 0x2e, 0xd3, 0xea, 0x17, 0x01, 0xd8, 0x10, 0x15, 0xdd, 0x70, 0x23, 0xd2, 
	0xb8, 0x20, 0x23, 0xd3, 0xd7, 0x01, 0xd5, 0xd8, 0x10, 0x2b, 0x80, 0x1c, 0x20, 0xe8, 0xe1, 0x01, 0xaa, 0x98, 0x20, 0x1c, 0x89, 0x70, 0x18, 0xc9, 0x82, 0x01, 0xaa, 0xe1, 0x04, 0x3e, 0x47, 0xf1, 
	0x01, 0xd8, 0xc8, 0x10, 0x15, 0xdd, 0x70, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd7, 0x01, 0xd5, 0xc8, 0x10, 0x7d, 0x90, 0x9f, 0x09, 0xcf, 0x10, 0x1a, 0xa9, 0x82, 0x01, 0xc8, 0x97, 0x01, 0x8c, 
	0x98, 0x20, 0x1a, 0xae, 0x10, 0x63, 0xbc, 0x0e, 0xb9, 0x1d, 0x00, 0xb8, 0x10, 0x15, 0xdd, 0x70, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0xd7, 0x01, 0xd5, 0xc8, 0x10, 0x55, 0xc7, 0x59, 0x01, 0x80, 
	0x0e, 0x10, 0x1a, 0xa9, 0x82, 0x01, 0xc8, 0x97, 0x01, 0x8c, 0x98, 0x20, 0x1a, 0xac, 0xb1, 0x01, 0x5d, 0xd7, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x3d, 0x3d, 0x70, 0x1d, 0x5c, 0xb1, 0x01, 0xaa, 0x98, 
	0x20, 0x1c, 0x89, 0x70, 0x18, 0xc9, 0x82, 0x01, 0xaa, 0xcb, 0x10, 0x14, 0xdd, 0x70, 0x23, 0xd3, 0xb8, 0x20, 0x22, 0xd3, 0xd7, 0x01, 0xe8, 0xbb, 0x16, 0x25, 0xba, 0x98, 0x20, 0x1c, 0x89, 0x70, 
	0x18, 0xc9, 0x82, 0x01, 0x7d, 0xcb, 0x1c, 0x1d, 0xad, 0x70, 0x23, 0xd3, 0xb8, 0x20, 0x22, 0xd3, 0x8c, 0x40, 0x1c, 0x89, 0x70, 0x18, 0xc8, 0xc4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x2d, 0x38, 0xc4, 
	0x01, 0xc8, 0x97, 0x01, 0x8c, 0x8c, 0x40, 0x23, 0xd3, 0xb8, 0x20, 0x23, 0xd3, 0x8c, 0x40, 0x1c, 0x89, 0x70, 0x18, 0xc8, 0xc4, 0x02, 0x3d, 0x3b, 0x82, 0x02, 0x2e, 0xb8, 0xc4, 0xa1, 0xe7, 0x97, 
	0x00, 0x3a, 0xc4, 0x90, 0x7a, 0x81, 0x00, 
}; // 4359 bytes, 389x170

#endif