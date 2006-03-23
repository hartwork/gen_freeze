//////////////////////////////////////////////////////////////////////////////// 
// Freeze Winamp Plugin
// 
// Copyright © 2006  Sebastian Pipping <webmaster@hartwork.org>
// 
// -->  http://www.hartwork.org
// 
// This source code is released under the GNU General Public License (GPL).
// See GPL.txt for details. Any non-GPL usage is strictly forbidden.
////////////////////////////////////////////////////////////////////////////////


#define WIN32_LEAN_AND_MEAN
#include <windows.h> 
#include "Winamp/Gen.h"
#include "Winamp/wa_ipc.h" 
#include "Winamp/wa_msgids.h" 



#define PLUGIN_TITLE    "Freeze Winamp Plugin"
#define PLUGIN_VERSION  "0.5"



int init();
void config(); 
void quit();



WNDPROC WndprocMainBackup = NULL;
LRESULT CALLBACK WndprocMain( HWND hwnd, UINT message, WPARAM wp, LPARAM lp );

WNDPROC WndprocEqualizerBackup = NULL;
LRESULT CALLBACK WndprocEqualizer( HWND hwnd, UINT message, WPARAM wp, LPARAM lp );

WNDPROC WndprocPlaylistBackup = NULL;
LRESULT CALLBACK WndprocPlaylist( HWND hwnd, UINT message, WPARAM wp, LPARAM lp );



bool bFreezeMain       = true;
bool bFreezeEqualizer  = true;
bool bFreezePlaylist   = true;

RECT rMain;
RECT rEqualizer;
RECT rPlaylist;

char * szWinampIni;



winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	PLUGIN_TITLE " " PLUGIN_VERSION,
	init,
	config, 
	quit,
	NULL,
	NULL
}; 



inline void ApplyRect( HWND h, RECT & r )
{
	SetWindowPos(
		h,
		NULL,
		r.left,
		r.top,
		r.right - r.left,
		r.bottom - r.top,
		SWP_NOACTIVATE
			| SWP_NOOWNERZORDER
			| SWP_NOSENDCHANGING
			| SWP_NOZORDER
	);
}



inline bool IsMouseDown()
{
	const SHORT iDown = 0x8000;
	return ( ( iDown & GetAsyncKeyState( GetSystemMetrics( SM_SWAPBUTTON ) ? VK_RBUTTON : VK_LBUTTON ) ) == iDown ); 
}



LRESULT CALLBACK WndprocMain( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
	switch( message )
	{
	case WM_MOVE:
		{
			if( !IsMouseDown() ) break;
			ApplyRect( hwnd, rEqualizer );
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			if( !bFreezeMain ) break;
			
			// Is window moved?
			WINDOWPOS * const winpos = ( WINDOWPOS * )lp;
			if( ( winpos->flags & SWP_NOMOVE ) != SWP_NOMOVE )
			{
				// Deny moving
				winpos->flags |= SWP_NOMOVE;
				return 0;
			}
			
		}
		break;

	}
	return CallWindowProc( WndprocMainBackup, hwnd, message, wp, lp );
}



LRESULT CALLBACK WndprocEqualizer( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
	switch( message )
	{
	case WM_MOVE:
		{
			if( !IsMouseDown() ) break;
			ApplyRect( hwnd, rEqualizer );
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			if( !bFreezeEqualizer ) break;
			
			// Is window moved?
			WINDOWPOS * const winpos = ( WINDOWPOS * )lp;
			if( ( winpos->flags & SWP_NOMOVE ) != SWP_NOMOVE )
			{
				// Deny moving
				winpos->flags |= SWP_NOMOVE;
				return 0;
			}
			
		}
		break;

	}
	return CallWindowProc( WndprocEqualizerBackup, hwnd, message, wp, lp );
}



LRESULT CALLBACK WndprocPlaylist( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
	switch( message )
	{
	case WM_MOVE:
		{
			if( !IsMouseDown() ) break;
			ApplyRect( hwnd, rEqualizer );
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			if( !bFreezePlaylist ) break;
			
			// Is window moved?
			WINDOWPOS * const winpos = ( WINDOWPOS * )lp;
			if( ( winpos->flags & SWP_NOMOVE ) != SWP_NOMOVE )
			{
				// Deny moving
				winpos->flags |= SWP_NOMOVE;
				return 0;
			}
			
		}
		break;

	}
	return CallWindowProc( WndprocPlaylistBackup, hwnd, message, wp, lp );
}



