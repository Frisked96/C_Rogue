#include "climate_hydrology.hpp"
#include "../game_map.hpp"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace hydro {

namespace {
constexpr float PI = 3.14159265358979323846f;

// Cached hardware thread count — evaluated once, reused everywhere.
inline int thread_count() {
  static const int n = [] {
    const int c = (int)std::thread::hardware_concurrency();
    return c > 0 ? c : 1;
  }();
  return n;
}

// -----------------------------------------------------------------------
// Minimal persistent thread pool to replace std::async. Avoids OS-level
// thread creation overhead per substep.
// -----------------------------------------------------------------------
class ThreadPool {
public:
  ThreadPool(size_t threads) : stop(false) {
    for (size_t i = 0; i < threads; ++i) {
      workers.emplace_back([this] {
        for (;;) {
          std::function<void()> task;
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->condition.wait(
                lock, [this] { return this->stop || !this->tasks.empty(); });
            if (this->stop && this->tasks.empty())
              return;
            task = std::move(this->tasks.front());
            this->tasks.pop();
          }
          task();
          {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            active_tasks--;
            if (tasks.empty() && active_tasks == 0) {
              wait_condition.notify_all();
            }
          }
        }
      });
    }
  }

  template <class F> void enqueue(F &&f) {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      tasks.emplace(std::forward<F>(f));
      active_tasks++;
    }
    condition.notify_one();
  }

  void wait() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    wait_condition.wait(lock,
                        [this] { return tasks.empty() && active_tasks == 0; });
  }

  ~ThreadPool() {
    {
      std::unique_lock<std::mutex> lock(queue_mutex);
      stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers)
      worker.join();
  }

private:
  std::vector<std::thread> workers;
  std::queue<std::function<void()>> tasks;
  std::mutex queue_mutex;
  std::condition_variable condition;
  std::condition_variable wait_condition;
  bool stop;
  size_t active_tasks = 0;
};

inline ThreadPool &get_pool() {
  static ThreadPool pool(thread_count());
  return pool;
}

// Helper for 2D grid parallelization. Passes thread_id, start_y, end_y to
// lambda.
template <typename Func> void parallel_for_2d(int height, Func f) {
  ThreadPool &pool = get_pool();
  int num_threads = thread_count();
  int chunk_size = (height + num_threads - 1) / num_threads;

  for (int t = 0; t < num_threads; ++t) {
    int start_y = t * chunk_size;
    int end_y = std::min(height, (t + 1) * chunk_size);
    if (start_y >= height)
      break;
    pool.enqueue([t, start_y, end_y, f]() { f(t, start_y, end_y); });
  }
  pool.wait();
}

// Mutable reference into the map's tile storage, mirroring the
// const_cast<Tile&>(get_tile(...)) pattern used by the original simulator
// for in-place numeric edits (no material change -> no set_tile needed).
inline Tile &mtile(Game_map &map, int x, int y, int z) {
  return const_cast<Tile &>(map.get_tile(x, y, z));
}
} // namespace

// -----------------------------------------------------------------------
float qsat(float temperature_K, const Params &p) {
  return p.qsat_ref * std::exp(p.qsat_k * (temperature_K - p.sea_level_temp_K));
}

// -----------------------------------------------------------------------
std::vector<int> compute_ground_heightmap(const Game_map &map) {
  int width = map.get_width();
  int height = map.get_height();
  int depth = map.get_depth();

  std::vector<int> ground_z((std::size_t)width * height, 0);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int gz = 0; // default: assume bedrock at z=0 if nothing else found
      for (int z = depth - 1; z >= 0; --z) {
        MaterialType m = map.get_tile(x, y, z).material;
        if (m != MaterialType::AIR) {
          gz = z;
          break;
        }
      }
      ground_z[(std::size_t)y * width + x] = gz;
    }
  }
  return ground_z;
}

// -----------------------------------------------------------------------
std::vector<float> compute_soil_variation(int width, int height,
                                          NoiseGen &noise, const Params &p) {
  std::vector<float> variation((std::size_t)width * height, 1.0f);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      float n = noise.fbm2D(x * 0.15f + 500.0f, y * 0.15f + 500.0f, 3);
      variation[(std::size_t)y * width + x] =
          1.0f + p.soil_variation_amplitude * n;
    }
  }
  return variation;
}

