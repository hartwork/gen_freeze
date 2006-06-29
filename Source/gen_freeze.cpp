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
#include "resource.h"
#include <map>

using namespace std;



#ifndef IPC_GETWND_MAIN
# define IPC_GETWND_MAIN (-1)
#endif


#define PLUGIN_TITLE    "Freeze Winamp Plugin"
#define PLUGIN_VERSION  "2.2"



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
bool freezeMain; 
bool freezeEqualizer;
bool freezePlaylist;
bool freezeGen; // "Winamp Gen" windows

char * winampIniPath = NULL;
bool winamp504orAbove = false;

map<HWND, WNDPROC> getWindowProc;
HWND hLibrary = NULL;



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



LRESULT CALLBACK wndProcEmbed(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_WINDOWPOSCHANGING:
		{
			if (hwnd != hLibrary) {
				break;
			}

			// Is window sized?
			WINDOWPOS * const winpos = reinterpret_cast<WINDOWPOS *>(lp);
			if ((winpos->flags & SWP_NOSIZE) != SWP_NOSIZE) {
				const SHORT leftDown = 0x8000;
				if ((leftDown & ::GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON)) != leftDown) {
					// Deny sizing
					winpos->flags |= SWP_NOSIZE;
					return 0;
				}
			}
		}
		break;

	case WM_LBUTTONDOWN:
		{
			if (!freezeGen) {
				break;
			}

			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}

			RECT r;
			::GetWindowRect(hwnd, &r);
			const int y = HIWORD(lp);
			const int tx = LOWORD(lp) + 275 - (r.right - r.left);
			const int ty = HIWORD(lp) + 116 - (r.bottom - r.top);
				
			if (((tx >= 256) && (tx <= 275) && (ty >= 102) && (ty <= 116)) // Size corner
					|| ((tx >= 264) && (tx <= 273) && (y >= 3) && (y <= 12)) // Close
					|| ((tx >= 267) && (tx <= 275) && (ty >= 96) && (ty <= 116))) { // Size corner
				break;
			}
		}
		return 0;

	}

	const map<HWND, WNDPROC>::iterator iter = getWindowProc.find(hwnd);
	if (iter != getWindowProc.end()) {
		return ::CallWindowProc(iter->second, hwnd, msg, wp, lp);
	} else {
		return ::DefWindowProc(hwnd, msg, wp, lp);
	}
}



