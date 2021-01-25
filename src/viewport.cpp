#include "viewport.h"

#include "CS248.h"

#include <iostream>

namespace CS248
{

  void ViewportImp::set_viewbox(float x, float y, float span)
  {

    // Task 3 (part 2):
    // Set svg to normalized device coordinate transformation. Your input
    // arguments are defined as SVG canvans coordinates.

    this->x = x;
    this->y = y;
    this->span = span;

    // You don't have to reflect here because we're already in the correct orientation due to SVG format.
    std::vector<double> translate{
      1.0, 0.0, span - x,
      0.0, 1.0, span - y,
      0.0, 0.0, 1.0
    };
    double s = 0.5 / span;
    std::vector<double> scale{
      s,   0.0, 0.0,
      0.0, s,   0.0,
      0.0, 0.0, 1.0
    };
    set_canvas_to_norm(Matrix3x3(scale.data()) * Matrix3x3(translate.data()));
  }

  void ViewportImp::update_viewbox(float dx, float dy, float scale)
  {

    this->x -= dx;
    this->y -= dy;
    this->span *= scale;
    set_viewbox(x, y, span);
  }

} // namespace CS248
