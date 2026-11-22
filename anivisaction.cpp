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

#include "dzapp.h"
#include "dzscene.h"
#include "dznode.h"
#include "dzboolproperty.h"

#include "anivisaction.h"
#include "wlsAnivisWatcher.h"

static void status(const QString &message)
{
	dzApp->statusLine("AniVis: " + message, true);
}

static wls_anivis_watcher *watcher()
{
	return wls_anivis_watcher::instance();
}

// ---------------------------------------------------------------------------

DzAniVisBaseAction::DzAniVisBaseAction(const QString &text, const QString &desc) :
	DzEditAction(text, desc)
{
	// Start watching as soon as the plugin loads, so saved scenes and
	// keying on toggle work without using a menu item first.
	watcher();
}

QString DzAniVisBaseAction::getDefaultMenuPath() const
{
	return "&Edit/AniVis";
}

QStringList DzAniVisBaseAction::getDefaultMenuPaths() const
{
	return QStringList() << getDefaultMenuPath() << "DzHierarchyPane::Edit/AniVis";
}

DzNodeList DzAniVisBaseAction::selected_nodes() const
{
	const DzNodeList nodes = dzScene->getSelectedNodeList();
	if (nodes.isEmpty())
		status("select one or more items first.");
	return nodes;
}

// ---------------------------------------------------------------------------

DzAniVisAction::DzAniVisAction() :
	DzAniVisBaseAction("Add Property",
		"Add the Animate Visibility property to the selected items (or restore it)")
{
}

void DzAniVisAction::executeAction()
{
	int count = 0;
	for (DzNode *node : selected_nodes())
		if (watcher()->ensure_property(node))
			++count;
	if (count)
		status(QString("Animate Visibility is on %1 item(s).").arg(count));
}

// ---------------------------------------------------------------------------

DzAniVisRemoveAction::DzAniVisRemoveAction() :
	DzAniVisBaseAction("Remove Property",
		"Remove Animate Visibility (and its keys) from the selected items; "
		"toggling visibility will not add it back")
{
}

void DzAniVisRemoveAction::executeAction()
{
	int count = 0;
	for (DzNode *node : selected_nodes())
		if (watcher()->remove_property(node))
			++count;
	status(QString("removed from %1 item(s).").arg(count));
}

// ---------------------------------------------------------------------------

DzAniVisKeyOnAction::DzAniVisKeyOnAction() :
	DzAniVisBaseAction("Key Visible Here",
		"Key the selected items visible from the current frame on")
{
}

void DzAniVisKeyOnAction::executeAction()
{
	for (DzNode *node : selected_nodes())
		watcher()->key_visibility(node, true);
}

// ---------------------------------------------------------------------------

DzAniVisKeyOffAction::DzAniVisKeyOffAction() :
	DzAniVisBaseAction("Key Hidden Here",
		"Key the selected items hidden from the current frame on")
{
}

void DzAniVisKeyOffAction::executeAction()
{
	for (DzNode *node : selected_nodes())
		watcher()->key_visibility(node, false);
}

// ---------------------------------------------------------------------------

DzAniVisClearKeysAction::DzAniVisClearKeysAction() :
	DzAniVisBaseAction("Clear Keys",
		"Delete all Animate Visibility keys on the selected items and make them visible")
{
}

void DzAniVisClearKeysAction::executeAction()
{
	int count = 0;
	for (DzNode *node : selected_nodes())
		if (watcher()->clear_keys(node))
			++count;
	status(QString("cleared keys on %1 item(s).").arg(count));
}

// ---------------------------------------------------------------------------

DzAniVisSelectAction::DzAniVisSelectAction() :
	DzAniVisBaseAction("Select Animated Items",
		"Select every item whose visibility is animated with AniVis")
{
}

void DzAniVisSelectAction::executeAction()
{
	dzScene->selectAllNodes(false);
	int count = 0;
	DzNodeListIterator nli = dzScene->nodeListIterator();
	while (nli.hasNext())
	{
		DzNode *node = nli.next();
		if (wls_anivis_watcher::is_animated(wls_anivis_watcher::active_property(node)))
		{
			node->select(true);
			++count;
		}
	}
	status(QString("%1 animated item(s) selected.").arg(count));
}

// ---------------------------------------------------------------------------

DzAniVisAutoAddAction::DzAniVisAutoAddAction() :
	DzAniVisBaseAction("Key Visibility When Toggled",
		"Toggling an item's visibility (eye icon) adds Animate Visibility and keys it at the current frame")
{
	setCheckable(true);
	setChecked(watcher()->auto_add());
}

void DzAniVisAutoAddAction::executeAction()
{
	watcher()->set_auto_add(isChecked());
	status(isChecked() ? "toggling visibility now keys it." : "toggling visibility no longer keys it.");
}
