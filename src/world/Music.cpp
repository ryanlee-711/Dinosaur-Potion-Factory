#include "Music.hpp"

#include "World.hpp"
#include "entities/Golem.hpp"
#include "entities/Machine.hpp"
#include "../starter/Load.hpp"
#include "../starter/data_path.hpp"

#include <algorithm>
#include <utility>

namespace
{
  item_id_t const golem_type = "golem";

  std::string audio_path(std::string const &filename)
  {
    return data_path("assets/audio/" + filename);
  }

  struct MusicSamples
  {
    // Both arrays follow Tilemap::Area order. Areas can share an ambience loop.
    std::array<Sound::Sample, Tilemap::AREA_COUNT> themes;
    std::array<std::shared_ptr<Sound::Sample const>, Tilemap::AREA_COUNT> ambience;
    std::unordered_map<item_id_t, Sound::Sample> machines;
    std::array<Sound::Sample, static_cast<size_t>(Music::SFX::Count)> effects;
  };

  Load<MusicSamples> samples(LoadTagLate, []() -> MusicSamples const *
                             {
    std::unordered_map<item_id_t, Sound::Sample> machine_samples;
    for (auto const &[type, def] : MachineDef::all())
    {
      if (!def.sound_name.empty())
        machine_samples.try_emplace(type, audio_path(def.sound_name));
    }
    machine_samples.try_emplace(golem_type, audio_path("machine.golem.wav"));

    auto cave = std::make_shared<Sound::Sample const>(audio_path("ambience.cave.wav"));
    auto mountain = std::make_shared<Sound::Sample const>(audio_path("ambience.mountain.wav"));
    auto woods = std::make_shared<Sound::Sample const>(audio_path("ambience.woods.wav"));
    return new MusicSamples{
      {
        Sound::Sample(audio_path("theme.cave.wav")),
        Sound::Sample(audio_path("theme.back_cave.wav")),
        Sound::Sample(audio_path("theme.mountanside.wav")),
        Sound::Sample(audio_path("theme.woods.wav")),
        Sound::Sample(audio_path("theme.peaks.wav")),
      },
      {cave, cave, mountain, woods, mountain},
      std::move(machine_samples),
      {
        Sound::Sample(audio_path("sfx.axe_mining.wav")),
        Sound::Sample(audio_path("sfx.bottle_filling.wav")),
        Sound::Sample(audio_path("sfx.boulder_break.wav")),
        Sound::Sample(audio_path("sfx.boulder_move.wav")),
        Sound::Sample(audio_path("sfx.drink_potion.wav")),
        Sound::Sample(audio_path("sfx.footsteps.wav")),
        Sound::Sample(audio_path("sfx.item_drop.wav")),
        Sound::Sample(audio_path("sfx.item_pickup.wav")),
        Sound::Sample(audio_path("sfx.machine_placement.wav")),
        Sound::Sample(audio_path("sfx.pickaxe_mine.wav")),
        Sound::Sample(audio_path("sfx.tree_fall.wav")),
      },
    }; });

  float fade(float distance, float max_distance)
  {
    if (distance <= 0.0f)
      return 1.0f;
    if (distance >= max_distance)
      return 0.0f;
    return 1.0f - distance / max_distance;
  }
} // namespace

Music::Music()
{
  // Keep the audio callback out until every loop is queued at sample zero.
  struct AudioLock
  {
    AudioLock() { Sound::lock(); }
    ~AudioLock() { Sound::unlock(); }
  } lock;

  for (size_t i = 0; i < themes.size(); ++i)
    themes[i] = Sound::loop(samples->themes[i], 0.0f);

  for (auto const &sample : samples->ambience)
  {
    if (!ambience.contains(sample.get()))
      ambience.emplace(sample.get(), Sound::loop(*sample, 0.0f));
  }

  for (auto const &[type, sample] : samples->machines)
    machines.emplace(type, Sound::loop(sample, 0.0f));

  footsteps = Sound::loop(samples->effects[static_cast<size_t>(SFX::Footsteps)], 0.0f);
}

Music::~Music()
{
  for (auto const &track : themes)
    track->stop();
  for (auto const &entry : ambience)
    entry.second->stop();
  for (auto const &entry : machines)
    entry.second->stop();
  footsteps->stop();
}

void Music::sfx(SFX effect)
{
  if (effect == SFX::Footsteps)
    set_footsteps(true);
  else
    Sound::play(samples->effects.at(static_cast<size_t>(effect)));
}

void Music::set_footsteps(bool audible)
{
  if (audible == footsteps_audible)
    return;
  footsteps_audible = audible;
  footsteps->set_volume(audible ? 1.0f : 0.0f, 0.2f);
}

void Music::update(World const &world, bool player_moving)
{
  set_footsteps(player_moving);

  glm::vec2 listener = world.player.position;
  Rect listener_point{listener.x, listener.y};

  struct NearbyMachine
  {
    Rect box;
    float distance;
  };
  std::unordered_map<item_id_t, NearbyMachine> nearest_machines;

  for (auto const &entity : world.entities)
  {
    if (entity->removed)
      continue;
    item_id_t const *type = nullptr;
    if (auto const *machine = dynamic_cast<Machine const *>(entity.get()); machine && machine->is_active())
      type = &machine->get_machine_type();
    else if (auto const *golem = dynamic_cast<Golem const *>(entity.get()); golem && golem->moving)
      type = &golem_type;
    if (!type || !machines.contains(*type))
      continue;

    Rect box = entity->hitbox();
    // Treat a player within one pixel of the hitbox as adjacent.
    float distance = box.distance_to(listener_point) - 1.0f;
    if (distance >= max_machine_distance)
      continue;

    auto nearest = nearest_machines.find(*type);
    if (nearest == nearest_machines.end() || distance < nearest->second.distance)
      nearest_machines.insert_or_assign(*type, NearbyMachine{box, distance});
  }

  float strongest_machine_volume = 0.0f;
  for (auto const &[type, track] : machines)
  {
    auto nearest = nearest_machines.find(type);
    if (nearest == nearest_machines.end())
    {
      track->set_volume(0.0f);
      continue;
    }

    NearbyMachine const &source = nearest->second;
    float volume = machine_volume * fade(source.distance, max_machine_distance);
    track->set_volume(volume);
    strongest_machine_volume = std::max(strongest_machine_volume, volume);

    float offset = source.box.x + 0.5f * source.box.w - listener.x;
    if (max_machine_distance > 0.0f && volume > 0.0f)
      track->set_pan(std::clamp(offset / max_machine_distance, -1.0f, 1.0f));
  }

  std::array<float, Tilemap::AREA_COUNT> area_volumes;
  for (size_t i = 0; i < area_volumes.size(); ++i)
  {
    float distance = world.distance_to_area(static_cast<Tilemap::Area>(i), listener);
    area_volumes[i] = fade(distance, max_area_distance);
    themes[i]->set_volume(music_volume * area_volumes[i]);
  }

  float ambient_gain = ambient_volume * (1.0f - std::min(strongest_machine_volume, 1.0f));
  for (auto const &[sample, track] : ambience)
  {
    float volume = 0.0f;
    for (size_t i = 0; i < area_volumes.size(); ++i)
    {
      if (samples->ambience[i].get() == sample)
        volume = std::max(volume, area_volumes[i]);
    }
    track->set_volume(ambient_gain * volume);
  }
}
