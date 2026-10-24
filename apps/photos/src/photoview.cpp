#include "photoview.h"
#include "library.h"
#include "starred.h"

#include <QDateTime>
#include <QLocale>
#include <cmath>

PhotoView::PhotoView(QObject *parent)
    : QAbstractListModel(parent)
    , m_lib(PhotoLibrary::instance())
{
    m_filterTimer.setSingleShot(true);
    m_filterTimer.setInterval(0);
    connect(&m_filterTimer, &QTimer::timeout, this, &PhotoView::refilter);
    m_layoutTimer.setSingleShot(true);
    m_layoutTimer.setInterval(0);
    connect(&m_layoutTimer, &QTimer::timeout, this, &PhotoView::relayout);
    connect(m_lib, &PhotoLibrary::changed, this, &PhotoView::scheduleFilter);
    connect(m_lib, &PhotoLibrary::textChanged, this, [this] {
        // OCR finishes often; only a search can change with it.
        if (!m_query.isEmpty() && !m_filterTimer.isActive()) {
            m_filterTimer.setInterval(400);
            m_filterTimer.start();
        }
    });
    connect(m_lib, &PhotoLibrary::favouritesChanged, this, [this] {
        if (m_filter == QLatin1String("favourites"))
            scheduleFilter();
        else
            refreshRows();
    });
    scheduleFilter();
}

int PhotoView::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

QHash<int, QByteArray> PhotoView::roleNames() const
{
    return {{KindRole, "kind"}, {TitleRole, "title"}, {SubtitleRole, "subtitle"}, {KeyRole, "key"}, {YRole, "rowY"}, {HeightRole, "rowHeight"}, {ItemsRole, "items"}};
}

QVariantMap PhotoView::tileMap(const Tile &t) const
{
    const Photo &p = m_lib->photos()[m_items[t.i]];
    return {{QStringLiteral("i"), t.i},
            {QStringLiteral("x"), t.x},
            {QStringLiteral("w"), t.w},
            {QStringLiteral("path"), p.path},
            {QStringLiteral("thumb"), PhotoLibrary::thumb(p.path, p.mtime)},
            {QStringLiteral("video"), p.video},
            {QStringLiteral("favourite"), m_lib->isFavourite(p.path)},
            {QStringLiteral("selected"), m_selected.contains(p.path)}};
}

QVariant PhotoView::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    switch (role) {
    case KindRole: return r.header ? QStringLiteral("header") : QStringLiteral("photos");
    case TitleRole: return r.title;
    case SubtitleRole: return r.subtitle;
    case KeyRole: return r.key;
    case YRole: return r.y;
    case HeightRole: return r.height;
    case ItemsRole: {
        QVariantList list;
        list.reserve(r.tiles.size());
        for (const Tile &t : r.tiles)
            list << tileMap(t);
        return list;
    }
    }
    return {};
}

void PhotoView::setFilter(const QString &f)
{
    if (m_filter == f)
        return;
    m_filter = f;
    emit filterChanged();
    scheduleFilter();
}

void PhotoView::setAlbum(const QString &a)
{
    if (m_album == a)
        return;
    m_album = a;
    emit filterChanged();
    scheduleFilter();
}

void PhotoView::setQuery(const QString &q)
{
    if (m_query == q)
        return;
    m_query = q;
    emit filterChanged();
    scheduleFilter();
}

void PhotoView::setGroup(const QString &g)
{
    if (m_group == g)
        return;
    m_group = g;
    emit layoutParamsChanged();
    m_layoutTimer.start();
}

void PhotoView::setWidth(qreal w)
{
    if (qFuzzyCompare(m_width, w))
        return;
    m_width = w;
    emit layoutParamsChanged();
    m_layoutTimer.start();
}

void PhotoView::setRowHeight(qreal h)
{
    if (qFuzzyCompare(m_rowHeight, h) || h < 16)
        return;
    m_rowHeight = h;
    emit layoutParamsChanged();
    m_layoutTimer.start();
}

void PhotoView::setGap(qreal g)
{
    if (qFuzzyCompare(m_gap + 1, g + 1))
        return;
    m_gap = g;
    emit layoutParamsChanged();
    m_layoutTimer.start();
}

