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

	Daz Studio 6 port, 2026.
**********************************************************************/

#include <QtGui/QColor>

#include "dzscene.h"
#include "dznode.h"
#include "dzboolproperty.h"
#include "dzpresentation.h"

#include "anivisaction.h"
#include "wlsAnivisWatcher.h"

DzAniVisAction::DzAniVisAction() :
	DzEditAction( "Add AniVis Property", "Add the Animate Visibility property to the selected node(s)" )
{
	// Start watching as soon as the plugin loads, so scenes saved with
	// AniVis properties work without using the menu item first.
	wls_anivis_watcher::instance();
}

void DzAniVisAction::executeAction()
{
	wls_anivis_watcher *watcher = wls_anivis_watcher::instance();

	const DzNodeList nodes = dzScene->getSelectedNodeList();
	for (DzNode *node : nodes)
	{
		if (!node || wls_anivis_watcher::anivis_property(node))
			continue;

		// Start from the node's current visibility.
		DzBoolProperty *visible = node->getVisibilityControl();
		const bool start_on = visible ? visible->getBoolValue() : true;

		auto prop = new DzBoolProperty(wls_anivis_watcher::property_name,
			true /*canAnimate*/, true /*isUserProperty*/, start_on);
		prop->setLabel("Animate Visibility");
		prop->setPath("Display");

		auto presentation = new DzPresentation();
		presentation->setType("Modifier/Pose");
		presentation->setColorA(QColor(255, 255, 125));
		presentation->setColorB(QColor(255, 255, 220));
		prop->setPresentation(presentation);

		node->addProperty(prop);
		watcher->watch(prop);
	}
}
