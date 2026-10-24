#pragma once
#include <QColor>
#include <QObject>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>
#include <memory>

namespace KSyntaxHighlighting { class Repository; class Theme; }

// Syntax colours drawn from Opal's palette: a KSyntaxHighlighting theme written from the current colour roles, so code wears
// the same colours as the shell. QML: singleton "SyntaxThemes". Call apply(Palette.roles, Theme.light) when the palette changes.
// Properties: revision (changes with the theme), background, currentLine, selection.
class SyntaxThemes : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY changed)
public:
    explicit SyntaxThemes(QObject *parent = nullptr);
    ~SyntaxThemes() override;
    static SyntaxThemes *instance();
    static SyntaxThemes *create(class QQmlEngine *, class QJSEngine *);

    int revision() const { return m_revision; }
    KSyntaxHighlighting::Repository &repository();
    KSyntaxHighlighting::Theme theme();

    Q_INVOKABLE void apply(const QVariantMap &roles, bool light);
    // The theme JSON for a set of roles, public for tests.
    static QByteArray themeJson(const QString &name, const QVariantMap &roles, bool light);

signals:
    void changed();

private:
    std::unique_ptr<KSyntaxHighlighting::Repository> m_repo;
    QString m_name;
    QByteArray m_last;
    int m_revision = 0;
};
