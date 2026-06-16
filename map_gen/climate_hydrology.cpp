#include "climate_hydrology.hpp"
#include "game_map.hpp" // adjust this include if your Game_map header has a different name/path

#include <cmath>
#include <algorithm>

namespace hydro {

namespace {
constexpr float PI = 3.14159265358979323846f;

// Mutable reference into the map's tile storage, mirroring the
// const_cast<Tile&>(get_tile(...)) pattern used by the original simulator
// for in-place numeric edits (no material change -> no set_tile needed).
inline Tile& mtile(Game_map& map, int x, int y, int z) {
    return const_cast<Tile&>(map.get_tile(x, y, z));
}
} // namespace

// -----------------------------------------------------------------------
float qsat(float temperature_K, const Params& p) {
    return p.qsat_ref * std::exp(p.qsat_k * (temperature_K - p.sea_level_temp_K));
}

// -----------------------------------------------------------------------
std::vector<int> compute_ground_heightmap(const Game_map& map) {
    int width = map.get_width();
    int height = map.get_height();
    int depth = map.get_depth();

    std::vector<int> ground_z((size_t)width * height, 0);

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
            ground_z[(size_t)y * width + x] = gz;
        }
    }
    return ground_z;
}

// -----------------------------------------------------------------------
std::vector<float> compute_soil_variation(const std::vector<int>& ground_z,
                                           int width, int height,
                                           NoiseGen& noise, const Params& p) {
    (void)ground_z;
    std::vector<float> variation((size_t)width * height, 1.0f);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float n = noise.fbm2D(x * 0.15f + 500.0f, y * 0.15f + 500.0f, 3);
            variation[(size_t)y * width + x] = 1.0f + p.soil_variation_amplitude * n;
        }
    }
    return variation;
}

// =========================================================================
// ClimateSystem
// =========================================================================
void ClimateSystem::init(int width, int height, const Params& params) {
    w_ = width;
    h_ = height;
    params_ = params;
    cells_.assign((size_t)w_ * h_, ClimateCell{});
}

void ClimateSystem::initialize(const std::vector<int>& ground_z, NoiseGen& noise) {
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            ClimateCell& c = cells_[idx(x, y)];
            c.temperature = params_.sea_level_temp_K
                          - params_.lapse_rate_K_per_tile * (float)ground_z[idx(x, y)];
            c.vapor = 0.5f * qsat(c.temperature, params_); // start at ~50% relative humidity
            c.wind_u = 0.0f;
            c.wind_v = 0.0f;
            c.upslope = 0.0f;
        }
    }
    update_wind(ground_z, noise, 0.0f);
}

void ClimateSystem::update_wind(const std::vector<int>& ground_z, NoiseGen& noise, float season_phase) {
    // Prevailing seasonal wind direction rotates once per year (stylized).
    float base_angle = season_phase * 2.0f * PI;
    float base_u = std::cos(base_angle) * 1.5f;
    float base_v = std::sin(base_angle) * 1.5f;

    auto elev = [&](int x, int y) -> float {
        x = std::clamp(x, 0, w_ - 1);
        y = std::clamp(y, 0, h_ - 1);
        return (float)ground_z[idx(x, y)];
    };

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            // Large-scale, slowly-varying noise perturbation on top of the
            // seasonal prevailing direction.
            float nu = noise.fbm2D((float)x * 0.015f + 1000.0f, (float)y * 0.015f + 1000.0f, 3);
            float nv = noise.fbm2D((float)x * 0.015f - 2000.0f, (float)y * 0.015f - 3000.0f, 3);

            float u0 = base_u + nu;
            float v0 = base_v + nv;
            float speed = std::sqrt(u0 * u0 + v0 * v0);
            if (speed < 1e-5f) {
                speed = 1e-5f;
                u0 = 1e-5f;
            }

            // Local elevation gradient (central difference, clamped at edges).
            float gx = (elev(x + 1, y) - elev(x - 1, y)) * 0.5f;
            float gy = (elev(x, y + 1) - elev(x, y - 1)) * 0.5f;
            float slope = std::sqrt(gx * gx + gy * gy);

            // Steeper terrain deflects wind toward flowing along contours
            // (valleys/canyons); flatter terrain leaves wind unperturbed.
            // A relief of ~4 tiles between neighbours counts as "very steep".
            float steepness = std::clamp(slope / 4.0f, 0.0f, 1.0f);

            float wu = u0, wv = v0;
            if (slope > 1e-5f) {
                float cx = -gy, cy = gx; // perpendicular to the gradient (contour direction)
                float dirx = u0 / speed, diry = v0 / speed;
                if (cx * dirx + cy * diry < 0.0f) { cx = -cx; cy = -cy; } // keep roughly aligned with prevailing flow
                float clen = std::sqrt(cx * cx + cy * cy);
                cx /= clen; cy /= clen;

                float bx = dirx * (1.0f - steepness) + cx * steepness;
                float by = diry * (1.0f - steepness) + cy * steepness;
                float blen = std::sqrt(bx * bx + by * by);
                if (blen > 1e-5f) {
                    wu = bx / blen * speed;
                    wv = by / blen * speed;
                }
            }

            ClimateCell& c = cells_[idx(x, y)];
            c.wind_u = wu;
            c.wind_v = wv;
            // dot(elevation gradient, wind): >0 means the wind is blowing
            // toward higher ground (windward / ascending air).
            c.upslope = gx * wu + gy * wv;
        }
    }
}

