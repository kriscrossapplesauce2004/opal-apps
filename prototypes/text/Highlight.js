.pragma library
// Tiny highlighters for the prototypes: C++ and Markdown to StyledText spans.
function esc(s) { return s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/ /g, "&nbsp;"); }
function span(t, c, b) { return (b ? "<b>" : "") + "<font color=\"" + c + "\">" + esc(t) + "</font>" + (b ? "</b>" : ""); }
var kw = /^(const|auto|return|if|else|for|while|void|bool|int|double|static|class|struct|namespace|QString|QColor|QVariantMap|QFile|emit|this|new|true|false|nullptr|constexpr|QLatin1String|QStringLiteral)$/;
function cpp(line, c) {
    var ci = line.indexOf("//");
    var code = ci >= 0 ? line.slice(0, ci) : line, comment = ci >= 0 ? line.slice(ci) : "";
    var out = "", re = /("[^"]*")|([A-Za-z_][A-Za-z0-9_]*)|(\d+(\.\d+)?)|(\s+)|(.)/g, m;
    while ((m = re.exec(code)) !== null) {
        if (m[1]) out += span(m[1], c.str);
        else if (m[2]) out += kw.test(m[2]) ? span(m[2], c.kw, true) : /^m_/.test(m[2]) ? span(m[2], c.member) : /^[A-Z]/.test(m[2]) ? span(m[2], c.type) : span(m[2], c.fg);
        else if (m[3]) out += span(m[3], c.num);
        else out += span(m[0], m[6] && "{}();,".indexOf(m[6]) >= 0 ? c.punct : c.fg);
    }
    if (comment) out += span(comment, c.comment);
    return out || "&nbsp;";
}
function md(line, c) {
    if (/^#{1,6} /.test(line)) return span(line, c.kw, true);
    if (/^\|/.test(line)) return span(line, c.type);
    if (/^- /.test(line) || /^\d+\. /.test(line)) { var b = line.match(/^(- |\d+\. )/)[0]; return span(b, c.num, true) + inline(line.slice(b.length), c); }
    return inline(line, c) || "&nbsp;";
}
function inline(t, c) {
    var out = "", re = /(\*\*[^*]+\*\*)|(`[^`]+`)|([^*`]+)|(.)/g, m;
    while ((m = re.exec(t)) !== null) {
        if (m[1]) out += span(m[1], c.fg, true);
        else if (m[2]) out += span(m[2], c.str);
        else out += span(m[0], c.fg);
    }
    return out;
}
