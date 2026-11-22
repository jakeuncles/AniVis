/**********************************************************************
	AniVis for Daz Studio 6 - the AniVis Timeline pane.

	This source code and software provided under the terms of the
	Mozilla Public License Version 2.0 (MPL) a copy of which is
	provided with it.
**********************************************************************/

#include "anivispane.h"

#include <functional>

#include <QtCore/QHash>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QWheelEvent>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollBar>
#include <QtWidgets/QVBoxLayout>

#include "dzapp.h"
#include "dzboolproperty.h"
#include "dznode.h"
#include "dzscene.h"
#include "dztimerange.h"

#include "wlsAnivisWatcher.h"

namespace {

const QColor c_visible(76, 175, 80);      // green
const QColor c_hidden(198, 64, 64);       // red
const QColor c_key(255, 213, 79);         // yellow, like the property's slider
const QColor c_now(255, 255, 255);
const QColor c_empty(90, 90, 90);

const int c_ruler_h = 22;
const int c_row_h = 22;
const int c_eye_w = 78;
const int c_indent = 12;
const int c_margin = 8;

wls_anivis_watcher *watcher() { return wls_anivis_watcher::instance(); }

// Daz deletes the scene before the main window (and this pane) on exit:
// never touch the scene once it is gone or the application is closing.
bool scene_usable()
{
	return dzApp && !dzApp->isClosing() && dzScene;
}

bool node_visible(const DzNode *node)
{
	const DzBoolProperty *vis = node ? node->getVisibilityControl() : nullptr;
	return vis ? vis->getBoolValue() : true;
}

void anim_range(DzTime &start, DzTime &end, DzTime &step)
{
	const DzTimeRange range = dzScene->getAnimRange();
	step = qMax<DzTime>(1, dzScene->getTimeStep());
	start = range.getStart();
	end = qMax(range.getEnd(), start);
}

} // namespace

// ---------------------------------------------------------------------------
// view

DzAniVisTimelineView::DzAniVisTimelineView(QWidget *parent) :
	QWidget(parent)
{
	setMinimumHeight(c_ruler_h + 2 * c_row_h);
	setMouseTracking(false);
	setFocusPolicy(Qt::NoFocus);
}

void DzAniVisTimelineView::set_rows(const QList<AniVisRow> &rows)
{
	rows_ = rows;
	first_row_ = qBound(0, first_row_, qMax(0, rows_.size() - 1));
	layout_columns();
	update();
}

void DzAniVisTimelineView::set_scroll(int first_row)
{
	first_row_ = qMax(0, first_row);
	update();
}

int DzAniVisTimelineView::visible_rows() const
{
	return qMax(1, (height() - c_ruler_h) / c_row_h);
}

void DzAniVisTimelineView::layout_columns()
{
	const QFontMetrics fm(font());
	int widest = fm.horizontalAdvance(tr("Item"));
	for (const AniVisRow &row : rows_)
		widest = qMax(widest, row.depth * c_indent + fm.horizontalAdvance(row.label));
	name_width_ = qBound(110, widest + 2 * c_margin, qMax(110, width() / 3));
}

void DzAniVisTimelineView::resizeEvent(QResizeEvent *e)
{
	layout_columns();
	QWidget::resizeEvent(e);
}

QRect DzAniVisTimelineView::bars_rect() const
{
	const int left = name_width_ + c_eye_w;
	return QRect(left, 0, qMax(10, width() - left - c_margin), height());
}

DzNode *DzAniVisTimelineView::node_at_row(int row) const
{
	return (row >= 0 && row < rows_.size()) ? rows_.at(row).node.data() : nullptr;
}

DzAniVisTimelineView::Area DzAniVisTimelineView::area_at(const QPoint &pos, int *row) const
{
	*row = -1;
	if (pos.y() < c_ruler_h)
		return pos.x() >= bars_rect().left() ? Ruler : Nowhere;
	const int r = first_row_ + (pos.y() - c_ruler_h) / c_row_h;
	if (r >= rows_.size())
		return pos.x() >= bars_rect().left() ? Bars : Nowhere;
	*row = r;
	if (pos.x() < name_width_)
		return Name;
	if (pos.x() < name_width_ + c_eye_w)
		return Eye;
	return Bars;
}