void ClimateSystem::update_temperature(const std::vector<int>& ground_z, float season_phase) {
    float seasonal = params_.seasonal_amplitude_K * std::sin(season_phase * 2.0f * PI);
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            ClimateCell& c = cells_[idx(x, y)];
            c.temperature = params_.sea_level_temp_K + seasonal
                          - params_.lapse_rate_K_per_tile * (float)ground_z[idx(x, y)];
        }
    }
}

void ClimateSystem::advect() {
    std::vector<float> delta(cells_.size(), 0.0f);

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            ClimateCell& c = cells_[idx(x, y)];
            if (c.vapor <= 0.0f) continue;

            float u = c.wind_u, v = c.wind_v;
            float speed = std::sqrt(u * u + v * v);
            float move_frac = std::clamp(speed * params_.wind_advect_scale, 0.0f, 0.9f);
            if (move_frac <= 0.0f) continue;

            float au = std::fabs(u), av = std::fabs(v);
            float denom = au + av;
            if (denom < 1e-6f) continue;

            float flux_x = c.vapor * move_frac * (au / denom);
            float flux_y = c.vapor * move_frac * (av / denom);

            int nx = x + (u > 0.0f ? 1 : -1);
            int ny = y + (v > 0.0f ? 1 : -1);

            delta[idx(x, y)] -= (flux_x + flux_y);
            if (nx >= 0 && nx < w_) delta[idx(nx, y)] += flux_x;
            // else: flows off the map edge - lost to "beyond the simulated region"
            if (ny >= 0 && ny < h_) delta[idx(x, ny)] += flux_y;
            // else: same
        }
    }

    for (size_t i = 0; i < cells_.size(); ++i) {
        cells_[i].vapor = std::max(0.0f, cells_[i].vapor + delta[i]);
    }

    // Windward-boundary inflow: edge cells whose wind blows INTO the domain
    // relax toward ocean_humidity, representing liquid_volume supplied by air
    // masses arriving from beyond the map (e.g. surrounding ocean).
    for (int y = 0; y < h_; ++y) {
        ClimateCell& left = cells_[idx(0, y)];
        if (left.wind_u > 0.0f) left.vapor += params_.boundary_relax * (params_.ocean_humidity - left.vapor);

        ClimateCell& right = cells_[idx(w_ - 1, y)];
        if (right.wind_u < 0.0f) right.vapor += params_.boundary_relax * (params_.ocean_humidity - right.vapor);
    }
    for (int x = 0; x < w_; ++x) {
        ClimateCell& bottom = cells_[idx(x, 0)];
        if (bottom.wind_v > 0.0f) bottom.vapor += params_.boundary_relax * (params_.ocean_humidity - bottom.vapor);

        ClimateCell& top = cells_[idx(x, h_ - 1)];
        if (top.wind_v < 0.0f) top.vapor += params_.boundary_relax * (params_.ocean_humidity - top.vapor);
    }
    for (auto& c : cells_) c.vapor = std::max(0.0f, c.vapor);
}

