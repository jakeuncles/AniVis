/**********************************************************************
	AniVis for Daz Studio 6 - "AniVis" page in the Scene pane.

	This source code and software provided under the terms of the
	Mozilla Public License Version 2.0 (MPL) a copy of which is
	provided with it.
**********************************************************************/

#include "anivispane.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QVBoxLayout>

#include "dzapp.h"
#include "dzboolproperty.h"
#include "dznode.h"
#include "dzscene.h"
#include "dzsettings.h"
#include "dztimerange.h"

#include "anivisguids.h"
#include "wlsAnivisWatcher.h"

namespace {

const QColor c_visible(76, 175, 80);      // green
const QColor c_hidden(198, 64, 64);       // red
const QColor c_key(255, 213, 79);         // yellow, like the property's slider
const QColor c_now(255, 255, 255);
const QColor c_empty(90, 90, 90);
const int c_role_node = Qt::UserRole + 1;

wls_anivis_watcher *watcher() { return wls_anivis_watcher::instance(); }

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
	end = qMax(range.getEnd(), start + step);
}

} // namespace

// ---------------------------------------------------------------------------
// delegate

DzAniVisDelegate::DzAniVisDelegate(DzAniVisSubPane *pane) :
	QStyledItemDelegate(pane), pane_(pane)
{
}

QSize DzAniVisDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	QSize size = QStyledItemDelegate::sizeHint(option, index);
	size.setHeight(qMax(size.height(), 22));
	if (index.column() == DzAniVisSubPane::NowColumn)
		size.setWidth(qMax(size.width(), 74));
	return size;
}

void DzAniVisDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
	const QModelIndex &index) const
{
	const int column = index.column();
	if (column != DzAniVisSubPane::NowColumn && column != DzAniVisSubPane::TimelineColumn)
	{
		QStyledItemDelegate::paint(painter, option, index);
		return;
	}

	// Selection background, no text.
	QStyleOptionViewItem opt(option);
	initStyleOption(&opt, index);
	opt.text.clear();
	QStyle *style = opt.widget ? opt.widget->style() : nullptr;
	if (style)
		style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, opt.widget);

	DzNode *node = pane_->node_for(index);
	if (!node)
		return;
	const DzBoolProperty *prop = wls_anivis_watcher::anivis_property(node);
	const bool active = prop && !prop->isHidden();

	painter->save();
	painter->setRenderHint(QPainter::Antialiasing, true);

	if (column == DzAniVisSubPane::NowColumn)
	{
		// Eye state right now: a filled dot plus a word, so it reads at a glance.
		const bool on = node_visible(node);
		const QRect r = option.rect.adjusted(6, 0, -2, 0);
		const int d = 10;
		const QRect dot(r.left(), r.center().y() - d / 2, d, d);
		painter->setPen(Qt::NoPen);
		painter->setBrush(on ? c_visible : c_hidden);
		painter->drawEllipse(dot);
		painter->setPen(option.palette.color(QPalette::Text));
		painter->drawText(r.adjusted(d + 6, 0, 0, 0), Qt::AlignVCenter | Qt::AlignLeft,
			on ? DzAniVisSubPane::tr("Visible") : DzAniVisSubPane::tr("Hidden"));
		painter->restore();
		return;
	}

	// Timeline strip.
	const QRect strip = DzAniVisSubPane::strip_rect(option.rect);
	DzTime start, end, step;
	anim_range(start, end, step);
	const DzTime span = end + step - start; // include the last frame
	auto x_at = [&](DzTime t) {
		const double f = double(qBound<DzTime>(start, t, end + step) - start) / double(span);
		return strip.left() + f * strip.width();
	};

	painter->setPen(Qt::NoPen);
	if (!active)
	{
		painter->setBrush(QColor(c_empty.red(), c_empty.green(), c_empty.blue(), 90));
		painter->drawRoundedRect(strip, 3, 3);
		painter->setPen(option.palette.color(QPalette::PlaceholderText));
		painter->drawText(strip, Qt::AlignCenter,
			prop ? DzAniVisSubPane::tr("removed") : DzAniVisSubPane::tr("no AniVis"));
	}
	else if (!wls_anivis_watcher::is_animated(prop))
	{
		// One value for the whole animation: show it, faded.
		QColor c = wls_anivis_watcher::value_at(prop, start) ? c_visible : c_hidden;
		c.setAlpha(110);
		painter->setBrush(c);
		painter->drawRoundedRect(strip, 3, 3);
		painter->setPen(option.palette.color(QPalette::Text));
		painter->drawText(strip, Qt::AlignCenter, DzAniVisSubPane::tr("not animated"));
	}
	else
	{
		// Visible / hidden segments: each key holds until the next one.
		QPainterPath clip;
		clip.addRoundedRect(strip, 3, 3);
		painter->setClipPath(clip);
		const int count = prop->getNumKeys();
		DzTime seg_start = start;
		bool seg_value = prop->getKeyBoolValue(0);
		for (int i = 0; i <= count; ++i)
		{
			const DzTime seg_end = (i < count) ? prop->getKeyTime(i) : end + step;
			if (seg_end > seg_start)
			{
				const double x0 = x_at(seg_start), x1 = x_at(seg_end);
				painter->fillRect(QRectF(x0, strip.top(), x1 - x0, strip.height()),
					seg_value ? c_visible : c_hidden);
			}
			if (i < count)
			{
				seg_start = qMax(seg_start, prop->getKeyTime(i));
				seg_value = prop->getKeyBoolValue(i);
			}
		}
		painter->setClipping(false);

		// Key markers.
		painter->setBrush(c_key);
		painter->setPen(QPen(QColor(40, 40, 40), 1));
		const double cy = strip.center().y() + 0.5;
		for (int i = 0; i < count; ++i)
		{
			const DzTime t = prop->getKeyTime(i);
			if (t < start || t > end)
				continue;
			const double cx = x_at(t) + 1;
			QPolygonF diamond;
			diamond << QPointF(cx, cy - 5) << QPointF(cx + 4, cy)
				<< QPointF(cx, cy + 5) << QPointF(cx - 4, cy);
			painter->drawPolygon(diamond);
		}
	}

	// Current frame.
	const double nx = x_at(dzScene->getTime());
	painter->setPen(QPen(c_now, 2));
	painter->drawLine(QPointF(nx, option.rect.top() + 1), QPointF(nx, option.rect.bottom() - 1));
	painter->restore();
}

