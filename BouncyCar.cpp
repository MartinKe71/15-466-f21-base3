#include "BouncyCar.hpp"

#include "LitColorTextureProgram.hpp"

#include "DrawLines.hpp"
#include "Mesh.hpp"
#include "Load.hpp"
#include "gl_errors.hpp"
#include "data_path.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <random>

#define GRAVITY 5.0f
#define BASE_SPEED 10.0f
#define MAX_AIR_TIME 1.5f

GLuint car_meshes_for_lit_color_texture_program = 0;
Load< MeshBuffer > car_meshes(LoadTagDefault, []() -> MeshBuffer const* {
	MeshBuffer const* ret = new MeshBuffer(data_path("car.pnct"));
	car_meshes_for_lit_color_texture_program = ret->make_vao_for_program(lit_color_texture_program->program);
	return ret;
});

Load< Scene > car_scene(LoadTagDefault, []() -> Scene const* {
	return new Scene(data_path("car.scene"), [&](Scene& scene, Scene::Transform* transform, std::string const& mesh_name) {
		Mesh const& mesh = car_meshes->lookup(mesh_name);

		scene.drawables.emplace_back(transform);
		Scene::Drawable& drawable = scene.drawables.back();

		drawable.pipeline = lit_color_texture_program_pipeline;

		drawable.pipeline.vao = car_meshes_for_lit_color_texture_program;
		drawable.pipeline.type = mesh.type;
		drawable.pipeline.start = mesh.start;
		drawable.pipeline.count = mesh.count;

	});
});

Load< Sound::Sample > car_honk_sample(LoadTagDefault, []() -> Sound::Sample const* {
	return new Sound::Sample(data_path("train_horn.opus"));
});

BouncyCar::BouncyCar() : scene(*car_scene) {
	for (auto& transform : scene.transforms) {
		if (transform.name == "Car") {
			gameCar.transform = &transform;

			//std::cout << "Car dimension" << "\n"
			//	<< "     " << gameCar.dims.w << "\n"
			//	<< "     " << gameCar.dims.x << "\n"
			//	<< "     " << gameCar.dims.y << "\n"
			//	<< "     " << gameCar.dims.z << "\n";
		}
		else if (transform.name.find("Cube") != std::string::npos) {
			std::cout << transform.name << "\n";
			tiles.emplace_back(&transform);
		}
		else if (transform.name.find("Box") != std::string::npos) {
			std::cout << "Box: " << transform.name << "\n";
			GameBox box;
			box.transform = &transform;
			boxes.emplace_back(box);
		}
	}
	if (gameCar.transform == nullptr) throw std::runtime_error("car not found.");
	if (tiles.size() != 11) throw std::runtime_error("Failed to capture all tiles");
	std::sort(tiles.begin(), tiles.end(),
		[](Scene::Transform* a, Scene::Transform* b) {return a->position.y > b->position.y; });

	gameCar.transform->position = glm::vec3(0.0f);
	car_base_rotation = gameCar.transform->rotation;

	//get pointer to camera for convenience:
	if (scene.cameras.size() != 1) throw std::runtime_error("Expecting scene to have exactly one camera, but it has " + std::to_string(scene.cameras.size()));
	camera = &scene.cameras.front();
	std::cout << "camera fov: " << camera->fovy << "\n";
	camera_base_rotation = camera->transform->rotation;

	if (scene.lights.size() < 1) throw std::runtime_error("Failed to find light source!");
	light = &scene.lights.front();

	upSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(0.0f, 0.0f, 10.0f), 12.0f);
	downSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(0.0f, 0.0f, -10.0f), 12.0f);
	leftSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(-10.0f, 0.0f, 0.0f), 12.0f);
	rightSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(10.0f, 0.0f, 0.0f), 12.0f);

	upSound->stop();
	downSound->stop();
	leftSound->stop();
	rightSound->stop();
}

BouncyCar::~BouncyCar() {
}