std::vector<float> ClimateSystem::step_precipitation(const std::vector<int>& ground_z,
                                                       NoiseGen& noise, float day_index) {
    (void)ground_z;
    std::vector<float> precip(cells_.size(), 0.0f);

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            ClimateCell& c = cells_[idx(x, y)];

            // Orographic precipitation: extra adiabatic cooling on windward
            // slopes lowers saturation capacity (more rain); leeward slopes
            // get extra warming (less rain - rain shadow).
            float t_eff = c.temperature - params_.orographic_gain * c.upslope;
            float cap = qsat(t_eff, params_);

            float rain = 0.0f;
            float excess = c.vapor - cap;
            if (excess > 0.0f) {
                rain = excess * params_.rain_out_fraction;
                c.vapor -= rain;
            }

            // Convective storms: noise-driven, independent of orography, so
            // flat plains still get occasional rain. `day_index` drives a
            // slow drift through the noise field so storms come and go.
            float storm = noise.fbm2D((float)x * 0.07f + day_index * 0.31f,
                                       (float)y * 0.07f - day_index * 0.17f, 2);
            if (storm > params_.convective_threshold) {
                float intensity = (storm - params_.convective_threshold)
                                 / (1.0f - params_.convective_threshold);
                float convective = params_.convective_intensity * intensity;
                float draw = std::min(c.vapor, convective);
                c.vapor -= draw;
                rain += draw;
            }

            c.vapor = std::max(0.0f, c.vapor);
            precip[idx(x, y)] = rain;
        }
    }
    return precip;
}

void ClimateSystem::add_vapor(int x, int y, float amount) {
    ClimateCell& c = cells_[idx(x, y)];
    c.vapor = std::max(0.0f, c.vapor + amount);
}

// =========================================================================
// GroundwaterGrid
// =========================================================================
void GroundwaterGrid::init(int width, int height, const Params& params) {
    w_ = width;
    h_ = height;
    params_ = params;
    table_.assign((size_t)w_ * h_, 0.0f);
}

void GroundwaterGrid::initialize(const std::vector<int>& ground_z, NoiseGen& noise) {
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            float gz = (float)ground_z[idx(x, y)];
            float n = noise.fbm01((float)x * 0.03f + 7000.0f, (float)y * 0.03f + 7000.0f, 4); // [0,1]
            // Most columns start with the water table below ground (30-100%
            // of column height); the wettest ~10% of the noise range starts
            // at/above ground level, i.e. pre-existing lakes/wetlands that
            // the simulation then reshapes via climate + D8 routing.
            float frac = 0.3f + 0.8f * n;
            table_[idx(x, y)] = gz * frac;
        }
    }
}

void GroundwaterGrid::recharge(int x, int y, float volume) {
    if (volume <= 0.0f) return;
    table_[idx(x, y)] += volume / params_.groundwater_porosity;
}

std::vector<float> GroundwaterGrid::update(const std::vector<int>& ground_z, const Game_map& map) {
    std::vector<float> discharge(table_.size(), 0.0f);
    std::vector<float> new_table = table_;

    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            int i = idx(x, y);

            // Sample local permeability near the water table to scale diffusion.
            // A floor keeps diffusion functional even under near-impermeable
            // bedrock (groundwater still moves, just more slowly).
            int gz = std::max(0, ground_z[i]);
            int z = std::clamp((int)std::lround(table_[i]), 0, gz);
            float perm = map.get_tile(x, y, z).mat().permeability;
            float diffusion_rate = params_.groundwater_diffusion_rate * (0.1f + 0.9f * std::clamp(perm, 0.0f, 1.0f));

            float sum = 0.0f;
            int nxs[4] = {x - 1, x + 1, x, x};
            int nys[4] = {y, y, y - 1, y + 1};
            for (int k = 0; k < 4; ++k) {
                int cx = std::clamp(nxs[k], 0, w_ - 1);
                int cy = std::clamp(nys[k], 0, h_ - 1);
                sum += table_[idx(cx, cy)];
            }
            float avg = sum * 0.25f;

            new_table[i] = table_[i] + diffusion_rate * (avg - table_[i]);
        }
    }
    table_ = new_table;

    // Baseflow / springs: where the table reaches or exceeds ground level,
    // the excess discharges to the surface and the table is clamped back down.
    for (int y = 0; y < h_; ++y) {
        for (int x = 0; x < w_; ++x) {
            int i = idx(x, y);
            float gz = (float)ground_z[i];
            if (table_[i] > gz) {
                float excess_height = table_[i] - gz;
                discharge[i] = excess_height * params_.groundwater_porosity;
                table_[i] = gz;
            }
            if (table_[i] < 0.0f) table_[i] = 0.0f;
        }
    }
    return discharge;
}

