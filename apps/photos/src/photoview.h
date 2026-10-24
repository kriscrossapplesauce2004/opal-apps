#pragma once
#include <QAbstractListModel>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class PhotoLibrary;

// One filtered, grouped and laid-out view of the library: justified rows that keep aspect ratios, with a header per group.
// QML: PhotoView { filter; album; query; group; width; rowHeight; gap; headerHeight }
//   filter: "all", "favourites", "screenshots", "videos" or "album" (with album = folder path). query narrows any filter.
//   group: "day", "month", "year" or "none". Months use smaller rows and years smaller still.
// Rows (roles): kind ("header" or "photos"), title, subtitle, key (group key), y, height, items.
//   items: [{i, x, w, path, thumb, video, favourite, selected}] where i is the view index of the photo.
// Properties: count (photos), contentHeight, marks (scrubber: [{label, y, row, year}]), selectedCount.
// Methods: path(i), photo(i), indexOfPath(path), rowOf(i), neighbour(i, "left|right|up|down"), groupOf(i),
//   select(i, on), toggle(i), selectRange(from, to), toggleGroup(key), groupSelected(key), selectAll(), clearSelection(),
//   selectedPaths(), isSelected(i), paths() (all paths in view order, for the viewer and the slideshow).
// Width or library changes keep the row count stable where they can, so the list does not jump.
class PhotoView : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString album READ album WRITE setAlbum NOTIFY filterChanged)
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY filterChanged)
    Q_PROPERTY(QString group READ group WRITE setGroup NOTIFY layoutParamsChanged)
    Q_PROPERTY(qreal width READ width WRITE setWidth NOTIFY layoutParamsChanged)
    Q_PROPERTY(qreal rowHeight READ rowHeight WRITE setRowHeight NOTIFY layoutParamsChanged)
    Q_PROPERTY(qreal gap READ gap WRITE setGap NOTIFY layoutParamsChanged)
    Q_PROPERTY(qreal headerHeight READ headerHeight WRITE setHeaderHeight NOTIFY layoutParamsChanged)
    Q_PROPERTY(int count READ count NOTIFY laidOut)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY laidOut)
    Q_PROPERTY(QVariantList marks READ marks NOTIFY laidOut)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY selectionChanged)

public:
    enum Roles { KindRole = Qt::UserRole + 1, TitleRole, SubtitleRole, KeyRole, YRole, HeightRole, ItemsRole };

    explicit PhotoView(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString filter() const { return m_filter; }
    void setFilter(const QString &f);
    QString album() const { return m_album; }
    void setAlbum(const QString &a);
    QString query() const { return m_query; }
    void setQuery(const QString &q);
    QString group() const { return m_group; }
    void setGroup(const QString &g);
    qreal width() const { return m_width; }
    void setWidth(qreal w);
    qreal rowHeight() const { return m_rowHeight; }
    void setRowHeight(qreal h);
    qreal gap() const { return m_gap; }
    void setGap(qreal g);
    qreal headerHeight() const { return m_headerHeight; }
    void setHeaderHeight(qreal h);
    int count() const { return int(m_items.size()); }
    qreal contentHeight() const { return m_contentHeight; }
    QVariantList marks() const { return m_marks; }
    int selectedCount() const { return int(m_selected.size()); }

    Q_INVOKABLE QString path(int i) const;
    Q_INVOKABLE QVariantMap photo(int i) const;
    Q_INVOKABLE int indexOfPath(const QString &path) const;
    Q_INVOKABLE int rowOf(int i) const;
    Q_INVOKABLE int neighbour(int i, const QString &direction) const;
    Q_INVOKABLE QString groupOf(int i) const;
    Q_INVOKABLE void select(int i, bool on);
    Q_INVOKABLE void toggle(int i);
    Q_INVOKABLE void selectRange(int from, int to);
    Q_INVOKABLE void toggleGroup(const QString &key);
    Q_INVOKABLE bool groupSelected(const QString &key) const;
    Q_INVOKABLE void selectAll();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE QStringList selectedPaths() const;
    Q_INVOKABLE bool isSelected(int i) const;
    Q_INVOKABLE QStringList paths() const;

signals:
    void filterChanged();
    void layoutParamsChanged();
    void laidOut();
    void selectionChanged();

private:
    struct Tile { int i; qreal x; qreal w; };
    struct Row
    {
        bool header = false;
        QString title;
        QString subtitle;
        QString key;
        qreal y = 0;
        qreal height = 0;
        QList<Tile> tiles;
    };

    void scheduleFilter();
    void refilter();
    void relayout();
    void layoutGroup(const QList<int> &members, qreal rowHeight, QList<Row> &out, qreal &y, const QString &key) const;
    QString keyFor(qint64 taken) const;
    QString titleFor(const QString &key, qint64 taken) const;
    void refreshRows();
    QVariantMap tileMap(const Tile &t) const;

    PhotoLibrary *m_lib;
    QString m_filter = QStringLiteral("all");
    QString m_album;
    QString m_query;
    QString m_group = QStringLiteral("day");
    qreal m_width = 0;
    qreal m_rowHeight = 190;
    qreal m_gap = 4;
    qreal m_headerHeight = 52;
    // Library indices in view order.
    QList<int> m_items;
    QStringList m_paths;
    QHash<QString, int> m_viewIndex;
    QList<Row> m_rows;
    QList<int> m_rowOfItem;
    QList<QString> m_groupOfItem;
    QHash<QString, QList<int>> m_groupItems;
    qreal m_contentHeight = 0;
    QVariantList m_marks;
    QSet<QString> m_selected;
    QTimer m_filterTimer;
    QTimer m_layoutTimer;
};