void DzAniVisTimelineView::scrub_to(int x)
{
	if (!scene_usable())
		return;
	DzTime start, end, step;
	anim_range(start, end, step);
	const QRect bars = bars_rect();
	const int frames = int((end - start) / step) + 1;
	const double f = qBound(0.0, double(x - bars.left()) / bars.width(), 0.999999);
	const DzTime t = start + DzTime(int(f * frames)) * step;
	if (t != dzScene->getTime())
		dzScene->setTime(t);
}

void DzAniVisTimelineView::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	const QPalette pal = palette();
	p.fillRect(rect(), pal.color(QPalette::Base));
	if (!scene_usable())
		return;

	DzTime start, end, step;
	anim_range(start, end, step);
	const QRect bars = bars_rect();
	const DzTime span = end + step - start; // include the last frame
	auto x_at = [&](DzTime t) {
		const double f = double(qBound<DzTime>(start, t, end + step) - start) / double(span);
		return bars.left() + f * bars.width();
	};
	const QColor text = pal.color(QPalette::Text);
	QColor dim = text;
	dim.setAlpha(130);

	// Ruler with frame numbers.
	p.fillRect(QRect(0, 0, width(), c_ruler_h), pal.color(QPalette::Window));
	p.setPen(dim);
	p.drawText(QRect(c_margin, 0, name_width_ - c_margin, c_ruler_h), Qt::AlignVCenter | Qt::AlignLeft, tr("Item"));
	p.drawText(QRect(name_width_, 0, c_eye_w, c_ruler_h), Qt::AlignVCenter | Qt::AlignLeft, tr("Now"));
	const int frames = int((end - start) / step) + 1;
	const double px_per_frame = double(bars.width()) / frames;
	int label_every = 1;
	for (int k : {1, 2, 5, 10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000})
	{
		label_every = k;
		if (k * px_per_frame >= 36)
			break;
	}
	const int first_frame = int(start / step);
	for (int f = 0; f < frames; ++f)
	{
		const int frame = first_frame + f;
		const double x = bars.left() + f * px_per_frame;
		const bool major = frame % label_every == 0;
		if (!major && px_per_frame < 4)
			continue;
		p.setPen(dim);
		p.drawLine(QPointF(x, c_ruler_h - (major ? 8 : 4)), QPointF(x, c_ruler_h - 1));
		if (major)
			p.drawText(QRectF(x + 2, 0, 40, c_ruler_h - 6), Qt::AlignLeft | Qt::AlignVCenter, QString::number(frame));
	}

	// Rows.
	const int shown = visible_rows() + 1;
	const QColor highlight = pal.color(QPalette::Highlight);
	if (rows_.isEmpty())
	{
		p.setPen(dim);
		p.drawText(rect().adjusted(c_margin, c_ruler_h, -c_margin, 0), Qt::AlignCenter | Qt::TextWordWrap,
			tr("No items use AniVis yet.\nToggle an item's eye icon in the Scene pane to key it at the current frame."));
	}
	for (int i = 0; i < shown; ++i)
	{
		const int r = first_row_ + i;
		if (r >= rows_.size())
			break;
		const AniVisRow &row = rows_.at(r);
		DzNode *node = row.node.data();
		if (!node)
			continue;
		const int y = c_ruler_h + i * c_row_h;
		const QRect line(0, y, width(), c_row_h);

		if (node->isSelected())
		{
			QColor h = highlight;
			h.setAlpha(110);
			p.fillRect(line, h);
		}
		else if (r % 2)
		{
			// Daz's AlternateBase is a strong accent in some themes: derive a
			// subtle stripe from the base color instead.
			const QColor base = pal.color(QPalette::Base);
			p.fillRect(line, base.lightness() < 128 ? base.lighter(125) : base.darker(106));
		}

		const DzBoolProperty *prop = wls_anivis_watcher::anivis_property(node);
		const bool active = prop && !prop->isHidden();

		// Name.
		p.setPen(active ? text : dim);
		const QRect name_rect(c_margin + row.depth * c_indent, y, name_width_ - c_margin - row.depth * c_indent, c_row_h);
		p.drawText(name_rect, Qt::AlignVCenter | Qt::AlignLeft,
			p.fontMetrics().elidedText(row.label, Qt::ElideRight, name_rect.width()));

		// Now: dot + word.
		const bool on = node_visible(node);
		p.setRenderHint(QPainter::Antialiasing, true);
		p.setPen(Qt::NoPen);
		p.setBrush(on ? c_visible : c_hidden);
		p.drawEllipse(QRect(name_width_, y + c_row_h / 2 - 5, 10, 10));
		p.setPen(text);
		p.drawText(QRect(name_width_ + 16, y, c_eye_w - 16, c_row_h), Qt::AlignVCenter | Qt::AlignLeft,
			on ? tr("Visible") : tr("Hidden"));

		// Bar.
		const QRect strip(bars.left(), y + 4, bars.width(), c_row_h - 8);
		p.setPen(Qt::NoPen);
		if (!active)
		{
			p.setBrush(QColor(c_empty.red(), c_empty.green(), c_empty.blue(), 80));
			p.drawRoundedRect(strip, 3, 3);
			p.setPen(dim);
			p.drawText(strip, Qt::AlignCenter, prop ? tr("removed") : tr("no AniVis"));
		}
		else if (!wls_anivis_watcher::is_animated(prop))
		{
			QColor c = wls_anivis_watcher::value_at(prop, start) ? c_visible : c_hidden;
			c.setAlpha(110);
			p.setBrush(c);
			p.drawRoundedRect(strip, 3, 3);
			p.setPen(text);
			p.drawText(strip, Qt::AlignCenter, tr("not animated"));
		}
		else
		{
			// Each key holds until the next one.
			QPainterPath clip;
			clip.addRoundedRect(strip, 3, 3);
			p.save();
			p.setClipPath(clip);
			const int count = prop->getNumKeys();
			DzTime seg_start = start;
			bool seg_value = prop->getKeyBoolValue(0);
			for (int k = 0; k <= count; ++k)
			{
				const DzTime seg_end = (k < count) ? prop->getKeyTime(k) : end + step;
				if (seg_end > seg_start)
				{
					const double x0 = x_at(seg_start), x1 = x_at(seg_end);
					p.fillRect(QRectF(x0, strip.top(), x1 - x0, strip.height()), seg_value ? c_visible : c_hidden);
				}
				if (k < count)
				{
					seg_start = qMax(seg_start, prop->getKeyTime(k));
					seg_value = prop->getKeyBoolValue(k);
				}
			}
			p.restore();

			p.setBrush(c_key);
			p.setPen(QPen(QColor(40, 40, 40), 1));
			const double cy = strip.center().y() + 0.5;
			for (int k = 0; k < count; ++k)
			{
				const DzTime t = prop->getKeyTime(k);
				if (t < start || t > end)
					continue;
				const double cx = x_at(t) + 1;
				QPolygonF diamond;
				diamond << QPointF(cx, cy - 5) << QPointF(cx + 4, cy) << QPointF(cx, cy + 5) << QPointF(cx - 4, cy);
				p.drawPolygon(diamond);
			}
		}
		p.setRenderHint(QPainter::Antialiasing, false);
	}

	// Current frame: line over ruler and rows, plus a marker in the ruler.
	const DzTime now = dzScene->getTime();
	const double nx = x_at(now) + 0.5 * px_per_frame;
	p.setPen(QPen(c_now, 2));
	p.drawLine(QPointF(nx, 0), QPointF(nx, height()));
	p.setRenderHint(QPainter::Antialiasing, true);
	p.setPen(Qt::NoPen);
	p.setBrush(c_now);
	QPolygonF marker;
	marker << QPointF(nx - 5, 0) << QPointF(nx + 5, 0) << QPointF(nx, 7);
	p.drawPolygon(marker);
}