// =========================================================================
// Surface / soil routines
// =========================================================================
void add_surface_water(Game_map& map, int x, int y, int ground_z, float depth,
                        const Params& p) {
    (void)p;
    if (depth <= 0.0f) return;
    int depth_map = map.get_depth();

    int z = ground_z + 1;
    float remaining = depth;

    while (remaining > 1e-6f && z < depth_map) {
        Tile& t = mtile(map, x, y, z);

        float cap = t.water_capacity(); // AIR both report 1.0
        float space = std::max(0.0f, cap - t.state.liquid_volume);
        float add = std::min(remaining, space);
        t.state.liquid_volume += add;
        remaining -= add;

        if (remaining <= 1e-6f) break;
        ++z;
        }

    // If remaining > 0 here, the column is full all the way to the map
    // ceiling - the excess is discarded (extremely rare edge case).
}

void apply_precipitation(Game_map& map, const std::vector<int>& ground_z,
                          const std::vector<float>& precip_depth, const Params& p) {
    int width = map.get_width();
    int height = map.get_height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            float rain = precip_depth[i];
            if (rain <= 0.0f) continue;

            int gz = ground_z[i];
            Tile& t = mtile(map, x, y, gz);

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
}

void soil_percolation_step(Game_map& map, const std::vector<int>& ground_z,
                            const std::vector<float>& soil_variation,
                            const Params& p, GroundwaterGrid& groundwater) {
    int width = map.get_width();
    int height = map.get_height();
    int depth_map = map.get_depth();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            int gz = ground_z[i];

            // --- Pond seepage into ground ---
            // The main top-down loop below never visits gz+1 (the surface
            // pond), so without this step standing water could only leave a
            // column via evaporation or lateral D8 flow - it would never
            // infiltrate the ground beneath it. Move water from a pond at
            // gz+1 into the ground tile at gz, limited by spare capacity and
            // by the ground's saturated permeability.
            if (gz + 1 < depth_map) {
                Tile& pond = mtile(map, x, y, gz + 1);
                if (pond.material == MaterialType::AIR && pond.state.liquid_volume > 1e-6f) {
                    Tile& ground = mtile(map, x, y, gz);
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

            // Process this column top-down so that water draining out of a
            // tile is visible to the tile below it within the same pass
            // (cascading drainage, and "free-fall" through any AIR voids).
            for (int z = gz; z >= 0; --z) {
                Tile& t = mtile(map, x, y, z);

                if (t.material == MaterialType::AIR) {
                    // Underground cave: any liquid_volume here (e.g. dropped into
                    // it by the tile above this substep) falls straight
                    // through to the tile below. It will eventually land on
                    // a solid floor, where normal drainage + the overflow
                    // check below turn it into a cave lake if the floor is
                    // saturated.
                    if (t.state.liquid_volume > 1e-6f) {
                        if (z > 0) {
                            mtile(map, x, y, z - 1).state.liquid_volume += t.state.liquid_volume;
                        } else {
                            groundwater.recharge(x, y, t.state.liquid_volume);
                        }
                        t.state.liquid_volume = 0.0f;
                    }
                    continue;
                }

                // soil_variation only perturbs porous solids - AIR and
                // WATER_FRESH always keep capacity == 1.0.
                float v = (t.material == MaterialType::AIR) ? 1.0f : soil_variation[i];

                float theta = t.state.liquid_volume;
                float theta_s = std::min(1.0f, t.water_capacity() * v);
                float theta_fc = std::min(theta_s, t.field_capacity() * v);
                float theta_wp = std::min(theta_fc, t.wilting_point() * v);

                float k_sat = t.mat().permeability;
                float span = theta_s - theta_wp;
                float k_theta;
                if (span < 1e-5f) {
                    // Degenerate (near-zero porosity, e.g. bedrock): allow a
                    // tiny permeability-limited seep if there's any liquid_volume.
                    k_theta = (theta > 1e-5f) ? k_sat : 0.0f;
                } else {
                    float wetness = std::clamp((theta - theta_wp) / span, 0.0f, 1.0f);
                    k_theta = k_sat * wetness * wetness * wetness;
                }

                float drainage = 0.0f;
                if (theta > theta_fc) {
                    float available = theta - theta_fc;
                    drainage = std::max(0.0f, std::min(available, k_theta * p.percolation_rate_scale));
                }

                if (drainage > 1e-6f) {
                    t.state.liquid_volume -= drainage;

                    if (z > 0) {
                        Tile& below = mtile(map, x, y, z - 1);

                        // "Soil" = has nonzero fertility (an existing
                        // material-db property). "Bedrock" = no fertility
                        // and solid. Water leaving the bottom of the soil
                        // column recharges the deep aquifer directly, per
                        // the design notes ("Recharge from percolation from
                        // the bottom of the soil column increases W"),
                        // rather than needing to slowly cascade tile-by-tile
                        // through many bedrock layers.
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
                                // Below is already saturated; keep the water here
                                // for now (it will try again next substep, or
                                // pond upward via the overflow check below).
                                t.state.liquid_volume += overflow_back;
                            }
                        }
                    } else {
                        groundwater.recharge(x, y, drainage);
                    }
                }

                // Saturation overflow: push any excess above capacity upward
                // (into a surface pond at z==gz, or into the tile above for
                // sub-surface tiles - e.g. a rising cave lake).
                if (t.state.liquid_volume > theta_s) {
                    float overflow = t.state.liquid_volume - theta_s;
                    t.state.liquid_volume = theta_s;

                    if (z == gz) {
                        add_surface_water(map, x, y, gz, overflow, p);
                    } else {
                        Tile& above = mtile(map, x, y, z + 1);
                        above.state.liquid_volume += overflow;
                    }
                }
            }
        }
    }
}

