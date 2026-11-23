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
#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "dztypes.h"

class DzNode;
class DzBoolProperty;
class DzProperty;

// Keeps each node's real "Visible" property in step with its animated
// "AnimateVisibility" property whenever the scene time changes.
//
// The property is added when, and only when, you toggle an item's
// visibility (the eye icon in the Scene pane, or its Visible parameter):
// the toggle is keyed at the current frame. On the first frame that is just
// a plain value; on a later frame the frames before it keep their old state.
//
// A property that is not animated (fewer than two keys) never overrides
// Visible. A hidden property means "removed": it is ignored and toggling
// does not bring it back; the hidden flag is saved with the scene.
class wls_anivis_watcher : public QObject
{
	Q_OBJECT
public:
	static wls_anivis_watcher *instance();

	// Name of the animatable property AniVis adds to nodes.
	static const QString property_name;

	// The node's AniVis property (hidden or not), or nullptr.
	static DzBoolProperty *anivis_property(const DzNode *node);

	// The node's AniVis property if it is active (exists and not removed).
	static DzBoolProperty *active_property(const DzNode *node);

	// Items AniVis works with: figures (incl. clothing and hair), props and
	// lights; not bones or cameras.
	static bool is_candidate(const DzNode *node);

	// True when the property has two or more keys (one key is a constant:
	// Daz saves every animatable value as one key).
	static bool is_animated(const DzBoolProperty *prop);

	// Visibility the property gives at `time`: each key holds until the next.
	static bool value_at(const DzBoolProperty *prop, DzTime time);

	// Adds the property, or restores one that was removed. Returns it.
	DzBoolProperty *ensure_property(DzNode *node);

	// "Removes" the property: deletes its keys, hides it, and makes the node
	// visible again. Returns false if the node had no active property.
	bool remove_property(DzNode *node);

	// Deletes the node's AniVis keys and makes it visible. False if it had none.
	bool clear_keys(DzNode *node);

	// Keys the node visible/hidden at the current frame, adding the property
	// if needed. `before` is what the node looked like before this change.
	void key_visibility(DzNode *node, bool visible, bool before);
	void key_visibility(DzNode *node, bool visible);

	// Key editing from the AniVis Timeline. Each is one undo step, except
	// move_key, which the caller wraps (a drag is one undo step).
	// Keys the node visible/hidden at `time` (any frame), adding the property.
	void key_at(DzNode *node, DzTime time, bool visible);
	// Deletes key `index`. One key left = a constant: the node shows it.
	bool delete_key(DzNode *node, int index);
	// Sets key `index` to visible/hidden.
	bool set_key(DzNode *node, int index, bool visible);
	// Moves key `index` to `to`. Returns its new index, or -1 when another
	// key is already there (nothing changes then).
	int  move_key(DzNode *node, int index, DzTime to);

	// Re-applies every animated property at the current time.
	void refresh();

	// Whether toggling an item's visibility adds the property (remembered).
	bool auto_add() const { return auto_add_; }
	void set_auto_add(bool on);

signals:
	// Something changed that the AniVis pane shows (properties, keys).
	void anivis_changed();
	// "Key Visibility When Toggled" was turned on or off.
	void auto_add_changed(bool on);

private:
	wls_anivis_watcher();

	void hook_scene();
	void watch_node(DzNode *node);
	void track(DzBoolProperty *prop);
	void rescan_scene();
	void flush_pending();
	void apply(DzBoolProperty *prop, DzTime time);
	void set_visible(DzNode *node, bool on);
	void notify();

	void on_scene_loaded();
	void on_scene_cleared();
	void on_node_added(DzNode *node);
	void on_time_changed(DzTime time);
	void apply_all(DzTime time);
	void apply_deferred();
	void on_anivis_value_changed();
	void on_anivis_edited();
	void on_visible_changed();
	void on_property_added(DzProperty *prop);
	void on_property_removed(DzProperty *prop);
	void schedule_refresh();
	void key_in_place(DzBoolProperty *prop, DzTime time, bool visible, bool before);

	QList<QPointer<DzBoolProperty>> tracked_;
	QList<QPointer<DzNode>> pending_;
	bool hooked_ = false;
	bool flush_scheduled_ = false;
	bool notify_scheduled_ = false;
	bool refresh_scheduled_ = false;
	bool auto_add_ = true;
	bool applying_ = false;  // AniVis itself is changing Visible
	bool handling_time_change_ = false;

	// Hiding or showing an item costs Daz 50-200 ms (viewport and Scene
	// pane updates). While a time slider is dragged, apply at most every
	// c_drag_interval_ms and once more where the drag stops.
	QElapsedTimer last_apply_;
	bool deferred_scheduled_ = false;
};
