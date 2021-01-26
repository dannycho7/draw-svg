#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CS248 {


// Implements SoftwareRenderer //

// fill a sample location with color
void SoftwareRendererImp::fill_sample(int sx, int sy, const Color &color) {
  // check bounds
  if (sx < 0 || sx >= supersample_w)
    return;
  if (sy < 0 || sy >= supersample_h)
    return;

  float inv255 = 1.0 / 255.0;
  Color pixel_color;

	pixel_color.r = supersample_target[4 * (sx + sy * supersample_w)] * inv255;
	pixel_color.g = supersample_target[4 * (sx + sy * supersample_w) + 1] * inv255;
	pixel_color.b = supersample_target[4 * (sx + sy * supersample_w) + 2] * inv255;
	pixel_color.a = supersample_target[4 * (sx + sy * supersample_w) + 3] * inv255;
  pixel_color = ref->alpha_blending_helper(pixel_color, color);
  supersample_target[4 * (sx + sy * supersample_w)] = (uint8_t)(pixel_color.r * 255);
  supersample_target[4 * (sx + sy * supersample_w) + 1] = (uint8_t)(pixel_color.g * 255);
  supersample_target[4 * (sx + sy * supersample_w) + 2] = (uint8_t)(pixel_color.b * 255);
  supersample_target[4 * (sx + sy * supersample_w) + 3] = (uint8_t)(pixel_color.a * 255);
}

// fill samples in the entire pixel specified by pixel coordinates
void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color)
{
  // Task 2: Re-implement this function
  for (int i = 0; i < sample_rate; ++i)
  {
    for (int j = 0; j < sample_rate; ++j)
    {
      int sx = x * sample_rate + i;
      int sy = y * sample_rate + j;
      fill_sample(sx, sy, color);
    }
  }
}

void SoftwareRendererImp::draw_svg(SVG &svg)
{

  // set top level transformation
  transformation = canvas_to_screen;
  transformations.push(transformation);
  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y--;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y++;

  transformations.pop();

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  resolve();
}

void SoftwareRendererImp::set_sample_rate(size_t sample_rate)
{

  // Task 2:
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;
  this->supersample_w = target_w * sample_rate;
  this->supersample_h = target_h * sample_rate;
  this->supersample_target.assign(4 * supersample_w * supersample_h, 255);
}

void SoftwareRendererImp::set_render_target(unsigned char *render_target,
                                            size_t width, size_t height)
{

  // Task 2:
  // You may want to modify this for supersampling support
  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
  this->supersample_w = target_w * sample_rate;
  this->supersample_h = target_h * sample_rate;
  this->supersample_target.assign(4 * supersample_w * supersample_h, 255);
}

void SoftwareRendererImp::draw_element(SVGElement *element)
{

  // Task 3 (part 1):
  // Modify this to implement the transformation stack
  transformation = transformations.top() * element->transform;
  transformations.push(transformation);

  switch (element->type)
  {
  case POINT:
		draw_point(static_cast<Point&>(*element));
		break;
	case LINE:
		draw_line(static_cast<Line&>(*element));
		break;
	case POLYLINE:
		draw_polyline(static_cast<Polyline&>(*element));
		break;
	case RECT:
		draw_rect(static_cast<Rect&>(*element));
		break;
	case POLYGON:
		draw_polygon(static_cast<Polygon&>(*element));
		break;
	case ELLIPSE:
		draw_ellipse(static_cast<Ellipse&>(*element));
		break;
	case IMAGE:
		draw_image(static_cast<Image&>(*element));
		break;
	case GROUP:
		draw_group(static_cast<Group&>(*element));
		break;
	default:
		break;
  }
  transformations.pop();
  transformation = transformations.top();
}

// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= target_w) return;
  if (sy < 0 || sy >= target_h) return;

  // fill sample - NOT doing alpha blending!
  fill_pixel(sx, sy, color);
}

void SoftwareRendererImp::rasterize_line(float x0, float y0,
                                         float x1, float y1,
                                         Color color)
{

  // Extra credit (delete the line below and implement your own)
  ref->rasterize_line_helper(x0, y0, x1, y1, target_w, target_h, color, this);
}