void capillary_rise_step(Game_map& map, const std::vector<int>& ground_z,
                          const std::vector<float>& soil_variation, const Params& p) {
    int width = map.get_width();
    int height = map.get_height();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            int gz = ground_z[i];
            float v = soil_variation[i];

            for (int z = 0; z < gz; ++z) {
                Tile& lower = mtile(map, x, y, z);
                Tile& upper = mtile(map, x, y, z + 1);

                if (lower.material == MaterialType::AIR || upper.material == MaterialType::AIR) continue;

                float upper_fc = upper.field_capacity() * v;
                if (upper.state.liquid_volume >= upper_fc) continue;

                float diff = lower.state.liquid_volume - upper.state.liquid_volume;
                if (diff <= 0.0f) continue;

                float rise = p.capillary_rate * diff;

                float lower_wp = lower.wilting_point() * v;
                rise = std::min(rise, lower.state.liquid_volume - lower_wp);
                rise = std::min(rise, upper_fc - upper.state.liquid_volume);
                if (rise <= 0.0f) continue;

                lower.state.liquid_volume -= rise;
                upper.state.liquid_volume += rise;
            }
        }
    }
}

void overland_flow_step(Game_map& map, const std::vector<int>& ground_z,
                         const Params& p, float& runoff_to_ocean) {
    int width = map.get_width();
    int height = map.get_height();
    int depth_map = map.get_depth();

    auto pond_depth = [&](int x, int y) -> float {
        int gz = ground_z[(size_t)y * width + x];
        if (gz + 1 >= depth_map) return 0.0f;
        const Tile& t = map.get_tile(x, y, gz + 1);
        if (t.material == MaterialType::AIR) return t.state.liquid_volume;
        return 0.0f;
    };

    std::vector<float> outflow((size_t)width * height, 0.0f);
    std::vector<float> inflow((size_t)width * height, 0.0f);

    static const int DX[8]      = {1, -1, 0, 0, 1, 1, -1, -1};
    static const int DY[8]      = {0, 0, 1, -1, 1, -1, 1, -1};
    static const float DIST[8]  = {1.0f, 1.0f, 1.0f, 1.0f, 1.41421356f, 1.41421356f, 1.41421356f, 1.41421356f};

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            float pd = pond_depth(x, y);
            if (pd <= 1e-5f) continue;

            float wse = (float)ground_z[i] + pd;

            float best_diff = 0.0f;
            int best_idx = -1;
            bool best_offmap = false;

            for (int d = 0; d < 8; ++d) {
                int nx = x + DX[d], ny = y + DY[d];
                float n_wse;
                bool offmap = (nx < 0 || nx >= width || ny < 0 || ny >= height);
                if (offmap) {
                    // Off-map neighbour: treat as a fixed low elevation so
                    // ponds at the map edge can drain off it (acts as "the
                    // surrounding sea/lowlands").
                    n_wse = (float)ground_z[i] - 1.0f;
                } else {
                    n_wse = (float)ground_z[(size_t)ny * width + nx] + pond_depth(nx, ny);
                }

                float diff = (wse - n_wse) / DIST[d];
                if (diff > best_diff) {
                    best_diff = diff;
                    if (offmap) { best_offmap = true; best_idx = -1; }
                    else { best_offmap = false; best_idx = ny * width + nx; }
                }
            }

            if (best_diff > 0.0f) {
                float flow = std::min(pd, p.overland_flow_fraction * best_diff);
                if (flow > 1e-6f) {
                    outflow[i] += flow;
                    if (best_offmap) runoff_to_ocean += flow;
                    else inflow[best_idx] += flow;
                }
            }
        }
    }

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            int gz = ground_z[i];
            float net = inflow[i] - outflow[i];
            if (std::fabs(net) < 1e-7f) continue;

            if (net > 0.0f) {
                add_surface_water(map, x, y, gz, net, p);
            } else if (gz + 1 < depth_map) {
                Tile& t = mtile(map, x, y, gz + 1);
                t.state.liquid_volume += net; // net is negative
                if (t.state.liquid_volume <= 1e-5f && t.material == MaterialType::AIR) {
                    t.state.liquid_volume = 0.0f;
                } else if (t.state.liquid_volume < 0.0f) {
                    t.state.liquid_volume = 0.0f;
                }
            }
        }
    }
}

