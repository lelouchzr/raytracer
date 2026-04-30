#include "rt/render/RowRenderer.hpp"

namespace rt {
void bonusRender(const Scene &s) {
    RenderControl control(s.settings.width, s.settings.height);
    renderImage(s, control);
    if (!control.cancelled())
        writePpm(s, control.snapshot());
}
} // namespace rt
