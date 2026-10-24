#include "glibloop.h"
#include "thumbnailer.h"

#include <QQmlEngine>

// Called by the render tool before the QML loads, so the interface can show thumbnails.
void opalPhotosRenderInit(QQmlEngine *engine)
{
    GlibLoop::checkDispatcher();
    Thumbnailer::attach(engine);
}
