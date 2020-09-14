#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//awaiter position:
	glm::vec2 awaiter_at = glm::vec2(230, 220);

	//peach's position
	glm::vec2 peach_at = glm::vec2(100, 100);

	//apple's position
	glm::vec2 apple_at = glm::vec2(50, 0);

	std::array<glm::vec2, 16> color_dots_at = {
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f),
		glm::vec2(0.0f)
	};

	bool player_facing_left = true;
	bool apple_facing_left = true;
	bool peach_facing_left = true;
	bool won = false;
	bool reset = false;

	int player_color_scheme = 3;
	int awaiter_color_scheme = 0;
	int collected = 0;

	float awaiter_timer = 0.0f;
	int awaiter_up = 0;
	float peach_timer = 0.75f;
	int peach_up = 1;
	float apple_timer = 0.5f;
	int apple_up = 0;
	float forgiving_timer = 0.0f;
	bool forgiven = false;

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
