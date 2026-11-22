/**********************************************************************
	AniVis for Daz Studio 6 - the AniVis pane (docks next to the Timeline).

	This source code and software provided under the terms of the
	Mozilla Public License Version 2.0 (MPL) a copy of which is
	provided with it.
**********************************************************************/
#pragma once

#include <QtCore/QList>
#include <QtCore/QMetaObject>
#include <QtCore/QPointer>
#include <QtWidgets/QWidget>

#include "dzaction.h"
#include "dzpane.h"
#include "dztypes.h"

class QCheckBox;
class QLabel;
class QScrollBar;
class DzNode;

// One row of the AniVis timeline.
struct AniVisRow
{
	QPointer<DzNode> node;
	QString label;
	int depth = 0;
};

// Draws a frame ruler and, per item, its visibility over the animation:
// green = visible, red = hidden, yellow diamonds = keys, white line = the
// current frame. Plain painting (no item views), so nothing in it reacts to
// Daz tearing down the scene.
class DzAniVisTimelineView : public QWidget {
	Q_OBJECT
public:
	explicit DzAniVisTimelineView(QWidget *parent = nullptr);

	void set_rows(const QList<AniVisRow> &rows);
	void set_scroll(int first_row);
	int  row_count() const { return rows_.size(); }
	int  visible_rows() const;

signals:
	void scroll_requested(int delta_rows);

protected:
	void paintEvent(QPaintEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void wheelEvent(QWheelEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	enum Area { Nowhere, Ruler, Name, Eye, Bars };
	Area area_at(const QPoint &pos, int *row) const;
	QRect bars_rect() const;
	void layout_columns();
	void scrub_to(int x);
	DzNode *node_at_row(int row) const;

	QList<AniVisRow> rows_;
	int first_row_ = 0;
	int name_width_ = 160;
	bool scrubbing_ = false;
};

// The "AniVis Timeline" pane: Window > Panes (Tabs) > AniVis Timeline.
// A pane of its own; dock it wherever you like, e.g. next to the Timeline.
class DzAniVisPane : public DzPane {
	Q_OBJECT
public:
	DzAniVisPane();
	~DzAniVisPane() override;

	Q_SLOT void refresh() override;

protected:
	void showEvent(QShowEvent *e) override;
	void resizeEvent(QResizeEvent *e) override;

private:
	void connect_scene();
	bool alive() const;
	void schedule_rebuild();
	void rebuild();
	void repaint_view();
	void update_scrollbar();
	QList<DzNode*> target_nodes() const;

	DzAniVisTimelineView *view_ = nullptr;
	QScrollBar *scroll_ = nullptr;
	QCheckBox *show_all_ = nullptr;
	QLabel *summary_ = nullptr;
	QList<QMetaObject::Connection> connections_;      // lifetime of the pane
	QList<QMetaObject::Connection> row_connections_;  // rebuilt with the rows
	bool rebuild_scheduled_ = false;
	bool dirty_ = true;
	bool closing_ = false;
};

// Window > Panes (Tabs) > AniVis Timeline.
class DzAniVisPaneAction : public DzPaneAction {
	Q_OBJECT
public:
	DzAniVisPaneAction() : DzPaneAction(DzAniVisPane::staticMetaObject.className()) {}
};
