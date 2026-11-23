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
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>

#include "dzapp.h"
#include "dzappsettings.h"
#include "dzscene.h"
#include "dznode.h"
#include "dzbone.h"
#include "dzcamera.h"
#include "dzlight.h"
#include "dzboolproperty.h"
#include "dzpresentation.h"
#include "dztimerange.h"

const QString wls_anivis_watcher::property_name = "AnimateVisibility";

static const char *settings_path = "AniVis";
static const char *auto_add_key = "AddWhenToggled";

// While a time slider is dragged: at most this often (~10 per second).
static const int c_drag_interval_ms = 100;

// ---------------------------------------------------------------------------
// helpers

// Daz interpolates bool keys as numbers and rounds down (whatever the key's
// interpolation type), so between an On key and a later Off key the value
// is already Off on the next frame. Hold each key's value until the next
// key instead, so "On at 0, Off at 10" means visible for frames 0-9.
bool wls_anivis_watcher::value_at(const DzBoolProperty *prop, DzTime time)
{
	const int count = prop->getNumKeys();
	if (count == 0)
		return prop->getBoolValue();

	// Keys are sorted by time: find the last key at or before `time`.
	// Before the first key, the first key's value applies.
	int lo = 0, hi = count - 1, found = 0;
	while (lo <= hi)
	{
		const int mid = (lo + hi) / 2;
		if (prop->getKeyTime(mid) <= time)
		{
			found = mid;
			lo = mid + 1;
		}
		else
		{
			hi = mid - 1;
		}
	}
	return prop->getKeyBoolValue(found);
}

bool wls_anivis_watcher::is_animated(const DzBoolProperty *prop)
{
	return prop && prop->getNumKeys() >= 2;
}

// Setting a value on a property that has keys adds a key at the current
// time; clear the keys first so the result is one constant value.
static void set_constant(DzBoolProperty *prop, bool on)
{
	prop->deleteAllKeys();
	prop->setBoolValue(on);
}

// Daz deletes the scene before the main window on exit.
static bool scene_usable()
{
	return dzApp && !dzApp->isClosing() && dzScene;
}

static DzNode *owner_node(const DzBoolProperty *prop)
{
	return prop ? qobject_cast<DzNode*>(prop->getOwner()) : nullptr;
}

void wls_anivis_watcher::set_visible(DzNode *node, bool on)
{
	DzBoolProperty *visible = node ? node->getVisibilityControl() : nullptr;
	if (!visible || visible->getBoolValue() == on)
		return;
	applying_ = true; // not a user toggle
	visible->setBoolValue(on);
	applying_ = false;
}

// ---------------------------------------------------------------------------
// setup

wls_anivis_watcher *wls_anivis_watcher::instance()
{
	// Deliberately never deleted: Daz tears down the scene and unloads
	// plugins in its own order on exit, and a parent that deletes this late
	// could run code after the plugin is gone. The OS frees it at exit.
	static wls_anivis_watcher *watcher = new wls_anivis_watcher();
	return watcher;
}

wls_anivis_watcher::wls_anivis_watcher()
{
	DzAppSettings settings(settings_path);
	auto_add_ = settings.getBoolValue(auto_add_key, true);
	hook_scene();
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
	connect(dzScene, &DzScene::sceneCleared, this, &wls_anivis_watcher::on_scene_cleared);
	connect(dzScene, &DzScene::nodeAdded, this, &wls_anivis_watcher::on_node_added);
	connect(dzScene, &DzScene::timeChanged, this, &wls_anivis_watcher::on_time_changed);

	// Nodes already in the scene when the plugin started.
	DzNodeListIterator nli = dzScene->nodeListIterator();
	while (nli.hasNext())
		pending_.append(nli.next());
	flush_pending();
}

void wls_anivis_watcher::set_auto_add(bool on)
{
	auto_add_ = on;
	DzAppSettings settings(settings_path);
	settings.setBoolValue(auto_add_key, on);
}

void wls_anivis_watcher::notify()
{
	// Coalesce bursts (e.g. keying many items) into one pane refresh.
	if (notify_scheduled_)
		return;
	notify_scheduled_ = true;
	QTimer::singleShot(0, this, [this]() {
		notify_scheduled_ = false;
		if (scene_usable())
			emit anivis_changed();
	});
}

// ---------------------------------------------------------------------------
// properties

DzBoolProperty *wls_anivis_watcher::anivis_property(const DzNode *node)
{
	if (!node)
		return nullptr;
	return qobject_cast<DzBoolProperty*>(node->findProperty(property_name));
}

