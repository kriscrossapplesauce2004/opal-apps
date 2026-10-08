#include <QDir>
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QImage>
#include <QOffscreenSurface>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickGraphicsDevice>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QQuickRenderTarget>
#include <QQuickWindow>
#include <QThread>
#include <rhi/qrhi.h>
#include <memory>

#ifdef OPAL_RENDER_INIT_HOOK
// An app build can link its own backend and register it on the engine before the QML loads.
void OPAL_RENDER_INIT_HOOK(QQmlEngine *engine);
#endif

// Renders one QML file to a PNG with the real GPU scene graph and no visible window:
// opal-render <file.qml> <out.png> <width>x<height> [delay-ms]. Posture and colours come from the usual sources.
int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    if (argc < 4) { qCritical("usage: opal-render file.qml out.png WxH [delay]"); return 2; }
    const QStringList dims = QString::fromLocal8Bit(argv[3]).split(QLatin1Char('x'));
    const QSize size(dims.value(0).toInt(), dims.value(1).toInt());
    const int delay = argc > 4 ? atoi(argv[4]) : 900;

    std::unique_ptr<QOffscreenSurface> fallback(QRhiGles2InitParams::newFallbackSurface());
    QRhiGles2InitParams params;
    params.fallbackSurface = fallback.get();
    std::unique_ptr<QRhi> rhi(QRhi::create(QRhi::OpenGLES2, &params));
    if (!rhi) { qCritical("no GPU context"); return 1; }

    QQuickRenderControl control;
    QQuickWindow window(&control);
    window.setGraphicsDevice(QQuickGraphicsDevice::fromRhi(rhi.get()));
    if (!control.initialize()) { qCritical("render control failed"); return 1; }

    QQmlEngine engine;
    engine.addImportPath(QStringLiteral(OPAL_QML_DIR));
#ifdef OPAL_RENDER_INIT_HOOK
    OPAL_RENDER_INIT_HOOK(&engine);
#endif
    QQmlComponent component(&engine, QUrl::fromLocalFile(QDir::current().absoluteFilePath(QString::fromLocal8Bit(argv[1]))));
    auto *root = qobject_cast<QQuickItem *>(component.create());
    if (!root) { for (const auto &e : component.errors()) qCritical("%s", qPrintable(e.toString())); return 1; }
    window.setGeometry(0, 0, size.width(), size.height());
    window.contentItem()->setSize(size);
    root->setParentItem(window.contentItem());
    root->setSize(size);

    std::unique_ptr<QRhiTexture> texture(rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource));
    texture->create();
    std::unique_ptr<QRhiRenderBuffer> depth(rhi->newRenderBuffer(QRhiRenderBuffer::DepthStencil, size, 1));
    depth->create();
    QRhiTextureRenderTargetDescription desc{QRhiColorAttachment(texture.get())};
    desc.setDepthStencilBuffer(depth.get());
    std::unique_ptr<QRhiTextureRenderTarget> target(rhi->newTextureRenderTarget(desc));
    std::unique_ptr<QRhiRenderPassDescriptor> pass(target->newCompatibleRenderPassDescriptor());
    target->setRenderPassDescriptor(pass.get());
    target->create();
    window.setRenderTarget(QQuickRenderTarget::fromRhiRenderTarget(target.get()));

    // Render frames until the delay ends, so images load and animations settle.
    QElapsedTimer clock;
    clock.start();
    do {
        app.processEvents();
        control.polishItems();
        control.beginFrame();
        control.sync();
        control.render();
        control.endFrame();
        QThread::msleep(16);
    } while (clock.elapsed() < delay);

    QRhiReadbackResult result;
    QRhiResourceUpdateBatch *batch = rhi->nextResourceUpdateBatch();
    batch->readBackTexture(texture.get(), &result);
    control.beginFrame();
    control.commandBuffer()->resourceUpdate(batch);
    control.endFrame();
    QImage image(reinterpret_cast<const uchar *>(result.data.constData()), result.pixelSize.width(), result.pixelSize.height(), QImage::Format_RGBA8888_Premultiplied);
    if (rhi->isYUpInFramebuffer()) image = image.flipped();
    const QString out = QString::fromLocal8Bit(argv[2]);
    const bool ok = image.convertToFormat(QImage::Format_RGB32).save(out);
    delete root;
    return ok ? 0 : 1;
}
