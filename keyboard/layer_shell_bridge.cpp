#include <LayerShellQt/Window>
#include <QSize>
#include <QWindow>

extern "C" bool configure_keyboard_layer(void *window_pointer, int height)
{
    auto *qt_window = static_cast<QWindow *>(window_pointer);
    if (!qt_window || height <= 0) {
        return false;
    }

    auto *layer_window = LayerShellQt::Window::get(qt_window);
    if (!layer_window) {
        return false;
    }

    LayerShellQt::Window::Anchors anchors;
    anchors |= LayerShellQt::Window::AnchorBottom;
    anchors |= LayerShellQt::Window::AnchorLeft;
    anchors |= LayerShellQt::Window::AnchorRight;
    layer_window->setAnchors(anchors);
    layer_window->setLayer(LayerShellQt::Window::LayerOverlay);
    layer_window->setKeyboardInteractivity(
        LayerShellQt::Window::KeyboardInteractivityNone
    );
    layer_window->setActivateOnShow(false);
    layer_window->setExclusiveEdge(LayerShellQt::Window::AnchorBottom);
    layer_window->setExclusiveZone(height);
    layer_window->setDesiredSize(QSize(0, height));
    layer_window->setScope(QStringLiteral("moonlight-controller-keyboard"));
    return true;
}
