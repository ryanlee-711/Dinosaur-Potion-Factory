#pragma once

#include "../main/GameConfig.hpp"
#include "../inventory/Item.hpp"
#include "../starter/Sound.hpp"
#include "TileMap.hpp"

#include <array>
#include <memory>
#include <unordered_map>

struct World;

class Music
{
public:
  enum class SFX
  {
    AxeMining,
    BottleFilling,
    BoulderBreak,
    BoulderMove,
    DrinkPotion,
    Footsteps,
    ItemDrop,
    ItemPickup,
    MachinePlacement,
    PickaxeMine,
    TreeFall,
    Count,
  };

  Music();
  ~Music();

  Music(Music const &) = delete;
  Music &operator=(Music const &) = delete;

  void update(World const &world, bool player_moving);
  void sfx(SFX effect);

  // Fade distances in world pixels.
  float max_area_distance = 10.0f * GameConfig::TILE_SIZE;
  float max_machine_distance = 8.0f * GameConfig::TILE_SIZE;

  // Volume multipliers from 0 (silent) to 1 (full volume).
  float ambient_volume = 1.0f;
  float music_volume = 1.0f;
  float machine_volume = 1.0f;

private:
  using Playback = std::shared_ptr<Sound::PlayingSample>;

  void set_footsteps(bool audible);

  std::array<Playback, Tilemap::AREA_COUNT> themes;
  std::unordered_map<Sound::Sample const *, Playback> ambience;
  std::unordered_map<item_id_t, Playback> machines;
  Playback footsteps;
  bool footsteps_audible = false;
};