// =========================================================================
// ClimateSystem
// =========================================================================
void ClimateSystem::init(int width, int height, const Params &params) {
  w_ = width;
  h_ = height;
  params_ = params;
  cells_.assign((std::size_t)w_ * h_, ClimateCell{});
  advect_delta_.assign((std::size_t)w_ * h_, 0.0f);
  advect_seed_delta_.assign((std::size_t)w_ * h_, 0.0f);
  precip_buf_.assign((std::size_t)w_ * h_, 0.0f);
}

void ClimateSystem::initialize(const std::vector<int> &ground_z,
                               NoiseGen &noise) {
  for (int y = 0; y < h_; ++y) {
    for (int x = 0; x < w_; ++x) {
      ClimateCell &c = cells_[idx(x, y)];
      c.temperature = params_.sea_level_temp_K - params_.lapse_rate_K_per_tile *
                                                     (float)ground_z[idx(x, y)];
      c.vapor = 0.5f *
                qsat(c.temperature, params_); // start at ~50% relative humidity
      c.wind_u = 0.0f;
      c.wind_v = 0.0f;
      c.upslope = 0.0f;
    }
  }
  update_wind(ground_z, noise, 0.0f);
}

void ClimateSystem::update_wind(const std::vector<int> &ground_z,
                                NoiseGen &noise, float season_phase) {
  float base_angle = season_phase * 2.0f * PI;
  float base_u = std::cos(base_angle) * 1.5f;
  float base_v = std::sin(base_angle) * 1.5f;

  auto elev = [&](int x, int y) -> float {
    x = std::clamp(x, 0, w_ - 1);
    y = std::clamp(y, 0, h_ - 1);
    return (float)ground_z[idx(x, y)];
  };

  parallel_for_2d(
      h_, [this, base_u, base_v, elev, noise](int, int start_y, int end_y) {
        NoiseGen local_noise = noise; // Thread-local copy
        for (int y = start_y; y < end_y; ++y) {
          for (int x = 0; x < w_; ++x) {
            float nu = local_noise.fbm2D((float)x * 0.015f + 1000.0f,
                                         (float)y * 0.015f + 1000.0f, 3);
            float nv = local_noise.fbm2D((float)x * 0.015f - 2000.0f,
                                         (float)y * 0.015f - 3000.0f, 3);

            float u0 = base_u + nu;
            float v0 = base_v + nv;
            float speed = std::sqrt(u0 * u0 + v0 * v0);
            if (speed < 1e-5f) {
              ClimateCell &c = cells_[idx(x, y)];
              c.wind_u = 0.0f;
              c.wind_v = 0.0f;
              c.upslope = 0.0f;
              continue;
            }

            float gx = (elev(x + 1, y) - elev(x - 1, y)) * 0.5f;
            float gy = (elev(x, y + 1) - elev(x, y - 1)) * 0.5f;
            float slope = std::sqrt(gx * gx + gy * gy);

            float steepness = std::clamp(slope / 4.0f, 0.0f, 1.0f);

            float wu = u0, wv = v0;
            if (slope > 1e-5f) {
              float cx = -gy, cy = gx;
              float dirx = u0 / speed, diry = v0 / speed;
              if (cx * dirx + cy * diry < 0.0f) {
                cx = -cx;
                cy = -cy;
              }
              float clen = std::sqrt(cx * cx + cy * cy);
              cx /= clen;
              cy /= clen;

              float bx = dirx * (1.0f - steepness) + cx * steepness;
              float by = diry * (1.0f - steepness) + cy * steepness;
              float blen = std::sqrt(bx * bx + by * by);
              if (blen > 1e-5f) {
                wu = bx / blen * speed;
                wv = by / blen * speed;
              }
            }

            ClimateCell &c = cells_[idx(x, y)];
            c.wind_u = wu;
            c.wind_v = wv;
            c.upslope = gx * wu + gy * wv;
          }
        }
      });
}