void PhotoView::setHeaderHeight(qreal h)
{
    if (qFuzzyCompare(m_headerHeight + 1, h + 1))
        return;
    m_headerHeight = h;
    emit layoutParamsChanged();
    m_layoutTimer.start();
}

void PhotoView::scheduleFilter()
{
    m_filterTimer.setInterval(0);
    m_filterTimer.start();
}

void PhotoView::refilter()
{
    const QList<Photo> &all = m_lib->photos();
    const QStringList terms = m_query.toLower().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QList<int> items;
    items.reserve(all.size());
    for (int i = 0; i < all.size(); ++i) {
        const Photo &p = all[i];
        if (m_filter == QLatin1String("favourites") && !m_lib->isFavourite(p.path))
            continue;
        if (m_filter == QLatin1String("screenshots") && !p.screenshot)
            continue;
        if (m_filter == QLatin1String("videos") && !p.video)
            continue;
        if (m_filter == QLatin1String("album") && p.folder != m_album)
            continue;
        if (!terms.isEmpty()) {
            const QString &hay = m_lib->haystack(i);
            bool all = true;
            for (const QString &t : terms)
                if (!hay.contains(t)) {
                    all = false;
                    break;
                }
            if (!all)
                continue;
        }
        items << i;
    }
    m_items = items;
    m_paths.clear();
    m_viewIndex.clear();
    m_paths.reserve(items.size());
    for (int i = 0; i < items.size(); ++i) {
        m_paths << all[items[i]].path;
        m_viewIndex.insert(m_paths.last(), i);
    }
    // A photo that left the view also leaves the selection.
    const int before = m_selected.size();
    for (auto it = m_selected.begin(); it != m_selected.end();)
        it = m_viewIndex.contains(*it) ? std::next(it) : m_selected.erase(it);
    if (before != m_selected.size())
        emit selectionChanged();
    relayout();
}

QString PhotoView::keyFor(qint64 taken) const
{
    const QDate d = QDateTime::fromMSecsSinceEpoch(taken).date();
    if (m_group == QLatin1String("year"))
        return QString::number(d.year());
    if (m_group == QLatin1String("month"))
        return d.toString(QStringLiteral("yyyy-MM"));
    if (m_group == QLatin1String("none"))
        return QStringLiteral("all");
    return d.toString(Qt::ISODate);
}

QString PhotoView::titleFor(const QString &key, qint64 taken) const
{
    const QDate d = QDateTime::fromMSecsSinceEpoch(taken).date(), today = QDate::currentDate();
    const QLocale l;
    if (m_group == QLatin1String("year"))
        return key;
    if (m_group == QLatin1String("month"))
        return d.year() == today.year() ? l.standaloneMonthName(d.month()) : l.standaloneMonthName(d.month()) + QLatin1Char(' ') + QString::number(d.year());
    if (m_group == QLatin1String("none"))
        return {};
    if (d == today)
        return tr("Today");
    if (d == today.addDays(-1))
        return tr("Yesterday");
    if (d.daysTo(today) < 7)
        return l.dayName(d.dayOfWeek());
    return l.toString(d, d.year() == today.year() ? QStringLiteral("ddd d MMM") : QStringLiteral("d MMM yyyy"));
}

void PhotoView::layoutGroup(const QList<int> &members, qreal rowHeight, QList<Row> &out, qreal &y, const QString &key) const
{
    const QList<Photo> &all = m_lib->photos();
    const qreal width = std::max<qreal>(m_width, 64);
    auto aspectOf = [&](int viewIndex) { return std::clamp(all[m_items[viewIndex]].aspect(), 0.25, 5.0); };
    auto emitRow = [&](const QList<int> &row, qreal h, bool stretch) {
        Row r;
        r.key = key;
        r.y = y;
        qreal x = 0;
        for (int k = 0; k < row.size(); ++k) {
            qreal w = h * aspectOf(row[k]);
            // The last tile absorbs rounding so a full row ends exactly at the edge.
            if (stretch && k == row.size() - 1)
                w = width - x;
            else if (stretch)
                w = std::round(w);
            r.tiles.append({row[k], x, std::min(w, width)});
            x += w + m_gap;
        }
        r.height = std::round(h) + m_gap;
        y += r.height;
        out.append(r);
    };
    QList<int> row;
    qreal sum = 0;
    for (int v : members) {
        const qreal a = aspectOf(v);
        const qreal availWith = width - m_gap * row.size();
        if ((sum + a) * rowHeight >= availWith) {
            const qreal hWith = availWith / (sum + a);
            const qreal hWithout = row.isEmpty() ? 0 : (width - m_gap * (row.size() - 1)) / sum;
            // Break where the row height stays closest to the target, and never let a row grow past 1.5 times.
            if (!row.isEmpty() && hWithout <= rowHeight * 1.5 && std::abs(std::log(hWithout / rowHeight)) < std::abs(std::log(hWith / rowHeight))) {
                emitRow(row, hWithout, true);
                row = {v};
                sum = a;
                continue;
            }
            row << v;
            emitRow(row, std::max(hWith, rowHeight * 0.4), true);
            row.clear();
            sum = 0;
            continue;
        }
        row << v;
        sum += a;
    }
    if (!row.isEmpty())
        emitRow(row, rowHeight, false);
}

