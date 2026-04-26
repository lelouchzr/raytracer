#pragma once
#include "rt/engine.hpp"
#include "rt/render/RenderControl.hpp"

namespace rt {
class RowRenderer {
    const Scene &scene_;
    CameraFrame camera_;

  public:
    explicit RowRenderer(const Scene &);
    void row(int y, RenderControl &) const;
};

void renderImage(const Scene &, RenderControl &, int begin = 0, int end = -1);
} // namespace rt
