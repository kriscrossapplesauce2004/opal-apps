#include "gamepad.h"

#include <QGuiApplication>
#include <QKeyEvent>
#include <QWindow>

#ifdef OPAL_HAVE_SDL
#include <SDL3/SDL.h>
#endif

namespace {
constexpr int RepeatDelay = 380;
constexpr int RepeatRate = 90;
constexpr int StickOn = 16000;
constexpr int StickOff = 9000;
constexpr int TriggerOn = 20000;
}

Gamepad::Gamepad(QObject *parent) : QObject(parent)
{
    m_repeat.setSingleShot(false);
    connect(&m_repeat, &QTimer::timeout, this, [this] {
        m_repeat.setInterval(RepeatRate);
        if (m_direction) sendKey(m_direction);
    });
#ifdef OPAL_HAVE_SDL
    // Use evdev only: HIDAPI opens hidraw devices and can disturb Steam Input and the shell.
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "0");
    SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (qEnvironmentVariableIsSet("OPAL_NO_GAMEPAD") || !SDL_Init(SDL_INIT_GAMEPAD)) return;
    m_ready = true;
    connect(&m_poll, &QTimer::timeout, this, &Gamepad::poll);
    m_poll.start(250);
    refreshPads();
#endif
}

Gamepad::~Gamepad()
{
#ifdef OPAL_HAVE_SDL
    if (m_ready) SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
#endif
}

Gamepad *Gamepad::instance()
{
    static Gamepad *pad = new Gamepad;
    return pad;
}

Gamepad *Gamepad::create(QQmlEngine *, QJSEngine *engine)
{
    Gamepad *g = instance();
    engine->setObjectOwnership(g, QJSEngine::CppOwnership);
    return g;
}

bool Gamepad::available() const { return m_ready; }

QString Gamepad::label(const QString &role) const
{
    if (m_family == QLatin1String("playstation")) {
        if (role == QLatin1String("a")) return QStringLiteral("✕");
        if (role == QLatin1String("b")) return QStringLiteral("○");
        if (role == QLatin1String("x")) return QStringLiteral("□");
        if (role == QLatin1String("y")) return QStringLiteral("△");
    }
    if (m_family == QLatin1String("nintendo")) {
        // Nintendo layouts swap the labels: the south button reads B.
        if (role == QLatin1String("a")) return QStringLiteral("B");
        if (role == QLatin1String("b")) return QStringLiteral("A");
        if (role == QLatin1String("x")) return QStringLiteral("Y");
        if (role == QLatin1String("y")) return QStringLiteral("X");
    }
    return role.toUpper();
}

void Gamepad::refreshPads()
{
#ifdef OPAL_HAVE_SDL
    int count = 0;
    SDL_JoystickID *ids = SDL_GetGamepads(&count);
    QString name, family = QStringLiteral("xbox");
    for (int i = 0; i < count; ++i) {
        SDL_Gamepad *pad = SDL_GetGamepadFromID(ids[i]);
        if (!pad) pad = SDL_OpenGamepad(ids[i]);
        if (!pad) continue;
        if (name.isEmpty()) {
            name = QString::fromUtf8(SDL_GetGamepadName(pad));
            switch (SDL_GetGamepadType(pad)) {
            case SDL_GAMEPAD_TYPE_PS3: case SDL_GAMEPAD_TYPE_PS4: case SDL_GAMEPAD_TYPE_PS5:
                family = QStringLiteral("playstation"); break;
            case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO: case SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR:
                family = QStringLiteral("nintendo"); break;
            default: break;
            }
        }
    }
    SDL_free(ids);
    if (name.isEmpty() && count > 0) name = QStringLiteral("Controller");
    if (name != m_name || family != m_family) {
        m_name = name;
        m_family = family;
        emit connectedChanged();
    }
    // Poll fast only while a controller is connected.
    m_poll.setInterval(m_name.isEmpty() ? 250 : 8);
#endif
}

