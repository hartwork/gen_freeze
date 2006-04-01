/*//////////////////////////////////////////////////////////////////////////////
// ExtraMessageBox
// 
// Copyright © 2006  Sebastian Pipping <webmaster@hartwork.org>
// 
// -->  http://emabox.sourceforge.net/
// 
// This source code is released under the GNU General Public License (GPL).
// See GPL.txt for details. Any non-GPL usage is strictly forbidden.
//////////////////////////////////////////////////////////////////////////////*/


/*
Changelog

0.8.2 ............................. 2006-03-11

[+] EmaBoxLive() and EmaBoxDie() hiding in Emabox.h
[+] More demos plus selection by index

[*] Checkbox alignment below/text uses constant space
    between buttons and checkbox now

0.8.1 ............................. 2006-03-09

[+] Checkbox position flags
[+] Dialog is now re-centered after modification
[+] Improved checkbox label customization
[+] Optional hiding of EmaBoxDie()
[+] Windows XP manifest for Demo

0.8 ............................... 2006-03-08
*/


#include "Emabox.h"



#define FUNCTION_NORMAL    0
#define FUNCTION_EXTENDED  1
#define FUNCTION_INDIRECT  2




TCHAR * const szNeverAgain      = TEXT( EMA_TEXT_NEVER_AGAIN );
TCHAR * const szRememberChoice  = TEXT( EMA_TEXT_REMEMBER_CHOICE );

DWORD dwTlsSlot = TLS_OUT_OF_INDEXES;

#ifdef EMA_AUTOLIVE
int bEmaInitDone = 0;

int EmaBoxLive();
#endif



struct StructEmaBoxData
{
	int * bCheckState;
	HHOOK hCBT;                   /* CBT hook handle   */
	WNDPROC WndprocMsgBoxBackup;  /* Old wndproc       */
	UINT uType;                   /* Message box type  */
	HWND hCheck;                  /* Checkbox handle   */
};

typedef struct StructEmaBoxData EmaBoxData;



void RectScreenToClient( const HWND h, RECT * const r )
{
	POINT p;
	RECT after;

	p.x = r->left;
	p.y = r->top;
	ScreenToClient( h, &p );
	
	after.left    = p.x;
	after.right   = p.x + r->right - r->left;
	after.top     = p.y;
	after.bottom  = p.y + r->bottom - r->top;
	
	memcpy( r, &after, sizeof( RECT ) );
}