void DzAniVisTimelineView::mousePressEvent(QMouseEvent *e)
{
	if (e->button() != Qt::LeftButton || !scene_usable())
		return;
	int row = -1;
	const QPoint pos = e->position().toPoint();
	switch (area_at(pos, &row))
	{
	case Ruler:
	case Bars:
		scrubbing_ = true;
		scrub_to(pos.x());
		break;
	case Eye:
		if (DzNode *node = node_at_row(row))
		{
			const bool on = node_visible(node);
			watcher()->key_visibility(node, !on, on);
		}
		break;
	case Name:
		if (DzNode *node = node_at_row(row))
		{
			if (e->modifiers() & (Qt::ControlModifier | Qt::ShiftModifier))
				node->select(!node->isSelected());
			else
			{
				dzScene->selectAllNodes(false);
				node->select(true);
			}
		}
		break;
	default:
		break;
	}
}

void DzAniVisTimelineView::mouseMoveEvent(QMouseEvent *e)
{
	if (scrubbing_)
		scrub_to(e->position().toPoint().x());
}

void DzAniVisTimelineView::mouseReleaseEvent(QMouseEvent *)
{
	scrubbing_ = false;
}

void DzAniVisTimelineView::wheelEvent(QWheelEvent *e)
{
	const int steps = e->angleDelta().y() / 120;
	if (steps)
		emit scroll_requested(-steps * 3);
	e->accept();
}

