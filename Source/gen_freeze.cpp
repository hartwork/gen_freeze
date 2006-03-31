//////////////////////////////////////////////////////////////////////////////// 
// Freeze Winamp Plugin
// 
// Copyright � 2006  Sebastian Pipping <webmaster@hartwork.org>
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
#include "resource.h"



#define PLUGIN_TITLE    "Freeze Winamp Plugin"
#define PLUGIN_VERSION  "1.0"



int init();
void config(); 
void quit();



WNDPROC WndprocMainBackup = NULL;
LRESULT CALLBACK WndprocMain( HWND hwnd, UINT message, WPARAM wp, LPARAM lp );

WNDPROC WndprocEqualizerBackup = NULL;
LRESULT CALLBACK WndprocEqualizer( HWND hwnd, UINT message, WPARAM wp, LPARAM lp );

WNDPROC WndprocPlaylistBackup = NULL;
LRESULT CALLBACK WndprocPlaylist( HWND hwnd, UINT message, WPARAM wp, LPARAM lp );


// During runtime
bool bMoveableMain; 
bool bMoveableEqualizer;
bool bMoveablePlaylist;

// Settings to apply for _next_ start
bool bMoveableMainNext;
bool bMoveableEqualizerNext;
bool bMoveablePlaylistNext;


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
	case WM_WINDOWPOSCHANGING:
		{
			if( bMoveableMain ) break;
			
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
			if( bMoveableEqualizer || !IsMouseDown() ) break;
			ApplyRect( hwnd, rEqualizer );
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			if( bMoveableEqualizer ) break;
			
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
			if( bMoveablePlaylist || !IsMouseDown() ) break;
			ApplyRect( hwnd, rEqualizer );
		}
		return 0;

	case WM_WINDOWPOSCHANGING:
		{
			if( bMoveablePlaylist ) break;
			
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



	// Read config
	if( szWinampIni )
	{
		int res;
		res = GetPrivateProfileInt( "gen_freeze", "MoveableMain", 0, szWinampIni );
		bMoveableMain = bMoveableMainNext = res ? true : false;

		res = GetPrivateProfileInt( "gen_freeze", "MoveableEqualizer", 0, szWinampIni );
		bMoveableEqualizer = bMoveableEqualizerNext = res ? true : false;

		res = GetPrivateProfileInt( "gen_freeze", "MoveablePlaylist", 0, szWinampIni );
		bMoveablePlaylist = bMoveablePlaylistNext = res ? true : false;
	}
	else
	{
		bMoveableMain = bMoveableMainNext = false;
		bMoveableEqualizer = bMoveableEqualizerNext = false;
		bMoveablePlaylist = bMoveablePlaylistNext = false;
	}
	
	return 0;
}



void UpdateAllBox( const HWND h, const bool a, const bool b, const bool c )
{
	int iCount = 0;
	if( a ) iCount++;
	if( b ) iCount++;
	if( c ) iCount++;

	CheckDlgButton(
		h,
		IDC_ALL,
		iCount ? ( ( iCount < 3 ) ? BST_INDETERMINATE : BST_CHECKED ): BST_UNCHECKED
	);
}