void ClimateSystem::update_temperature(const std::vector<int> &ground_z,
                                       float season_phase) {
  float seasonal =
      params_.seasonal_amplitude_K * std::sin(season_phase * 2.0f * PI);

  parallel_for_2d(h_, [this, seasonal, &ground_z](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < w_; ++x) {
        ClimateCell &c = cells_[idx(x, y)];
        c.temperature =
            params_.sea_level_temp_K + seasonal -
            params_.lapse_rate_K_per_tile * (float)ground_z[idx(x, y)];
      }
    }
  });
}

void ClimateSystem::advect() {
  std::fill(advect_delta_.begin(), advect_delta_.end(), 0.0f);
  std::fill(advect_seed_delta_.begin(), advect_seed_delta_.end(), 0.0f);

  // Gather-based advection: pulls mass from upwind neighbors.
  // This is perfectly thread-safe as cells only write to their own index.
  parallel_for_2d(h_, [this](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < w_; ++x) {
        const ClimateCell &c = cells_[idx(x, y)];

        float u = c.wind_u, v = c.wind_v;
        float speed = std::sqrt(u * u + v * v);
        float move_frac =
            std::clamp(speed * params_.wind_advect_scale, 0.0f, 0.9f);

        float out_x = 0.0f, out_y = 0.0f;
        float seed_out_x = 0.0f, seed_out_y = 0.0f;
        
        if (move_frac > 0.0f) {
          float au = std::fabs(u), av = std::fabs(v);
          float denom = au + av;
          if (denom >= 1e-6f) {
            if (c.vapor > 0.0f) {
              out_x = c.vapor * move_frac * (au / denom);
              out_y = c.vapor * move_frac * (av / denom);
            }
            if (c.seed_factor > 0.0f) {
              seed_out_x = c.seed_factor * move_frac * (au / denom);
              seed_out_y = c.seed_factor * move_frac * (av / denom);
            }
          }
        }

        float in_x = 0.0f, in_y = 0.0f;
        float seed_in_x = 0.0f, seed_in_y = 0.0f;

        // Inflow from X neighbors
        if (x > 0) {
          const ClimateCell &p = cells_[idx(x - 1, y)];
          if (p.wind_u > 0.0f) { // Blowing right (towards current cell)
            float p_speed =
                std::sqrt(p.wind_u * p.wind_u + p.wind_v * p.wind_v);
            float p_move =
                std::clamp(p_speed * params_.wind_advect_scale, 0.0f, 0.9f);
            float p_au = std::fabs(p.wind_u), p_av = std::fabs(p.wind_v);
            float p_denom = p_au + p_av;
            if (p_move > 0.0f && p_denom >= 1e-6f) {
              if (p.vapor > 0.0f) in_x += p.vapor * p_move * (p_au / p_denom);
              if (p.seed_factor > 0.0f) seed_in_x += p.seed_factor * p_move * (p_au / p_denom);
            }
          }
        }
        if (x < w_ - 1) {
          const ClimateCell &p = cells_[idx(x + 1, y)];
          if (p.wind_u < 0.0f) { // Blowing left (towards current cell)
            float p_speed =
                std::sqrt(p.wind_u * p.wind_u + p.wind_v * p.wind_v);
            float p_move =
                std::clamp(p_speed * params_.wind_advect_scale, 0.0f, 0.9f);
            float p_au = std::fabs(p.wind_u), p_av = std::fabs(p.wind_v);
            float p_denom = p_au + p_av;
            if (p_move > 0.0f && p_denom >= 1e-6f) {
              if (p.vapor > 0.0f) in_x += p.vapor * p_move * (p_au / p_denom);
              if (p.seed_factor > 0.0f) seed_in_x += p.seed_factor * p_move * (p_au / p_denom);
            }
          }
        }

        // Inflow from Y neighbors
        if (y > 0) {
          const ClimateCell &p = cells_[idx(x, y - 1)];
          if (p.wind_v > 0.0f) {
            float p_speed =
                std::sqrt(p.wind_u * p.wind_u + p.wind_v * p.wind_v);
            float p_move =
                std::clamp(p_speed * params_.wind_advect_scale, 0.0f, 0.9f);
            float p_au = std::fabs(p.wind_u), p_av = std::fabs(p.wind_v);
            float p_denom = p_au + p_av;
            if (p_move > 0.0f && p_denom >= 1e-6f) {
              if (p.vapor > 0.0f) in_y += p.vapor * p_move * (p_av / p_denom);
              if (p.seed_factor > 0.0f) seed_in_y += p.seed_factor * p_move * (p_av / p_denom);
            }
          }
        }
        if (y < h_ - 1) {
          const ClimateCell &p = cells_[idx(x, y + 1)];
          if (p.wind_v < 0.0f) {
            float p_speed =
                std::sqrt(p.wind_u * p.wind_u + p.wind_v * p.wind_v);
            float p_move =
                std::clamp(p_speed * params_.wind_advect_scale, 0.0f, 0.9f);
            float p_au = std::fabs(p.wind_u), p_av = std::fabs(p.wind_v);
            float p_denom = p_au + p_av;
            if (p_move > 0.0f && p_denom >= 1e-6f) {
              if (p.vapor > 0.0f) in_y += p.vapor * p_move * (p_av / p_denom);
              if (p.seed_factor > 0.0f) seed_in_y += p.seed_factor * p_move * (p_av / p_denom);
            }
          }
        }

        advect_delta_[idx(x, y)] = c.vapor + in_x + in_y - out_x - out_y;
        advect_seed_delta_[idx(x, y)] = c.seed_factor + seed_in_x + seed_in_y - seed_out_x - seed_out_y;
      }
    }
  });

  // Apply advection delta
  for (std::size_t i = 0; i < cells_.size(); ++i) {
    cells_[i].vapor = std::max(0.0f, advect_delta_[i]);
    cells_[i].seed_factor = std::max(0.0f, advect_seed_delta_[i]);
  }

  // Windward-boundary inflow (serial, very cheap)
  // Seeds don't inflow from boundaries
  for (int y = 0; y < h_; ++y) {
    ClimateCell &left = cells_[idx(0, y)];
    if (left.wind_u > 0.0f)
      left.vapor +=
          params_.boundary_relax * (params_.ocean_humidity - left.vapor);

    ClimateCell &right = cells_[idx(w_ - 1, y)];
    if (right.wind_u < 0.0f)
      right.vapor +=
          params_.boundary_relax * (params_.ocean_humidity - right.vapor);
  }
  for (int x = 0; x < w_; ++x) {
    ClimateCell &bottom = cells_[idx(x, 0)];
    if (bottom.wind_v > 0.0f)
      bottom.vapor +=
          params_.boundary_relax * (params_.ocean_humidity - bottom.vapor);

    ClimateCell &top = cells_[idx(x, h_ - 1)];
    if (top.wind_v < 0.0f)
      top.vapor +=
          params_.boundary_relax * (params_.ocean_humidity - top.vapor);
  }
  for (auto &c : cells_) {
    c.vapor = std::max(0.0f, c.vapor);
  }
}

