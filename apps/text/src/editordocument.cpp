#include "editordocument.h"
#include "syntaxthemes.h"

#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Theme>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStringDecoder>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <algorithm>

EditorDocument::EditorDocument(QObject *parent)
    : QObject(parent)
{
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &p) {
        if (m_selfWrite) {
            m_selfWrite = false;
            watch();
            return;
        }
        // Editors that save by renaming drop the watch; it is added again once the file is back.
        QTimer::singleShot(150, this, [this, p] {
            if (p != m_path)
                return;
            watch();
            m_changedOnDisk = true;
            emit diskChanged();
        });
    });
    connect(SyntaxThemes::instance(), &SyntaxThemes::changed, this, &EditorDocument::applyHighlighter);
}

EditorDocument::~EditorDocument() = default;

QTextDocument *EditorDocument::doc() const
{
    return m_quick ? m_quick->textDocument() : nullptr;
}

void EditorDocument::setTextDocument(QQuickTextDocument *d)
{
    if (m_quick == d)
        return;
    if (m_quick && doc())
        doc()->disconnect(this);
    m_quick = d;
    if (QTextDocument *td = doc()) {
        connect(td, &QTextDocument::modificationChanged, this, &EditorDocument::modifiedChanged);
        connect(td, &QTextDocument::blockCountChanged, this, &EditorDocument::linesChanged);
        connect(td, &QTextDocument::contentsChanged, this, &EditorDocument::contentsChanged);
        td->setDocumentMargin(0);
    }
    applyHighlighter();
    emit textDocumentChanged();
    emit linesChanged();
}

QString EditorDocument::name() const
{
    return m_path.isEmpty() ? tr("Untitled") : QFileInfo(m_path).fileName();
}

QStringList EditorDocument::languages()
{
    QStringList out;
    for (const auto &d : SyntaxThemes::instance()->repository().definitions())
        if (!d.isHidden())
            out << d.name();
    out.sort(Qt::CaseInsensitive);
    return out;
}

void EditorDocument::setLanguage(const QString &l)
{
    if (m_language == l)
        return;
    m_language = l;
    applyHighlighter();
    emit languageChanged();
}

QString EditorDocument::commentToken() const
{
    static const QHash<QString, QString> hash{{QStringLiteral("Python"), QStringLiteral("#")}, {QStringLiteral("Bash"), QStringLiteral("#")},
                                              {QStringLiteral("Zsh"), QStringLiteral("#")}, {QStringLiteral("CMake"), QStringLiteral("#")},
                                              {QStringLiteral("YAML"), QStringLiteral("#")}, {QStringLiteral("TOML"), QStringLiteral("#")},
                                              {QStringLiteral("Fish"), QStringLiteral("#")}, {QStringLiteral("Ruby"), QStringLiteral("#")},
                                              {QStringLiteral("Perl"), QStringLiteral("#")}, {QStringLiteral("Makefile"), QStringLiteral("#")},
                                              {QStringLiteral("INI Files"), QStringLiteral(";")}, {QStringLiteral("Lua"), QStringLiteral("--")},
                                              {QStringLiteral("SQL"), QStringLiteral("--")}, {QStringLiteral("Haskell"), QStringLiteral("--")}};
    return hash.value(m_language, QStringLiteral("//"));
}

void EditorDocument::applyHighlighter()
{
    QTextDocument *td = doc();
    if (!td)
        return;
    auto &repo = SyntaxThemes::instance()->repository();
    const auto def = m_language.isEmpty() ? repo.definitionForFileName(m_path) : repo.definitionForName(m_language);
    if (!m_highlighter || m_highlighter->document() != td) {
        delete m_highlighter;
        m_highlighter = new KSyntaxHighlighting::SyntaxHighlighter(td);
    }
    m_highlighter->setTheme(SyntaxThemes::instance()->theme());
    m_highlighter->setDefinition(def);
    if (m_language.isEmpty() && def.isValid()) {
        m_language = def.name();
        emit languageChanged();
    }
    m_highlighter->rehighlight();
}

bool EditorDocument::modified() const
{
    return doc() && doc()->isModified();
}

void EditorDocument::setLineEnding(const QString &e)
{
    const bool crlf = e == QLatin1String("CRLF");
    if (crlf == m_crlf)
        return;
    m_crlf = crlf;
    if (doc())
        doc()->setModified(true);
    emit formatChanged();
}

int EditorDocument::lineCount() const
{
    return doc() ? doc()->blockCount() : 0;
}

void EditorDocument::setError(const QString &e)
{
    if (m_error == e)
        return;
    m_error = e;
    emit errorChanged();
}