void PhotoView::relayout()
{
    m_layoutTimer.stop();
    const QList<Photo> &all = m_lib->photos();
    QList<Row> rows;
    QList<int> rowOfItem(m_items.size(), -1);
    QList<QString> groupOfItem(m_items.size());
    QHash<QString, QList<int>> groupItems;
    QVariantList marks;
    qreal y = 0;
    const qreal scale = m_group == QLatin1String("year") ? 0.5 : m_group == QLatin1String("month") ? 0.72 : 1.0;
    const qreal rowHeight = std::max<qreal>(24, m_rowHeight * scale);
    QString lastMonth;
    int start = 0;
    while (start < m_items.size()) {
        const qint64 taken = all[m_items[start]].taken;
        const QString key = keyFor(taken);
        int end = start;
        int videos = 0;
        while (end < m_items.size() && keyFor(all[m_items[end]].taken) == key) {
            videos += all[m_items[end]].video;
            ++end;
        }
        QList<int> members;
        for (int v = start; v < end; ++v)
            members << v;
        groupItems.insert(key, members);
        const QDate d = QDateTime::fromMSecsSinceEpoch(taken).date();
        const QString month = d.toString(QStringLiteral("yyyy-MM"));
        if (month != lastMonth && m_group != QLatin1String("none")) {
            const bool newYear = lastMonth.isEmpty() ? d.year() != QDate::currentDate().year() : lastMonth.left(4) != month.left(4);
            marks << QVariantMap{{QStringLiteral("label"), newYear ? QString::number(d.year()) : QLocale().standaloneMonthName(d.month(), QLocale::ShortFormat)},
                                 {QStringLiteral("y"), y}, {QStringLiteral("row"), int(rows.size())}, {QStringLiteral("year"), newYear},
                                 {QStringLiteral("month"), QLocale().toString(d, QStringLiteral("MMM yyyy"))}};
            lastMonth = month;
        }
        if (m_group != QLatin1String("none")) {
            Row h;
            h.header = true;
            h.key = key;
            h.title = titleFor(key, taken);
            const int photos = members.size() - videos;
            QStringList parts;
            if (photos)
                parts << tr("%n photo(s)", "", photos);
            if (videos)
                parts << tr("%n video(s)", "", videos);
            h.subtitle = parts.join(QStringLiteral(" · "));
            h.y = y;
            h.height = m_headerHeight;
            y += h.height;
            rows.append(h);
        }
        const int firstRow = rows.size();
        layoutGroup(members, rowHeight, rows, y, key);
        for (int r = firstRow; r < rows.size(); ++r)
            for (const Tile &t : rows[r].tiles) {
                rowOfItem[t.i] = r;
                groupOfItem[t.i] = key;
            }
        start = end;
    }
    // Keep the row count stable where possible: update rows in place, then add or remove only at the end.
    const int oldCount = m_rows.size(), newCount = rows.size();
    if (newCount < oldCount) {
        beginRemoveRows({}, newCount, oldCount - 1);
        m_rows = rows;
        endRemoveRows();
    } else if (newCount > oldCount) {
        beginInsertRows({}, oldCount, newCount - 1);
        m_rows = rows;
        endInsertRows();
    } else {
        m_rows = rows;
    }
    if (std::min(oldCount, newCount) > 0)
        emit dataChanged(index(0), index(std::min(oldCount, newCount) - 1));
    m_rowOfItem = rowOfItem;
    m_groupOfItem = groupOfItem;
    m_groupItems = groupItems;
    m_contentHeight = y;
    m_marks = marks;
    emit laidOut();
}