const std::vector<float> &ClimateSystem::step_precipitation(NoiseGen &noise,
                                                            float day_index) {
  std::fill(precip_buf_.begin(), precip_buf_.end(), 0.0f);

  parallel_for_2d(h_, [this, day_index, &noise](int, int start_y, int end_y) {
    NoiseGen local_noise = noise; // Thread-local copy
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < w_; ++x) {
        ClimateCell &c = cells_[idx(x, y)];

        float t_eff = c.temperature - params_.orographic_gain * c.upslope;
        float cap = qsat(t_eff, params_);

        float rain = 0.0f;
        float excess = c.vapor - cap;
        if (excess > 0.0f) {
          rain = excess * params_.rain_out_fraction;
          c.vapor -= rain;
        }

        float storm =
            local_noise.fbm2D((float)x * 0.07f + day_index * 0.31f,
                              (float)y * 0.07f - day_index * 0.17f, 2);
        if (storm > params_.convective_threshold) {
          const float span = 1.0f - params_.convective_threshold;
          float intensity = (span > 1e-6f)
                                ? (storm - params_.convective_threshold) / span
                                : 1.0f;
          float convective = params_.convective_intensity * intensity;
          float draw = std::min(c.vapor, convective);
          c.vapor -= draw;
          rain += draw;
        }

        c.vapor = std::max(0.0f, c.vapor);
        precip_buf_[idx(x, y)] = rain;
      }
    }
  });

  return precip_buf_;
}

void ClimateSystem::add_vapor(int x, int y, float amount) {
  ClimateCell &c = cells_[idx(x, y)];
  c.vapor = std::max(0.0f, c.vapor + amount);
}

