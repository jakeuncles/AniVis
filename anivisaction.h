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

#ifndef ANAVIS_PLUGIN_ACTION_H
#define ANAVIS_PLUGIN_ACTION_H

#include "dzaction.h"
#include "dztypes.h"

// Base for the AniVis menu items: puts them under Edit > AniVis and in the
// Scene pane's right-click Edit menu.
class DzAniVisBaseAction : public DzEditAction {
	Q_OBJECT
public:
	DzAniVisBaseAction(const QString &text, const QString &desc);

protected:
	QString		getDefaultMenuPath() const override;
	QStringList	getDefaultMenuPaths() const override;

	// The selected nodes, warning in the status bar if there are none.
	DzNodeList	selected_nodes() const;
};

// Edit > AniVis > Add Property (also restores a removed one).
class DzAniVisAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisAction();
protected:
	void executeAction() override;
};

// Edit > AniVis > Remove Property.
class DzAniVisRemoveAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisRemoveAction();
protected:
	void executeAction() override;
};

// Edit > AniVis > Key Visible Here.
class DzAniVisKeyOnAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisKeyOnAction();
protected:
	void executeAction() override;
};

// Edit > AniVis > Key Hidden Here.
class DzAniVisKeyOffAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisKeyOffAction();
protected:
	void executeAction() override;
};

// Edit > AniVis > Clear Keys.
class DzAniVisClearKeysAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisClearKeysAction();
protected:
	void executeAction() override;
};

// Edit > AniVis > Select Animated Nodes.
class DzAniVisSelectAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisSelectAction();
protected:
	void executeAction() override;
};

// Edit > AniVis > Auto-Add to New Items (on/off, remembered).
class DzAniVisAutoAddAction : public DzAniVisBaseAction {
	Q_OBJECT
public:
	DzAniVisAutoAddAction();
protected:
	void executeAction() override;
};

#endif
