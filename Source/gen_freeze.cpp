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



#ifndef IPC_GETWND_MAIN
# define IPC_GETWND_MAIN (-1)
#endif


#define PLUGIN_TITLE    "Freeze Winamp Plugin"
#define PLUGIN_VERSION  "2.0"



int init();
void config(); 
void quit();



WNDPROC WndprocMainBackup = NULL;
WNDPROC WndprocEqualizerBackup = NULL;
WNDPROC WndprocPlaylistBackup = NULL;

LRESULT CALLBACK WndprocMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK WndprocEqualizer(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
LRESULT CALLBACK WndprocPlaylist(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);


// During runtime
bool bMoveableMain; 
bool bMoveableEqualizer;
bool bMoveablePlaylist;

char * szWinampIni = NULL;



winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,
	PLUGIN_TITLE " " PLUGIN_VERSION,
	init,
	config, 
	quit,
	NULL,
	NULL
}; 


HINSTANCE & hInstance = plugin.hDllInstance;

HWND & hMain     = plugin.hwndParent;
HWND hPlaylist   = NULL;
HWND hEqualizer  = NULL;



LRESULT CALLBACK WndprocMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			if (bMoveableMain) {
				break;
			}

			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}
			
			const int x = LOWORD(lp);
			const int y = HIWORD(lp);
			const bool shadeMode = ::SendMessage(hMain, WM_WA_IPC, static_cast<WPARAM>(IPC_GETWND_MAIN), IPC_IS_WNDSHADE) == 1;

			if (shadeMode) {
				if (((x >= 6) && (x <= 15) && (y >= 3) && (y <= 12)) // Menu
						|| ((x >= 79) && (x <= 117) && (y >= 5) && (y <= 10)) // Vis
						|| ((x >= 126) && (x <= 144) && (y >= 4) && (y <= 10)) // Time 1
						|| ((x >= 147) && (x <= 157) && (y >= 4) && (y <= 10)) // Time 2
						|| ((x >= 168) && (x <= 225) && (y >= 2) && (y <= 12)) // Playback control
						|| ((x >= 226) && (x <= 243) && (y >= 4) && (y <= 11)) // Seekbar
						|| ((x >= 244) && (x <= 253) && (y >= 3) && (y <= 12)) // Minimize
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12)) // Winshade
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) { // Close
					break;
				}
			} else {
				if (((x >= 6) && (x <= 15) && (y >= 3) && (y <= 12))
						|| ((x >= 10) && (x <= 18) && (y >= 22) && (y <= 65))
						|| ((x >= 16) && (x <= 130) && (y >= 88) && (y <= 106))
						|| ((x >= 16) && (x <= 264) && (y >= 72) && (y <= 82))
						|| ((x >= 24) && (x <= 35) && (y >= 28) && (y <= 37))
						|| ((x >= 24) && (x <= 100) && (y >= 43) && (y <= 59))
						|| ((x >= 36) && (x <= 45) && (y >= 26) && (y <= 39))
						|| ((x >= 48) && (x <= 57) && (y >= 26) && (y <= 39))
						|| ((x >= 60) && (x <= 69) && (y >= 26) && (y <= 39))
						|| ((x >= 78) && (x <= 87) && (y >= 26) && (y <= 39))
						|| ((x >= 90) && (x <= 99) && (y >= 26) && (y <= 39))
						|| ((x >= 107) && (x <= 175) && (y >= 57) && (y <= 70))
						|| ((x >= 111) && (x <= 126) && (y >= 43) && (y <= 49))
						|| ((x >= 111) && (x <= 265) && (y >= 24) && (y <= 36))
						|| ((x >= 136) && (x <= 158) && (y >= 89) && (y <= 105))
						|| ((x >= 156) && (x <= 166) && (y >= 43) && (y <= 49))
						|| ((x >= 164) && (x <= 238) && (y >= 89) && (y <= 104))
						|| ((x >= 177) && (x <= 215) && (y >= 57) && (y <= 70))
						|| ((x >= 212) && (x <= 268) && (y >= 41) && (y <= 53))
						|| ((x >= 219) && (x <= 265) && (y >= 58) && (y <= 70))
						|| ((x >= 244) && (x <= 253) && (y >= 3) && (y <= 12))
						|| ((x >= 253) && (x <= 266) && (y >= 91) && (y <= 106))
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12))
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) {
					break;
				}
			}
			
			return 0;
		}

		break;
	}
	return ::CallWindowProc(WndprocMainBackup, hwnd, msg, wp, lp);
}



