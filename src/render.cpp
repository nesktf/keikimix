#include "./render.hpp"

namespace keiki::render {

namespace {

struct quad_data {
  shogle::gl_buffer vbo;
  shogle::gl_buffer ebo;
  shogle::gl_vertex_layout layout;
  shogle::gl_graphics_pipeline pipeline;
  u32 u_proj;
  u32 u_model;
  u32 u_tex;
};

struct render_context {
public:
  render_context(shogle::glfw_win&& win_) : win(std::move(win_)), gl(win), imgui(win) {}

public:
  shogle::glfw_win win;
  shogle::gl_context gl;
  shogle::glfw_imgui imgui;
  optional<quad_data> quad;
  std::vector<shogle::gl_texture> textures;
  shogle::mat4 proj{1.f};
};

optional<render_context> g_ctx;

// clang-format off
constexpr auto vertices = std::to_array<vertex>({
  // pos               // tex
  {{-.5f, -.5f,  0.f}, {0.f, 1.f}},
  {{ .5f, -.5f,  0.f}, {1.f, 1.f}},
  {{ .5f,  .5f,  0.f}, {1.f, 0.f}},
  {{-.5f,  .5f,  0.f}, {0.f, 0.f}},
});
constexpr size_t vbo_size = vertices.size() * sizeof(vertices[0]);

constexpr auto indices = std::to_array<u16>({
  0, 1, 2, // bottom right triangle
  2, 3, 0, // top left triangle
});
constexpr size_t ebo_size = indices.size() * sizeof(indices[0]);
// clang-format on

constexpr std::string_view vert_src = R"glsl(
#version 460 core

layout (location = 0) in vec3 att_pos;
layout (location = 1) in vec2 att_uvs;

layout (location = 0) out vec2 frag_uvs;

uniform mat4 u_proj;
uniform mat4 u_model;

void main() {
  gl_Position = u_proj*u_model*vec4(att_pos, 1.0f);
  frag_uvs = att_uvs;
}  
)glsl";

constexpr std::string_view frag_src = R"glsl(
#version 460 core

layout (location = 0) in vec2 frag_uvs;

layout (location = 0) out vec4 out_color;

uniform sampler2D u_tex;
  
void main() {
  vec4 color = texture(u_tex, frag_uvs);
  if (color.a < 0.01) {
    discard;
  }
  out_color = color;
}
)glsl";

} // namespace

void initialize(u32 win_w, u32 win_h) {
  SHOGLE_ASSERT(!g_ctx.has_value());
  const auto hints = shogle::glfw_gl_hints::make_default(4, 6);
  shogle::glfw_win win(win_w, win_h, "keikimix", hints);
  g_ctx.emplace(std::move(win));

  shogle::gl_vertex_layout layout(g_ctx->gl, shogle::aos_vertex_arg<vertex>{});
  shogle::gl_buffer vbo(g_ctx->gl, shogle::gl_buffer::TYPE_VERTEX, vbo_size);
  vbo.upload_data(g_ctx->gl, vertices.data(), vbo_size, 0).value();
  shogle::gl_buffer ebo(g_ctx->gl, shogle::gl_buffer::TYPE_INDEX, ebo_size);
  ebo.upload_data(g_ctx->gl, indices.data(), ebo_size, 0).value();

  shogle::gl_shader vertex_shader(g_ctx->gl, vert_src, shogle::gl_shader::STAGE_VERTEX);
  shogle::gl_shader fragment_shader(g_ctx->gl, frag_src, shogle::gl_shader::STAGE_FRAGMENT);

  shogle::gl_shader_builder shader_builder;
  const auto pipeline_shaders =
    shader_builder.add_shader(vertex_shader).add_shader(fragment_shader).build();
  shogle::gl_graphics_pipeline pipeline(g_ctx->gl, pipeline_shaders);
  pipeline.set_blending(shogle::gl_blending_props::make_default(true));
  const auto u_model = pipeline.uniform_location(g_ctx->gl, "u_model").value();
  const auto u_proj = pipeline.uniform_location(g_ctx->gl, "u_proj").value();
  const auto u_tex = pipeline.uniform_location(g_ctx->gl, "u_tex").value();
  g_ctx->quad.emplace(vbo, ebo, layout, pipeline, u_proj, u_model, u_tex);

  static constexpr u32 MISSING_TEX_SIZE = 2;

  shogle::gl_texture tex(g_ctx->gl, shogle::gl_texture::TEX_FORMAT_RGBA8,
                         shogle::extent2d(MISSING_TEX_SIZE, MISSING_TEX_SIZE), 1, 1);
  tex.set_sampler_mag(g_ctx->gl, shogle::gl_texture::SAMPLER_MAG_NEAREST);
  tex.set_sampler_min(g_ctx->gl, shogle::gl_texture::SAMPLER_MIN_NEAREST);
  const shogle::gl_texture::image_data missing_image{
    .data = shogle::missing_albedo_bitmap<MISSING_TEX_SIZE>.data(),
    .extent = shogle::extent3d(MISSING_TEX_SIZE, MISSING_TEX_SIZE, 1),
    .format = shogle::gl_texture::PIXEL_FORMAT_RGBA,
    .datatype = shogle::gl_texture::PIXEL_TYPE_U8,
    .alignment = shogle::gl_texture::ALIGN_4BYTES,
  };
  tex.upload_image(g_ctx->gl, missing_image).value();
  tex.generate_mipmaps(g_ctx->gl);
  g_ctx->textures.emplace_back(tex);
}