bool BouncyCar::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {

	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			if (gameCar.isGround) {
				space.downs += 1;
				space.pressed = true;
				space.released = false;
			}		
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_ESCAPE) {
			SDL_SetRelativeMouseMode(SDL_FALSE);
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a) {
			left.downs += 1;
			left.pressed = true;
			left.released = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.downs += 1;
			right.pressed = true;
			right.released = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.downs += 1;
			up.pressed = true;
			up.released = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.downs += 1;
			down.pressed = true;
			down.released = false;
			return true;
		}
	}
	else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_SPACE) {
			space.pressed = false;
			space.released = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_a) {
			left.pressed = false;
			left.released = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_d) {
			right.pressed = false;
			right.released = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_w) {
			up.pressed = false;
			up.released = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_s) {
			down.pressed = false;
			down.released = true;
			return true;
		}
	}

	return false;
}

void BouncyCar::update(float elapsed) {
	if (gameOver) {
		if (space.pressed) {
			for (auto& box : boxes) {
				if (box.transform->position.y < camera->transform->position.y) {
					box.transform->position.y += 10.0f;
				}
			}

			BouncyCar();
			gameOver = false;
			instructionText = "Press SPACE bar to make the car FLYYYY";
			gameStateText = "";
		}
		return;
	}

	//move car
	{
		if (space.pressed) {
			SetCarRotation();

			if (gameCar.isGround) {
				gameCar.isGround = false;
			}
				
			gameCar.transform->position.z += gameCar.verticalSpeed * elapsed;
			gameCar.jumpTime += elapsed;
			if (gameCar.jumpTime > MAX_AIR_TIME) {
				space.pressed = false;
				space.released = true;
			}

			if (gameCar.transform->position.z > 3.0f)
				gameCar.transform->position.z = 3.0f;
		}
		if (space.released && !gameCar.isGround) {
			gameCar.transform->position.z -= GRAVITY * elapsed;
		}

		if (!gameCar.isGround) {
			gameCar.transform->rotation = glm::normalize(gameCar.transform->rotation
				* glm::angleAxis(gameCar.angularSpeed * elapsed, gameCar.rotatingAxis));
		}

		if (gameCar.transform->position.z < 0.0f) {
			gameCar.isGround = true;
			gameCar.transform->position.z = 0.0f;
			gameCar.transform->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
			gameCar.totalDowns = 0;
			gameCar.forwardSpeed = 0.0f;
			gameCar.jumpTime = 0.0f;

		}

		UpdateCarBB();

		//std::cout << "isGround: " << gameCar.isGround << "\n";



		//if (space.pressed) {
		//	if (space.downs > 10) space.downs = 10;
		//	gameCar.transform->scale = glm::vec3(1.0f - space.downs * 0.03f);
		//}
		//else if (space.released) {
		//	//if (space.downs > 3) {
		//		gameCar.jumpTime = space.downs * 0.1f + 0.5f;
		//		BounceCar();
		//	//}		

		//	space.downs = 0;
		//	space.released = false;
		//}

		//if (!gameCar.isGround) {
		//	gameCar.transform->position.z += gameCar.verticalSpeed * elapsed;
		//	gameCar.verticalSpeed -= GRAVITY * elapsed;
		//	gameCar.transform->rotation = glm::normalize(gameCar.transform->rotation
		//		* glm::angleAxis(gameCar.angularSpeed * elapsed, gameCar.rotatingAxis));
		//	if (gameCar.transform->position.z > 7.0f) {
		//		gameCar.transform->position.z = 7.0f;
		//		gameCar.verticalSpeed *= -0.1f;
		//		//gameCar.velocity.z *= -0.1f;
		//	}

		//	if (gameCar.transform->scale.x < 1.0f)
		//		gameCar.transform->scale += glm::vec3(elapsed);
		//	else
		//		gameCar.transform->scale = glm::vec3(1.0f);

		//	CalculateScore();
		//}

		//if (gameCar.transform->position.z < 0.0f) {
		//	gameCar.isGround = true;
		//	gameCar.transform->position.z = 0.0f;
		//	gameCar.transform->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		//	gameCar.totalDowns = 0;
		//	gameCar.forwardSpeed = 0.0f;
		//	gameCar.verticalSpeed = 0.0f;
		//	gameCar.jumpTime = 0.0f;

		//	score = 0;
		//	scoreText = "Flying Distance: " + std::to_string(score);
		//	performanceText = "C";
		//	textColor = glm::u8vec4(0xff, 0xff, 0xff, 0x00);
		//}

		//UpdateCarBB();
	}

	//calculate collision
	if (CheckCollision()) {
		std::cout << "GAME OVER" << "\n";
		gameStateText = "GAME OVER";
		instructionText = "Press SPACE again to restart";
		gameOver = true;
		space.pressed = false;
		gameCar.isGround = true;
	}

	//move tiles
	{
		for (auto& tile : tiles) {
			tile->position.y += (gameCar.forwardSpeed + BASE_SPEED) * elapsed;
			if (tile->position.y > camera->transform->position.y) {

				tile->position.y = tiles.back()->position.y - 6.0f ;

				std::sort(tiles.begin(), tiles.end(),
					[](Scene::Transform* a, Scene::Transform* b) {return a->position.y > b->position.y; });

				//for (auto& tile : tiles) {
				//	std::cout << "tile position " <<
				//		tile->position.x <<
				//		" " << tile->position.y <<
				//		" " << tile->position.z <<
				//		std::endl;
				//}
			}
		}
	}

	// play audios
	{
		//if (left.pressed && leftSound->stopped)
		//	leftSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(10.0f, 0.0f, 0.0f), 5.0f);
		//if (right.pressed && rightSound->stopped)
		//	rightSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(-10.0f, 0.0f, 0.0f), 5.0f);
		//if (up.pressed && upSound->stopped)
		//	upSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(0.0f, -10.0f, 0.0f), 5.0f);
		//if (down.pressed && downSound->stopped)
		//	downSound = Sound::play_3D(*car_honk_sample, 1.0f, glm::vec3(0.0f, 0.0f, 0.0f), 5.0f);

	}

	// move boxes
	{
		if (boxes[boxes.size()- boxNum].transform->position.y > camera->transform->position.y) {
			GameBox box = boxes[0];
			boxes.erase(boxes.begin());

			// scale and place the box
			box.transform->scale.x = 2.0f;
			box.transform->scale.y = 0.5f;
			box.transform->scale.z = 0.5f;

			float ofs = (rand() % 5 + 1) * 20.0f;
			box.transform->position = tiles.back()->position + glm::vec3(0.0f, -ofs, 0.0f);
			//box.sfx = Sound::play_3D(*car_honk_sample, 1.0f, box.transform->position, 5.0f);

			boxes.emplace_back(box);

			//std::cout << "\n------------------\n";
			//for (auto& box : boxes) {
			//	std::cout << box.transform->name << "\n";
			//}
			//std::cout << "------------------\n";
		}

		//std::cout << "\n------------------\n";
		for (auto& box : boxes) {
			if (box.transform->position.y < camera->transform->position.y) {
				box.transform->position.y += (3.0f * BASE_SPEED) * elapsed;
				if (box.transform->position.y > -80.0f) {
					if (box.sfx == nullptr || box.sfx->stopped) {
						box.sfx = Sound::play_3D(*car_honk_sample, 1.0f, box.transform->position, 5.0f);
					}

					if (!box.sfx->stopped) {
						box.sfx->set_position(box.transform->position);
					}
				}
			}
		}
		//std::cout << "------------------\n";
	}

	//update listener to camera position:
	{ 
		glm::mat4x3 frame = camera->transform->make_local_to_parent();
		glm::vec3 right = frame[0];
		glm::vec3 at = frame[3];
		Sound::listener.set_position_right(at, right, 1.0f / 60.0f);
	}
}