void PhotoView::refreshRows()
{
    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(int(m_rows.size()) - 1), {ItemsRole});
}

QString PhotoView::path(int i) const
{
    return i >= 0 && i < m_paths.size() ? m_paths[i] : QString();
}

QVariantMap PhotoView::photo(int i) const
{
    return m_lib->photo(path(i));
}

int PhotoView::indexOfPath(const QString &path) const
{
    return m_viewIndex.value(path, -1);
}

int PhotoView::rowOf(int i) const
{
    return i >= 0 && i < m_rowOfItem.size() ? m_rowOfItem[i] : -1;
}

QString PhotoView::groupOf(int i) const
{
    return i >= 0 && i < m_groupOfItem.size() ? m_groupOfItem[i] : QString();
}

int PhotoView::neighbour(int i, const QString &direction) const
{
    if (m_items.isEmpty())
        return -1;
    if (i < 0 || i >= m_items.size())
        return 0;
    if (direction == QLatin1String("left"))
        return std::max(0, i - 1);
    if (direction == QLatin1String("right"))
        return std::min(int(m_items.size()) - 1, i + 1);
    const int row = rowOf(i);
    if (row < 0)
        return i;
    qreal centre = 0;
    for (const Tile &t : m_rows[row].tiles)
        if (t.i == i)
            centre = t.x + t.w / 2;
    const int step = direction == QLatin1String("up") ? -1 : 1;
    for (int r = row + step; r >= 0 && r < m_rows.size(); r += step) {
        if (m_rows[r].header)
            continue;
        // The tile under the same horizontal position, as the eye expects.
        int best = -1;
        qreal bestDist = 1e9;
        for (const Tile &t : m_rows[r].tiles) {
            const qreal dist = centre < t.x ? t.x - centre : centre > t.x + t.w ? centre - t.x - t.w : 0;
            if (dist < bestDist) {
                bestDist = dist;
                best = t.i;
            }
        }
        return best;
    }
    return i;
}

void PhotoView::select(int i, bool on)
{
    const QString p = path(i);
    if (p.isEmpty() || m_selected.contains(p) == on)
        return;
    if (on)
        m_selected.insert(p);
    else
        m_selected.remove(p);
    const int r = rowOf(i);
    if (r >= 0)
        emit dataChanged(index(r), index(r), {ItemsRole});
    emit selectionChanged();
}

void PhotoView::toggle(int i)
{
    select(i, !isSelected(i));
}

void PhotoView::selectRange(int from, int to)
{
    if (from > to)
        std::swap(from, to);
    for (int i = std::max(0, from); i <= to && i < m_paths.size(); ++i)
        m_selected.insert(m_paths[i]);
    refreshRows();
    emit selectionChanged();
}

bool PhotoView::groupSelected(const QString &key) const
{
    const QList<int> members = m_groupItems.value(key);
    if (members.isEmpty())
        return false;
    for (int i : members)
        if (!m_selected.contains(m_paths[i]))
            return false;
    return true;
}

void PhotoView::toggleGroup(const QString &key)
{
    const bool on = !groupSelected(key);
    for (int i : m_groupItems.value(key)) {
        if (on)
            m_selected.insert(m_paths[i]);
        else
            m_selected.remove(m_paths[i]);
    }
    refreshRows();
    emit selectionChanged();
}

void PhotoView::selectAll()
{
    m_selected = QSet<QString>(m_paths.cbegin(), m_paths.cend());
    refreshRows();
    emit selectionChanged();
}

void PhotoView::clearSelection()
{
    if (m_selected.isEmpty())
        return;
    m_selected.clear();
    refreshRows();
    emit selectionChanged();
}

QStringList PhotoView::selectedPaths() const
{
    QStringList out;
    for (const QString &p : m_paths)
        if (m_selected.contains(p))
            out << p;
    return out;
}

bool PhotoView::isSelected(int i) const
{
    return m_selected.contains(path(i));
}

QStringList PhotoView::paths() const
{
    return m_paths;
}