// ---------------------------------------------------------------------------
// pane

DzAniVisPane::DzAniVisPane() :
	DzPane(tr("AniVis Timeline"))
{
	setObjectName("AniVisTimelinePane");

	auto layout = new QVBoxLayout();
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	auto buttons = new QHBoxLayout();
	buttons->setSpacing(4);
	auto make_button = [&](const QString &text, const QString &tip) {
		auto b = new QPushButton(text);
		b->setToolTip(tip);
		buttons->addWidget(b);
		return b;
	};
	QPushButton *key_on = make_button(tr("Key Visible"),
		tr("Make the selected items visible from the current frame on"));
	QPushButton *key_off = make_button(tr("Key Hidden"),
		tr("Make the selected items hidden from the current frame on"));
	QPushButton *clear = make_button(tr("Clear Keys"),
		tr("Delete the visibility keys of the selected items and make them visible"));
	QPushButton *remove = make_button(tr("Remove"),
		tr("Remove Animate Visibility from the selected items; toggling their "
		   "visibility will not add it back"));
	buttons->addStretch(1);
	show_all_ = new QCheckBox(tr("All items"));
	show_all_->setToolTip(tr("Also list figures, props and lights without Animate Visibility"));
	buttons->addWidget(show_all_);
	layout->addLayout(buttons);

	auto body = new QHBoxLayout();
	body->setSpacing(0);
	view_ = new DzAniVisTimelineView();
	view_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	scroll_ = new QScrollBar(Qt::Vertical);
	body->addWidget(view_, 1);
	body->addWidget(scroll_);
	layout->addLayout(body, 1);

	summary_ = new QLabel();
	summary_->setWordWrap(true);
	summary_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	layout->addWidget(summary_);

	setLayout(layout);
	setMinimumSize(320, 160);
	resize(760, 260);

	// Widgets owned by this pane: these die with it.
	connect(key_on, &QPushButton::clicked, this, [this]() {
		if (!alive()) return;
		for (DzNode *n : target_nodes())
			if (wls_anivis_watcher::is_candidate(n)) watcher()->key_visibility(n, true);
	});
	connect(key_off, &QPushButton::clicked, this, [this]() {
		if (!alive()) return;
		for (DzNode *n : target_nodes())
			if (wls_anivis_watcher::is_candidate(n)) watcher()->key_visibility(n, false);
	});
	connect(clear, &QPushButton::clicked, this, [this]() {
		if (!alive()) return;
		for (DzNode *n : target_nodes()) watcher()->clear_keys(n);
	});
	connect(remove, &QPushButton::clicked, this, [this]() {
		if (!alive()) return;
		for (DzNode *n : target_nodes()) watcher()->remove_property(n);
	});
	connect(show_all_, &QCheckBox::toggled, this, &DzAniVisPane::schedule_rebuild);
	connect(scroll_, &QScrollBar::valueChanged, view_, &DzAniVisTimelineView::set_scroll);
	connect(view_, &DzAniVisTimelineView::scroll_requested, this, [this](int delta) {
		scroll_->setValue(scroll_->value() + delta);
	});

	connect_scene();
}

void DzAniVisPane::connect_scene()
{
	if (closing_ || !dzApp || dzApp->isClosing())
		return;
	// Daz can restore this pane from the layout before the scene exists.
	if (!dzScene)
	{
		QTimer::singleShot(250, this, &DzAniVisPane::connect_scene);
		return;
	}
	if (!connections_.isEmpty())
		return;

	// Outside objects: kept so the destructor can cut them all.
	connections_ << connect(watcher(), &wls_anivis_watcher::anivis_changed, this, &DzAniVisPane::schedule_rebuild);
	{
		connections_
			<< connect(dzScene, &DzScene::nodeListChanged, this, &DzAniVisPane::schedule_rebuild)
			<< connect(dzScene, &DzScene::sceneLoaded, this, &DzAniVisPane::schedule_rebuild)
			<< connect(dzScene, &DzScene::sceneCleared, this, &DzAniVisPane::schedule_rebuild)
			<< connect(dzScene, &DzScene::timeChanged, this, &DzAniVisPane::repaint_view)
			<< connect(dzScene, &DzScene::animRangeChanged, this, &DzAniVisPane::repaint_view)
			<< connect(dzScene, &DzScene::nodeSelectionListChanged, this, &DzAniVisPane::repaint_view)
			<< connect(dzScene, &QObject::destroyed, this, [this]() {
				// The scene is gone (Daz is closing): stop everything.
				closing_ = true;
				view_->set_rows(QList<AniVisRow>());
			});
	}
	schedule_rebuild();
}