void BouncyCar::SetCarRotation() {
	/*if (gameCar.isGround) {

		gameCar.rotatingAxis = glm::vec3(0.0f);
		if (left.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else if (right.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, -1.0f, 0.0f);
		}
		if (up.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, 0.0f, 1.0f);
		}

		std::cout << "car rotation " <<
			gameCar.rotatingAxis.x <<
			" " << gameCar.rotatingAxis.y <<
			" " << gameCar.rotatingAxis.z <<
			std::endl;
	}
	else if (gameCar.rotatingAxis == glm::vec3(0.0f)) {
		if (left.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else if (right.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, -1.0f, 0.0f);
		}
		if (up.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, 0.0f, 1.0f);
		}
	}*/

	if (gameCar.isGround)
		gameCar.rotatingAxis = glm::vec3(0.0f);

	if (gameCar.rotatingAxis == glm::vec3(0.0f)) {
		if (left.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, 1.0f, 0.0f);
		}
		else if (right.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, -1.0f, 0.0f);
		}
		if (up.pressed) {
			gameCar.rotatingAxis = glm::vec3(0.0f, 0.0f, 1.0f);
		}
	}

	//if (gameCar.isGround) {
	//	gameCar.jumpStart = gameCar.transform->position.y;
	//}

	//gameCar.isGround = false;
	//gameCar.bounceNum++;
	//gameCar.totalDowns += space.downs;
	//gameCar.verticalSpeed = GRAVITY * gameCar.jumpTime;
	//gameCar.forwardSpeed = gameCar.totalDowns * 0.5f;


}