// ---------------------------------------------------------------------------
// sub-pane

DzAniVisSubPane::DzAniVisSubPane()
{
	setObjectName("AniVisSubPane");

	auto layout = new QVBoxLayout();
	layout->setContentsMargins(4, 4, 4, 4);
	layout->setSpacing(4);

	// Buttons act on the rows selected here (= the scene selection).
	auto buttons = new QHBoxLayout();
	buttons->setSpacing(4);
	auto add_button = [&](const QString &text, const QString &tip) {
		auto b = new QPushButton(text);
		b->setToolTip(tip);
		buttons->addWidget(b);
		return b;
	};
	QPushButton *key_on = add_button(tr("Key Visible"),
		tr("Make the selected items visible from the current frame on"));
	QPushButton *key_off = add_button(tr("Key Hidden"),
		tr("Make the selected items hidden from the current frame on"));
	QPushButton *clear = add_button(tr("Clear Keys"),
		tr("Delete the visibility keys of the selected items and make them visible"));
	QPushButton *remove = add_button(tr("Remove"),
		tr("Remove Animate Visibility from the selected items; toggling their "
		   "visibility will not add it back"));
	buttons->addStretch(1);
	show_all_ = new QCheckBox(tr("All items"));
	show_all_->setToolTip(tr("Also list figures, props and lights without Animate Visibility"));
	buttons->addWidget(show_all_);
	layout->addLayout(buttons);

	tree_ = new QTreeWidget();
	tree_->setColumnCount(4);
	tree_->setHeaderLabels(QStringList() << tr("Item") << tr("Now") << tr("Keys") << tr("Timeline"));
	tree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
	tree_->setUniformRowHeights(true);
	tree_->setAllColumnsShowFocus(true);
	tree_->setItemDelegate(new DzAniVisDelegate(this));
	tree_->header()->setStretchLastSection(true);
	tree_->header()->setSectionResizeMode(ItemColumn, QHeaderView::Interactive);
	tree_->header()->setSectionResizeMode(NowColumn, QHeaderView::ResizeToContents);
	tree_->header()->setSectionResizeMode(KeysColumn, QHeaderView::ResizeToContents);
	tree_->setColumnWidth(ItemColumn, 170);
	tree_->viewport()->installEventFilter(this);
	tree_->viewport()->setMouseTracking(false);
	layout->addWidget(tree_, 1);

	summary_ = new QLabel();
	summary_->setWordWrap(true);
	summary_->setToolTip(tr(
		"Toggling an item's eye icon in the Scene tab keys it at the current frame.\n"
		"Green = visible, red = hidden, yellow diamond = key, white line = current frame.\n"
		"Click or drag on a timeline to move the timeline there; click Now to toggle at the current frame."));
	layout->addWidget(summary_);

	setLayout(layout);
	setMinimumSize(220, 150);

	connect(key_on, &QPushButton::clicked, this, [this]() { key_selected(true); });
	connect(key_off, &QPushButton::clicked, this, [this]() { key_selected(false); });
	connect(clear, &QPushButton::clicked, this, &DzAniVisSubPane::clear_selected);
	connect(remove, &QPushButton::clicked, this, &DzAniVisSubPane::remove_selected);
	connect(show_all_, &QCheckBox::toggled, this, &DzAniVisSubPane::schedule_rebuild);
	connect(tree_, &QTreeWidget::itemSelectionChanged, this, &DzAniVisSubPane::on_tree_selection_changed);

	connect(watcher(), &wls_anivis_watcher::anivis_changed, this, &DzAniVisSubPane::schedule_rebuild);
	connect(dzScene, &DzScene::nodeListChanged, this, &DzAniVisSubPane::schedule_rebuild);
	connect(dzScene, &DzScene::sceneLoaded, this, &DzAniVisSubPane::schedule_rebuild);
	connect(dzScene, &DzScene::sceneCleared, this, &DzAniVisSubPane::schedule_rebuild);
	connect(dzScene, &DzScene::timeChanged, this, &DzAniVisSubPane::repaint_rows);
	connect(dzScene, &DzScene::animRangeChanged, this, &DzAniVisSubPane::repaint_rows);
	connect(dzScene, &DzScene::nodeSelectionListChanged, this, &DzAniVisSubPane::on_scene_selection_changed);

	schedule_rebuild();
}

