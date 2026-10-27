/*
 * Defining components (capabilities, or Interfaces) for entities.
 *
 */

#include <glm/glm.hpp>

#include "../inventory/Item.hpp"

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
    };

    virtual ~IInteractable() = default;
    virtual void on_interact(Context &ctx) = 0;
};