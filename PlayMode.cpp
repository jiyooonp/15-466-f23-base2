#include "PlayMode.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

bool AinB(glm::vec3 A, glm::vec3 B, float epsilon)
{
	return std::abs(A.x - B.x) < epsilon && std::abs(A.y - B.y) < epsilon;
}

GLuint tobby_meshes_for_lit_color_texture_program = 0;
Load<MeshBuffer> tobby_meshes(
	LoadTagDefault, []() -> MeshBuffer const *
	{
		MeshBuffer const *ret = new MeshBuffer(data_path("tobby.pnct"));
		tobby_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
		return ret; });

Load<Scene> tobby_scene(LoadTagDefault, []() -> Scene const *
						{ return new Scene(data_path("tobby.scene"), [&](Scene &scene, Scene::Transform *transform, std::string const &mesh_name)
										   {
		Mesh const &mesh = tobby_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable &drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = tobby_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count; }); });

PlayMode::PlayMode() : scene(*tobby_scene)
{
	// get pointers to leg for convenience:
	for (auto &transform : scene.transforms)
	{
		// assigning the transform to member variables
		if (transform.name == "Tobby")
			tobby = &transform;
		else if (transform.name.substr(0, 6) == "Tobby.")
			fake_tobbies.push_back(&transform);
		else if (transform.name == "House")
			house = &transform;
		else if (transform.name == "Storgae")
			storage = &transform;
	}

	// error check to see if the pointers are assigned
	if (tobby == nullptr)
		throw std::runtime_error("Tobby not found.");
	if (house == nullptr)
		throw std::runtime_error("House not found.");
	if (storage == nullptr)
		throw std::runtime_error("Storgae not found.");

	// get pointer to camera for convenience:
	if (scene.cameras.size() != 1)
		throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();

	// store all fake tobbies in the storage
	for (uint8_t i = 0; i < fake_tobbies.size(); i++)
	{
		fake_tobbies[i]->position = storage->position;
		fake_tobbies_alive.push_back(false);
		fake_tobbies_speeds.push_back((std::rand() % 100) / 10);
		fake_tobbies_params.push_back(glm::vec4((std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0));
	}

	// light intial setup
	light_energy = glm::vec3(1.0f, 1.0f, 0.95f);
	light_direction = glm::vec3(0.0f, 0.0f, -1.0f);
	light_type = 1;
}

void fitInPlayground(glm::vec3 &position)
{
	const double minX = 4;
	const double minY = 4;
	const double maxX = 96;
	const double maxY = 96;

	if (position.x > maxX)
		position.x = position.x - maxX * std::floor(position.x / maxX);
	else if (position.x < minX)
		position.x = position.x - maxX * std::floor(position.x / maxX);
	if (position.y > maxY)
		position.y = position.y - maxY * std::floor(position.y / maxY);
	else if (position.y < minY)
		position.y = position.y - maxY * std::floor(position.y / maxY);
}
PlayMode::~PlayMode()
{
}

bool PlayMode::startNewLevel()
{
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution<double> distribution(min, max);

	// spawn fake tobbies
	for (uint8_t i = 0; i < level && i < fake_tobbies.size(); i++)
	{
		double randomValueX = distribution(gen);
		double randomValueY = distribution(gen);
		fake_tobbies[i]->position = glm::vec3(randomValueX, randomValueY, 0.0f);
		fake_tobbies_alive[i] = true;

		// update fake_tobbies speed

		fake_tobbies_speeds[i] = (std::rand() % 100) / 10.0f;
		fake_tobbies_params[i].x = (std::rand() % 100) / 100.0;
		fake_tobbies_params[i].y = (std::rand() % 100) / 100.0;
		fake_tobbies_params[i].z = (std::rand() % 100) / 100.0;
		fake_tobbies_params[i].w = (std::rand() % 100) / 100.0;
	}
	// relocate real tobby
	double randomValueX = distribution(gen);
	double randomValueY = distribution(gen);
	tobby->position = glm::vec3(randomValueX, randomValueY, 0.0f);

	// light intial setup for fun
	light_energy = glm::vec3((std::rand() % 70) / 100.0, (std::rand() % 80) / 100.0, (std::rand() % 90) / 100.0);
	light_energy += glm::vec3(0.3f, 0.2f, 0.1f);
	light_direction = glm::vec3((std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0, (std::rand() % 100) / 100.0);
	random_tobby = std::rand() % level - 1;

	return true;
}
bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_KEYDOWN)
	{
		if (evt.key.keysym.sym == SDLK_ESCAPE)
		{
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			// check if real Tobby is at the house
			if (AinB(tobby->position, house->position, 3))
			{
				std::printf("Tobby is at the house\n");
				level += 1;
				score += 1;
				startNewLevel();
			}
			else
			{
				std::printf("Tobby is NOT at the house\n");
				score -= 1;
			}
		}
	}
	else if (evt.type == SDL_KEYUP)
	{
		if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_SPACE)
		{
			// check if real Tobby is at the house
			// std::cout << "Tobby position: " << tobby->position.x << ", " << tobby->position.y << ", " << tobby->position.z << "\n";
		}
	}
	else if (evt.type == SDL_MOUSEBUTTONDOWN)
	{
		if (SDL_GetRelativeMouseMode() == SDL_FALSE)
		{
			SDL_SetRelativeMouseMode(SDL_TRUE);
			return true;
		}
	}
	else if (evt.type == SDL_MOUSEMOTION)
	{
		if (SDL_GetRelativeMouseMode() == SDL_TRUE)
		{
			glm::vec2 motion = glm::vec2(
				evt.motion.xrel / float(window_size.y),
				-evt.motion.yrel / float(window_size.y));
			camera->transform->rotation = glm::normalize(
				camera->transform->rotation * glm::angleAxis(-motion.x * camera->fovy, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::angleAxis(motion.y * camera->fovy, glm::vec3(1.0f, 0.0f, 0.0f)));
			return true;
		}
	}

	return false;
}

