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

#include <QtCore/QObject>

#include "dztypes.h"

class DzNode;
class DzBoolProperty;

// Keeps each node's real "Visible" property in step with its animated
// "AnimateVisibility" property whenever the scene time changes.
class wls_anivis_watcher : public QObject
{
	Q_OBJECT
public:
	static wls_anivis_watcher *instance();

	// Name of the animatable property AniVis adds to nodes.
	static const QString property_name;

	// Returns the node's AniVis property, or nullptr.
	static DzBoolProperty *anivis_property(const DzNode *node);

	// Starts watching a node's AniVis property (safe to call repeatedly).
	void watch(DzBoolProperty *anivis_property);

private:
	wls_anivis_watcher();

	void hook_scene();
	void rescan_scene();
	void apply(DzNode *node, DzBoolProperty *anivis_property, DzTime time);

	void on_scene_loaded();
	void on_node_added(DzNode *node);
	void on_time_changed(DzTime time);
	void on_anivis_value_changed();

	bool hooked_ = false;
	bool handling_time_change_ = false;
};
