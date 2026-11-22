/**********************************************************************
	Copyright (C) 2015, 2017 by West Leitrim Software. 
	All Rights Reserved.
	
	This source code and software provided under the terms of the 
	Mozilla Public License Version 2.0 (MPL) a copy of which is 
	provided with it.
	
	Without limitation or modification of the license, note that 
	key terms of the license are approximately:
	
	(1) free to use for personal and commercial purposes
	(2) you must publish source to any derivative you distribute
	(3) there is ABSOLUTELY NO WARRANTY

**********************************************************************/

#include "dzplugin.h"
#include "dzapp.h"

#include "version.h"
#include "anivisaction.h"
#include "anivisguids.h"
#include "anivispane.h"

DZ_PLUGIN_DEFINITION( "AniVis - Animate Node Visibility (Daz Studio 6)" );

DZ_PLUGIN_AUTHOR( "West Leitrim Software" );

DZ_PLUGIN_VERSION( ANAVIS_MAJOR, ANAVIS_MINOR, ANAVIS_REV, ANAVIS_BUILD );

DZ_PLUGIN_DESCRIPTION( QString(
					  "<a href=\"%1/AniVis/index.htm\">Documentation</a><br><br>"
					  "This plugin fills a hole in use of the DAZ Studio timeline "
					  "by making it possible to animate the visibility of a node. "
					  "Toggling an item's visibility keys it at the current frame (Animate Visibility). "
					  "See the AniVis page in the Scene pane and Edit &gt; AniVis."
					  ).arg( dzApp->getDocumentationPath() ) );

DZ_PLUGIN_CLASS_GUID( DzAniVisAction, 309CD52D-FC6E-4434-A60E-FC99EFB44080 );
DZ_PLUGIN_CLASS_GUID( DzAniVisRemoveAction, 7F30C8C3-3252-4350-AE65-7C6534BBC549 );
DZ_PLUGIN_CLASS_GUID( DzAniVisKeyOnAction, 79E4AD13-D30C-469C-9E24-662FFEB5C7F9 );
DZ_PLUGIN_CLASS_GUID( DzAniVisKeyOffAction, A618D2E9-3BFB-435E-A416-C74D534BADDF );
DZ_PLUGIN_CLASS_GUID( DzAniVisClearKeysAction, 65C9C950-C354-4E7C-ABA6-1D681820A065 );
DZ_PLUGIN_CLASS_GUID( DzAniVisSelectAction, 95446FC8-3D4D-4A61-A7BA-BC64674A047B );
DZ_PLUGIN_CLASS_GUID( DzAniVisSubPane, ANIVIS_SUBPANE_GUID );
DZ_PLUGIN_CLASS_GUID( DzAniVisSubPaneAction, ANIVIS_SUBPANE_ACTION_GUID );
DZ_PLUGIN_CLASS_GUID( DzAniVisAutoAddAction, E001CAA3-8D95-4AFB-B79D-61A224C1CF47 );

