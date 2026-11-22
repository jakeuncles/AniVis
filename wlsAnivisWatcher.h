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

#include <QtCore/QList>
#include <QtCore/QObject>
#include <QtCore/QPointer>

#include "dztypes.h"

class DzNode;
class DzBoolProperty;

// Keeps each node's real "Visible" property in step with its animated
// "AnimateVisibility" property whenever the scene time changes, and adds
// that property to new figures, props and lights when auto-add is on.
//
// A property that is not animated (fewer than two keys) never overrides
// Visible, so the normal eye icon keeps working until you key "Animate
// Visibility" on two or more frames. A hidden property means
// "removed": it is ignored and auto-add does not bring it back, and because
// the hidden flag is saved with the scene that choice survives save/load.
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

	// Whether auto-add would give this kind of node a property:
	// figures (incl. clothing and hair), props and lights; not bones or cameras.
	static bool is_candidate(const DzNode *node);

	// Adds the property, or restores one that was removed. Returns it.
	DzBoolProperty *ensure_property(DzNode *node);

	// "Removes" the property: deletes its keys, hides it, and makes the node
	// visible again. Returns false if the node had no active property.
	bool remove_property(DzNode *node);

	// Deletes the node's AniVis keys and makes it visible. False if it had none.
	bool clear_keys(DzNode *node);

	// True when the property has two or more keys (one key is a constant).
	static bool is_animated(const DzBoolProperty *prop);

	// Sets a key on the node's property at the scene's current time.
	void key_visibility(DzNode *node, bool visible);

	// Re-applies every keyed property at the current time.
	void refresh();

	bool auto_add() const { return auto_add_; }
	void set_auto_add(bool on);

private:
	wls_anivis_watcher();

	void hook_scene();
	void track(DzBoolProperty *prop);
	void rescan_scene();
	void flush_pending();
	void apply(DzBoolProperty *prop, DzTime time);

	void on_scene_loaded();
	void on_scene_cleared();
	void on_node_added(DzNode *node);
	void on_time_changed(DzTime time);
	void on_anivis_value_changed();

	QList<QPointer<DzBoolProperty>> tracked_;
	QList<QPointer<DzNode>> pending_;
	bool hooked_ = false;
	bool flush_scheduled_ = false;
	bool auto_add_ = true;
	bool handling_time_change_ = false;
};