// =========================================================================
// GroundwaterGrid
// =========================================================================
void GroundwaterGrid::init(int width, int height, const Params &params) {
  w_ = width;
  h_ = height;
  params_ = params;
  table_.assign((std::size_t)w_ * h_, 0.0f);
  discharge_buf_.assign((std::size_t)w_ * h_, 0.0f);
  new_table_buf_.assign((std::size_t)w_ * h_, 0.0f);
}

void GroundwaterGrid::initialize(const std::vector<int> &ground_z,
                                 NoiseGen &noise) {
  for (int y = 0; y < h_; ++y) {
    for (int x = 0; x < w_; ++x) {
      float gz = (float)ground_z[idx(x, y)];
      float n = noise.fbm01((float)x * 0.03f + 7000.0f,
                            (float)y * 0.03f + 7000.0f, 4); // [0,1]
      float frac = 0.3f + 0.8f * n;
      table_[idx(x, y)] = gz * frac;
    }
  }
}

void GroundwaterGrid::recharge(int x, int y, float volume) {
  if (volume <= 0.0f)
    return;
  table_[idx(x, y)] += volume / params_.groundwater_porosity;
}

const std::vector<float> &
GroundwaterGrid::update(const std::vector<int> &ground_z, const Game_map &map) {
  std::fill(discharge_buf_.begin(), discharge_buf_.end(), 0.0f);

  parallel_for_2d(h_, [this, &ground_z, &map](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < w_; ++x) {
        int i = idx(x, y);
        int gz = std::max(0, ground_z[i]);
        int z = std::clamp((int)std::lround(table_[i]), 0, gz);

        float perm = map.get_tile(x, y, z).mat().permeability;
        float diffusion_rate = params_.groundwater_diffusion_rate *
                               (0.1f + 0.9f * std::clamp(perm, 0.0f, 1.0f));

        float sum = 0.0f;
        int nxs[4] = {x - 1, x + 1, x, x};
        int nys[4] = {y, y, y - 1, y + 1};
        for (int k = 0; k < 4; ++k) {
          int cx = std::clamp(nxs[k], 0, w_ - 1);
          int cy = std::clamp(nys[k], 0, h_ - 1);
          sum += table_[idx(cx, cy)];
        }
        float avg = sum * 0.25f;

        new_table_buf_[i] = table_[i] + diffusion_rate * (avg - table_[i]);
      }
    }
  });

  table_.swap(new_table_buf_);

  // Calculate baseflow / springs
  parallel_for_2d(h_, [this, &ground_z](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < w_; ++x) {
        int i = idx(x, y);
        float gz = (float)ground_z[i];
        if (table_[i] > gz) {
          float excess_height = table_[i] - gz;
          discharge_buf_[i] = excess_height * params_.groundwater_porosity;
          table_[i] = gz;
        }
        if (table_[i] < 0.0f)
          table_[i] = 0.0f;
      }
    }
  });

  return discharge_buf_;
}

// =========================================================================
// Surface / soil routines
// =========================================================================
void add_surface_water(Game_map &map, int x, int y, int ground_z, float depth,
                       const Params &p) {
  (void)p;
  if (depth <= 0.0f)
    return;
  int depth_map = map.get_depth();

  int z = ground_z + 1;
  float remaining = depth;

  while (remaining > 1e-6f && z < depth_map) {
    Tile &t = mtile(map, x, y, z);

    float cap = t.water_capacity(); // AIR both report 1.0
    float space = std::max(0.0f, cap - t.state.liquid_volume);
    float add = std::min(remaining, space);
    t.state.liquid_volume += add;
    remaining -= add;

    if (remaining <= 1e-6f)
      break;
    ++z;
  }
}

void apply_precipitation(Game_map &map, const std::vector<int> &ground_z,
                         const std::vector<float> &precip_depth,
                         const Params &p) {
  int width = map.get_width();
  int height = map.get_height();

  parallel_for_2d(height, [&](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        float rain = precip_depth[i];
        if (rain <= 0.0f)
          continue;

        int gz = ground_z[i];
        Tile &t = mtile(map, x, y, gz);

        float cap = t.water_capacity();
        float space = std::max(0.0f, cap - t.state.liquid_volume);
        float add = std::min(rain, space);
        t.state.liquid_volume += add;

        float overflow = rain - add;
        if (overflow > 1e-6f) {
          add_surface_water(map, x, y, gz, overflow, p);
        }
      }
    }
  });
}

