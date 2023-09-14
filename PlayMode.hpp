#include "Mode.hpp"

#include "Scene.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>
#include <random>

struct PlayMode : Mode
{
	PlayMode();
	virtual ~PlayMode();

	// functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;
	void updateFakeTobbies();
	bool startNewLevel();
	//----- game state -----

	// input tracking:
	struct Button
	{
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	// local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	// hexapod leg to wobble:
	Scene::Transform *tobby = nullptr;

	Scene::Transform *house = nullptr;
	// glm::quat tobby_base_rotation;

	float tobby_speed = 30.0f;

	// fake Tobbies
	std::vector<Scene::Transform *> fake_tobbies;
	std::vector<float> fake_tobbies_speeds;
	std::vector<bool> fake_tobbies_alive;
	std::vector<glm::vec4> fake_tobbies_params;

	double min = 0;
	double max = 100;

	// Game state
	int level = 0;
	int score = 0;

	// Storage Information
	Scene::Transform *storage = nullptr;

	// camera:
	Scene::Camera *camera = nullptr;

	// Fun stuff
	// lighting
	glm::vec3 light_energy, light_direction;
	int light_type;
	int random_tobby;
};