void EditorDocument::watch()
{
    if (!m_watcher.files().isEmpty())
        m_watcher.removePaths(m_watcher.files());
    if (!m_path.isEmpty() && QFileInfo::exists(m_path))
        m_watcher.addPath(m_path);
}

QString EditorDocument::decode(const QByteArray &bytes, QString *encoding, bool *crlf)
{
    QByteArray data = bytes;
    if (data.startsWith("\xEF\xBB\xBF"))
        data.remove(0, 3);
    QStringDecoder utf8(QStringDecoder::Utf8, QStringDecoder::Flag::Stateless);
    QString text = utf8.decode(data);
    if (utf8.hasError()) {
        text = QString::fromLatin1(data);
        if (encoding)
            *encoding = QStringLiteral("ISO-8859-1");
    } else if (encoding) {
        *encoding = QStringLiteral("UTF-8");
    }
    const int crlfCount = int(data.count("\r\n"));
    const int lfCount = int(data.count('\n'));
    if (crlf)
        *crlf = crlfCount > 0 && crlfCount * 2 >= lfCount;
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    return text;
}

bool EditorDocument::load(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        setError(tr("Can't open %1: %2").arg(QFileInfo(path).fileName(), f.errorString()));
        return false;
    }
    // Very large or binary files would freeze the editor.
    if (f.size() > 40 * 1024 * 1024) {
        setError(tr("%1 is too big to edit here").arg(QFileInfo(path).fileName()));
        return false;
    }
    const QByteArray bytes = f.readAll();
    if (bytes.left(8000).contains('\0')) {
        setError(tr("%1 is not a text file").arg(QFileInfo(path).fileName()));
        return false;
    }
    QString enc;
    bool crlf = false;
    const QString text = decode(bytes, &enc, &crlf);
    m_path = QFileInfo(path).absoluteFilePath();
    m_readOnly = !QFileInfo(path).isWritable();
    m_encoding = enc;
    m_crlf = crlf;
    m_language.clear();
    m_loaded = true;
    m_changedOnDisk = false;
    if (QTextDocument *td = doc()) {
        td->setPlainText(text);
        td->setModified(false);
        td->clearUndoRedoStacks();
    }
    applyHighlighter();
    watch();
    setError({});
    emit pathChanged();
    emit formatChanged();
    emit diskChanged();
    emit languageChanged();
    return true;
}

bool EditorDocument::save()
{
    if (m_path.isEmpty())
        return false;
    QSaveFile f(m_path);
    if (!f.open(QIODevice::WriteOnly)) {
        setError(tr("Can't save %1: %2").arg(name(), f.errorString()));
        return false;
    }
    QString text = this->text();
    if (m_crlf)
        text.replace(QLatin1Char('\n'), QStringLiteral("\r\n"));
    const QByteArray bytes = m_encoding == QLatin1String("ISO-8859-1") ? text.toLatin1() : text.toUtf8();
    m_selfWrite = true;
    if (f.write(bytes) != bytes.size() || !f.commit()) {
        m_selfWrite = false;
        setError(tr("Can't save %1: %2").arg(name(), f.errorString()));
        return false;
    }
    // QSaveFile renames over the old file, which ends the watch; add it again.
    QTimer::singleShot(200, this, [this] { m_selfWrite = false; watch(); });
    if (doc())
        doc()->setModified(false);
    m_changedOnDisk = false;
    setError({});
    emit diskChanged();
    emit saved(m_path);
    return true;
}

bool EditorDocument::saveAs(const QString &path)
{
    const QString old = m_path;
    m_path = QFileInfo(path).absoluteFilePath();
    m_readOnly = false;
    if (!save()) {
        m_path = old;
        return false;
    }
    m_language.clear();
    applyHighlighter();
    emit pathChanged();
    emit languageChanged();
    return true;
}

bool EditorDocument::reload()
{
    return !m_path.isEmpty() && load(m_path);
}

void EditorDocument::markSaved()
{
    if (doc())
        doc()->setModified(false);
}

QString EditorDocument::text() const
{
    return doc() ? doc()->toPlainText() : QString();
}

void EditorDocument::setText(const QString &text)
{
    if (QTextDocument *td = doc()) {
        QTextCursor c(td);
        c.select(QTextCursor::Document);
        c.insertText(text);
    }
}

QRegularExpression EditorDocument::pattern(const QString &query, const QString &flags) const
{
    QString p = flags.contains(QLatin1String("regex")) ? query : QRegularExpression::escape(query);
    if (flags.contains(QLatin1String("word")))
        p = QStringLiteral("\\b") + p + QStringLiteral("\\b");
    QRegularExpression::PatternOptions o = QRegularExpression::MultilineOption;
    if (!flags.contains(QLatin1String("case")))
        o |= QRegularExpression::CaseInsensitiveOption;
    return QRegularExpression(p, o);
}