LRESULT CALLBACK WndprocMsgBox( HWND hwnd, UINT message, WPARAM wp, LPARAM lp )
{
	/* Find data */
	EmaBoxData * const data = ( EmaBoxData * )TlsGetValue( dwTlsSlot );
	
	switch( message )
	{
	case WM_COMMAND:
		if( HIWORD( wp ) == BN_CLICKED )
		{
			if( !data->hCheck || ( ( HWND )lp != data->hCheck ) ) break;
			
			{
				const LRESULT res = SendMessage( ( HWND )lp, BM_GETSTATE, 0, 0 );
				const int bCheckedAfter = ( ( res & BST_CHECKED ) == 0 );
				
				/* Update external variable */
				*( data->bCheckState ) = bCheckedAfter ? 1 : 0;
				
				SendMessage( ( HWND )lp, BM_SETCHECK, ( bCheckedAfter ) ? BST_CHECKED : 0, 0 );
			}
		}
		break;
		
	case WM_INITDIALOG:
		{
			/* Add checkbox */
			if( ( data->uType & MB_CHECKMASC ) != 0 )
			{
				int iSpaceTextToButtonRow;
				HDC hdc;
				RECT rw;                  /* Window rect    */
				RECT rc;                  /* Client rect    */
				HWND hText;               /* Message handle */
				RECT rt;                  /* Message rect   */
				int iLabelHeight;
				TCHAR * szCheckboxLabel;  /* Checkbox label */

				int iWindowWidthBefore;
				int iWindowHeightBefore;
				int iClientWidthBefore;
				int iClientHeightBefore;
				int iCheckboxLeft;
				int iCheckboxTop;
				int iCheckboxWidth;
				int iCheckboxHeight;

				RECT rab;


				/* Get original window dimensions */
				GetWindowRect( hwnd, &rw );
				iWindowWidthBefore   = rw.right - rw.left;
				iWindowHeightBefore  = rw.bottom - rw.top;

				GetClientRect( hwnd, &rc );
				iClientWidthBefore   = rc.right - rc.left;
				iClientHeightBefore  = rc.bottom - rc.top;

				
				/* Find handle of the text label */
				{
					HWND hFirstStatic;
					HWND hSecondStatic;

					hFirstStatic = FindWindowEx( hwnd, NULL, TEXT( "STATIC" ), NULL );
					if( !hFirstStatic ) break;
					
					hSecondStatic = FindWindowEx( hwnd, hFirstStatic, TEXT( "STATIC" ), NULL );
					if( !hSecondStatic )
					{
						/* Only one static means no icon. */
						/* So hFirstStatic must be the text window. */
						hText = hFirstStatic;
					}
					else
					{
						TCHAR szBuf[ 2 ] = TEXT( "" );
						if( !GetWindowText( hSecondStatic, szBuf, 2 ) ) break;
						
						if( *szBuf != TEXT( '\0' ) )
						{
							/* Has text so it must be the label */
							hText = hSecondStatic;
						}
						else
						{
							hText = hFirstStatic;
						}
					}
				}


				/* Get height of the text label */
				GetWindowRect( hText, &rt );
				RectScreenToClient( hwnd, &rt );
				iLabelHeight = rt.bottom - rt.top;


				/* Get original distance between */
				/* label and the button row      */
				{
					const HWND hAnyButton = FindWindowEx( hwnd, NULL, TEXT( "BUTTON" ), NULL );
					if( !hAnyButton ) break;
					
					GetWindowRect( hAnyButton, &rab );
					RectScreenToClient( hwnd, &rab );
					
					iSpaceTextToButtonRow = rab.top - rt.bottom;
				}


				/* Select checkbox label text */
				szCheckboxLabel =	( data->uType & MB_CHECKNEVERAGAIN )
									? szNeverAgain
									: szRememberChoice;


				/* Create checkbox (resized and move later) */
				data->hCheck = CreateWindow(
					TEXT( "BUTTON" ),
					szCheckboxLabel,
					WS_CHILD |
						WS_VISIBLE |
						WS_TABSTOP |
						BS_VCENTER |
						BS_CHECKBOX,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					CW_USEDEFAULT,
					hwnd,
					NULL,
					GetModuleHandle( NULL ),
					NULL
				);
				
				/* Set initial check state */
				SendMessage( data->hCheck, BM_SETCHECK, *( data->bCheckState ) ? BST_CHECKED : 0, 0 );
				
				{
					/* Apply default font */
					const int cyMenuSize  = GetSystemMetrics( SM_CYMENUSIZE );
					const int cxMenuSize  = GetSystemMetrics( SM_CXMENUSIZE );
					const HFONT hNewFont  = ( HFONT )GetStockObject( DEFAULT_GUI_FONT );
					HFONT hOldFont;
					SIZE size;

					SendMessage( data->hCheck, WM_SETFONT, ( WPARAM )hNewFont, ( LPARAM )TRUE );

					/* Get "real" checkbox label width */
					hdc = GetDC( data->hCheck );
					hOldFont = ( HFONT )SelectObject( hdc, GetStockObject( DEFAULT_GUI_FONT ) );
					GetTextExtentPoint32( hdc, szCheckboxLabel, _tcslen( szCheckboxLabel ), &size );
					SelectObject( hdc, hOldFont );
					ReleaseDC( data->hCheck, hdc );

					iCheckboxWidth   = cxMenuSize + size.cx + 1;
					iCheckboxHeight  = ( cyMenuSize > size.cy ) ? cyMenuSize : size.cy;
				}

				/* Move and size checkbox */
				switch( data->uType & MB_CHECKALIGNMASC )
				{
				case MB_CHECKALIGNBELOWLEFT:
					iCheckboxTop   = iClientHeightBefore + VSPACE_EXTRA_BUTTONS_TO_SEP + 2 + 1 + VSPACE_CHECKBOX;
					iCheckboxLeft  = HSPACE_CHECKBOX_LEFT;
					break;

				case MB_CHECKALIGNBELOWTEXT:
					iCheckboxTop   = rab.bottom + VSPACE_BUTTONS_TO_CHECKBOX;
					iCheckboxLeft  = rt.left;
					break;
					
				case MB_CHECKALIGNABOVETEXT:
					iCheckboxTop   = rt.top + iLabelHeight + iSpaceTextToButtonRow;
					iCheckboxLeft  = rt.left;
					break;

				case MB_CHECKALIGNABOVECENTER:
				default:
					iCheckboxTop   = rt.top + iLabelHeight + iSpaceTextToButtonRow;
					iCheckboxLeft  = ( iClientWidthBefore - iCheckboxWidth ) / 2;
					break;
				
				}

				MoveWindow(
					data->hCheck,
					iCheckboxLeft,
					iCheckboxTop,
					iCheckboxWidth,
					iCheckboxHeight,
					FALSE
				);


				/* Move all buttons down (except the checkbox)              */
				/* Needed if the checkbox goes between label and button row */
				if( ( data->uType & MB_CHECKALIGNBELOWMASC ) == 0 )
				{
					const int iDistance = iCheckboxHeight + VSPACE_CHECKBOX_TO_BUTTONS;

					/* Original formula for the message box top is              */
					/* top := top_owner + ( height_owner - height_msgbox ) / 2; */
					/* If [height_msgbox] increases now we have to add          */
					/* ( old_height_msgbox - new_height_msgbox ) / 2            */
					/* which is negative and thus moves the box upwards.        */
					HWND hLastButton = NULL;
					RECT rb;

					for( ; ; )
					{
						hLastButton = FindWindowEx( hwnd, hLastButton, TEXT( "BUTTON" ), NULL );
						if( !hLastButton ) break;
						if( hLastButton == data->hCheck ) continue;
						
						GetWindowRect( hLastButton, &rb );
						RectScreenToClient( hwnd, &rb );

						MoveWindow( hLastButton, rb.left, rb.top + iDistance, rb.right - rb.left, rb.bottom - rb.top, FALSE );
					}
				}
				else if( ( data->uType & MB_CHECKALIGNMASC ) == MB_CHECKALIGNBELOWLEFT )
				{
					/* Draw 3D separator */
					CreateWindow(
						TEXT( "STATIC" ),
						szCheckboxLabel,
						WS_CHILD |
							WS_VISIBLE |
							SS_SUNKEN, 
						0,
						iClientHeightBefore + VSPACE_EXTRA_BUTTONS_TO_SEP,
						iClientWidthBefore,
						2,
						hwnd,
						NULL,
						GetModuleHandle( NULL ),
						NULL
					);
				}

				/* Enlarge and re-center dialog */
				{
					int iWindowHeightAfter;
					int iWindowTopAfter;

					if( ( data->uType & MB_CHECKALIGNBELOWMASC ) == 0 )
					{
						/* Checkbox above button row */
						iWindowHeightAfter = iWindowHeightBefore
												+ iCheckboxHeight
												+ VSPACE_CHECKBOX_TO_BUTTONS
												+ VSPACE_EXTRA_BOTTOM;
					}
					else
					{
						/* Checkbox below button row */
						if( ( data->uType & MB_CHECKALIGNMASC ) == MB_CHECKALIGNBELOWLEFT )
						{
							iWindowHeightAfter = iWindowHeightBefore
													+ VSPACE_EXTRA_BUTTONS_TO_SEP
													+ 2
													+ 1
													+ VSPACE_CHECKBOX
													+ iCheckboxHeight
													+ VSPACE_CHECKBOX;
						}
						else
						{
							iWindowHeightAfter = iWindowHeightBefore
													+ VSPACE_BUTTONS_TO_CHECKBOX
													+ iCheckboxHeight
													- ( iClientHeightBefore - rab.bottom )
													+ VSPACE_CHECKBOX_TO_BOTTOM;
						}
					}

					iWindowTopAfter = rw.top + ( iWindowHeightBefore - iWindowHeightAfter ) / 2;
					MoveWindow( hwnd, rw.left, iWindowTopAfter, iWindowWidthBefore, iWindowHeightAfter, FALSE );
				}
			}
			else
			{
				data->hCheck = NULL;
			}
			
			/* Modify close button */
			switch( data->uType & MB_CLOSEMASK )
			{
			case MB_DISABLECLOSE:
				{
					const HMENU hSysMenu = GetSystemMenu( hwnd, FALSE );
					EnableMenuItem( hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_GRAYED );
				}
				break;
				
			case MB_NOCLOSE:
				{
					const LONG style = GetWindowLong( hwnd, GWL_STYLE );
					if( ( style & WS_SYSMENU ) == 0 ) break;
					SetWindowLong( hwnd, GWL_STYLE, ( LONG )( style - WS_SYSMENU ) );
				}
				break;
				
			}
		}
		break;

	}
	return CallWindowProc( data->WndprocMsgBoxBackup, hwnd, message, wp, lp );
}	