LRESULT CALLBACK WndprocMain(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	switch (msg) {
	case WM_WA_IPC:
		switch (lp) {
		case IPC_GET_GENSKINBITMAP:
			{
				if (wp != 0) {
					break;
				}

				HWND walk = ::GetWindow(GetDesktopWindow(), GW_CHILD);
				while (walk != NULL) {
					// Right classname?
					char className[11] = "";
					if (GetClassName(walk, className, 11)) {
						if (!strcmp(className, "Winamp Gen")) {
							// Same process?
							DWORD processId;
							::GetWindowThreadProcessId(walk, &processId);
							if (processId == GetCurrentProcessId()) {
								// Window already monitored?
								const map<HWND, WNDPROC>::iterator iter = getWindowProc.find(walk);
								if (iter == getWindowProc.end()) {
									// Media Library?
									char windowTitle[100] = "";
									::GetWindowText(walk, windowTitle, 100);
									if (!strcmp(windowTitle, "Winamp Library")) {
										hLibrary = walk;
									}

									// Monitor this window
									const WNDPROC walkProc = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(walk, GWLP_WNDPROC));
									if (walkProc != NULL) {
										getWindowProc.insert(pair<HWND, WNDPROC>(walk, walkProc));
										::SetWindowLongPtr(walk, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProcEmbed));
									}
								}
							}
						}
					}
					walk = GetWindow(walk, GW_HWNDNEXT);
				}
				break;
			}
		}
		break;

	case WM_LBUTTONDOWN:
		{
			if (!freezeMain) {
				break;
			}

			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}
			
			const int doublesize = static_cast<int>(::SendMessage(hMain, WM_WA_IPC, 0, IPC_ISDOUBLESIZE));
			const bool notStopped = ::SendMessage(hMain, WM_WA_IPC, 0, IPC_ISPLAYING) != 0;
			const int x = LOWORD(lp) >> doublesize;
			const int y = HIWORD(lp) >> doublesize;
			
			bool shadeMode;
			if (winamp504orAbove) {
				shadeMode = ::SendMessage(hMain, WM_WA_IPC, static_cast<WPARAM>(IPC_GETWND_MAIN), IPC_IS_WNDSHADE) == 1;
			} else {
				RECT r;
				::GetWindowRect(hwnd, &r);
				shadeMode = (r.bottom - r.top) == 14;
			}

			if (shadeMode) {
				if (((x >= 6) && (x <= 15) && (y >= 3) && (y <= 12)) // Menu
						|| ((x >= 79) && (x <= 117) && (y >= 5) && (y <= 10)) // Vis
						|| ((x >= 126) && (x <= 144) && (y >= 4) && (y <= 10)) // Time 1
						|| ((x >= 147) && (x <= 157) && (y >= 4) && (y <= 10)) // Time 2
						|| ((x >= 168) && (x <= 225) && (y >= 2) && (y <= 12)) // Playback control
						|| (notStopped && (x >= 226) && (x <= 243) && (y >= 4) && (y <= 11)) // Seekbar
						|| ((x >= 244) && (x <= 253) && (y >= 3) && (y <= 12)) // Minimize
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12)) // Winshade
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) { // Close
					break;
				}
			} else {
				if (((x >= 6) && (x <= 15) && (y >= 3) && (y <= 12))
						|| ((x >= 10) && (x <= 18) && (y >= 22) && (y <= 65)) // Menu
						|| ((x >= 16) && (x <= 130) && (y >= 88) && (y <= 106)) // Playback
						|| (notStopped && (x >= 16) && (x <= 264) && (y >= 72) && (y <= 82)) // Seekbar
						|| ((x >= 24) && (x <= 35) && (y >= 28) && (y <= 37)) // Status
						|| ((x >= 24) && (x <= 100) && (y >= 43) && (y <= 59)) // Vis
						|| ((x >= 36) && (x <= 45) && (y >= 26) && (y <= 39)) // Minus
						|| ((x >= 48) && (x <= 57) && (y >= 26) && (y <= 39)) // Time Minutes Ten
						|| ((x >= 60) && (x <= 69) && (y >= 26) && (y <= 39)) // Time Minutes One
						|| ((x >= 78) && (x <= 87) && (y >= 26) && (y <= 39)) // Time Seconds Ten
						|| ((x >= 90) && (x <= 99) && (y >= 26) && (y <= 39)) // Time Minutes One
						// || ((x >= 107) && (x <= 175) && (y >= 57) && (y <= 70)) // Volume
						|| ((x >= 107) && (x <= 175) && (y >= 58) && (y <= 66)) // Volume
						|| ((x >= 111) && (x <= 126) && (y >= 43) && (y <= 49)) // Kbps
						|| ((x >= 111) && (x <= 265) && (y >= 24) && (y <= 36)) // Title
						|| ((x >= 136) && (x <= 158) && (y >= 89) && (y <= 105)) // Open
						|| ((x >= 156) && (x <= 166) && (y >= 43) && (y <= 49)) // Khz
						|| ((x >= 164) && (x <= 238) && (y >= 89) && (y <= 104)) // Shuffle and Repeat
						// || ((x >= 177) && (x <= 215) && (y >= 57) && (y <= 70)) // Panning
						|| ((x >= 177) && (x <= 215) && (y >= 58) && (y <= 66)) // Panning
						|| ((x >= 212) && (x <= 268) && (y >= 41) && (y <= 53)) // Mono/Stereo 
						|| ((x >= 219) && (x <= 265) && (y >= 58) && (y <= 70)) // EQ and Playlist
						|| ((x >= 244) && (x <= 253) && (y >= 3) && (y <= 12)) // Minimize
						|| ((x >= 253) && (x <= 266) && (y >= 91) && (y <= 106)) // About
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12)) // Winshade
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) { // Close
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
			if (!freezeEqualizer) {
				break;
			}

			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}

			const int doublesize = static_cast<int>(::SendMessage(hMain, WM_WA_IPC, 0, IPC_ISDOUBLESIZE));
			const int x = LOWORD(lp) >> doublesize;
			const int y = HIWORD(lp) >> doublesize;

			bool shadeMode;
			if (winamp504orAbove) {
				shadeMode = ::SendMessage(hMain, WM_WA_IPC, IPC_GETWND_EQ, IPC_IS_WNDSHADE) == 1;
			} else {
				RECT r;
				::GetWindowRect(hwnd, &r);
				shadeMode = (r.bottom - r.top) == 14;
			}

			if (shadeMode) {
				if (((x >= 61) && (x <= 158) && (y >= 4) && (y <= 11)) // Volume
						|| ((x >= 164) && (x <= 207) && (y >= 4) && (y <= 11)) // Panning
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12)) // Winshade
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) { // Close
					break;
				}
			} else {
				if (((x >= 14) && (x <= 73) && (y >= 18) && (y <= 30)) // On and Auto
						|| ((x >= 21) && (x <= 35) && (y >= 38) && (y <= 101)) // Preamp
						|| ((x >= 42) && (x <= 68) && (y >= 65) && (y <= 75)) // Reset
						|| ((x >= 78) && (x <= 92) && (y >= 38) && (y <= 101)) // Slider 01
						|| ((x >= 96) && (x <= 110) && (y >= 38) && (y <= 101)) // Slider 02
						|| ((x >= 114) && (x <= 128) && (y >= 38) && (y <= 101)) // Slider 03
						|| ((x >= 132) && (x <= 146) && (y >= 38) && (y <= 101)) // Slider 04
						|| ((x >= 150) && (x <= 164) && (y >= 38) && (y <= 101)) // Slider 05
						|| ((x >= 168) && (x <= 182) && (y >= 38) && (y <= 101)) // Slider 06
						|| ((x >= 186) && (x <= 200) && (y >= 38) && (y <= 101)) // Slider 07
						|| ((x >= 204) && (x <= 218) && (y >= 38) && (y <= 101)) // Slider 08
						|| ((x >= 217) && (x <= 261) && (y >= 18) && (y <= 30)) // Preset
						|| ((x >= 222) && (x <= 236) && (y >= 38) && (y <= 101)) // Slider 09
						|| ((x >= 240) && (x <= 254) && (y >= 38) && (y <= 101)) // Slider 10
						|| ((x >= 254) && (x <= 263) && (y >= 3) && (y <= 12)) // Winshade
						|| ((x >= 264) && (x <= 273) && (y >= 3) && (y <= 12))) { // Close
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
			if (!freezePlaylist) {
				break;
			}
			
			const bool controlDown = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
			if (controlDown) {
				break;
			}
			
			const int x = LOWORD(lp);
			const int y = HIWORD(lp);

			RECT r;
			::GetWindowRect(hwnd, &r);
			const int width = r.right - r.left;
			const int tx = x + 275 - width;

			bool shadeMode;
			if (winamp504orAbove) {
				shadeMode = ::SendMessage(hMain, WM_WA_IPC, IPC_GETWND_PE, IPC_IS_WNDSHADE) == 1;
			} else {
				shadeMode = (r.bottom - r.top) == 14;
			}

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
						|| ((tx >= 52) && (tx <= 124) && (ty >= 90) && (ty <= 106)) // Vis
						|| ((tx >= 131) && (tx <= 185) && (ty >= 101) && (ty <= 109)) // Playback
						|| ((tx >= 132) && (tx <= 222) && (ty >= 88) && (ty <= 94)) // Time stats
						|| ((tx >= 189) && (tx <= 208) && (ty >= 101) && (ty <= 107)) // Time Minutes
						|| ((tx >= 211) && (tx <= 221) && (ty >= 101) && (ty <= 107)) // // Time Seconds
						|| ((tx >= 228) && (tx <= 253) && (ty >= 78) && (ty <= 104)) // List menu
						|| ((tx >= 256) && (tx <= 275) && (ty >= 105) && (ty <= 116)) // Size corner
						|| ((tx >= 257) && (tx <= 275) && (ty >= 104) && (ty <= 116)) // Size corner
						|| ((tx >= 258) && (tx <= 275) && (ty >= 103) && (ty <= 116)) // Size corner
						|| ((tx >= 259) && (tx <= 275) && (ty >= 102) && (ty <= 116)) // Size corner
						|| ((tx >= 260) && (tx <= 268) && (ty >= 80) && (ty <= 90)) // Updown
						|| ((tx >= 260) && (tx <= 275) && (ty >= 101) && (ty <= 116)) // Size corner
						|| ((tx >= 261) && (tx <= 275) && (ty >= 100) && (ty <= 116)) // Size corner
						|| ((tx >= 262) && (tx <= 275) && (ty >= 99) && (ty <= 116)) // Size corner
						|| ((tx >= 263) && (tx <= 275) && (ty >= 98) && (ty <= 116)) // Size corner
						|| ((tx >= 264) && (tx <= 275) && (ty >= 97) && (ty <= 116))) { // Size corner
					break;
				}
			}
			
			return 0;
		}

		break;
	}
	return ::CallWindowProc(WndprocPlaylistBackup, hwnd, msg, wp, lp);
}