void Gamepad::poll()
{
#ifdef OPAL_HAVE_SDL
    SDL_Event e;
    const bool focused = QGuiApplication::focusWindow() != nullptr;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_EVENT_GAMEPAD_ADDED:
        case SDL_EVENT_GAMEPAD_REMOVED:
            refreshPads();
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (focused) handleButton(e.gbutton.button, e.gbutton.down);
            break;
        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            if (!focused) break;
            const int v = e.gaxis.value;
            switch (e.gaxis.axis) {
            case SDL_GAMEPAD_AXIS_LEFTX:
            case SDL_GAMEPAD_AXIS_LEFTY: {
                SDL_Gamepad *pad = SDL_GetGamepadFromID(e.gaxis.which);
                const int x = pad ? SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX) : 0;
                const int y = pad ? SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTY) : 0;
                // Hysteresis keeps a resting stick from chattering between keys.
                const int limit = m_stickKey ? StickOff : StickOn;
                int key = 0;
                if (std::abs(x) > limit || std::abs(y) > limit)
                    key = std::abs(x) > std::abs(y) ? (x > 0 ? Qt::Key_Right : Qt::Key_Left) : (y > 0 ? Qt::Key_Down : Qt::Key_Up);
                if (key != m_stickKey) {
                    m_stickKey = key;
                    setDirection(m_dpadKey ? m_dpadKey : key);
                }
                break;
            }
            case SDL_GAMEPAD_AXIS_RIGHTX: m_rx = std::abs(v) > 6000 ? v / 32767.0 : 0; break;
            case SDL_GAMEPAD_AXIS_RIGHTY: m_ry = std::abs(v) > 6000 ? v / 32767.0 : 0; break;
            case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                if ((v > TriggerOn) != m_ltDown) { m_ltDown = v > TriggerOn; emit activity(); emit button(QStringLiteral("lt"), m_ltDown); if (m_ltDown) sendKey(Qt::Key_PageUp); }
                break;
            case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                if ((v > TriggerOn) != m_rtDown) { m_rtDown = v > TriggerOn; emit activity(); emit button(QStringLiteral("rt"), m_rtDown); if (m_rtDown) sendKey(Qt::Key_PageDown); }
                break;
            default: break;
            }
            break;
        }
        default: break;
        }
    }
    if (focused && (m_rx != 0 || m_ry != 0)) { emit activity(); emit scroll(m_rx, m_ry); }
#endif
}

void Gamepad::handleButton(int b, bool down)
{
#ifdef OPAL_HAVE_SDL
    emit activity();
    struct Map { int button; const char *name; int key; Qt::KeyboardModifiers mods; };
    static const Map maps[] = {
        {SDL_GAMEPAD_BUTTON_SOUTH, "a", Qt::Key_Return, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_EAST, "b", Qt::Key_Escape, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_WEST, "x", Qt::Key_Menu, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_NORTH, "y", Qt::Key_Search, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, "lb", Qt::Key_PageUp, Qt::ControlModifier},
        {SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, "rb", Qt::Key_PageDown, Qt::ControlModifier},
        {SDL_GAMEPAD_BUTTON_START, "start", 0, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_BACK, "select", 0, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_LEFT_STICK, "ls", 0, Qt::NoModifier},
        {SDL_GAMEPAD_BUTTON_RIGHT_STICK, "rs", 0, Qt::NoModifier},
    };
    const int dpad[][2] = {{SDL_GAMEPAD_BUTTON_DPAD_UP, Qt::Key_Up}, {SDL_GAMEPAD_BUTTON_DPAD_DOWN, Qt::Key_Down},
                           {SDL_GAMEPAD_BUTTON_DPAD_LEFT, Qt::Key_Left}, {SDL_GAMEPAD_BUTTON_DPAD_RIGHT, Qt::Key_Right}};
    for (const auto &d : dpad) {
        if (d[0] != b) continue;
        if (down) m_dpadKey = d[1];
        else if (m_dpadKey == d[1]) m_dpadKey = 0;
        setDirection(m_dpadKey ? m_dpadKey : m_stickKey);
        return;
    }
    for (const Map &m : maps) {
        if (m.button != b) continue;
        emit button(QString::fromLatin1(m.name), down);
        if (down && m.key) sendKey(m.key, m.mods);
        return;
    }
    // The Guide button belongs to the shell; apps ignore it.
#else
    Q_UNUSED(b) Q_UNUSED(down)
#endif
}

void Gamepad::setDirection(int key)
{
    if (key == m_direction) return;
    m_direction = key;
    m_repeat.stop();
    if (!key) return;
    emit activity();
    sendKey(key);
    m_repeat.start(RepeatDelay);
}

void Gamepad::sendKey(int key, Qt::KeyboardModifiers mods)
{
    QWindow *w = QGuiApplication::focusWindow();
    if (!w) return;
    QKeyEvent press(QEvent::KeyPress, key, mods);
    QKeyEvent release(QEvent::KeyRelease, key, mods);
    QGuiApplication::sendEvent(w, &press);
    QGuiApplication::sendEvent(w, &release);
}