void PlayMode::updateFakeTobbies()
{
	float tobby_x = tobby->position.x;
	float tobby_y = tobby->position.y;
	for (uint8_t i = 0; i < level; i++)
	{
		if (i % 2)
		{
			fake_tobbies[i]->position.x = (fake_tobbies_params[i].x * std::pow(tobby_x, 3) + fake_tobbies_params[i].y * std::pow(tobby_x, 2) + fake_tobbies_params[i].z * tobby_x + fake_tobbies_params[i].w * std::cos(tobby_x)) / std::pow(tobby_x, 2);
			fake_tobbies[i]->position.y = (fake_tobbies_params[i].w * std::pow(tobby_y, 3) + fake_tobbies_params[i].z * std::pow(tobby_y, 2) + fake_tobbies_params[i].y * tobby_y + fake_tobbies_params[i].x * std::cos(tobby_y)) / std::pow(tobby_y, 2);
		}
		else
		{
			fake_tobbies[i]->position.x = fake_tobbies_params[i].x * std::cos(tobby_x / 100 * 2 * M_PI) * fake_tobbies_params[i].w * 100;
			fake_tobbies[i]->position.y = fake_tobbies_params[i].y * std::cos(tobby_y / 100 * 2 * M_PI) * fake_tobbies_params[i].w * 100;
		}
	}

	for (uint8_t i = 0; i < level; i++)
	{
		fitInPlayground(fake_tobbies[i]->position);
	}
}
void PlayMode::update(float elapsed)
{
	// move tobby:
	{
		// combine inputs into a move:
		glm::vec2 move = glm::vec2(0.0f);
		if (left.pressed && !right.pressed)
			tobby->position.x -= tobby_speed * elapsed;
		if (!left.pressed && right.pressed)
			tobby->position.x += tobby_speed * elapsed;
		if (down.pressed && !up.pressed)
			tobby->position.y -= tobby_speed * elapsed;
		if (!down.pressed && up.pressed)
			tobby->position.y += tobby_speed * elapsed;
		fitInPlayground(tobby->position);

		updateFakeTobbies();

		// make it so that moving diagonally doesn't go faster:
		if (move != glm::vec2(0.0f))
			move = glm::normalize(move) * tobby_speed * elapsed;

		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 frame_right = frame[0];
		// glm::vec3 up = frame[1];
		glm::vec3 frame_forward = -frame[2];

		camera->transform->position += move.x * frame_right + move.y * frame_forward;
	}

	// reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// update light direction for fun
	if (level > 0 && fake_tobbies_alive[random_tobby])
	{
		light_direction.x += fake_tobbies[random_tobby]->position.x / 5.0f;
		light_direction.y += fake_tobbies[random_tobby]->position.y / 5.0f;
		light_direction = glm::normalize(light_direction);
	}
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	// update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	// set up light type and position for lit_color_texture_program:
	//  TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, light_type);
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(light_direction));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(light_energy));

	glUseProgram(0);

	glClearColor(0.5f, 0.7f, 0.9f, 1.0f);
	glClearDepth(1.0f); // 1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); // this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); // print any errors produced by this setup code

	scene.draw(*camera);

	{ // use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f));

		constexpr float H = 0.09f;
		std::string score_str = "Score: " + std::to_string(score);
		std::string level_str = "Level: " + std::to_string(level);
		std::string display_str = "Mouse motion rotates camera; escape ungrabs mouse | " + score_str + " | " + level_str;
		lines.draw_text(display_str,
						glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(display_str,
						glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
						glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
						glm::u8vec4(0x00, 0x00, 0x00, 0xff));
	}
}