int init() {
	// Check Winamp version
	winamp504orAbove = ::SendMessage(hMain, WM_WA_IPC, 0, IPC_GETVERSION) >= 0x5004;

	
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
	winampIniPath = reinterpret_cast<char *>(::SendMessage(hMain, WM_WA_IPC, 0, IPC_GETINIFILE));



	// Read config
	if (winampIniPath != NULL) {
		int res;
		res = ::GetPrivateProfileInt("gen_freeze", "FreezeMain", 0, winampIniPath);
		freezeMain = res ? true : false;

		res = ::GetPrivateProfileInt("gen_freeze", "FreezeEqualizer", 0, winampIniPath);
		freezeEqualizer = res ? true : false;

		res = ::GetPrivateProfileInt("gen_freeze", "FreezePlaylist", 0, winampIniPath);
		freezePlaylist = res ? true : false;

		res = ::GetPrivateProfileInt("gen_freeze", "FreezeGen", 0, winampIniPath);
		freezeGen = res ? true : false;
	} else {
		freezeMain = true;
		freezeEqualizer = true;
		freezePlaylist = true;
		freezeGen = true;
	}

	return 0;
}



void UpdateAllBox(HWND h, bool a, bool b, bool c, bool d) {
	int count = 0;
	if (a) count++;
	if (b) count++;
	if (c) count++;
	if (d) count++;

	::CheckDlgButton(h,	IDC_ALL, count
			? ((count < 4)
				? BST_INDETERMINATE
				: BST_CHECKED)
			: BST_UNCHECKED);
}



