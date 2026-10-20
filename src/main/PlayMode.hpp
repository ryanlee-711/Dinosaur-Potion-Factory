#include "../starter/Mode.hpp"
#include "../starter/Scene.hpp"

#include "../rendering/TextRenderer.hpp"
#include "../rendering/Sprites.hpp"

#include <glm/glm.hpp>

#include <algorithm>
#include <list>
#include <vector>
#include <unordered_map>
#include <deque>

const float PI = 3.1415926535897932384626f;
const float TIMESTEP = 1.0f / 120.0f;
const float DEACCEL = 0.5f;
const float SHOOT_MOMENTUM = 25.0f;
const float COLL_ROT_ACCEL = 80.0f;

// RigidBody!
// It will always use a *Circle* collision shape, with configurable radius.
// I have been borrowing Godot's terminology; Though implementation-wise it's quite different.
struct RigidBody {

	glm::vec2 position;     // position of the rigid body
	glm::vec2 velocity;     // velocity!
	float mass = 1.0f;             // mass of object!
	float radius = 0.0f;           // radius of the collision shape
	float rotation = 0.0f;         // radian rotation
	float rotation_speed = 0.0f;   // rotation_speed in radian

	std::string name;       // Name of this thingy.
	Sprite2D *sprite;    	// sprite!

	RigidBody();
	RigidBody(glm::vec2 position, float mass, float radius, Sprite2D *sprite);
	~RigidBody();

	// // _physics_process(), a.k.a. a physics frame
	// void physics_process(float delta);
	// // queue_free, a.k.a. queuing to free this body. I think it would not be implemented at all.
	// void queue_free();
};


struct RBRecord {
	size_t frame;           // which frame?
	glm::vec2 position;     // position of the rigid body
	glm::vec2 velocity;     // velocity!
	float mass = 1.0f;             // mass of object!
	float radius = 0.0f;           // radius of the collision shape
	float rotation = 0.0f;         // radian rotation
	float rotation_speed = 0.0f;   // rotation_speed in radian
};


struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//helper functions to add stuff
	virtual Sprite2D* add_sprite(std::string file_path, glm::vec2 position, glm::vec2 scale);
	virtual Sprite2D* add_sprite(SharedTexture const &shared_texture, glm::vec2 position, glm::vec2 scale);
	virtual RigidBody* add_body(std::string name, glm::vec2 position, float mass, float radius, Sprite2D *sprite);

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up, shoot, rewind;

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;
	
	//camera:
	Scene::Camera *camera = nullptr;

	//sprites!
	std::list<Sprite2D> sprites;
	//bodies!
	std::list<RigidBody> bodies;

	// smokes!
	std::list<std::list<Sprite2D>::iterator> smoke_list;

	//ROCKET
	RigidBody *rocket = nullptr;
	Sprite2D *flame = nullptr;

	//TEXTRENDER
	size_t text_w = 0;
	size_t text_h = 0;
	TextRenderer text_renderer;
	SharedTexture text_texture;
	Sprite2D *win_sprite = nullptr;


	//action
	size_t shoot_held_frame = 0;
	float physics_frame_delta = 0.0f;
	float shoot_cooldown = 0.0f;
	float shoot_pressed = 0.0f;
	float framestepping_interval = TIMESTEP;
	bool shot = false;
	int flaming = 0; // 1 = counterclockwise, -1=clockwise, 0=stop

	//statistics
	size_t current_frame = 0;
	float used_time = 0.0f;
	int bullets_shot = 0;
	int rocket_collision_count = 0;
	int damage = 0;
	bool game_clear = false;

	// history tracking
	std::unordered_map<std::string, std::list<RBRecord>> history;
};