float BouncyCar::CheckDistance(Scene::Transform* box) {
	float distance = glm::length(box->position - gameCar.transform->position);
	//std::cout << "Distance: " << distance << "\n";
	return distance;
}

bool BouncyCar::SeparateAxes(Scene::Transform* box) {
	// create box boundbing box
	glm::vec3 box_ofs = box->scale * 0.5f;
	glm::vec3 box_pos = box->position;
	std::vector<glm::vec3> boxBounds{
		glm::vec3(box_pos.x + box_ofs.x, box_pos.y + box_ofs.y, box_pos.z + box_ofs.z),
		glm::vec3(box_pos.x + box_ofs.x, box_pos.y - box_ofs.y, box_pos.z + box_ofs.z),
		glm::vec3(box_pos.x + box_ofs.x, box_pos.y + box_ofs.y, box_pos.z - box_ofs.z),
		glm::vec3(box_pos.x + box_ofs.x, box_pos.y - box_ofs.y, box_pos.z - box_ofs.z),
		glm::vec3(box_pos.x - box_ofs.x, box_pos.y + box_ofs.y, box_pos.z + box_ofs.z),
		glm::vec3(box_pos.x - box_ofs.x, box_pos.y - box_ofs.y, box_pos.z + box_ofs.z),
		glm::vec3(box_pos.x - box_ofs.x, box_pos.y + box_ofs.y, box_pos.z - box_ofs.z),
		glm::vec3(box_pos.x - box_ofs.x, box_pos.y - box_ofs.y, box_pos.z - box_ofs.z),
	};
	//std::cout << "Checking Axes" << "\n";
	std::vector<glm::vec3> axes{
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	};
	glm::vec3 car_x = glm::normalize(glm::vec3(1.0f, 0.0f, 0.0f) * gameCar.transform->rotation);
	glm::vec3 car_y = glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f) * gameCar.transform->rotation);
	glm::vec3 car_z = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f) * gameCar.transform->rotation);
	axes.emplace_back(car_x);
	axes.emplace_back(car_y);
	axes.emplace_back(car_z);

	for (auto& axis : axes) {
		float carMin = FLT_MAX;
		float carMax = -FLT_MAX;

		for (auto& bound : gameCar.updatedBounds) {
			float val = glm::dot(bound, axis);
			if (val < carMin) carMin = val;
			if (val > carMax) carMax = val;
		}

		float boxMin = FLT_MAX;
		float boxMax = -FLT_MAX;
		for (auto& bound : boxBounds) {
			float val = glm::dot(bound, axis);
			if (val < boxMin) boxMin = val;
			if (val > boxMax) boxMax = val;
		}

		//std::cout << "---------------\n";
		//std::cout << "Car Min: " << carMin << "\n";
		//std::cout << "Car Max: " << carMax << "\n";
		//std::cout << "Box Min: " << boxMin << "\n";
		//std::cout << "Box Max: " << boxMax << "\n";
		//std::cout << "---------------\n\n";

		//if ((carMin > boxMin && carMin < boxMax) ||
		//	(boxMin > carMin && boxMin < carMax)) {
		//	return false;
		//}
		if (carMax < boxMin || boxMax < carMin) {
			return false;
		}
	}

	return true;
}