DzAniVisSubPane::~DzAniVisSubPane() = default;

QString DzAniVisSubPane::getLabel() const
{
	return tr("AniVis");
}

// Builds "tip pages/DzHierarchyPane/AniVis" in the user's Daz data folder
// from Daz's own installed Outline tip page (copied on this machine, not
// shipped with the plugin), with AniVis tips instead of the Outline ones.
static void ensure_tip_page()
{
	const QString src = dzApp->getResourcesPath() + "/tip pages/DzHierarchyPane";
	const QString dst = dzApp->getAppDataPath() + "/resources/tip pages/DzHierarchyPane";
	const QString page = dst + "/AniVis";
	if (!QFileInfo::exists(src + "/Outline/TipPage.dsa"))
		return;

	if (!QFileInfo::exists(page + "/TipPage.dsa") || !QFileInfo::exists(dst + "/AniVis.dsa"))
	{
		QDir().mkpath(page + "/Tips");
		QFile::copy(src + "/Outline.dsa", dst + "/AniVis.dsa");
		QFile::copy(src + "/Outline/TipPage.dsa", page + "/TipPage.dsa");
		QFile::copy(src + "/Outline/TipPage.ui", page + "/TipPage.ui");
		QFile::copy(src + "/Outline/Tips/Back.dsa", page + "/Tips/Back.dsa");
		QFile::copy(src + "/Outline/Tips/Next.dsa", page + "/Tips/Next.dsa");
	}

	const QStringList tips = QStringList()
		<< "Click an item's eye icon in the Outline tab to key its visibility at the "
		   "current frame. AniVis adds the Animate Visibility property the first time."
		<< "Timeline colors: green = visible, red = hidden, yellow diamond = key, "
		   "white line = current frame. Click or drag on a timeline to move there."
		<< "Click Now to toggle an item at the current frame. Key Visible and Key Hidden "
		   "do the same for every selected row."
		<< "Remove takes AniVis off an item: its eye icon no longer keys it. "
		   "Edit > AniVis > Add Property brings it back.";
	for (int i = 0; i < tips.size(); ++i)
	{
		QFile f(QString("%1/Tips/Tip %2.txt").arg(page).arg(i + 1));
		if (f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
			f.write(tips.at(i).toUtf8());
	}
}

DzSettings *DzAniVisSubPane::getInfoTabsSettings() const
{
	if (!tip_settings_)
	{
		ensure_tip_page();
		auto settings = new DzSettings();
		settings->setStringValue("tipUIScriptPath", QStringLiteral("AniVis"));
		settings->setParent(const_cast<DzAniVisSubPane*>(this));
		tip_settings_ = settings;
	}
	return tip_settings_;
}

QRect DzAniVisSubPane::strip_rect(const QRect &cell)
{
	return cell.adjusted(6, 4, -8, -4);
}

DzNode *DzAniVisSubPane::node_for(const QModelIndex &index) const
{
	const quintptr key = index.sibling(index.row(), ItemColumn).data(c_role_node).value<quintptr>();
	return nodes_.value(key).data();
}

void DzAniVisSubPane::refresh()
{
	schedule_rebuild();
}

void DzAniVisSubPane::showEvent(QShowEvent *e)
{
	if (dirty_)
		schedule_rebuild();
	DzSubPane::showEvent(e);
}

void DzAniVisSubPane::schedule_rebuild()
{
	dirty_ = true;
	if (rebuild_scheduled_ || !isVisible() || dzApp->isClosing())
		return;
	rebuild_scheduled_ = true;
	QTimer::singleShot(0, this, &DzAniVisSubPane::rebuild);
}

void DzAniVisSubPane::repaint_rows()
{
	if (isVisible())
		tree_->viewport()->update();
}

QTreeWidgetItem *DzAniVisSubPane::item_for(DzNode *node, QHash<DzNode*, QTreeWidgetItem*> &items,
	const QHash<DzNode*, bool> &listed)
{
	if (QTreeWidgetItem *existing = items.value(node))
		return existing;

	// Nest under the nearest listed ancestor, like the Scene tab.
	DzNode *parent = node->getNodeParent();
	while (parent && !listed.contains(parent))
		parent = parent->getNodeParent();

	QTreeWidgetItem *parent_item = parent ? item_for(parent, items, listed) : nullptr;
	auto item = parent_item ? new QTreeWidgetItem(parent_item) : new QTreeWidgetItem(tree_);
	items.insert(node, item);

	const quintptr key = reinterpret_cast<quintptr>(node);
	nodes_.insert(key, node);
	item->setData(ItemColumn, c_role_node, QVariant::fromValue(key));
	item->setText(ItemColumn, node->getLabel());

	const DzBoolProperty *prop = wls_anivis_watcher::anivis_property(node);
	const bool active = prop && !prop->isHidden();
	if (active && wls_anivis_watcher::is_animated(prop))
		item->setText(KeysColumn, QString::number(prop->getNumKeys()));
	else
		item->setText(KeysColumn, QStringLiteral("-"));
	if (!active)
	{
		const QColor dim = tree_->palette().color(QPalette::PlaceholderText);
		item->setForeground(ItemColumn, dim);
		item->setForeground(KeysColumn, dim);
	}
	item->setToolTip(TimelineColumn, tr("Click or drag to move the timeline"));
	item->setToolTip(NowColumn, tr("Click to toggle visibility at the current frame"));

	// Repaint when the eye icon changes without AniVis keying it.
	if (DzBoolProperty *vis = node->getVisibilityControl())
		connect(vis, &DzProperty::currentValueChanged, this,
			&DzAniVisSubPane::repaint_rows, Qt::UniqueConnection);
	return item;
}

void DzAniVisSubPane::rebuild()
{
	rebuild_scheduled_ = false;
	if (dzScene->isLoading())
	{
		// Wait for the file to finish loading.
		rebuild_scheduled_ = true;
		QTimer::singleShot(250, this, &DzAniVisSubPane::rebuild);
		return;
	}
	dirty_ = false;

	const bool all = show_all_->isChecked();
	QList<DzNode*> order;
	QHash<DzNode*, bool> listed;
	int animated = 0, with_property = 0;
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
			order.append(node);
			listed.insert(node, true);
		}
	}

	syncing_selection_ = true;
	tree_->clear();
	nodes_.clear();
	QHash<DzNode*, QTreeWidgetItem*> items;
	for (DzNode *node : order)
		item_for(node, items, listed);
	tree_->expandAll();
	for (auto it = items.constBegin(); it != items.constEnd(); ++it)
		if (it.key()->isSelected())
			it.value()->setSelected(true);
	syncing_selection_ = false;

	if (with_property == 0)
		summary_->setText(tr("No items use AniVis yet. Toggle an item's eye icon in the "
			"Scene tab to key it at the current frame."));
	else
		summary_->setText(tr("%1 item(s) with Animate Visibility, %2 animated. "
			"Click a timeline to jump there; click Now to toggle.")
			.arg(with_property).arg(animated));
}