QVariantMap EditorDocument::find(const QString &query, int from, const QString &flags) const
{
    if (query.isEmpty())
        return {};
    const QRegularExpression re = pattern(query, flags);
    if (!re.isValid())
        return {};
    const QString t = text();
    if (flags.contains(QLatin1String("back"))) {
        // The last match that ends before from, wrapping to the end.
        QRegularExpressionMatch last;
        auto it = re.globalMatch(t);
        QRegularExpressionMatch lastAny;
        while (it.hasNext()) {
            const auto m = it.next();
            if (m.capturedLength() == 0)
                continue;
            if (m.capturedEnd() <= from)
                last = m;
            lastAny = m;
        }
        const auto &m = last.hasMatch() ? last : lastAny;
        if (!m.hasMatch())
            return {};
        return {{QStringLiteral("start"), m.capturedStart()}, {QStringLiteral("end"), m.capturedEnd()}};
    }
    auto m = re.match(t, std::clamp(from, 0, int(t.size())));
    if (!m.hasMatch() || m.capturedLength() == 0)
        m = re.match(t, 0);
    if (!m.hasMatch() || m.capturedLength() == 0)
        return {};
    return {{QStringLiteral("start"), m.capturedStart()}, {QStringLiteral("end"), m.capturedEnd()}};
}

QVariantList EditorDocument::findAll(const QString &query, const QString &flags, int limit) const
{
    QVariantList out;
    if (query.isEmpty())
        return out;
    const QRegularExpression re = pattern(query, flags);
    if (!re.isValid())
        return out;
    auto it = re.globalMatch(text());
    while (it.hasNext() && out.size() < limit) {
        const auto m = it.next();
        if (m.capturedLength() > 0)
            out << QVariantMap{{QStringLiteral("start"), m.capturedStart()}, {QStringLiteral("end"), m.capturedEnd()}};
    }
    return out;
}

int EditorDocument::replaceAll(const QString &query, const QString &replacement, const QString &flags)
{
    QTextDocument *td = doc();
    if (!td || query.isEmpty())
        return 0;
    const QRegularExpression re = pattern(query, flags);
    if (!re.isValid())
        return 0;
    const QString before = text();
    QList<QRegularExpressionMatch> matches;
    auto it = re.globalMatch(before);
    while (it.hasNext()) {
        const auto m = it.next();
        if (m.capturedLength() > 0)
            matches << m;
    }
    if (matches.isEmpty())
        return 0;
    // Back to front, so earlier offsets stay right; one edit block makes it one undo step.
    QTextCursor c(td);
    c.beginEditBlock();
    for (auto m = matches.crbegin(); m != matches.crend(); ++m) {
        QString rep = replacement;
        if (flags.contains(QLatin1String("regex")))
            for (int g = m->lastCapturedIndex(); g >= 0; --g)
                rep.replace(QStringLiteral("\\") + QString::number(g), m->captured(g)).replace(QLatin1Char('$') + QString::number(g), m->captured(g));
        c.setPosition(m->capturedStart());
        c.setPosition(m->capturedEnd(), QTextCursor::KeepAnchor);
        c.insertText(rep);
    }
    c.endEditBlock();
    return int(matches.size());
}

void EditorDocument::replaceRange(int start, int end, const QString &text)
{
    applyEdits({QVariantMap{{QStringLiteral("start"), start}, {QStringLiteral("end"), end}, {QStringLiteral("text"), text}}});
}

void EditorDocument::applyEdits(const QVariantList &edits)
{
    QTextDocument *td = doc();
    if (!td || edits.isEmpty())
        return;
    QList<QVariantMap> list;
    for (const QVariant &v : edits)
        list << v.toMap();
    std::sort(list.begin(), list.end(), [](const QVariantMap &a, const QVariantMap &b) { return a.value(QStringLiteral("start")).toInt() > b.value(QStringLiteral("start")).toInt(); });
    const int size = td->characterCount() - 1;
    QTextCursor c(td);
    c.beginEditBlock();
    for (const QVariantMap &e : list) {
        const int s = std::clamp(e.value(QStringLiteral("start")).toInt(), 0, size);
        const int en = std::clamp(e.value(QStringLiteral("end")).toInt(), s, size);
        c.setPosition(s);
        c.setPosition(en, QTextCursor::KeepAnchor);
        c.insertText(e.value(QStringLiteral("text")).toString());
    }
    c.endEditBlock();
}

