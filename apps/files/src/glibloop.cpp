#include "glibloop.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include <glib.h>

namespace GlibLoop {

static bool s_checked = false;
static bool s_glib = false;
static bool s_fallback = false;

bool checkDispatcher()
{
    if (s_checked)
        return s_glib;
    auto *dispatcher = QAbstractEventDispatcher::instance();
    if (!dispatcher)
        return false;
    s_checked = true;
    s_glib = QByteArray(dispatcher->metaObject()->className()).contains("Glib");
    if (!s_glib) {
        qWarning() << "Qt uses" << dispatcher->metaObject()->className() << "instead of the GLib dispatcher; pumping the GLib context by timer";
        // Polling is the only way to deliver GIO callbacks without the GLib dispatcher.
        auto *timer = new QTimer(QCoreApplication::instance());
        timer->setInterval(5);
        QObject::connect(timer, &QTimer::timeout, timer, [] {
            for (int i = 0; i < 64 && g_main_context_iteration(nullptr, FALSE); ++i) {}
        });
        timer->start();
        s_fallback = true;
    }
    return s_glib;
}

bool usesFallbackPump() { return s_fallback; }

}