int init()
{
	HWND & hMain = plugin.hwndParent;

	
	// Get equalizer window
	const HWND hEqualizer = ( HWND )SendMessage( hMain, WM_WA_IPC, IPC_GETWND_EQ, IPC_GETWND );
	if( !hEqualizer || !IsWindow( hEqualizer ) ) return 1;

	// Exchange equalizer window procedure
	WndprocEqualizerBackup = ( WNDPROC )GetWindowLong( hEqualizer, GWL_WNDPROC );
	if( WndprocEqualizerBackup == NULL ) return 1;
	SetWindowLong( hEqualizer, GWL_WNDPROC, ( LONG )WndprocEqualizer );



	// Get playlist window
	const HWND hPlaylist = ( HWND )SendMessage( hMain, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND );
	if( !hPlaylist || !IsWindow( hPlaylist ) )
	{
		// Revert equalizer
		SetWindowLong( hEqualizer, GWL_WNDPROC, ( LONG )WndprocEqualizerBackup );
		return 1;
	}
	
	// Exchange playlist window procedure
	WndprocPlaylistBackup = ( WNDPROC )GetWindowLong( hPlaylist, GWL_WNDPROC );
	if( WndprocPlaylistBackup == NULL )
	{
		// Revert equalizer
		SetWindowLong( hEqualizer, GWL_WNDPROC, ( LONG )WndprocEqualizerBackup );
		return 1;
	}
	SetWindowLong( hPlaylist, GWL_WNDPROC, ( LONG )WndprocPlaylist );


	
	// Exchange main window procedure
	WndprocMainBackup = ( WNDPROC )GetWindowLong( hMain, GWL_WNDPROC );
	if( WndprocMainBackup == NULL )
	{
		// Revert equalizer
		SetWindowLong( hEqualizer, GWL_WNDPROC, ( LONG )WndprocEqualizerBackup );

		// Revert playlist
		SetWindowLong( hPlaylist, GWL_WNDPROC, ( LONG )WndprocPlaylistBackup );
		return 1;
	}
	SetWindowLong( hMain, GWL_WNDPROC, ( LONG )WndprocMain );



	// Get current window rects
	GetWindowRect( hMain, &rMain );
	GetWindowRect( hEqualizer, &rEqualizer );
	GetWindowRect( hPlaylist, &rPlaylist );

	// Get Winamp.ini path
	szWinampIni = ( char * )SendMessage( hMain, WM_WA_IPC, 0, IPC_GETINIFILE );	

	return 0;
}



void config()
{
	MessageBox(
		GetForegroundWindow(),
		PLUGIN_TITLE " " PLUGIN_VERSION "\n"
			"\n"
			"Copyright © 2006 Sebastian Pipping  \n"
			"<webmaster@hartwork.org>\n"
			"\n"
			"-->  http://www.hartwork.org",
		"About",
		MB_ICONINFORMATION
	);
} 



BOOL WritePrivateProfileInt( LPCTSTR lpAppName, LPCTSTR lpKeyName, int iValue, LPCTSTR lpFileName )
{
	TCHAR szBuffer[ 16 ];
	wsprintf( szBuffer, TEXT( "%i" ), iValue );
    return( WritePrivateProfileString( lpAppName, lpKeyName, szBuffer, lpFileName ) );
}



void quit()
{
	if( szWinampIni == NULL ) return;
	
	// Winamp wrote its config before us so we can overwrite it
	// If we don't do this the windows will be placed where
	// they were "virtually" moved which is no good idea
	WritePrivateProfileInt( "Winamp", "wx", rMain.left, szWinampIni );
	WritePrivateProfileInt( "Winamp", "wy", rMain.top, szWinampIni );
	WritePrivateProfileInt( "Winamp", "eq_wx", rEqualizer.left, szWinampIni );
	WritePrivateProfileInt( "Winamp", "eq_wy", rEqualizer.top, szWinampIni );
	WritePrivateProfileInt( "Winamp", "pe_wx", rPlaylist.left, szWinampIni );
	WritePrivateProfileInt( "Winamp", "pe_wy", rPlaylist.top, szWinampIni );
}



extern "C" __declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &plugin;
}
