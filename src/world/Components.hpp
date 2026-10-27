#pragma once
/*
 * Defining components (capabilities, or Interfaces) for entities.
 *
 */

#include <glm/glm.hpp>
#include <string>
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