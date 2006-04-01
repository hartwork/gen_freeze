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

#ifndef EMABOX_CONFIG_H
#define EMABOX_CONFIG_H 1



/*//////////////////////////////////////////////////////////////////////////////
// SPACING
//////////////////////////////////////////////////////////////////////////////*/

/* Used with MB_CHECKALIGNABOVExxx */
#define VSPACE_CHECKBOX_TO_BUTTONS       10
#define VSPACE_EXTRA_BOTTOM              4

/* Used with MB_CHECKALIGNBELOWLEFT */
#define HSPACE_CHECKBOX_LEFT             5
#define VSPACE_CHECKBOX                  1
#define VSPACE_EXTRA_BUTTONS_TO_SEP      4

/* Used with MB_CHECKALIGNBELOWTEXT */
#define VSPACE_BUTTONS_TO_CHECKBOX       8
#define VSPACE_CHECKBOX_TO_BOTTOM        6



/*//////////////////////////////////////////////////////////////////////////////
// CODEFLOW
//////////////////////////////////////////////////////////////////////////////*/

/* Makes EmaBoxLive() be called automatically */
#define EMA_AUTOLIVE

/* Removes EmaBoxDie() if not needed */
#define EMA_NEVERDIE



/*//////////////////////////////////////////////////////////////////////////////
// BUTTON TEXT
//////////////////////////////////////////////////////////////////////////////*/

#define EMA_TEXT_NEVER_AGAIN      "Do not ask again"

#define EMA_TEXT_REMEMBER_CHOICE  "Remember my choice"



#endif /* EMABOX_CONFIG_H */