QList<DzNode*> DzAniVisSubPane::target_nodes() const
{
	QList<DzNode*> nodes;
	for (QTreeWidgetItem *item : tree_->selectedItems())
		if (DzNode *node = nodes_.value(item->data(ItemColumn, c_role_node).value<quintptr>()))
			nodes.append(node);
	if (nodes.isEmpty())
		nodes = dzScene->getSelectedNodeList();
	return nodes;
}

void DzAniVisSubPane::jump_to(const QModelIndex &index, int x)
{
	const QRect strip = strip_rect(tree_->visualRect(index));
	if (strip.width() <= 0)
		return;
	DzTime start, end, step;
	anim_range(start, end, step);
	const int frames = int((end - start) / step) + 1;
	const double f = qBound(0.0, double(x - strip.left()) / strip.width(), 0.999999);
	dzScene->setTime(start + DzTime(int(f * frames)) * step);
}

bool DzAniVisSubPane::eventFilter(QObject *watched, QEvent *event)
{
	if (watched != tree_->viewport())
		return DzSubPane::eventFilter(watched, event);

	const QEvent::Type type = event->type();
	if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick)
	{
		auto me = static_cast<QMouseEvent*>(event);
		if (me->button() != Qt::LeftButton)
			return false;
		const QPoint pos = me->position().toPoint();
		const QModelIndex index = tree_->indexAt(pos);
		if (!index.isValid())
			return false;
		if (index.column() == TimelineColumn)
		{
			scrubbing_ = true;
			jump_to(index, pos.x());
			return true;
		}
		if (index.column() == NowColumn && type == QEvent::MouseButtonPress)
		{
			if (DzNode *node = node_for(index))
			{
				const bool on = node_visible(node);
				watcher()->key_visibility(node, !on, on);
			}
			return true;
		}
	}
	else if (type == QEvent::MouseMove && scrubbing_)
	{
		auto me = static_cast<QMouseEvent*>(event);
		const QPoint pos = me->position().toPoint();
		QModelIndex index = tree_->indexAt(QPoint(pos.x(), pos.y()));
		if (!index.isValid() || index.column() != TimelineColumn)
			index = tree_->indexAt(QPoint(tree_->header()->sectionPosition(TimelineColumn) + 1, pos.y()));
		if (index.isValid())
			jump_to(index.sibling(index.row(), TimelineColumn), pos.x());
		return true;
	}
	else if (type == QEvent::MouseButtonRelease && scrubbing_)
	{
		scrubbing_ = false;
		return true;
	}
	return false;
}