LRESULT CALLBACK WndprocEqualizer(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			if (bMoveableEqualizer) {
				break;
			}

			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}

			const int x = LOWORD(lp);
			const int y = HIWORD(lp);
			const bool shadeMode = ::SendMessage(hMain, WM_WA_IPC, IPC_GETWND_EQ, IPC_IS_WNDSHADE) == 1;

			if (shadeMode) {
				if (((x >= 61) && (x <= 158) && (y >= 4) && (y <= 11)) // Volume
						|| ((x >= 164) && (x <= 207) && (y >= 4) && (y <= 11)) // Panning
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12)) // Winshade
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) { // Close
					break;
				}
			} else {
				if (((x >= 14) && (x <= 73) && (y >= 18) && (y <= 30))
						|| ((x >= 21) && (x <= 35) && (y >= 38) && (y <= 101))
						|| ((x >= 42) && (x <= 68) && (y >= 65) && (y <= 75))
						|| ((x >= 78) && (x <= 92) && (y >= 38) && (y <= 101))
						|| ((x >= 96) && (x <= 110) && (y >= 38) && (y <= 101))
						|| ((x >= 114) && (x <= 128) && (y >= 38) && (y <= 101))
						|| ((x >= 132) && (x <= 146) && (y >= 38) && (y <= 101))
						|| ((x >= 150) && (x <= 164) && (y >= 38) && (y <= 101))
						|| ((x >= 168) && (x <= 182) && (y >= 38) && (y <= 101))
						|| ((x >= 186) && (x <= 200) && (y >= 38) && (y <= 101))
						|| ((x >= 204) && (x <= 218) && (y >= 38) && (y <= 101))
						|| ((x >= 217) && (x <= 261) && (y >= 18) && (y <= 30))
						|| ((x >= 222) && (x <= 236) && (y >= 38) && (y <= 101))
						|| ((x >= 240) && (x <= 254) && (y >= 38) && (y <= 101))
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12))
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) {
					break;
				}
			}
			
			return 0;
		}

		break;
	}
	return ::CallWindowProc(WndprocEqualizerBackup, hwnd, msg, wp, lp);
}



LRESULT CALLBACK WndprocPlaylist(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			if (bMoveablePlaylist) {
				break;
			}
			
			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}
			
			const int x = LOWORD(lp);
			const int y = HIWORD(lp);
			const bool shadeMode = ::SendMessage(hMain, WM_WA_IPC, IPC_GETWND_PE, IPC_IS_WNDSHADE) == 1;

			RECT r;
			::GetWindowRect(hwnd, &r);
			const int width = r.right - r.left;
			const int tx = x + 275 - width;

			if (shadeMode) {
				if (((tx >= 247) && (tx <= 255) && (y >= 0) && (y <= 14)) // Sizer
						|| ((tx >= 255) && (tx <= 273) && (y >= 3) && (y <= 12))) { // Winshade and Close
					break;
				}
			} else {
				const int height = r.bottom - r.top;
				const int ty = y + 116 - height;
				
				if (((x >= 25) && (tx <= 250) && (y >= 20) && (ty <= 78)) // Entry area?
						|| ((x >= 11) && (x <= 36) && (ty >= 78) && (ty <= 104)) // Add menu
						|| ((x >= 40) && (x <= 65) && (ty >= 78) && (ty <= 104)) // Sub menu
						|| ((x >= 69) && (x <= 94) && (ty >= 78) && (ty <= 104)) // Sel menu
						|| ((x >= 98) && (x <= 123) && (ty >= 78) && (ty <= 104)) // Misc menu
						|| ((tx >= 255) && (tx <= 273) && (y >= 3) && (y <= 12)) // Winshade and Close
						|| ((tx >= 260) && (tx <= 268) && (y >= 20) && (ty <= 78)) // Scrollbar
						|| ((tx >= 52) && (tx <= 124) && (ty >= 90) && (ty <= 106))
						|| ((tx >= 131) && (tx <= 185) && (ty >= 101) && (ty <= 109))
						|| ((tx >= 132) && (tx <= 222) && (ty >= 88) && (ty <= 94))
						|| ((tx >= 189) && (tx <= 208) && (ty >= 101) && (ty <= 107))
						|| ((tx >= 211) && (tx <= 221) && (ty >= 101) && (ty <= 107))
						|| ((tx >= 228) && (tx <= 253) && (ty >= 78) && (ty <= 104))
						|| ((tx >= 256) && (tx <= 275) && (ty >= 105) && (ty <= 116))
						|| ((tx >= 257) && (tx <= 275) && (ty >= 104) && (ty <= 116))
						|| ((tx >= 258) && (tx <= 275) && (ty >= 103) && (ty <= 116))
						|| ((tx >= 259) && (tx <= 275) && (ty >= 102) && (ty <= 116))
						|| ((tx >= 260) && (tx <= 268) && (ty >= 80) && (ty <= 90))
						|| ((tx >= 260) && (tx <= 275) && (ty >= 101) && (ty <= 116))
						|| ((tx >= 261) && (tx <= 275) && (ty >= 100) && (ty <= 116))
						|| ((tx >= 262) && (tx <= 275) && (ty >= 99) && (ty <= 116))
						|| ((tx >= 263) && (tx <= 275) && (ty >= 98) && (ty <= 116))
						|| ((tx >= 264) && (tx <= 275) && (ty >= 97) && (ty <= 116))) {
					break;
				}
			}
			
			return 0;
		}

		break;
	}
	return ::CallWindowProc(WndprocPlaylistBackup, hwnd, msg, wp, lp);
}



