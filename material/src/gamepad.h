#pragma once
#include <QObject>
#include <QQmlEngine>
#include <QTimer>

// Reads game controllers through SDL3 (evdev only, read-only) and turns them into key events for the focused window.
// D-pad and left stick give arrow keys with repeat, A gives Return, B gives Escape, X gives Menu, Y gives Search,
// the shoulders give Ctrl+PageUp/PageDown and the triggers give PageUp/PageDown. Every press is also sent as a signal.
class Gamepad : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(bool available READ available CONSTANT)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString name READ name NOTIFY connectedChanged)
    // xbox, playstation, nintendo or generic: selects the face-button labels in hints.
    Q_PROPERTY(QString family READ family NOTIFY connectedChanged)
public:
    explicit Gamepad(QObject *parent = nullptr);
    ~Gamepad() override;
    static Gamepad *instance();
    static Gamepad *create(QQmlEngine *, QJSEngine *);

    bool available() const;
    bool connected() const { return !m_name.isEmpty(); }
    QString name() const { return m_name; }
    QString family() const { return m_family; }

    // Face-button label for a role (a, b, x, y) in the connected controller's family.
    Q_INVOKABLE QString label(const QString &role) const;

signals:
    void connectedChanged();
    // Any controller input while a window of this app has focus.
    void activity();
    void button(const QString &name, bool pressed);
    // Right stick, from -1 to 1 on each axis, sent while it is held.
    void scroll(qreal dx, qreal dy);

private:
    void poll();
    void refreshPads();
    void handleButton(int button, bool down);
    void setDirection(int key);
    void sendKey(int key, Qt::KeyboardModifiers mods = Qt::NoModifier);

    QTimer m_poll;
    QTimer m_repeat;
    int m_direction = 0;
    int m_stickKey = 0;
    int m_dpadKey = 0;
    qreal m_rx = 0, m_ry = 0;
    bool m_ltDown = false, m_rtDown = false;
    QString m_name;
    QString m_family = QStringLiteral("xbox");
    bool m_ready = false;
};
