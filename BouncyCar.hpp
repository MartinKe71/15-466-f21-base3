#include "Mode.hpp"

#include "Scene.hpp"
#include "Sound.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct BouncyCar : Mode {
	BouncyCar();
	virtual ~BouncyCar();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
		uint8_t released = 0;
	} left, right, down, up, space;

	struct GameCar {
		// transform related
		Scene::Transform* transform = nullptr;

		// bounding box
		glm::vec4 dims{ 2.5f, 5.6f, 1.7f, 1.6f};

		std::vector<glm::vec3> originalBounds{
			glm::vec3(dims.x / 2.0f, dims.y / 2.0f, 0.0f),
			glm::vec3(dims.x / 2.0f, dims.y / 2.0f, -dims.z / 2.0f),
			glm::vec3(dims.x / 2.0f, -dims.y / 2.0f, 0.0f),
			glm::vec3(dims.x / 2.0f, -dims.y / 2.0f, -dims.z / 2.0f),
			glm::vec3(-dims.x / 2.0f, dims.y / 2.0f, 0.0f),
			glm::vec3(-dims.x / 2.0f, dims.y / 2.0f, -dims.z / 2.0f),
			glm::vec3(-dims.x / 2.0f, -dims.y / 2.0f, 0.0f),
			glm::vec3(-dims.x / 2.0f, -dims.y / 2.0f, -dims.z / 2.0f),
			glm::vec3(dims.x / 2.0f, dims.w / 2.0f, dims.z / 2.0f),
			glm::vec3(dims.x / 2.0f, -dims.w / 2.0f, dims.z / 2.0f),
			glm::vec3(-dims.x / 2.0f, dims.w / 2.0f, dims.z / 2.0f),
			glm::vec3(-dims.x / 2.0f, -dims.w / 2.0f, dims.z / 2.0f),
		};
		std::vector<glm::vec3> updatedBounds;

		// rotation
		float verticalSpeed = 3.0f;
		float forwardSpeed = 0.0f;
		//glm::vec3 velocity = glm::vec3(0.0f);
		glm::vec3 rotatingAxis;
		float angularSpeed = 2 * glm::pi<float>();;
		float jumpTime = 0.0f;
		float jumpStart = 0.0f;

		// player input
		uint16_t bounceNum = 0;
		uint16_t totalDowns = 0;

		// game state
		bool isGround = true;
	} gameCar;

	struct GameBox {
		Scene::Transform* transform;
		std::shared_ptr< Sound::PlayingSample > sfx;
	};

	bool gameOver = false;
	std::string instructionText = "Press SPACE bar to make the car FLYYYY";
	std::string gameStateText = "";
	glm::u8vec4 textColor = glm::u8vec4(0xff, 0xff, 0xff, 0x00);

	//local copy of the game scene (so code can change it during gameplay):
	Scene scene;

	//hexapod leg to wobble:
	Scene::Transform* car = nullptr;

	glm::quat car_base_rotation;
	glm::quat camera_base_rotation;

	//camera:
	Scene::Camera* camera = nullptr;

	//light
	Scene::Light* light = nullptr;

	//tiles
	std::vector<Scene::Transform*> tiles;

	//boxes
	int boxNum = 1;
	std::vector<GameBox> boxes;

	//music coming from the tip of the leg (as a demonstration):
	std::shared_ptr< Sound::PlayingSample > upSound;
	std::shared_ptr< Sound::PlayingSample > downSound;
	std::shared_ptr< Sound::PlayingSample > leftSound;
	std::shared_ptr< Sound::PlayingSample > rightSound;

	//help functions
	void SetCarRotation();
	void UpdateCarBB();
	bool CheckCollision();
	float CheckDistance(Scene::Transform* box);
	bool SeparateAxes(Scene::Transform* box);
};
