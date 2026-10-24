#pragma once
#include <QFileSystemWatcher>
#include <QObject>
#include <QPointer>
#include <QQuickTextDocument>
#include <QRegularExpression>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

namespace KSyntaxHighlighting { class SyntaxHighlighter; }
class QTextDocument;

// One open file behind a TextArea: load and save (UTF-8 or Latin-1, LF or CRLF), syntax colours, find and replace,
// whole-document edits as one undo step, and a warning when the file changes on disk.
// QML: EditorDocument { textDocument: area.textDocument; path }.
// Properties: path, name, language (definition name, settable), languages (all names), modified, readOnly, encoding, lineEnding,
//   lineCount, loaded, changedOnDisk, missing, error, untitledNumber.
// Methods: load(path), save(), saveAs(path), reload(), text(), setText(text), find(query, from, flags) -> {start, end} or {},
//   findAll(query, flags) -> [{start, end}], replaceAll(query, replacement, flags) -> count, replaceRange(start, end, text),
//   applyEdits([{start, end, text}]) as one undo step, lineStart(line), lineAt(pos), columnAt(pos), wordAt(pos) -> {start, end, text},
//   indentOf(line), toggleComment(start, end), moveLines(start, end, delta), duplicateLines(start, end).
// flags: "regex", "case", "word", "back" in any combination.
class EditorDocument : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QQuickTextDocument *textDocument READ textDocument WRITE setTextDocument NOTIFY textDocumentChanged)
    Q_PROPERTY(QString path READ path NOTIFY pathChanged)
    Q_PROPERTY(QString name READ name NOTIFY pathChanged)
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QStringList languages READ languages CONSTANT)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(bool readOnly READ readOnly NOTIFY pathChanged)
    Q_PROPERTY(QString encoding READ encoding NOTIFY formatChanged)
    Q_PROPERTY(QString lineEnding READ lineEnding WRITE setLineEnding NOTIFY formatChanged)
    Q_PROPERTY(int lineCount READ lineCount NOTIFY linesChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY pathChanged)
    Q_PROPERTY(bool changedOnDisk READ changedOnDisk NOTIFY diskChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString commentToken READ commentToken NOTIFY languageChanged)

public:
    explicit EditorDocument(QObject *parent = nullptr);
    ~EditorDocument() override;

    QQuickTextDocument *textDocument() const { return m_quick; }
    void setTextDocument(QQuickTextDocument *d);
    QString path() const { return m_path; }
    QString name() const;
    QString language() const { return m_language; }
    void setLanguage(const QString &l);
    static QStringList languages();
    bool modified() const;
    bool readOnly() const { return m_readOnly; }
    QString encoding() const { return m_encoding; }
    QString lineEnding() const { return m_crlf ? QStringLiteral("CRLF") : QStringLiteral("LF"); }
    void setLineEnding(const QString &e);
    int lineCount() const;
    bool loaded() const { return m_loaded; }
    bool changedOnDisk() const { return m_changedOnDisk; }
    QString error() const { return m_error; }
    QString commentToken() const;

    Q_INVOKABLE bool load(const QString &path);
    Q_INVOKABLE bool save();
    Q_INVOKABLE bool saveAs(const QString &path);
    Q_INVOKABLE bool reload();
    Q_INVOKABLE QString text() const;
    Q_INVOKABLE void setText(const QString &text);
    Q_INVOKABLE QVariantMap find(const QString &query, int from, const QString &flags) const;
    Q_INVOKABLE QVariantList findAll(const QString &query, const QString &flags, int limit = 5000) const;
    Q_INVOKABLE int replaceAll(const QString &query, const QString &replacement, const QString &flags);
    Q_INVOKABLE void replaceRange(int start, int end, const QString &text);
    Q_INVOKABLE void applyEdits(const QVariantList &edits);
    Q_INVOKABLE int lineStart(int line) const;
    Q_INVOKABLE int lineAt(int pos) const;
    Q_INVOKABLE int columnAt(int pos) const;
    Q_INVOKABLE QVariantMap wordAt(int pos) const;
    Q_INVOKABLE QString indentOf(int line) const;
    Q_INVOKABLE int toggleComment(int start, int end);
    Q_INVOKABLE void moveLines(int start, int end, int delta);
    Q_INVOKABLE void duplicateLines(int start, int end);
    Q_INVOKABLE void markSaved();

    // Decoding as load does it, public for tests: UTF-8 when valid, else Latin-1; CRLF when most line ends are CRLF.
    static QString decode(const QByteArray &bytes, QString *encoding, bool *crlf);

signals:
    void textDocumentChanged();
    void pathChanged();
    void languageChanged();
    void modifiedChanged();
    void formatChanged();
    void linesChanged();
    void diskChanged();
    void errorChanged();
    void saved(const QString &path);
    void contentsChanged();

private:
    QTextDocument *doc() const;
    void applyHighlighter();
    void setError(const QString &e);
    void watch();
    QRegularExpression pattern(const QString &query, const QString &flags) const;

    QPointer<QQuickTextDocument> m_quick;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter = nullptr;
    QString m_path;
    QString m_language;
    QString m_encoding = QStringLiteral("UTF-8");
    bool m_crlf = false;
    bool m_readOnly = false;
    bool m_loaded = false;
    bool m_changedOnDisk = false;
    bool m_selfWrite = false;
    QString m_error;
    QFileSystemWatcher m_watcher;
};