int init()
{
	// Get equalizer window
	hEqualizer = reinterpret_cast<HWND>(::SendMessage(hMain, WM_WA_IPC, IPC_GETWND_EQ, IPC_GETWND));
	if (!hEqualizer || !IsWindow(hEqualizer)) {
		return 1;
	}

	// Exchange equalizer window procedure
	WndprocEqualizerBackup = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hEqualizer, GWLP_WNDPROC));
	if (WndprocEqualizerBackup == NULL) {
		return 1;
	}
	::SetWindowLongPtr(hEqualizer, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocEqualizer));


	// Get playlist window
	hPlaylist = reinterpret_cast<HWND>(::SendMessage(hMain, WM_WA_IPC, IPC_GETWND_PE, IPC_GETWND));
	if (!hPlaylist || !::IsWindow(hPlaylist)) {
		// Revert equalizer
		::SetWindowLongPtr(hEqualizer, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocEqualizerBackup));
		return 1;
	}
	
	// Exchange playlist window procedure
	WndprocPlaylistBackup = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hPlaylist, GWLP_WNDPROC));
	if (WndprocPlaylistBackup == NULL) {
		// Revert equalizer
		::SetWindowLongPtr(hEqualizer, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocEqualizerBackup));
		return 1;
	}
	::SetWindowLongPtr(hPlaylist, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocPlaylist));

	
	// Exchange main window procedure
	WndprocMainBackup = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(hMain, GWLP_WNDPROC));
	if (WndprocMainBackup == NULL) {
		// Revert equalizer
		::SetWindowLongPtr(hEqualizer, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocEqualizerBackup));

		// Revert playlist
		::SetWindowLongPtr(hPlaylist, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocPlaylistBackup));
		return 1;
	}
	::SetWindowLongPtr(hMain, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WndprocMain));


	// Get Winamp.ini path
	szWinampIni = reinterpret_cast<char *>(::SendMessage(hMain, WM_WA_IPC, 0, IPC_GETINIFILE));	



	// Read config
	if (szWinampIni != NULL) {
		int res;
		res = ::GetPrivateProfileInt("gen_freeze", "MoveableMain", 0, szWinampIni);
		bMoveableMain = res ? true : false;

		res = ::GetPrivateProfileInt("gen_freeze", "MoveableEqualizer", 0, szWinampIni);
		bMoveableEqualizer = res ? true : false;

		res = ::GetPrivateProfileInt("gen_freeze", "MoveablePlaylist", 0, szWinampIni);
		bMoveablePlaylist = res ? true : false;
	} else {
		bMoveableMain = false;
		bMoveableEqualizer = false;
		bMoveablePlaylist = false;
	}

	return 0;
}



void UpdateAllBox(HWND h, bool a, bool b, bool c) {
	int iCount = 0;
	if (a) iCount++;
	if (b) iCount++;
	if (c) iCount++;

	::CheckDlgButton(h,	IDC_ALL, iCount
			? ((iCount < 3)
				? BST_INDETERMINATE
				: BST_CHECKED)
			: BST_UNCHECKED);
}



