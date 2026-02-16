#include "./render.hpp"

#include "./threadpool.hpp"

namespace {

using namespace shogle::numdefs;

struct image_rect {
  f32 xpos, ypos;
  f32 width, height;
  f32 rotation;
};

struct item_data {
  u32 texture;
  image_rect rect;
};

bool check_collision(const image_rect& rect, f32 mouse_x, f32 mouse_y) {
  const f32 s = std::sin(-rect.rotation);
  const f32 c = std::cos(-rect.rotation);
  const f32 dx = mouse_x - rect.xpos;
  const f32 dy = mouse_y - rect.ypos;
  const f32 localX = c * dx - s * dy;
  const f32 localY = s * dx + c * dy;
  const f32 halfW = rect.width / 2.0f;
  const f32 halfH = rect.height / 2.0f;
  const f32 closestX = std::max(-halfW, std::min(localX, halfW));
  const f32 closestY = std::max(-halfH, std::min(localY, halfH));
  const f32 distanceX = localX - closestX;
  const f32 distanceY = localY - closestY;
  const f32 distanceSquared = (distanceX * distanceX) + (distanceY * distanceY);
  return distanceSquared <= 4.f;
}

shogle::vec2 raycast(const shogle::mat4& proj_inv, f32 vp_x, f32 vp_y, f32 x, f32 y) {
  const auto pos =
    proj_inv * shogle::vec4((2.f * x) / vp_x - 1.f, (1.f - (2.f * y)) / vp_y, -1.f, 0.f);
  return {pos.x, pos.y + vp_y * .5f};
}

} // namespace

int main() {
  shogle::logger::set_level(shogle::logger::LEVEL_VERBOSE);
  const auto glfw = shogle::glfw_win::initialize_lib();
  keiki::render::initialize(800, 600);
  const shogle::scope_end render_clean = [&]() {
    keiki::render::destroy();
  };
  auto& win = keiki::render::get_window();

  u32 vp_w = 800;
  u32 vp_h = 600;
  shogle::mat4 proj_inv =
    shogle::math::inverse(shogle::math::ortho(0.f, (f32)vp_w, 0.f, (f32)vp_h));
  shogle::vec2 mouse_pos(1.f, 1.f);
  win.set_cursor_pos_callback([&](auto, f64 xoff, f64 yoff) {
    mouse_pos = raycast(proj_inv, vp_w, vp_h, (f32)xoff, (f32)yoff);
    shogle::logger::log_info("main", "{} {}", mouse_pos.x, mouse_pos.y);
  });
  win.set_viewport_callback([&](auto, const shogle::extent2d& ext) {
    vp_w = ext.width;
    vp_h = ext.height;
    proj_inv = shogle::math::inverse(shogle::math::ortho(0.f, (f32)vp_w, 0.f, (f32)vp_h));
  });
  bool do_thing = false;
  win.set_key_input_callback([&](auto, const shogle::glfw_key_data& key) {
    if (key.key == GLFW_KEY_SPACE && key.action == GLFW_PRESS) {
      do_thing = !do_thing;
    }
  });

  chima::context chima;
  std::queue<std::function<void()>> tasks;
  bool loading = false;
  std::mutex load_mtx;
  keiki::thread_pool pool;

  std::vector<item_data> items;
  items.emplace_back(0, image_rect(0.f, 0.f, 250.f, 250.f, 0.f));
  // items.emplace_back(cirno_tex, shogle::vec2{100, 100}, shogle::vec2{250, 250});

  f32 t = 0.f;
  const auto draw_things = [&]() {
    items[0].rect.rotation = t * shogle::math::pi<f32>;
    for (const auto& item : items) {
      shogle::vec2 scale(item.rect.width, item.rect.height);
      shogle::vec2 pos(item.rect.xpos, item.rect.ypos);
      f32 rot = item.rect.rotation;
      if (check_collision(item.rect, mouse_pos.x, mouse_pos.y)) {
        scale *= 1.2f;
      }
      keiki::render::draw_quad(item.texture, pos, scale, rot);
    }
  };

  const auto draw_ui = [&]() {
    static char buff[128] = "./lib/shogle/demos/res/cirno_cpp.jpg";
    ImGui::ShowDemoWindow();
    ImGui::Begin("loader");
    ImGui::InputText("path", buff, sizeof(buff), 0);
    if (ImGui::Button("load")) {
      loading = true;
      pool.enqueue([&]() {
        chima::error err;
        auto image = chima::image::load(chima, CHIMA_DEPTH_8U, buff, &err);
        std::unique_lock lock(load_mtx);
        if (image) {
          tasks.emplace([&chima, &loading, &items, image = *image]() mutable {
            auto tex = keiki::render::create_texture(image);
            f32 aspect = (f32)image.get().extent.width / (f32)image.get().extent.height;
            items.emplace_back(tex, image_rect(100, 100, 400.f * aspect, 400.f, 0.f));
            chima::image::destroy(chima, image);
            loading = false;
          });
        } else {
          shogle::logger::log_error("loader", "{}", err.what());
          loading = false;
        }
      });
    }
    if (loading) {
      ImGui::ProgressBar(-1.f * (f32)ImGui::GetTime(), ImVec2(0.f, 0.f), "Loading");
    }
    ImGui::End();
  };

  shogle::render_loop(win, [&](f64 dt) {
    if (do_thing) {
      t += (f32)dt;
    }
    if (win.poll_key(GLFW_KEY_ESCAPE) == GLFW_PRESS) {
      win.close();
    }

    while (!tasks.empty()) {
      tasks.front()();
      tasks.pop();
    }

    keiki::render::start_frame();
    draw_things();
    draw_ui();
    keiki::render::end_frame();
  });

  return EXIT_SUCCESS;
}