bool BouncyCar::CheckCollision() {
	for (auto& box : boxes) {
		//std::cout << "Checking Collision" << "\n";
		if (CheckDistance(box.transform) < 6.0f && SeparateAxes(box.transform)) {
			return true;
		}
	}
	return false;
}

void BouncyCar::UpdateCarBB() {
	gameCar.updatedBounds.clear();
	assert(gameCar.updatedBounds.empty());

	for (auto& bound : gameCar.originalBounds) {
		glm::vec3 newBound = glm::normalize(gameCar.transform->rotation) * bound
			+ gameCar.transform->position;
		gameCar.updatedBounds.emplace_back(newBound);
	}
	return;
}

void BouncyCar::draw(glm::uvec2 const& drawable_size) {
	//update camera aspect ratio for drawable:
	camera->aspect = float(drawable_size.x) / float(drawable_size.y);

	//set up light type and position for lit_color_texture_program:
	// TODO: consider using the Light(s) in the scene to do this
	glUseProgram(lit_color_texture_program->program);
	glUniform1i(lit_color_texture_program->LIGHT_TYPE_int, 2);
	glUniform3fv(lit_color_texture_program->LIGHT_LOCATION_vec3, 1, glm::value_ptr(gameCar.transform->position + glm::vec3(0.0f, 1.4f, 6.0f)));
	glUniform3fv(lit_color_texture_program->LIGHT_DIRECTION_vec3, 1, glm::value_ptr(glm::vec3(0.0f, -0.3f, -0.7f)));
	glUniform1f(lit_color_texture_program->LIGHT_CUTOFF_float, std::cos(3.1415926f * 0.3f));
	glUniform3fv(lit_color_texture_program->LIGHT_ENERGY_vec3, 1, glm::value_ptr(50.0f * glm::vec3(1.0f, 1.0f, 0.9f)));
	glUseProgram(0);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f); //1.0 is actually the default value to clear the depth buffer to, but FYI you can change it.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS); //this is the default depth comparison function, but FYI you can change it.

	GL_ERRORS(); //print any errors produced by this setup code

	scene.draw(*camera);

	{ //use DrawLines to overlay some text:
		glDisable(GL_DEPTH_TEST);
		float aspect = float(drawable_size.x) / float(drawable_size.y);
		DrawLines lines(glm::mat4(
			1.0f / aspect, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f
		));

		constexpr float H = 0.09f;
		float ofs = 2.0f / drawable_size.y;
		lines.draw_text(instructionText,
			glm::vec3(-aspect + 0.1f * H, -1.0 + 0.1f * H, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(instructionText,
			glm::vec3(-aspect + 0.1f * H + ofs, -1.0 + +0.1f * H + ofs, 0.0),
			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
			glm::u8vec4(0xff, 0xff, 0xff, 0x00));

		lines.draw_text(gameStateText,
			glm::vec3(aspect * 0.4f, -0.4f + 2.1f * H, 0.0),
			glm::vec3(2.0f * H, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f * H, 0.0f),
			glm::u8vec4(0x00, 0x00, 0x00, 0x00));
		lines.draw_text(gameStateText,
			glm::vec3(aspect * 0.4f + ofs, -0.4f + 2.1f * H + ofs, 0.0),
			glm::vec3(2.0f * H, 0.0f, 0.0f), glm::vec3(0.0f, 2.0f * H, 0.0f),
			textColor);
	}
}