/* int bFound = 0; */

LRESULT CALLBACK HookprocMsgBox( int code, WPARAM wp, LPARAM lp )
{
	/* Get hook handle */
	EmaBoxData * const data = ( EmaBoxData * )TlsGetValue( dwTlsSlot );
	
	if( code == HCBT_CREATEWND )
	{
		/* MSDN says WE CANNOT TRUST "CBT_CREATEWND"                 */
		/* so we use only the window handle                          */
		/* and get the class name using "GetClassName". (-> Q106079) */
		HWND hwnd = ( HWND )wp;

		/* Check windowclass */
		TCHAR szClass[ 7 ] = TEXT( "" );
		GetClassName( hwnd, szClass, 7 );
		if( !_tcscmp( szClass, TEXT( "#32770" ) ) )
		{
/*			
			if( bFound )
			{
				return CallNextHookEx( hCBT, code, wp, lp );
			}
			
			bFound = 1;
*/			
			/* Exchange window procedure */
			data->WndprocMsgBoxBackup = ( WNDPROC )GetWindowLong( hwnd, GWL_WNDPROC );
			if( data->WndprocMsgBoxBackup != NULL )
			{
				SetWindowLong( hwnd, GWL_WNDPROC, ( LONG )WndprocMsgBox );
			}
		}
	}
	return CallNextHookEx( data->hCBT, code, wp, lp );
}