void start_frame() {
  SHOGLE_ASSERT(g_ctx.has_value());
  shogle::gl_clear_builder clear_builder;
  const auto frame_clear = clear_builder.set_clear_color(.3f, .3f, .3f, 1.f)
                             .set_clear_flag(shogle::gl_clear_opts::CLEAR_COLOR)
                             .build();
  g_ctx->gl.start_frame(frame_clear);
  const auto [w, h] = g_ctx->win.surface_extent();
  g_ctx->proj = shogle::math::ortho(0.f, (f32)w, 0.f, (f32)h);
  g_ctx->imgui.start_frame();
}

void end_frame() {
  g_ctx->imgui.end_frame();
  g_ctx->gl.end_frame();
}

u32 create_texture(const chima::image& image) {
  SHOGLE_ASSERT(g_ctx.has_value());
  SHOGLE_ASSERT(image.depth() == CHIMA_DEPTH_8U);
  SHOGLE_ASSERT(image.channels() == 3 || image.channels() == 4);
  const auto [w, h] = image.extent();
  const auto format = image.channels() == 3 ? shogle::gl_texture::PIXEL_FORMAT_RGB
                                            : shogle::gl_texture::PIXEL_FORMAT_RGBA;
  const shogle::gl_texture::image_data image_data{
    .data = image.data(),
    .extent = shogle::extent3d(w, h, 1),
    .format = format,
    .datatype = shogle::gl_texture::PIXEL_TYPE_U8,
    .alignment = shogle::gl_texture::ALIGN_4BYTES,
  };
  shogle::gl_texture tex(g_ctx->gl, shogle::gl_texture::TEX_FORMAT_RGBA8, shogle::extent2d(w, h));
  tex.upload_image(g_ctx->gl, image_data).value();
  tex.generate_mipmaps(g_ctx->gl);
  g_ctx->textures.emplace_back(tex);
  return (u32)g_ctx->textures.size() - 1;
}

void draw_quad(u32 texture, shogle::vec2 pos, shogle::vec2 scale, f32 rot) {
  SHOGLE_ASSERT(g_ctx.has_value());
  const auto [win_w, win_h] = g_ctx->win.surface_extent();
  auto model = shogle::math::translate(
    shogle::mat4(1.f), shogle::vec3(win_w * .5f + pos.x, win_h * .5f + pos.y, 1.f));
  model = shogle::math::rotate(model, rot, shogle::vec3(0.f, 0.f, 1.f));
  model = shogle::math::scale(model, shogle::vec3(scale.x, scale.y, 1.f));

  shogle::gl_command_builder cmd_builder;
  const auto& quad = *g_ctx->quad;
  const auto cmd = cmd_builder.set_vertex_layout(quad.layout)
                     .set_pipeline(quad.pipeline)
                     .set_index_buffer(quad.ebo, shogle::gl_draw_command::INDEX_FORMAT_U16)
                     .set_draw_count(indices.size())
                     .add_texture(g_ctx->textures[texture], 0)
                     .add_uniform(g_ctx->proj, quad.u_proj)
                     .add_uniform(model, quad.u_model)
                     .add_uniform(0, quad.u_tex)
                     .add_vertex_buffer(quad.vbo)
                     .build();
  g_ctx->gl.submit_command(cmd);
}

void destroy() {
  SHOGLE_ASSERT(g_ctx.has_value());
  g_ctx->imgui.destroy();
  g_ctx.reset();
}

shogle::glfw_win& get_window() {
  SHOGLE_ASSERT(g_ctx.has_value());
  return g_ctx->win;
}

shogle::mat4 get_proj() {
  SHOGLE_ASSERT(g_ctx.has_value());
  return g_ctx->proj;
}

} // namespace keiki::render