void soil_percolation_step(Game_map &map, const std::vector<int> &ground_z,
                           const std::vector<float> &soil_variation,
                           const Params &p, GroundwaterGrid &groundwater) {
  int width = map.get_width();
  int height = map.get_height();
  int depth_map = map.get_depth();

  parallel_for_2d(height, [&](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        int gz = ground_z[i];

        // --- Pond seepage into ground ---
        if (gz + 1 < depth_map) {
          Tile &pond = mtile(map, x, y, gz + 1);
          if (pond.material == MaterialType::AIR &&
              pond.state.liquid_volume > 1e-6f) {
            Tile &ground = mtile(map, x, y, gz);
            float v = soil_variation[i];
            float theta_s = std::min(1.0f, ground.water_capacity() * v);
            float space = theta_s - ground.state.liquid_volume;
            if (space > 1e-6f) {
              float k_sat = ground.mat().permeability;
              float seep = std::min(std::min(pond.state.liquid_volume, space),
                                    k_sat * p.percolation_rate_scale);
              if (seep > 1e-6f) {
                pond.state.liquid_volume -= seep;
                ground.state.liquid_volume += seep;
                if (pond.state.liquid_volume <= 1e-5f) {
                  pond.state.liquid_volume = 0.0f;
                }
              }
            }
          }
        }

        for (int z = gz; z >= 0; --z) {
          Tile &t = mtile(map, x, y, z);

          if (t.material == MaterialType::AIR) {
            if (t.state.liquid_volume > 1e-6f) {
              if (z > 0) {
                mtile(map, x, y, z - 1).state.liquid_volume +=
                    t.state.liquid_volume;
              } else {
                groundwater.recharge(x, y, t.state.liquid_volume);
              }
              t.state.liquid_volume = 0.0f;
            }
            continue;
          }

          float v = soil_variation[i];

          float theta = t.state.liquid_volume;
          float theta_s = std::min(1.0f, t.water_capacity() * v);
          float theta_fc = std::min(theta_s, t.field_capacity() * v);
          float theta_wp = std::min(theta_fc, t.wilting_point() * v);

          float k_sat = t.mat().permeability;
          float span = theta_s - theta_wp;
          float k_theta;
          if (span < 1e-5f) {
            k_theta = (theta > 1e-5f) ? k_sat : 0.0f;
          } else {
            float wetness = std::clamp((theta - theta_wp) / span, 0.0f, 1.0f);
            k_theta = k_sat * wetness * wetness * wetness;
          }

          float drainage = 0.0f;
          if (theta > theta_fc) {
            float available = theta - theta_fc;
            drainage = std::max(
                0.0f, std::min(available, k_theta * p.percolation_rate_scale));
          }

          if (drainage > 1e-6f) {
            t.state.liquid_volume -= drainage;

            if (z > 0) {
              Tile &below = mtile(map, x, y, z - 1);
              bool t_is_soil = t.material == MaterialType::SOIL_BASE;
              bool below_is_bedrock =
                  below.material == MaterialType::STONE_BASE &&
                  below.mat().is_solid;

              if (t_is_soil && below_is_bedrock) {
                groundwater.recharge(x, y, drainage);
              } else {
                float below_cap = below.water_capacity();
                float below_space =
                    std::max(0.0f, below_cap - below.state.liquid_volume);
                float into_below = std::min(drainage, below_space);
                below.state.liquid_volume += into_below;

                float overflow_back = drainage - into_below;
                if (overflow_back > 1e-6f) {
                  t.state.liquid_volume += overflow_back;
                }
              }
            } else {
              groundwater.recharge(x, y, drainage);
            }
          }

          if (t.state.liquid_volume > theta_s) {
            float overflow = t.state.liquid_volume - theta_s;
            t.state.liquid_volume = theta_s;

            if (z == gz) {
              add_surface_water(map, x, y, gz, overflow, p);
            } else {
              Tile &above = mtile(map, x, y, z + 1);
              above.state.liquid_volume += overflow;
            }
          }
        }
      }
    }
  });
}

