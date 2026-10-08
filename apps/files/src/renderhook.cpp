#include "glibloop.h"
#include "thumbnailer.h"

#include <QQmlEngine>

// Called by the render tool before the QML loads, so the debug UI can show thumbnails.
void opalFilesRenderInit(QQmlEngine *engine)
{
    GlibLoop::checkDispatcher();
    Thumbnailer::attach(engine);
}