int ExtraAllTheSame( const HWND hWnd, const LPCTSTR lpText, const LPCTSTR lpCaption, const UINT uType, const WORD wLanguageId, const LPMSGBOXPARAMS lpMsgBoxParams, int * const pbCheckRes, const int iFunction )
{
	EmaBoxData * data;
	HHOOK hCBT;
	int res;

#ifdef EMA_AUTOLIVE
	if( !bEmaInitDone )
	{
		EmaBoxLive();
		bEmaInitDone = 1;
	}
#endif

	/* Create thread data */
	data = ( EmaBoxData * )LocalAlloc( NONZEROLPTR, sizeof( EmaBoxData ) );
	TlsSetValue( dwTlsSlot, data );
	data->bCheckState  = pbCheckRes;
	data->uType        = ( iFunction != FUNCTION_INDIRECT ) ? uType : lpMsgBoxParams->dwStyle;
	
	/* Setup this-thread-only hook */
	hCBT = SetWindowsHookEx( WH_CBT, &HookprocMsgBox, GetModuleHandle( NULL ), GetCurrentThreadId() );

	switch( iFunction )
	{
	case FUNCTION_NORMAL:
		res = MessageBox( hWnd, lpText, lpCaption, uType );
		break;

	case FUNCTION_EXTENDED:
		res = MessageBoxEx( hWnd, lpText, lpCaption, uType, wLanguageId );
		break;

	case FUNCTION_INDIRECT:
		res = MessageBoxIndirect( lpMsgBoxParams );
		break;

	}

	/* Remove hook */
	if( hCBT != NULL ) UnhookWindowsHookEx( hCBT );

	/* Destroy thread data */
	LocalFree( ( HLOCAL )data );

	return res;
}



int EmaBox( HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, int * pbCheckRes )
{
	/* Check extra flags */
	if( ( uType & MB_EXTRAMASC ) == 0 )
	{
		/* No extra */
		return MessageBox( hWnd, lpText, lpCaption, uType );
	}
	
	return ExtraAllTheSame( hWnd, lpText, lpCaption, uType, 0, NULL, pbCheckRes, FUNCTION_NORMAL );
}



int EmaBoxEx( HWND hWnd, LPCTSTR lpText, LPCTSTR lpCaption, UINT uType, WORD wLanguageId, int * pbCheckRes )
{
	/* Check extra flags */
	if( ( uType & MB_EXTRAMASC ) == 0 )
	{
		/* No extra */
		return MessageBoxEx( hWnd, lpText, lpCaption, uType, wLanguageId );
	}
	
	return ExtraAllTheSame( hWnd, lpText, lpCaption, uType, wLanguageId, NULL, pbCheckRes, FUNCTION_EXTENDED );
}



int EmaBoxIndirect( const LPMSGBOXPARAMS lpMsgBoxParams, int * pbCheckRes )
{
	/* Check extra flags */
	if( ( lpMsgBoxParams->dwStyle & MB_EXTRAMASC ) == 0 )
	{
		/* No extra */
		return MessageBoxIndirect( lpMsgBoxParams );
	}
	
	return ExtraAllTheSame( NULL, NULL, NULL, 0, 0, lpMsgBoxParams, pbCheckRes, FUNCTION_INDIRECT );
}



int EmaBoxLive()
{
	dwTlsSlot = TlsAlloc();
	if( dwTlsSlot == TLS_OUT_OF_INDEXES ) return 0;
	
	return 1;
}


#ifndef EMA_NEVERDIE
int EmaBoxDie()
{
	if( dwTlsSlot == TLS_OUT_OF_INDEXES ) return 0;

	TlsFree( dwTlsSlot );
	return 1;
}
#endif