DzBoolProperty *wls_anivis_watcher::active_property(const DzNode *node)
{
	DzBoolProperty *prop = anivis_property(node);
	return (prop && !prop->isHidden()) ? prop : nullptr;
}

bool wls_anivis_watcher::is_candidate(const DzNode *node)
{
	if (!node || qobject_cast<const DzBone*>(node))
		return false;
	// Lights are cameras in the SDK; plain cameras are never rendered.
	if (qobject_cast<const DzCamera*>(node) && !qobject_cast<const DzLight*>(node))
		return false;
	return true;
}

DzBoolProperty *wls_anivis_watcher::ensure_property(DzNode *node)
{
	if (!node)
		return nullptr;

	DzBoolProperty *visible = node->getVisibilityControl();
	const bool visible_now = visible ? visible->getBoolValue() : true;

	DzBoolProperty *prop = anivis_property(node);
	if (prop)
	{
		if (prop->isHidden())
		{
			// Restore a removed property, starting from the current visibility.
			prop->setHidden(false);
			if (!is_animated(prop))
				set_constant(prop, visible_now);
			notify();
		}
		track(prop);
		return prop;
	}

	prop = new DzBoolProperty(property_name,
		true /*canAnimate*/, true /*isUserProperty*/, visible_now);
	prop->setLabel("Animate Visibility");
	prop->setPath("Display");

	auto presentation = new DzPresentation();
	presentation->setType("Modifier/Pose");
	presentation->setColorA(QColor(255, 255, 125));
	presentation->setColorB(QColor(255, 255, 220));
	prop->setPresentation(presentation);

	node->addProperty(prop);
	track(prop);
	notify();
	return prop;
}

bool wls_anivis_watcher::remove_property(DzNode *node)
{
	DzBoolProperty *prop = active_property(node);
	if (!prop)
		return false;

	// Hide first so the value changes below are not mirrored.
	prop->setHidden(true);
	set_constant(prop, true);
	set_visible(node, true);
	notify();
	return true;
}

bool wls_anivis_watcher::clear_keys(DzNode *node)
{
	DzBoolProperty *prop = active_property(node);
	if (!is_animated(prop))
		return false;
	handling_time_change_ = true; // do not mirror the intermediate values
	set_constant(prop, true);
	handling_time_change_ = false;
	set_visible(node, true);
	notify();
	return true;
}

void wls_anivis_watcher::key_visibility(DzNode *node, bool visible)
{
	DzBoolProperty *vis = node ? node->getVisibilityControl() : nullptr;
	key_visibility(node, visible, vis ? vis->getBoolValue() : true);
}

void wls_anivis_watcher::key_visibility(DzNode *node, bool visible, bool before)
{
	DzBoolProperty *prop = ensure_property(node);
	if (!prop)
		return;

	const DzTime now = dzScene->getTime();
	const DzTime start = dzScene->getAnimRange().getStart();

	handling_time_change_ = true; // apply once, at the end
	// First real key: keep what the node looked like before this change on
	// the earlier frames, so the key only applies from this frame on.
	if (!is_animated(prop))
	{
		prop->deleteAllKeys();
		if (now > start && before != visible)
			prop->setBoolValue(start, before);
	}
	prop->setBoolValue(now, visible);
	handling_time_change_ = false;

	apply(prop, now);
	notify();
}

// ---------------------------------------------------------------------------
// tracking

void wls_anivis_watcher::watch_node(DzNode *node)
{
	if (!is_candidate(node))
		return;
	if (DzBoolProperty *vis = node->getVisibilityControl())
		connect(vis, &DzProperty::currentValueChanged, this,
			&wls_anivis_watcher::on_visible_changed, Qt::UniqueConnection);
	track(anivis_property(node));
}

void wls_anivis_watcher::track(DzBoolProperty *prop)
{
	if (!prop)
		return;
	for (const QPointer<DzBoolProperty> &p : tracked_)
		if (p == prop)
			return;
	tracked_.append(prop);
	connect(prop, &DzProperty::currentValueChanged, this,
		&wls_anivis_watcher::on_anivis_value_changed, Qt::UniqueConnection);
	// Keys added, moved or deleted (also from the timeline) and removal.
	connect(prop, &DzProperty::valueChanged, this,
		&wls_anivis_watcher::notify, Qt::UniqueConnection);
	connect(prop, &DzProperty::hiddenChanged, this,
		&wls_anivis_watcher::notify, Qt::UniqueConnection);
}

void wls_anivis_watcher::rescan_scene()
{
	tracked_.removeAll(QPointer<DzBoolProperty>());
	DzNodeListIterator nli = dzScene->nodeListIterator();
	while (nli.hasNext())
		watch_node(nli.next());
	notify();
}