void SoftwareRendererImp::rasterize_triangle(float x0, float y0,
                                             float x1, float y1,
                                             float x2, float y2,
                                             Color color)
{
  // Task 1:
  // Implement triangle rasterization (you may want to call fill_sample here)
  std::vector<std::pair<float, float>> P{{x0, y0}, {x1, y1}, {x2, y2}};
  auto L = [&P](int i, float x, float y) -> float {
    auto p_i = P.at(i);
    auto p_i1 = P.at((i + 1) % P.size());
    double A = p_i1.second - p_i.second;
    double B = p_i1.first - p_i.first;
    double C = p_i.second * B - p_i.first * A;
    return A * x - B * y + C;
  };

  if (L(0, x2, y2) > 0)
  {
    // TODO: Did this line not matter?
    P = {{x0, y0}, {x2, y2}, {x1, y1}};
  }
  float sample_inc = 1 / static_cast<float>(this->sample_rate);  
  int min_x = floor(min(x0, min(x1, x2)));
  int max_x = ceil(max(x0, max(x1, x2)));
  int min_y = floor(min(y0, min(y1, y2)));
  int max_y = ceil(max(y0, max(y1, y2)));
  for (int x = min_x; x < max_x; ++x)
  {
    for (int y = min_y; y < max_y; ++y)
    {
      for (int i = 0; i < sample_rate; ++i)
      {
        for (int j = 0; j < sample_rate; ++j)
        {
          float sx = x + sample_inc * (i + 0.5);
          float sy = y + sample_inc * (j + 0.5);
          bool inside = L(0, sx, sy) <= 0 && L(1, sx, sy) <= 0 && L(2, sx, sy) <= 0;
          if (inside)
          {
            fill_sample(x * sample_rate + i, y * sample_rate + j, color);
          }
        }
      }
    }
  }
}

void SoftwareRendererImp::rasterize_image(float x0, float y0,
                                          float x1, float y1,
                                          Texture &tex)
{
  // Task 4: 
  // Implement image rasterization (you may want to call fill_sample here)
  int n_sx = floor((x1 - x0) * sample_rate);
  int n_sy = floor((y1 - y0) * sample_rate);

  float sample_inc = 1 / static_cast<float>(this->sample_rate);
  for (int i = 0; i < n_sx; ++i)
  {
    for (int j = 0; j < n_sy; ++j)
    {
      float sx = x0 + sample_inc * (i + 0.5);
      float sy = y0 + sample_inc * (j + 0.5);
      float u = (sx - x0) / (x1 - x0);
      float v = (sy - y0) / (y1 - y0);
      fill_sample(sx, sy, sampler->sample_nearest(tex, u, v));
    }
  }
}

// resolve samples to render target
void SoftwareRendererImp::resolve(void)
{
  // Task 2:d
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 2".
  for (int x = 0; x < target_w; ++x)
  {
    for (int y = 0; y < target_h; ++y)
    {
      int r = 0, g = 0, b = 0, a = 0;
      for (int i = 0; i < sample_rate; ++i)
      {
        for (int j = 0; j < sample_rate; ++j)
        {
          int sx = x * sample_rate + i;
          int sy = y * sample_rate + j;
          r += supersample_target[4 * (sx + sy * supersample_w)];
          g += supersample_target[4 * (sx + sy * supersample_w) + 1];
          b += supersample_target[4 * (sx + sy * supersample_w) + 2];
          a += supersample_target[4 * (sx + sy * supersample_w) + 3];
        }
      }
      render_target[4 * (x + y * target_w)] = r / pow(sample_rate, 2);
      render_target[4 * (x + y * target_w) + 1] = g / pow(sample_rate, 2);
      render_target[4 * (x + y * target_w) + 2] = b / pow(sample_rate, 2);
      render_target[4 * (x + y * target_w) + 3] = a / pow(sample_rate, 2);
    }
  }
}

Color SoftwareRendererImp::alpha_blending(Color pixel_color, Color color)
{
  // Task 5
  // Implement alpha compositing
  return pixel_color;
}

} // namespace CS248