void capillary_rise_step(Game_map &map, const std::vector<int> &ground_z,
                         const std::vector<float> &soil_variation,
                         const Params &p) {
  int width = map.get_width();
  int height = map.get_height();

  parallel_for_2d(height, [&](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        int gz = ground_z[i];
        float v = soil_variation[i];

        for (int z = 0; z < gz; ++z) {
          Tile &lower = mtile(map, x, y, z);
          Tile &upper = mtile(map, x, y, z + 1);

          if (lower.material == MaterialType::AIR ||
              upper.material == MaterialType::AIR)
            continue;

          float upper_fc = upper.field_capacity() * v;
          if (upper.state.liquid_volume >= upper_fc)
            continue;

          float diff = lower.state.liquid_volume - upper.state.liquid_volume;
          if (diff <= 0.0f)
            continue;

          float rise = p.capillary_rate * diff;

          float lower_wp = lower.wilting_point() * v;
          rise = std::min(rise, lower.state.liquid_volume - lower_wp);
          rise = std::min(rise, upper_fc - upper.state.liquid_volume);
          if (rise <= 0.0f)
            continue;

          lower.state.liquid_volume -= rise;
          upper.state.liquid_volume += rise;
        }
      }
    }
  });
}

void overland_flow_step(Game_map &map, const std::vector<int> &ground_z,
                        const Params &p, float &runoff_to_ocean,
                        OverlandFlowBuffers &buffers) {
  int width = map.get_width();
  int height = map.get_height();
  int depth_map = map.get_depth();

  size_t total_cells = (size_t)width * height;
  if (buffers.outflow.size() != total_cells) {
    buffers.outflow.assign(total_cells, 0.0f);
    buffers.inflow.assign(total_cells, 0.0f);
    buffers.best_idx.assign(total_cells, -1);
  } else {
    std::fill(buffers.outflow.begin(), buffers.outflow.end(), 0.0f);
    std::fill(buffers.inflow.begin(), buffers.inflow.end(), 0.0f);
    std::fill(buffers.best_idx.begin(), buffers.best_idx.end(), -1);
  }

  auto pond_depth = [&](int x, int y) -> float {
    int gz = ground_z[(std::size_t)y * width + x];
    if (gz + 1 >= depth_map)
      return 0.0f;
    const Tile &t = map.get_tile(x, y, gz + 1);
    if (t.material == MaterialType::AIR)
      return t.state.liquid_volume;
    return 0.0f;
  };

  static const int DX[8] = {1, -1, 0, 0, 1, 1, -1, -1};
  static const int DY[8] = {0, 0, 1, -1, 1, -1, 1, -1};
  static const float DIST[8] = {1.0f,        1.0f,        1.0f,
                                1.0f,        1.41421356f, 1.41421356f,
                                1.41421356f, 1.41421356f};

  // Pass 1: Compute outflows and best_idx per cell (Parallel)
  parallel_for_2d(height, [&](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        float pd = pond_depth(x, y);
        if (pd <= 1e-5f)
          continue;

        float wse = (float)ground_z[i] + pd;

        float best_diff = 0.0f;
        int best_idx = -1; // -1: nowhere, -2: offmap

        for (int d = 0; d < 8; ++d) {
          int nx = x + DX[d], ny = y + DY[d];
          float n_wse;
          bool offmap = (nx < 0 || nx >= width || ny < 0 || ny >= height);
          if (offmap) {
            n_wse = (float)ground_z[i] - 1.0f;
          } else {
            n_wse = (float)ground_z[(std::size_t)ny * width + nx] +
                    pond_depth(nx, ny);
          }

          float diff = (wse - n_wse) / DIST[d];
          if (diff > best_diff) {
            best_diff = diff;
            if (offmap) {
              best_idx = -2;
            } else {
              best_idx = ny * width + nx;
            }
          }
        }

        if (best_diff > 0.0f) {
          float blockage = map.get_surface(x, y).flow_blockage;
          float flow_multiplier = std::max(0.01f, 1.0f - blockage);
          float flow = std::min(pd, p.overland_flow_fraction * best_diff *
                                        flow_multiplier);
          if (flow > 1e-6f) {
            buffers.outflow[i] = flow;
            buffers.best_idx[i] = best_idx;
          }
        }
      }
    }
  });

  // Pass 2: Compute inflows (Parallel gather based on best_idx)
  parallel_for_2d(height, [&](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        float in_f = 0.0f;
        for (int d = 0; d < 8; ++d) {
          int nx = x + DX[d], ny = y + DY[d];
          if (nx >= 0 && nx < width && ny >= 0 && ny < height) {
            int ni = ny * width + nx;
            if (buffers.best_idx[ni] == i) {
              in_f += buffers.outflow[ni];
            }
          }
        }
        buffers.inflow[i] = in_f;
      }
    }
  });

  // Accumulate ocean runoff (Serial, extremely fast)
  runoff_to_ocean = 0.0f;
  for (size_t i = 0; i < total_cells; ++i) {
    if (buffers.best_idx[i] == -2) {
      runoff_to_ocean += buffers.outflow[i];
    }
  }

  // Pass 3: Apply net flow (Parallel)
  parallel_for_2d(height, [&](int, int start_y, int end_y) {
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        int gz = ground_z[i];
        float net = buffers.inflow[i] - buffers.outflow[i];
        if (std::fabs(net) < 1e-7f)
          continue;

        if (net > 0.0f) {
          add_surface_water(map, x, y, gz, net, p);
        } else if (gz + 1 < depth_map) {
          Tile &t = mtile(map, x, y, gz + 1);
          t.state.liquid_volume += net; // net is negative
          if (t.state.liquid_volume <= 1e-5f &&
              t.material == MaterialType::AIR) {
            t.state.liquid_volume = 0.0f;
          } else if (t.state.liquid_volume < 0.0f) {
            t.state.liquid_volume = 0.0f;
          }
        }
      }
    }
  });
}

