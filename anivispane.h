/**********************************************************************
	AniVis for Daz Studio 6 - "AniVis" page in the Scene pane.

	This source code and software provided under the terms of the
	Mozilla Public License Version 2.0 (MPL) a copy of which is
	provided with it.
**********************************************************************/
#pragma once

#include <QtCore/QHash>
#include <QtCore/QPointer>
#include <QtWidgets/QStyledItemDelegate>

#include "dzaction.h"
#include "dzsubpane.h"
#include "dztypes.h"

class QCheckBox;
class QLabel;
class QTreeWidget;
class QTreeWidgetItem;
class DzNode;
class DzSettings;
class DzAniVisSubPane;

// Paints the "Now" and "Timeline" columns.
class DzAniVisDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit DzAniVisDelegate(DzAniVisSubPane *pane);
	void paint(QPainter *painter, const QStyleOptionViewItem &option,
		const QModelIndex &index) const override;
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
	DzAniVisSubPane *pane_;
};

// A page in Daz's Scene pane that lists the items with an Animate Visibility
// property, shows when each one is visible on a timeline strip, and lets you
// jump to a frame or toggle visibility at the current frame.
class DzAniVisSubPane : public DzSubPane {
	Q_OBJECT
	Q_CLASSINFO( "subpane_type", "DzHierarchyPane" ) // a page of the "Scene" pane
public:
	enum Column { ItemColumn = 0, NowColumn, KeysColumn, TimelineColumn };

	DzAniVisSubPane();
	~DzAniVisSubPane() override;

	QString	getLabel() const override;

	// Points the Scene pane's Tips tab at an "AniVis" tip page (created on
	// first use), so Daz does not ask to create a placeholder page.
	DzSettings *getInfoTabsSettings() const override;

	DzNode	*node_for(const QModelIndex &index) const;

	// The timeline strip's drawing area inside a cell.
	static QRect strip_rect(const QRect &cell);

	Q_SLOT void refresh();

protected:
	bool	eventFilter(QObject *watched, QEvent *event) override;
	void	showEvent(QShowEvent *e) override;

private:
	void	schedule_rebuild();
	void	rebuild();
	void	repaint_rows();
	QTreeWidgetItem *item_for(DzNode *node, QHash<DzNode*, QTreeWidgetItem*> &items,
		const QHash<DzNode*, bool> &listed);
	QList<DzNode*> target_nodes() const;
	void	jump_to(const QModelIndex &index, int x);

	void	on_tree_selection_changed();
	void	on_scene_selection_changed();
	void	key_selected(bool visible);
	void	clear_selected();
	void	remove_selected();

	QTreeWidget	*tree_ = nullptr;
	QCheckBox	*show_all_ = nullptr;
	QLabel		*summary_ = nullptr;
	QHash<quintptr, QPointer<DzNode>> nodes_;
	mutable QPointer<DzSettings> tip_settings_;
	bool	rebuild_scheduled_ = false;
	bool	dirty_ = true;
	bool	syncing_selection_ = false;
	bool	scrubbing_ = false;
};

// Shows the AniVis page (Window > Panes, and the Scene pane's page menu).
class DzAniVisSubPaneAction : public DzSubPaneAction {
	Q_OBJECT
public:
	DzAniVisSubPaneAction();
};
