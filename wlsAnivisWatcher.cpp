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

#include "wlsAnivisWatcher.h"

#include <QtCore/QTimer>

#include "dzapp.h"
#include "dzscene.h"
#include "dznode.h"
#include "dzboolproperty.h"

const QString wls_anivis_watcher::property_name = "AnimateVisibility";

wls_anivis_watcher *wls_anivis_watcher::instance()
{
	// Parented to the application so it lives as long as Daz Studio.
	static wls_anivis_watcher *watcher = nullptr;
	if (!watcher)
	{
		watcher = new wls_anivis_watcher();
		watcher->setParent(dzApp);
	}
	return watcher;
}

wls_anivis_watcher::wls_anivis_watcher()
{
	hook_scene();
}

DzBoolProperty *wls_anivis_watcher::anivis_property(const DzNode *node)
{
	if (!node)
		return nullptr;
	return qobject_cast<DzBoolProperty*>(node->findProperty(property_name));
}

void wls_anivis_watcher::hook_scene()
{
	if (hooked_)
		return;
	// The plugin can load before the scene object exists: try again once
	// the event loop is running.
	if (!dzScene)
	{
		QTimer::singleShot(250, this, &wls_anivis_watcher::hook_scene);
		return;
	}
	hooked_ = true;

	// Function-pointer connects are checked by the compiler, so a renamed
	// slot can no longer silently break the plugin.
	connect(dzScene, &DzScene::sceneLoaded, this, &wls_anivis_watcher::on_scene_loaded);
	connect(dzScene, &DzScene::nodeAdded, this, &wls_anivis_watcher::on_node_added);
	connect(dzScene, &DzScene::timeChanged, this, &wls_anivis_watcher::on_time_changed);

	rescan_scene();
}

void wls_anivis_watcher::watch(DzBoolProperty *prop)
{
	if (!prop)
		return;
	connect(prop, &DzProperty::currentValueChanged, this,
		&wls_anivis_watcher::on_anivis_value_changed, Qt::UniqueConnection);
}

void wls_anivis_watcher::rescan_scene()
{
	DzNodeListIterator nli = dzScene->nodeListIterator();
	while (nli.hasNext())
		watch(anivis_property(nli.next()));
}

void wls_anivis_watcher::on_scene_loaded()
{
	// Covers both opening and merging a scene: the saved properties are
	// only complete once loading has finished.
	rescan_scene();
	on_time_changed(dzScene->getTime());
}

void wls_anivis_watcher::on_node_added(DzNode *node)
{
	watch(anivis_property(node));
}

// Daz interpolates bool keys as numbers and rounds down (whatever the key's
// interpolation type), so between an On key and a later Off key the value
// is already Off on the next frame. Hold each key's value until the next
// key instead, so "On at 0, Off at 10" means visible for frames 0-9.
static bool stepped_value(const DzBoolProperty *prop, DzTime time)
{
	const int count = prop->getNumKeys();
	if (count == 0)
		return prop->getBoolValue(time);

	int index = 0; // before the first key: use the first key's value
	for (int i = 0; i < count; ++i)
	{
		if (prop->getKeyTime(i) > time)
			break;
		index = i;
	}
	return prop->getKeyBoolValue(index);
}

void wls_anivis_watcher::apply(DzNode *node, DzBoolProperty *prop, DzTime time)
{
	DzBoolProperty *visible = node ? node->getVisibilityControl() : nullptr;
	if (!visible || !prop)
		return;
	const bool on = stepped_value(prop, time);
	if (visible->getBoolValue() != on)
		visible->setBoolValue(on);
}

void wls_anivis_watcher::on_time_changed(DzTime time)
{
	handling_time_change_ = true;
	DzNodeListIterator nli = dzScene->nodeListIterator();
	while (nli.hasNext())
	{
		DzNode *node = nli.next();
		if (DzBoolProperty *prop = anivis_property(node))
			apply(node, prop, time);
	}
	handling_time_change_ = false;
}

void wls_anivis_watcher::on_anivis_value_changed()
{
	if (handling_time_change_)
		return;

	auto prop = qobject_cast<DzBoolProperty*>(sender());
	if (!prop)
		return;

	apply(qobject_cast<DzNode*>(prop->getOwner()), prop, dzScene->getTime());
}
