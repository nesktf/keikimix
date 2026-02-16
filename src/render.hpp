#pragma once

#include "./common.hpp"

#include <shogle/math/transform.hpp>
#include <shogle/render/data.hpp>
#include <shogle/render/opengl.hpp>
#include <shogle/render/window.hpp>

#include <chimatools/chimatools.hpp>

namespace keiki::render {

struct vertex {
public:
  static constexpr u32 attribute_count = 2u;
  static constexpr inline auto attributes() noexcept;

public:
  shogle::vec3 pos;
  shogle::vec2 uvs;
};

constexpr inline auto vertex::attributes() noexcept {
  return std::to_array<shogle::vertex_attribute>({
    {.location = 0, .type = shogle::attribute_type::vec3, .offset = offsetof(vertex, pos)},
    {.location = 1, .type = shogle::attribute_type::vec2, .offset = offsetof(vertex, uvs)},
  });
};

void initialize(u32 win_w, u32 win_h);

void start_frame();
void draw_quad(u32 texture, shogle::vec2 pos, shogle::vec2 scale, f32 rot);
u32 create_texture(const chima::image& image);
void end_frame();

void destroy();

shogle::glfw_win& get_window();

shogle::mat4 get_proj();

} // namespace keiki::render
