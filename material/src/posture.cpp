#include "posture.h"
#include "gamepad.h"
#include "palette.h"

#include <QFile>
#include <QGuiApplication>
#include <QInputDevice>
#include <QMouseEvent>
#include <QScreen>
#include <QTouchEvent>
#include <cmath>

namespace {
// Product names of handhelds that run desktop Linux; these get the couch posture.
const char *const Handhelds[] = {"Jupiter", "Galileo", "ROG Ally", "Legion Go", "AYANEO", "AYN", "GPD Win", "OneXPlayer", "MSI Claw"};
// A screen with a diagonal above this size in millimetres counts as a TV.
constexpr double TvDiagonal = 1000;
}

Posture::Posture(QObject *parent) : QObject(parent)
{
    QFile dmi(QStringLiteral("/sys/class/dmi/id/product_name"));
    if (dmi.open(QIODevice::ReadOnly)) {
        const QByteArray product = dmi.readAll().trimmed();
        for (const char *h : Handhelds)
            if (product.contains(h)) m_handheld = true;
    }
    // Start as touch when the device has a touchscreen and no mouse or touchpad.
    bool hasTouch = false, hasPointer = false;
    for (const QInputDevice *d : QInputDevice::devices()) {
        if (d->type() == QInputDevice::DeviceType::TouchScreen) hasTouch = true;
        if (d->type() == QInputDevice::DeviceType::Mouse || d->type() == QInputDevice::DeviceType::TouchPad) hasPointer = true;
    }
    if (hasTouch && !hasPointer) m_lastHand = Touch;
    if (Palette::instance()->opalMode() == QLatin1String("phone")) m_lastHand = Touch;
    m_forced = QString::fromUtf8(qgetenv("OPAL_POSTURE"));
    qApp->installEventFilter(this);
    connect(Palette::instance(), &Palette::changed, this, &Posture::update);
    connect(Gamepad::instance(), &Gamepad::activity, this, [this] {
        if (m_controller) return;
        m_controller = true;
        update();
    });
    connect(qApp, &QGuiApplication::primaryScreenChanged, this, &Posture::update);
    update();
}

Posture *Posture::instance()
{
    static Posture *posture = new Posture;
    return posture;
}

Posture *Posture::create(QQmlEngine *, QJSEngine *engine)
{
    Posture *p = instance();
    engine->setObjectOwnership(p, QJSEngine::CppOwnership);
    return p;
}

QString Posture::name() const
{
    switch (m_mode) {
    case Touch: return QStringLiteral("touch");
    case Couch: return QStringLiteral("couch");
    default: return QStringLiteral("pointer");
    }
}

void Posture::setAllowCouch(bool allow)
{
    if (allow == m_allowCouch) return;
    m_allowCouch = allow;
    update();
}

void Posture::setForced(const QString &value)
{
    if (value == m_forced) return;
    m_forced = value;
    update();
}

bool Posture::eventFilter(QObject *watched, QEvent *event)
{
    // Only window-level events count, so each input is seen once.
    if (!watched->isWindowType()) return false;
    Mode hand = m_lastHand;
    bool seen = false;
    switch (event->type()) {
    case QEvent::TouchBegin:
        hand = Touch; seen = true;
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::Wheel: {
        const auto *pe = static_cast<QSinglePointEvent *>(event);
        // Mouse events made from touch keep the touch posture.
        if (pe->device() && pe->device()->type() == QInputDevice::DeviceType::TouchScreen) break;
        // A resting mouse sends small moves when a window appears; only a press or a move of a few pixels counts.
        static QPointF last;
        const QPointF pos = pe->globalPosition();
        const bool moved = (pos - last).manhattanLength() > 6;
        last = pos;
        if (event->type() == QEvent::MouseMove && !moved) break;
        hand = Pointer; seen = true;
        break;
    }
    case QEvent::KeyPress:
        // Keyboard arrows show the focus ring like a controller, but do not change the posture.
        break;
    default:
        return false;
    }
    if (seen && (hand != m_lastHand || m_controller)) {
        m_lastHand = hand;
        m_controller = false;
        update();
    }
    return false;
}

void Posture::detectContext()
{
    bool couch = m_handheld;
    const QString mode = Palette::instance()->opalMode();
    if (mode == QLatin1String("console")) couch = true;
    if (qEnvironmentVariableIsSet("GAMESCOPE_WAYLAND_DISPLAY") || qgetenv("XDG_CURRENT_DESKTOP").contains("gamescope")) couch = true;
    if (const QScreen *s = QGuiApplication::primaryScreen()) {
        const QSizeF mm = s->physicalSize();
        if (std::hypot(mm.width(), mm.height()) > TvDiagonal) couch = true;
    }
    m_couchContext = couch;
}

void Posture::update()
{
    detectContext();
    Mode mode = m_lastHand;
    if (m_couchContext && m_allowCouch) mode = Couch;
    if (m_forced == QLatin1String("pointer")) mode = Pointer;
    else if (m_forced == QLatin1String("touch")) mode = Touch;
    else if (m_forced == QLatin1String("couch") && m_allowCouch) mode = Couch;
    m_mode = mode;
    emit changed();
}
