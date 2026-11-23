#pragma once
/*
 * Defining components (capabilities, or Interfaces) for entities.
 *
 */

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "../inventory/Item.hpp"

struct Player;

struct IInventoryHolder {
    /*
     * Entities with a container/inventory.
     * calls `get_inventory()` to acquire a reference to the container.
     *
     */
    // The entity has an inventory that can be accessed.
    virtual ~IInventoryHolder() = default;

    virtual Inventory* get_inventory() = 0;
};

struct IInteractable {
    /*
     * Entities that can be interacted with (E / Space while facing them).
     * Each entity decides for itself what happens, inside `on_interact()`:
     *
     * The Context gives the entity what it needs to act (inputs), and lets it
     * ask PlayMode for things only PlayMode can do (outputs).
     */
    struct Context {
        // --- inputs: what the entity can use ---
        Player &player;                  // who interacted (position, facing, ...)
        Inventory &player_inventory;     // add or remove items the player carries

        // --- outputs: set these to ask PlayMode to do something afterwards ---
        Inventory *open_container = nullptr; // show this inventory as an open panel (chests, cauldron contents...)
        std::string message;                 // short text to show the player, e.g. "Your bag is full"
        // Add more as we go
    };

    virtual ~IInteractable() = default;
    virtual void on_interact(Context &ctx) = 0;
};

struct IItemPort {
    /*
     * Entities that have I/O support.
     * Each port s logically a coord and a port_type (I/O).
     * Given a grid position, the machine may be able to push to/pull from that position.
     * Also exposes methods for pushing/pulling items.
     *
     */
    enum class PortType {
        In,
        Out,
        Both,
        None
    };

    virtual ~IItemPort() = default;
    virtual PortType get_port(glm::ivec2 grid_coord) = 0;
    virtual int push_item(ItemRecord item_record) = 0;

};

struct ParticlePreset;

struct IBreakable {
    /*
     * Entities that can be broken.
     * Have a hardness. There is no callback for breaking, because it entirely resides in PlayMode.
     * Inheritance = breakable.
     *
     */
    virtual ~IBreakable() = default;

    // Holding the pickaxe deals up to MAX_HOLD_DAMAGE damage, times the strength effect level (if any).
    // Breaks once the damage reaches get_hardness(), so hardness above MAX_HOLD_DAMAGE needs strength.
    static constexpr int MAX_HOLD_DAMAGE = 5;

    virtual int get_hardness() {return 3;};
    virtual item_id_t get_drop_id() = 0;

    // Feedback while the pickaxe is held on it: PlayMode calls on_break_hit() when you start holding and then
    // once per damage tick, and sprays break_hit_particles() if not nullptr. Only when the dwarf is strong
    // enough to break it (MAX_HOLD_DAMAGE x strength level >= hardness); otherwise nothing happens.
    // Default: no reaction (machines).
    virtual void on_break_hit() { }
    virtual ParticlePreset const *break_hit_particles() const { return nullptr; }
};

struct IProgress {
    // Machine that work over time like brewer
    virtual ~IProgress() = default;
    virtual float get_progress() const = 0;
};

struct IMineable {
    /*
     * Resources you click with a tool to get items from: ore veins (pickaxe), trees (axe).
     * Unlike IBreakable, the thing itself stays; it runs out for a while and regrows.
     */
    virtual ~IMineable() = default;

    // The tool item that works on this ("pickaxe", "axe"). Other tools just get a message.
    virtual item_id_t required_tool() const = 0;

    // One click with the right tool.
    //   power: how hard the hit is (1, more with the strength effect)
    //   from:  where the hit comes from (the player's position), e.g. so a tree falls away from you
    //   drops: add whatever comes loose right now. PlayMode pops these out toward the player.
    // Returns false if there's nothing to get right now (mined out, a stump): PlayMode shows empty_message().
    virtual bool mine(int power, glm::vec2 from, std::vector<ItemRecord> &drops) = 0;
    virtual std::string empty_message() const { return "Nothing left. It grows back slowly"; }
};

struct IHintable {
    /*
     * Things that float a hint bubble over themselves while the dwarf can act on them (PlayMode draws it):
     * the icon of the tool they need (ore veins, trees, big boulders), or the E key (forageables, small boulders).
     * A tool hint shows within tool reach; an E hint shows on the thing E would act on (what the dwarf faces).
     */
    virtual ~IHintable() = default;

    // false = no hint right now (mined out, picked bare, a stump...).
    virtual bool show_hint() const = 0;

    // The tool whose icon to show ("pickaxe", "axe"), or "" for the E key.
    virtual item_id_t hint_tool() const = 0;

    // Where the hint floats, in world pixels: the bottom-center of the bubble (just above the sprite).
    virtual glm::vec2 hint_anchor() const = 0;

    // true = the dwarf can't do it yet (e.g. a boulder that needs the strength effect): the hint is drawn faded.
    // (Tool hints are also faded while the dwarf isn't holding that tool.)
    virtual bool hint_locked(Player const &player) const { return false; }
};

struct IWorkable {
    /*
     * Anything that can be worked by the golem automatically.
     * Different workable entities work differently:
     * - Plants (LichenRock, Apple tree, ...) regrows to full when worked with haste crystal (costs at 100% chance);
     * - Mines (Copper ore) can only be worked when there is strength crystal (costs at 5% chance).
     */
    virtual ~IWorkable() = default;
    // Called on a tool click. Returns the item knocked loose, or "" if there's nothing to mine right now.
    virtual item_id_t get_producer_name() const = 0;
    virtual int work(bool use_powerup) = 0;

};