BOOL CALLBACK WndprocConfig(HWND hwnd, UINT msg, WPARAM wp, LPARAM /*lp*/) {
	static bool freezeMainBefore;
	static bool freezeEqualizerBefore;
	static bool freezePlaylistBefore;
	static bool freezeGenBefore;
	static bool discardSettings;
	
	switch (msg) {
	case WM_INITDIALOG:
		{
			// Save current settings
			freezeMainBefore       = freezeMain;
			freezeEqualizerBefore  = freezeEqualizer;
			freezePlaylistBefore   = freezePlaylist;
			freezeGenBefore        = freezeGen;
			
			discardSettings = true;


			// Apply settings to checkboxes
			::CheckDlgButton(hwnd, IDC_MAIN, freezeMain ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwnd, IDC_EQUALIZER, freezeEqualizer ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwnd, IDC_PLAYLIST, freezePlaylist ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwnd, IDC_GEN, freezeGen ? BST_CHECKED : BST_UNCHECKED);
			UpdateAllBox(hwnd, freezeMain, freezeEqualizer, freezePlaylist, freezeGen);
			
			
			// Set window title
			char szTitle[512] = "";
			wsprintf(szTitle, "%s %s", PLUGIN_TITLE, PLUGIN_VERSION);
			::SetWindowText(hwnd, szTitle);


			// Center window on parent
			RECT rp;
			::GetWindowRect(::GetForegroundWindow(), &rp);
			const int parentWidth   = rp.right - rp.left;
			const int parentHeight  = rp.bottom - rp.top;

			RECT rf;
			::GetWindowRect(hwnd, &rf);
			const int dialogWidth   = rf.right - rf.left;
			const int dialogHeight  = rf.bottom - rf.top;

			int ox = (parentWidth - dialogWidth) / 2 + rp.left;
			int oy = (parentHeight - dialogHeight) / 2 + rp.top;

			::MoveWindow(hwnd, ox, oy, dialogWidth, dialogHeight, false);
		}
		return TRUE;

	case WM_DESTROY:
		if (discardSettings) {
			// Restore old settings
			freezeMain       = freezeMainBefore;
			freezeEqualizer  = freezeEqualizerBefore;
			freezePlaylist   = freezePlaylistBefore;
			freezeGen        = freezeGenBefore;
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
					::CheckDlgButton(hwnd, IDC_GEN, BST_UNCHECKED);
					freezeMain       = false;
					freezeEqualizer  = false;
					freezePlaylist   = false;
					freezeGen        = false;
					break;

				case BST_INDETERMINATE:
				case BST_UNCHECKED:
					// Check all
					::CheckDlgButton(hwnd, IDC_ALL, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_MAIN, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_EQUALIZER, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_PLAYLIST, BST_CHECKED);
					::CheckDlgButton(hwnd, IDC_GEN, BST_CHECKED);
					freezeMain       = true;
					freezeEqualizer  = true;
					freezePlaylist   = true;
					freezeGen        = true;
					break;

				}
				break;

			case IDC_MAIN:
				freezeMain = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_MAIN);
				UpdateAllBox(hwnd, freezeMain, freezeEqualizer, freezePlaylist, freezeGen);
				break;
				
			case IDC_EQUALIZER:
				freezeEqualizer = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_EQUALIZER);
				UpdateAllBox(hwnd, freezeMain, freezeEqualizer, freezePlaylist, freezeGen);
				break;
				
			case IDC_PLAYLIST:
				freezePlaylist = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_PLAYLIST);
				UpdateAllBox(hwnd, freezeMain, freezeEqualizer, freezePlaylist, freezeGen);
				break;
				
			case IDC_GEN:
				freezeGen = BST_CHECKED == ::IsDlgButtonChecked(hwnd, IDC_GEN);
				UpdateAllBox(hwnd, freezeMain, freezeEqualizer, freezePlaylist, freezeGen);
				break;
				
			case IDOK:
				discardSettings = false;
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
	if (winampIniPath == NULL) {
		return;
	}
	
	WritePrivateProfileInt("gen_freeze", "FreezeMain", freezeMain ? 1 : 0, winampIniPath);
	WritePrivateProfileInt("gen_freeze", "FreezeEqualizer", freezeEqualizer ? 1 : 0, winampIniPath);
	WritePrivateProfileInt("gen_freeze", "FreezePlaylist", freezePlaylist ? 1 : 0, winampIniPath);
	WritePrivateProfileInt("gen_freeze", "FreezeGen", freezeGen ? 1 : 0, winampIniPath);
}



extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
	return &plugin;
}