BOOL CALLBACK WndprocConfig( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
	static bool bMoveableMainNextBefore;
	static bool bMoveableEqualizerNextBefore;
	static bool bMoveablePlaylistNextBefore;
	
	static bool bDiscardSettings;
	
	switch( message )
	{
		case WM_INITDIALOG:
		{
			// Save current settings
			bMoveableMainNextBefore       = bMoveableMainNext;
			bMoveableEqualizerNextBefore  = bMoveableEqualizerNext;
			bMoveablePlaylistNextBefore   = bMoveablePlaylistNext;
			
			bDiscardSettings = true;


			// Apply settings to checkboxes
			CheckDlgButton( hwnd, IDC_MAIN, ( bMoveableMainNext ? BST_CHECKED : BST_UNCHECKED ) );
			CheckDlgButton( hwnd, IDC_EQUALIZER, ( bMoveableEqualizerNext ? BST_CHECKED : BST_UNCHECKED ) );
			CheckDlgButton( hwnd, IDC_PLAYLIST, ( bMoveablePlaylistNext ? BST_CHECKED : BST_UNCHECKED ) );
			UpdateAllBox( hwnd, bMoveableMainNext, bMoveableEqualizerNext, bMoveablePlaylistNext );
			
			
			// Set window title
			char szTitle[ 512 ] = "";
			wsprintf( szTitle, "%s %s", PLUGIN_TITLE, PLUGIN_VERSION );
			SetWindowText( hwnd, szTitle );


			// Center window on parent
			RECT rp;
			GetWindowRect( GetForegroundWindow(), &rp );
			const int iParentWidth   = rp.right - rp.left;
			const int iParentHeight  = rp.bottom - rp.top;

			RECT rf;
			GetWindowRect( hwnd, &rf );
			const int iFreezeWidth   = rf.right - rf.left;
			const int iFreezeHeight  = rf.bottom - rf.top;

			int ox = ( iParentWidth - iFreezeWidth ) / 2 + rp.left;
			int oy = ( iParentHeight - iFreezeHeight ) / 2 + rp.top;

			MoveWindow( hwnd, ox, oy, iFreezeWidth, iFreezeHeight, false );


			return TRUE;
		}

		case WM_DESTROY:
			if( bDiscardSettings )
			{
				// Restore old settings
				bMoveableMainNext       = bMoveableMainNextBefore;
				bMoveableEqualizerNext  = bMoveableEqualizerNextBefore;
				bMoveablePlaylistNext   = bMoveablePlaylistNextBefore;
			}
			break;

		case WM_SYSCOMMAND:
			switch( wp )
			{
				case SC_CLOSE:
				{
					EndDialog( hwnd, FALSE );
					return TRUE;
				}
			}
			break;

		case WM_COMMAND:
		{
			switch( LOWORD( wp ) )
			{
				case IDC_ALL:
					switch( IsDlgButtonChecked( hwnd, IDC_ALL ) )
					{
					case BST_CHECKED:
						// Uncheck all
						CheckDlgButton( hwnd, IDC_ALL, BST_UNCHECKED );
						CheckDlgButton( hwnd, IDC_MAIN, BST_UNCHECKED );
						CheckDlgButton( hwnd, IDC_EQUALIZER, BST_UNCHECKED );
						CheckDlgButton( hwnd, IDC_PLAYLIST, BST_UNCHECKED );
						bMoveableMainNext       = false;
						bMoveableEqualizerNext  = false;
						bMoveablePlaylistNext   = false;
						break;

					case BST_INDETERMINATE:
					case BST_UNCHECKED:
						// Check all
						CheckDlgButton( hwnd, IDC_ALL, BST_CHECKED );
						CheckDlgButton( hwnd, IDC_MAIN, BST_CHECKED );
						CheckDlgButton( hwnd, IDC_EQUALIZER, BST_CHECKED );
						CheckDlgButton( hwnd, IDC_PLAYLIST, BST_CHECKED );
						bMoveableMainNext       = true;
						bMoveableEqualizerNext  = true;
						bMoveablePlaylistNext   = true;
						break;

					}
					break;

				case IDC_MAIN:
					bMoveableMainNext = ( BST_CHECKED == IsDlgButtonChecked( hwnd, IDC_MAIN ) );
					UpdateAllBox( hwnd, bMoveableMainNext, bMoveableEqualizerNext, bMoveablePlaylistNext );
					break;
					
				case IDC_EQUALIZER:
					bMoveableEqualizerNext = ( BST_CHECKED == IsDlgButtonChecked( hwnd, IDC_EQUALIZER ) );
					UpdateAllBox( hwnd, bMoveableMainNext, bMoveableEqualizerNext, bMoveablePlaylistNext );
					break;
					
				case IDC_PLAYLIST:
					bMoveablePlaylistNext = ( BST_CHECKED == IsDlgButtonChecked( hwnd, IDC_PLAYLIST ) );
					UpdateAllBox( hwnd, bMoveableMainNext, bMoveableEqualizerNext, bMoveablePlaylistNext );
					break;
					
				case IDOK:
					bDiscardSettings = false;
					// NO BREAK!

				case IDCANCEL:	// Button or [ESCAPE]
					EndDialog( hwnd, FALSE );
					return TRUE;

			}
			break;

		}
	}
	return 0;
}



void config()
{
	DialogBox( plugin.hDllInstance, MAKEINTRESOURCE( IDD_CONFIG ), GetForegroundWindow(), WndprocConfig );
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
	
	if( !bMoveableMain )
	{
		WritePrivateProfileInt( "Winamp", "wx", rMain.left, szWinampIni );
		WritePrivateProfileInt( "Winamp", "wy", rMain.top, szWinampIni );
	}

	if( !bMoveableEqualizer )
	{
		WritePrivateProfileInt( "Winamp", "eq_wx", rEqualizer.left, szWinampIni );
		WritePrivateProfileInt( "Winamp", "eq_wy", rEqualizer.top, szWinampIni );
	}

	if( !bMoveablePlaylist )
	{
		WritePrivateProfileInt( "Winamp", "pe_wx", rPlaylist.left, szWinampIni );
		WritePrivateProfileInt( "Winamp", "pe_wy", rPlaylist.top, szWinampIni );
	}

	WritePrivateProfileInt( "gen_freeze", "MoveableMain", bMoveableMainNext ? 1 : 0, szWinampIni );
	WritePrivateProfileInt( "gen_freeze", "MoveableEqualizer", bMoveableEqualizerNext ? 1 : 0, szWinampIni );
	WritePrivateProfileInt( "gen_freeze", "MoveablePlaylist", bMoveablePlaylistNext ? 1 : 0, szWinampIni );
}



extern "C" __declspec( dllexport ) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin()
{
	return &plugin;
}