void DzAniVisSubPane::on_tree_selection_changed()
{
	if (syncing_selection_)
		return;
	syncing_selection_ = true;
	dzScene->selectAllNodes(false);
	for (QTreeWidgetItem *item : tree_->selectedItems())
		if (DzNode *node = nodes_.value(item->data(ItemColumn, c_role_node).value<quintptr>()))
			node->select(true);
	syncing_selection_ = false;
}

void DzAniVisSubPane::on_scene_selection_changed()
{
	if (syncing_selection_ || !isVisible())
		return;
	syncing_selection_ = true;
	QTreeWidgetItemIterator it(tree_);
	while (*it)
	{
		DzNode *node = nodes_.value((*it)->data(ItemColumn, c_role_node).value<quintptr>());
		(*it)->setSelected(node && node->isSelected());
		++it;
	}
	syncing_selection_ = false;
}

void DzAniVisSubPane::key_selected(bool visible)
{
	for (DzNode *node : target_nodes())
		if (wls_anivis_watcher::is_candidate(node))
			watcher()->key_visibility(node, visible);
}

void DzAniVisSubPane::clear_selected()
{
	for (DzNode *node : target_nodes())
		watcher()->clear_keys(node);
}

void DzAniVisSubPane::remove_selected()
{
	for (DzNode *node : target_nodes())
		watcher()->remove_property(node);
}

// ---------------------------------------------------------------------------

DzAniVisSubPaneAction::DzAniVisSubPaneAction() :
	DzSubPaneAction("DzHierarchyPane", ANIVIS_SUBPANE_GUID_STRING)
{
	setObjectName(DzAniVisSubPaneAction::metaObject()->className());
}