float evapotranspiration_step(Game_map& map, ClimateSystem& climate,
                               const std::vector<int>& ground_z, const Params& p) {
    int width = map.get_width();
    int height = map.get_height();
    int depth_map = map.get_depth();
    float total = 0.0f;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int i = y * width + x;
            int gz = ground_z[i];

            int z = gz;
            bool is_pond = false;
            if (gz + 1 < depth_map) {
                const Tile& above = map.get_tile(x, y, gz + 1);
                if (above.material == MaterialType::AIR && above.state.liquid_volume > 1e-5f) {
                    z = gz + 1;
                    is_pond = true;
                }
            }

            Tile& t = mtile(map, x, y, z);

            float wetness;
            if (is_pond) {
                wetness = 1.0f;
            } else {
                float fc = t.field_capacity();
                float wp = t.wilting_point();
                float span = fc - wp;
                wetness = (span > 1e-6f) ? std::clamp((t.state.liquid_volume - wp) / span, 0.0f, 1.0f) : 0.0f;
            }
            if (wetness <= 0.0f) continue;

            const ClimateCell& c = climate.at(x, y);
            float wind_speed = std::sqrt(c.wind_u * c.wind_u + c.wind_v * c.wind_v);
            float surface_qsat = qsat(t.state.temperature, p);
            float vpd = std::max(0.0f, surface_qsat - c.vapor);
            if (vpd <= 0.0f) continue;

            float evap = p.evap_coeff * wind_speed * vpd * wetness;

            float min_liquid_volume = is_pond ? 0.0f : t.wilting_point();
            evap = std::min(evap, std::max(0.0f, t.state.liquid_volume - min_liquid_volume));
            if (evap <= 1e-7f) continue;

            t.state.liquid_volume -= evap;
            total += evap;
            climate.add_vapor(x, y, evap);

            if (is_pond && t.state.liquid_volume <= 1e-5f) {
                t.state.liquid_volume = 0.0f;
            }
        }
    }
    return total;
}

} // namespace hydro
