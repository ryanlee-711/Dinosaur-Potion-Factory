#pragma once
/*
 * Defining components (capabilities, or Interfaces) for entities.
 *
 */

#include <glm/glm.hpp>

#include "../inventory/Item.hpp"
#include "../main/PlayMode.hpp"

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
     * Entities that can be interacted with (clicked?)
     * uses `on_interact()` as callback.
     *
     */
    struct Context {
        enum class InteractionType {
            None,
            OpenInventory,
            Locked,
        } flag;
        PlayMode *playmode;
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