float evapotranspiration_step(Game_map &map, ClimateSystem &climate,
                              const std::vector<int> &ground_z,
                              const Params &p) {
  int width = map.get_width();
  int height = map.get_height();
  int depth_map = map.get_depth();

  std::vector<float> thread_totals(thread_count(), 0.0f);

  parallel_for_2d(height, [&](int tid, int start_y, int end_y) {
    float local_total = 0.0f;
    for (int y = start_y; y < end_y; ++y) {
      for (int x = 0; x < width; ++x) {
        int i = y * width + x;
        int gz = ground_z[i];

        int z = gz;
        bool is_pond = false;
        if (gz + 1 < depth_map) {
          const Tile &above = map.get_tile(x, y, gz + 1);
          if (above.material == MaterialType::AIR &&
              above.state.liquid_volume > 1e-5f) {
            z = gz + 1;
            is_pond = true;
          }
        }

        Tile &t = mtile(map, x, y, z);

        float wetness;
        if (is_pond) {
          wetness = 1.0f;
        } else {
          float fc = t.field_capacity();
          float wp = t.wilting_point();
          float span = fc - wp;
          wetness =
              (span > 1e-6f)
                  ? std::clamp((t.state.liquid_volume - wp) / span, 0.0f, 1.0f)
                  : 0.0f;
        }
        if (wetness <= 0.0f)
          continue;

        const ClimateCell &c = climate.at(x, y);
        float wind_speed = std::sqrt(c.wind_u * c.wind_u + c.wind_v * c.wind_v);
        float surface_qsat = qsat(t.state.temperature, p);
        float vpd = std::max(0.0f, surface_qsat - c.vapor);
        if (vpd <= 0.0f)
          continue;

        float evap = p.evap_coeff * wind_speed * vpd * wetness;

        float min_liquid_volume = is_pond ? 0.0f : t.wilting_point();
        evap = std::min(
            evap, std::max(0.0f, t.state.liquid_volume - min_liquid_volume));
        if (evap <= 1e-7f)
          continue;

        t.state.liquid_volume -= evap;
        local_total += evap;
        climate.add_vapor(x, y, evap);

        if (is_pond && t.state.liquid_volume <= 1e-5f) {
          t.state.liquid_volume = 0.0f;
        }
      }
    }
    thread_totals[tid] = local_total;
  });

  float total = 0.0f;
  for (float val : thread_totals)
    total += val;
  return total;
}

} // namespace hydro