void DzAniVisPane::resizeEvent(QResizeEvent *e)
{
	DzPane::resizeEvent(e);
	update_scrollbar();
}

DzAniVisPane::~DzAniVisPane()
{
	closing_ = true;
	for (const QMetaObject::Connection &c : connections_)
		QObject::disconnect(c);
	for (const QMetaObject::Connection &c : row_connections_)
		QObject::disconnect(c);
	connections_.clear();
	row_connections_.clear();
}

bool DzAniVisPane::alive() const
{
	return !closing_ && !connections_.isEmpty() && scene_usable();
}

void DzAniVisPane::refresh()
{
	schedule_rebuild();
}

void DzAniVisPane::showEvent(QShowEvent *e)
{
	if (dirty_)
		schedule_rebuild();
	DzPane::showEvent(e);
}

void DzAniVisPane::schedule_rebuild()
{
	dirty_ = true;
	if (!alive() || rebuild_scheduled_ || !isVisible())
		return;
	rebuild_scheduled_ = true;
	QTimer::singleShot(0, this, &DzAniVisPane::rebuild);
}

void DzAniVisPane::repaint_view()
{
	if (alive() && isVisible())
		view_->update();
}

void DzAniVisPane::rebuild()
{
	rebuild_scheduled_ = false;
	if (!alive())
		return;
	if (dzScene->isLoading())
	{
		rebuild_scheduled_ = true;
		QTimer::singleShot(250, this, &DzAniVisPane::rebuild);
		return;
	}
	dirty_ = false;

	for (const QMetaObject::Connection &c : row_connections_)
		QObject::disconnect(c);
	row_connections_.clear();

	// Which items to list, in scene order.
	const bool all = show_all_->isChecked();
	QList<DzNode*> listed;
	QHash<DzNode*, bool> is_listed;
	int with_property = 0, animated = 0;
	DzNodeListIterator nli = dzScene->nodeListIterator();
	while (nli.hasNext())
	{
		DzNode *node = nli.next();
		if (!wls_anivis_watcher::is_candidate(node))
			continue;
		const DzBoolProperty *prop = wls_anivis_watcher::active_property(node);
		if (prop)
		{
			++with_property;
			if (wls_anivis_watcher::is_animated(prop))
				++animated;
		}
		if (prop || all)
		{
			listed.append(node);
			is_listed.insert(node, true);
		}
	}

	// Nest each item under its nearest listed ancestor, like the Scene pane.
	QHash<DzNode*, QList<DzNode*>> children;
	QList<DzNode*> roots;
	for (DzNode *node : listed)
	{
		DzNode *parent = node->getNodeParent();
		while (parent && !is_listed.contains(parent))
			parent = parent->getNodeParent();
		if (parent)
			children[parent].append(node);
		else
			roots.append(node);
	}
	QList<AniVisRow> rows;
	std::function<void(DzNode*, int)> add = [&](DzNode *node, int depth) {
		AniVisRow row;
		row.node = node;
		row.label = node->getLabel();
		row.depth = depth;
		rows.append(row);
		for (DzNode *child : children.value(node))
			add(child, depth + 1);
	};
	for (DzNode *root : roots)
		add(root, 0);

	// Repaint when an item's eye icon changes.
	for (DzNode *node : listed)
		if (DzBoolProperty *vis = node->getVisibilityControl())
			row_connections_ << connect(vis, &DzProperty::currentValueChanged, this, &DzAniVisPane::repaint_view);

	view_->set_rows(rows);
	update_scrollbar();

	if (with_property == 0)
		summary_->setText(tr("Toggle an item's eye icon in the Scene pane to key its visibility at the current frame."));
	else
		summary_->setText(tr("%1 item(s) with Animate Visibility, %2 animated. Click or drag the bars "
			"to move the timeline; click Now to toggle at the current frame.")
			.arg(with_property).arg(animated));
}

void DzAniVisPane::update_scrollbar()
{
	const int visible = view_->visible_rows();
	scroll_->setRange(0, qMax(0, view_->row_count() - visible));
	scroll_->setPageStep(visible);
	scroll_->setVisible(view_->row_count() > visible);
}

QList<DzNode*> DzAniVisPane::target_nodes() const
{
	return dzScene->getSelectedNodeList();
}