void wls_anivis_watcher::flush_pending()
{
	flush_scheduled_ = false;
	if (!scene_usable())
		return;
	// Wait until a file has finished loading: its saved AniVis properties
	// (including removed ones) only exist once loading is complete.
	if (dzScene->isLoading())
	{
		flush_scheduled_ = true;
		QTimer::singleShot(200, this, &wls_anivis_watcher::flush_pending);
		return;
	}

	const QList<QPointer<DzNode>> nodes = pending_;
	pending_.clear();
	for (const QPointer<DzNode> &node : nodes)
		if (node)
			watch_node(node);
	notify();
}

void wls_anivis_watcher::on_node_added(DzNode *node)
{
	pending_.append(node);
	if (!flush_scheduled_)
	{
		flush_scheduled_ = true;
		QTimer::singleShot(0, this, &wls_anivis_watcher::flush_pending);
	}
}

void wls_anivis_watcher::on_scene_loaded()
{
	// Covers opening and merging files (scenes, figures, props).
	flush_pending();
	rescan_scene();
	refresh();
}

void wls_anivis_watcher::on_scene_cleared()
{
	tracked_.clear();
	pending_.clear();
	notify();
}

// ---------------------------------------------------------------------------
// applying

void wls_anivis_watcher::apply(DzBoolProperty *prop, DzTime time)
{
	if (!prop || prop->isHidden())
		return;
	set_visible(owner_node(prop), value_at(prop, time));
}

void wls_anivis_watcher::refresh()
{
	if (scene_usable())
		apply_all(dzScene->getTime());
}

void wls_anivis_watcher::on_time_changed(DzTime time)
{
	if (!scene_usable())
		return;
	// Dragging a time slider (the Timeline, the AniVis pane, ...) sends a
	// time change per mouse move. Hiding or showing items is slow in Daz, so
	// apply at most every c_drag_interval_ms and again where the drag stops.
	// Playback, renders, scripts and single clicks/steps apply right away.
	const bool dragging = !dzScene->isPlaying()
		&& QGuiApplication::mouseButtons() != Qt::NoButton;
	if (dragging && last_apply_.isValid() && last_apply_.elapsed() < c_drag_interval_ms)
	{
		if (!deferred_scheduled_)
		{
			deferred_scheduled_ = true;
			QTimer::singleShot(c_drag_interval_ms - int(last_apply_.elapsed()), this,
				&wls_anivis_watcher::apply_deferred);
		}
		return;
	}
	apply_all(time);
}

void wls_anivis_watcher::apply_deferred()
{
	deferred_scheduled_ = false;
	if (scene_usable())
		apply_all(dzScene->getTime());
}

void wls_anivis_watcher::apply_all(DzTime time)
{
	last_apply_.start();
	handling_time_change_ = true;
	bool stale = false;
	for (const QPointer<DzBoolProperty> &prop : tracked_)
	{
		if (!prop)
		{
			stale = true;
			continue;
		}
		// Unanimated properties leave Visible (the eye icon) alone.
		if (is_animated(prop))
			apply(prop, time);
	}
	if (stale)
		tracked_.removeAll(QPointer<DzBoolProperty>());
	handling_time_change_ = false;
}

void wls_anivis_watcher::on_anivis_value_changed()
{
	if (handling_time_change_ || !scene_usable())
		return;
	auto prop = qobject_cast<DzBoolProperty*>(sender());
	if (!prop)
		return;
	if (is_animated(prop))
	{
		// Daz re-evaluates animated properties when the time changes; that
		// is handled (and throttled) by on_time_changed. Edits to the keys
		// arrive through valueChanged, which notifies the pane.
		if (!deferred_scheduled_)
			apply(prop, dzScene->getTime());
		return;
	}
	// The user changed a constant "Animate Visibility": show it right away.
	apply(prop, dzScene->getTime());
	notify();
}

void wls_anivis_watcher::on_visible_changed()
{
	// Only real toggles: not AniVis's own changes and not file loading.
	if (applying_ || handling_time_change_ || !auto_add_ || !scene_usable() || dzScene->isLoading())
		return;

	auto vis = qobject_cast<DzBoolProperty*>(sender());
	DzNode *node = owner_node(vis);
	if (!vis || !node)
		return;

	// A removed property stays removed.
	DzBoolProperty *prop = anivis_property(node);
	if (prop && prop->isHidden())
		return;

	const bool now_on = vis->getBoolValue();
	key_visibility(node, now_on, !now_on);
}