BOOL CALLBACK WndprocConfig(HWND hwnd, UINT msg, WPARAM wp, LPARAM /*lp*/) {
	static bool bMoveableMainBefore;
	static bool bMoveableEqualizerBefore;
	static bool bMoveablePlaylistBefore;
	static bool bDiscardSettings;
	
	switch (msg) {
	case WM_INITDIALOG:
		{
			// Save current settings
			bMoveableMainBefore       = bMoveableMain;
			bMoveableEqualizerBefore  = bMoveableEqualizer;
			bMoveablePlaylistBefore   = bMoveablePlaylist;
			
			bDiscardSettings = true;


			// Apply settings to checkboxes
			::CheckDlgButton(hwnd, IDC_MAIN, bMoveableMain ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwnd, IDC_EQUALIZER, bMoveableEqualizer ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwnd, IDC_PLAYLIST, bMoveablePlaylist ? BST_CHECKED : BST_UNCHECKED);
			UpdateAllBox(hwnd, bMoveableMain, bMoveableEqualizer, bMoveablePlaylist);
			
			
			// Set window title
			char szTitle[512] = "";
			wsprintf(szTitle, "%s %s", PLUGIN_TITLE, PLUGIN_VERSION);
			::SetWindowText(hwnd, szTitle);


			// Center window on parent
			RECT rp;
			::GetWindowRect(::GetForegroundWindow(), &rp);
			const int iParentWidth   = rp.right - rp.left;
			const int iParentHeight  = rp.bottom - rp.top;

			RECT rf;
			GetWindowRect(hwnd, &rf);
			const int iFreezeWidth   = rf.right - rf.left;
			const int iFreezeHeight  = rf.bottom - rf.top;

			int ox = (iParentWidth - iFreezeWidth) / 2 + rp.left;
			int oy = (iParentHeight - iFreezeHeight) / 2 + rp.top;

			::MoveWindow(hwnd, ox, oy, iFreezeWidth, iFreezeHeight, false);
		}
		return TRUE;

	case WM_DESTROY:
		if (bDiscardSettings) {
			// Restore old settings
			bMoveableMain       = bMoveableMainBefore;
			bMoveableEqualizer  = bMoveableEqualizerBefore;
			bMoveablePlaylist   = bMoveablePlaylistBefore;
		}
		break;

	case WM_SYSCOMMAND:
		switch (wp) {
		case SC_CLOSE:
			::EndDialog(hwnd, FALSE);
			return TRUE;

		}
		break;

	case WM_COMMAND:
		{
			switch (LOWORD(wp)) {
			case IDC_ALL:
				switch (IsDlgButtonChecked(hwnd, IDC_ALL)) {
				case BST_CHECKED:
					// Uncheck all
					::CheckDlgButton(hwnd, IDC_ALL, BST_UNCHECKED);
					::CheckDlgButton(hwnd, IDC_MAIN, BST_UNCHECKED);
					::CheckDlgButton(hwnd, IDC_EQUALIZER, BST_UNCHECKED);
					::CheckDlgButton(hwnd, IDC_PLAYLIST, BST_UNCHECKED);
					bMoveableMain       = false;
					bMoveableEqualizer  = false;
					bMoveablePlaylist   = false;
					break;

				case BST_INDETERMINATE:
				case BST_UNCHECKED:
					// Check all
					::CheckDlgButton(hwnd, IDC_ALL, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_MAIN, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_EQUALIZER, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_PLAYLIST, BST_CHECKED);
					bMoveableMain       = true;
					bMoveableEqualizer  = true;
					bMoveablePlaylist   = true;
					break;

				}
				break;

			case IDC_MAIN:
				bMoveableMain = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_MAIN);
				UpdateAllBox(hwnd, bMoveableMain, bMoveableEqualizer, bMoveablePlaylist);
				break;
				
			case IDC_EQUALIZER:
				bMoveableEqualizer = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_EQUALIZER);
				UpdateAllBox(hwnd, bMoveableMain, bMoveableEqualizer, bMoveablePlaylist);
				break;
				
			case IDC_PLAYLIST:
				bMoveablePlaylist = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_PLAYLIST);
				UpdateAllBox(hwnd, bMoveableMain, bMoveableEqualizer, bMoveablePlaylist);
				break;
				
			case IDOK:
				bDiscardSettings = false;
				// NO BREAK!

			case IDCANCEL:	// Button or [ESCAPE]
				::EndDialog(hwnd, FALSE);
				return TRUE;

			}

			break;
		}
	}
	return 0;
}



void config() {
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_CONFIG), ::GetForegroundWindow(), WndprocConfig);
} 



BOOL WritePrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, int iValue, LPCTSTR lpFileName) {
	TCHAR szBuffer[16];
	wsprintf(szBuffer, TEXT("%i"), iValue);
    return ::WritePrivateProfileString(lpAppName, lpKeyName, szBuffer, lpFileName);
}



void quit() {
	if (szWinampIni == NULL) {
		return;
	}
	
	WritePrivateProfileInt("gen_freeze", "MoveableMain", bMoveableMain ? 1 : 0, szWinampIni);
	WritePrivateProfileInt("gen_freeze", "MoveableEqualizer", bMoveableEqualizer ? 1 : 0, szWinampIni);
	WritePrivateProfileInt("gen_freeze", "MoveablePlaylist", bMoveablePlaylist ? 1 : 0, szWinampIni);
}



extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
	return &plugin;
}