int EditorDocument::lineStart(int line) const
{
    if (!doc())
        return 0;
    const QTextBlock b = doc()->findBlockByNumber(std::clamp(line, 0, lineCount() - 1));
    return b.isValid() ? b.position() : 0;
}

int EditorDocument::lineAt(int pos) const
{
    return doc() ? doc()->findBlock(std::max(0, pos)).blockNumber() : 0;
}

int EditorDocument::columnAt(int pos) const
{
    if (!doc())
        return 0;
    const QTextBlock b = doc()->findBlock(std::max(0, pos));
    return pos - b.position();
}

QVariantMap EditorDocument::wordAt(int pos) const
{
    if (!doc())
        return {};
    const QTextBlock b = doc()->findBlock(std::max(0, pos));
    const QString line = b.text();
    int i = pos - b.position(), s = i, e = i;
    auto isWord = [](QChar c) { return c.isLetterOrNumber() || c == QLatin1Char('_'); };
    while (s > 0 && isWord(line.at(s - 1)))
        --s;
    while (e < line.size() && isWord(line.at(e)))
        ++e;
    return {{QStringLiteral("start"), b.position() + s}, {QStringLiteral("end"), b.position() + e}, {QStringLiteral("text"), line.mid(s, e - s)}};
}

QString EditorDocument::indentOf(int line) const
{
    if (!doc())
        return {};
    const QString t = doc()->findBlockByNumber(line).text();
    int i = 0;
    while (i < t.size() && (t.at(i) == QLatin1Char(' ') || t.at(i) == QLatin1Char('\t')))
        ++i;
    return t.left(i);
}

int EditorDocument::toggleComment(int start, int end)
{
    QTextDocument *td = doc();
    if (!td)
        return 0;
    const QString tok = commentToken();
    const int first = lineAt(start), last = lineAt(std::max(start, end - (end > start ? 1 : 0)));
    // Comment when any non-empty line lacks the token; otherwise remove it.
    bool all = true;
    for (int l = first; l <= last; ++l) {
        const QString t = td->findBlockByNumber(l).text().trimmed();
        if (!t.isEmpty() && !t.startsWith(tok))
            all = false;
    }
    QTextCursor c(td);
    c.beginEditBlock();
    for (int l = first; l <= last; ++l) {
        const QTextBlock b = td->findBlockByNumber(l);
        const QString t = b.text();
        const int indent = int(indentOf(l).size());
        if (t.trimmed().isEmpty())
            continue;
        if (all) {
            const int at = int(t.indexOf(tok));
            c.setPosition(b.position() + at);
            c.setPosition(b.position() + at + int(tok.size()) + (t.mid(at + tok.size(), 1) == QLatin1String(" ") ? 1 : 0), QTextCursor::KeepAnchor);
            c.removeSelectedText();
        } else {
            c.setPosition(b.position() + indent);
            c.insertText(tok + QLatin1Char(' '));
        }
    }
    c.endEditBlock();
    return last - first + 1;
}

void EditorDocument::moveLines(int start, int end, int delta)
{
    QTextDocument *td = doc();
    if (!td || delta == 0)
        return;
    const int first = lineAt(start), last = lineAt(std::max(start, end - (end > start ? 1 : 0)));
    if ((delta < 0 && first == 0) || (delta > 0 && last >= lineCount() - 1))
        return;
    QStringList lines = text().split(QLatin1Char('\n'));
    const QStringList block = lines.mid(first, last - first + 1);
    if (delta < 0) {
        const QString above = lines.at(first - 1);
        for (int i = 0; i < block.size(); ++i)
            lines[first - 1 + i] = block.at(i);
        lines[last] = above;
    } else {
        const QString below = lines.at(last + 1);
        for (int i = block.size() - 1; i >= 0; --i)
            lines[first + 1 + i] = block.at(i);
        lines[first] = below;
    }
    const int s = lineStart(std::min(first, first + delta));
    const int e = lineStart(std::max(last, last + delta)) + int(td->findBlockByNumber(std::max(last, last + delta)).text().size());
    const int lo = std::min(first, first + delta), hi = std::max(last, last + delta);
    replaceRange(s, e, lines.mid(lo, hi - lo + 1).join(QLatin1Char('\n')));
}

void EditorDocument::duplicateLines(int start, int end)
{
    QTextDocument *td = doc();
    if (!td)
        return;
    const int first = lineAt(start), last = lineAt(std::max(start, end - (end > start ? 1 : 0)));
    const int s = lineStart(first);
    const QTextBlock lb = td->findBlockByNumber(last);
    const int e = lb.position() + lb.text().size();
    const QString chunk = text().mid(s, e - s);
    replaceRange(e, e, QLatin1Char('\n') + chunk);
}
