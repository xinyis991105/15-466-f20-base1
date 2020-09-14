#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "Load.hpp"
#include "data_path.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>
#include <fstream>

struct StreamContainer {
	std::vector<PPU466::Palette> palettes;
};

std::ifstream palette_stream;
std::ifstream tile_stream;

Load<void> ps(LoadTagDefault, []() {
	palette_stream.open(data_path("../palettes.asset"));
	tile_stream.open(data_path("../tiles.asset"));
	return;
});

PlayMode::PlayMode() {
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	std::vector<PPU466::Palette> palettes;
	std::vector<PPU466::Tile> tiles;
	read_chunk(palette_stream, std::string("pale"), &palettes);
	read_chunk(tile_stream, std::string("tile"), &tiles);
	palette_stream.close();
	tile_stream.close();
	for (int i=0; i < tiles.size(); i++) {
		ppu.tile_table[32+i] = tiles[i];
	}
	for (int i=0; i < palettes.size(); i++) {
		ppu.palette_table[i] = palettes[i];
	}
	ppu.background_color = glm::u8vec4(0,255,0,255);
	for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
		for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
			uint32_t a = x % 2;
			uint32_t b = y % 2;
			if (!a && !b) {
				ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 80;
			} else if (!a && b) {
				ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 81;
			} else if (a && !b) {
				ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 82;
			} else {
				ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 83;
			}
		}
	}
	for (int i=0; i < 4; i++) {
		for (int j=0; j < 4; j++) {
			color_dots_at[i * 4 + j].x = i * 60 + rand() % 50;
			color_dots_at[i * 4 + j].y = j * 60 + rand() % 50;
		}
	}
}

PlayMode::~PlayMode() {
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.downs += 1;
			left.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.downs += 1;
			right.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.downs += 1;
			up.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.downs += 1;
			down.pressed = true;
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE && !won) {
			if (collected == 65535) {
				player_color_scheme = 0;
			} else {
				player_color_scheme = player_color_scheme % 3 + 1;
			}
			return true;
		} else if (evt.key.keysym.sym == SDLK_SPACE && won) {
			// reset
			reset = true;
			return true;
		}
	} else if (evt.type == SDL_KEYUP) {
		if (evt.key.keysym.sym == SDLK_LEFT) {
			left.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_RIGHT) {
			right.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_UP) {
			up.pressed = false;
			return true;
		} else if (evt.key.keysym.sym == SDLK_DOWN) {
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed) {
	if (reset) {
		player_at = glm::vec2(0.0f);
		awaiter_at = glm::vec2(230,220);
		won = false;
		reset = false;
		collected = 0;
		player_color_scheme = 3;
		awaiter_timer = 0.0f;
		for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
			for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
				uint32_t a = x % 2;
				uint32_t b = y % 2;
				if (!a && !b) {
					ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 80;
				} else if (!a && b) {
					ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 81;
				} else if (a && !b) {
					ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 82;
				} else {
					ppu.background[x+PPU466::BackgroundWidth*y] = 6 << 8 | 83;
				}
			}
		}
		for (int i=0; i < 4; i++) {
			for (int j=0; j < 4; j++) {
				color_dots_at[i * 4 + j].x = i * 60 + rand() % 50;
				color_dots_at[i * 4 + j].y = j * 60 + rand() % 50;
			}
		}
		return;
	}
	auto x_dist = player_at.x - awaiter_at.x;
	auto y_dist = player_at.y - awaiter_at.y;
	if (y_dist * y_dist + x_dist * x_dist <= 240.0f) {
		printf("%d\n", collected);
		if (collected == 65535 && player_color_scheme == 0) {
			won = true;
			return;
		}
	}

	for (int i=0; i < 16; i++) {
		float x_rel = color_dots_at[i].x - player_at.x;
		float y_rel = color_dots_at[i].y - player_at.y;
		if (x_rel * x_rel + y_rel * y_rel <= 100.0f && player_color_scheme != 3) {
			collected = collected | (1 << i);
			break;
		}
	}

	//slowly rotates through [0,1):
	// (will be used to set background color)
	awaiter_timer += elapsed;
	peach_timer += elapsed;
	apple_timer += elapsed;
	forgiving_timer += elapsed;
	if (awaiter_timer >= 1.0f) {
		awaiter_up = !awaiter_up;
		awaiter_timer = 0.0f;
	}
	if (peach_timer >= 1.0f) {
		peach_up = !peach_up;
		peach_timer = 0.0f;
	}
	if (apple_timer >= 1.0f) {
		apple_up = !apple_up;
		apple_timer = 0.0f;
	}
	if (forgiving_timer >= 2.0f && forgiven) {
		forgiven = false;
	}
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	float x_rel_apple = player_at.x - apple_at.x;
	float y_rel_apple = player_at.y - apple_at.y;
	float x_rel_peach = player_at.x - peach_at.x;
	float y_rel_peach = player_at.y - peach_at.y;

	if ((x_rel_apple * x_rel_apple + y_rel_apple * y_rel_apple <= 100.0f || 
		 x_rel_peach * x_rel_peach + y_rel_peach * y_rel_peach <= 100.0f) &&
		!forgiven && player_color_scheme != 0) {
		for (int i=0; i < 16; i++) {
			if ((collected >> i) & 1) {
				collected = collected & (~(1 << i));
				forgiven = true;
				forgiving_timer = 0.0f;
				break;
			}
		}
	}

	constexpr float PlayerSpeed = 30.0f;
	if (left.pressed) {
		player_facing_left = true;
		player_at.x -= PlayerSpeed * elapsed;
	}
	if (right.pressed) {
		player_facing_left = false;
		player_at.x += PlayerSpeed * elapsed;
	}
	if (down.pressed) player_at.y -= PlayerSpeed * elapsed;
	if (up.pressed) player_at.y += PlayerSpeed * elapsed;

	if (player_color_scheme == 1) {
		// peach color; apple unhappy
		float div = int(x_rel_apple / 100.0f);
		div = div > 0.0f ? div : -div;
		div += 2.0f;
		apple_at.x = apple_at.x + x_rel_apple * div * elapsed;
		apple_at.y = apple_at.y + y_rel_apple * div * elapsed;
		if (x_rel_apple > 0.0f) {
			apple_facing_left = false;
		} else {
			apple_facing_left = true;
		}

		if (peach_facing_left) {
			peach_at.x = peach_at.x - PlayerSpeed / 2.0f * elapsed;
		} else {
			peach_at.x = peach_at.x + PlayerSpeed / 2.0f * elapsed;
		}
	} else if (player_color_scheme == 2) {
		// apple color; peach unhappy
		float div = int(x_rel_peach / 100.0f);
		div = div > 0.0f ? div : -div;
		div += 2.0f;
		peach_at.x = peach_at.x + x_rel_peach * div * elapsed;
		peach_at.y = peach_at.y + y_rel_peach * div * elapsed;
		if (x_rel_peach > 0.0f) {
			peach_facing_left = false;
		} else {
			peach_facing_left = true;
		}

		if (apple_facing_left) {
			apple_at.x = apple_at.x - PlayerSpeed / 2.0f * elapsed;
		} else {
			apple_at.x = apple_at.x + PlayerSpeed / 2.0f * elapsed;
		}
	} else if (player_color_scheme == 3) {
		// clear color; cherry unhappy (unable to collect dot)
		float div = 0.1f;
		peach_facing_left = x_rel_peach <= 0.0f;
		apple_facing_left = x_rel_apple <= 0.0f;
		peach_at.x = peach_at.x + x_rel_peach * div * elapsed;
		peach_at.y = peach_at.y + y_rel_peach * div * elapsed;
		apple_at.x = apple_at.x + x_rel_apple * div * elapsed;
		apple_at.y = apple_at.y + y_rel_apple * div * elapsed;
	} else {
		// do nothing
	}

	if (player_at.y < 0.0f) {
		player_at.y = 0.0f;
	} else if (player_at.y > 224.0f) {
		player_at.y = 224.0f;
	}
	if (player_at.x < 0.0f) {
		player_at.x = 0.0f;
	} else if (player_at.x > 240.0f) {
		player_at.x = 240.0f;
	}

	if (apple_at.y < 0.0f) {
		apple_at.y = 0.0f;
	} else if (apple_at.y > 224.0f) {
		apple_at.y = 224.0f;
	}
	if (apple_at.x < 0.0f) {
		apple_at.x = 0.0f;
	} else if (apple_at.x > 240.0f) {
		apple_at.x = 240.0f;
	}

	if (peach_at.y < 0.0f) {
		peach_at.y = 0.0f;
	} else if (peach_at.y > 224.0f) {
		peach_at.y = 224.0f;
	}
	if (peach_at.x < 0.0f) {
		peach_at.x = 0.0f;
		peach_facing_left = false;
	} else if (player_at.x > 240.0f) {
		peach_at.x = 240.0f;
		peach_facing_left = true;
	}

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;
}

void PlayMode::draw(glm::uvec2 const &drawable_size) {
	//--- set ppu state based on game state ---
	if (won) {
		for (uint32_t y = 0; y < PPU466::BackgroundHeight; ++y) {
			for (uint32_t x = 0; x < PPU466::BackgroundWidth; ++x) {
				uint32_t a = x % 2;
				uint32_t b = y % 2;
				if (!a && !b) {
					ppu.background[x+PPU466::BackgroundWidth*y] = 80;
				} else if (!a && b) {
					ppu.background[x+PPU466::BackgroundWidth*y] = 81;
				} else if (a && !b) {
					ppu.background[x+PPU466::BackgroundWidth*y] = 82;
				} else {
					ppu.background[x+PPU466::BackgroundWidth*y] = 83;
				}
			}
		}
		ppu.background_color = glm::u8vec4(0,255,0,255);
		player_at.x = 128.0f - 16.0f;
		player_at.y = 120.0f;
		awaiter_at.x = 128.0f;
		awaiter_at.y = 120.f;
		player_facing_left = false;
		player_color_scheme = 0;
		//background color will be some hsv-like fade:
		ppu.background_color = glm::u8vec4(
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 0.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 1.0f / 3.0f) ) ) ))),
		std::min(255,std::max(0,int32_t(255 * 0.5f * (0.5f + std::sin( 2.0f * M_PI * (background_fade + 2.0f / 3.0f) ) ) ))),
		0xff
		);
	}

	//player sprite 0-4:
	if (player_facing_left) {
		ppu.sprites[0].x = int32_t(player_at.x);
		ppu.sprites[0].y = int32_t(player_at.y);
		ppu.sprites[0].index = 32;
		ppu.sprites[0].attributes = player_color_scheme;
		ppu.sprites[1].x = ppu.sprites[0].x;
		ppu.sprites[1].y = ppu.sprites[0].y + 8;
		ppu.sprites[1].index = 33;
		ppu.sprites[1].attributes = player_color_scheme;
		ppu.sprites[2].x = ppu.sprites[0].x + 8;
		ppu.sprites[2].y = ppu.sprites[0].y;
		ppu.sprites[2].index = 34;
		ppu.sprites[2].attributes = player_color_scheme;
		ppu.sprites[3].x = ppu.sprites[0].x + 8;
		ppu.sprites[3].y = ppu.sprites[0].y + 8;
		ppu.sprites[3].index = 35;
		ppu.sprites[3].attributes = player_color_scheme;
	} else {
		if (won && !awaiter_up) {
			ppu.sprites[0].y = int32_t(player_at.y) + 2;
		} else {
			ppu.sprites[0].y = int32_t(player_at.y);
		}
		ppu.sprites[0].x = int32_t(player_at.x);
		ppu.sprites[0].index = 36;
		ppu.sprites[0].attributes = player_color_scheme;
		ppu.sprites[1].x = ppu.sprites[0].x;
		ppu.sprites[1].y = ppu.sprites[0].y + 8;
		ppu.sprites[1].index = 37;
		ppu.sprites[1].attributes = player_color_scheme;
		ppu.sprites[2].x = ppu.sprites[0].x + 8;
		ppu.sprites[2].y = ppu.sprites[0].y;
		ppu.sprites[2].index = 38;
		ppu.sprites[2].attributes = player_color_scheme;
		ppu.sprites[3].x = ppu.sprites[0].x + 8;
		ppu.sprites[3].y = ppu.sprites[0].y + 8;
		ppu.sprites[3].index = 39;
		ppu.sprites[3].attributes = player_color_scheme;
	}

	if (awaiter_up) {
		ppu.sprites[4].y = int32_t(awaiter_at.y) + 2;
	} else {
		ppu.sprites[4].y = int32_t(awaiter_at.y);
	}
	ppu.sprites[4].x = int32_t(awaiter_at.x);
	ppu.sprites[4].index = 32;
	ppu.sprites[4].attributes = awaiter_color_scheme;

	ppu.sprites[5].x = ppu.sprites[4].x;
	ppu.sprites[5].y = ppu.sprites[4].y + 8;
	ppu.sprites[5].index = 33;
	ppu.sprites[5].attributes = awaiter_color_scheme;

	ppu.sprites[6].x = ppu.sprites[4].x + 8;
	ppu.sprites[6].y = ppu.sprites[4].y;
	ppu.sprites[6].index = 34;
	ppu.sprites[6].attributes = awaiter_color_scheme;

	ppu.sprites[7].x = ppu.sprites[4].x + 8;
	ppu.sprites[7].y = ppu.sprites[4].y + 8;
	ppu.sprites[7].index = 35;
	ppu.sprites[7].attributes = awaiter_color_scheme;

	// draw peach
	int index = 8;
	int tile_index = 64;
	if (!peach_facing_left) {
		tile_index = 68;
	}
	if (player_color_scheme == 1 && peach_up) {
		ppu.sprites[index].y = int32_t(peach_at.y) + 2;
	} else {
		ppu.sprites[index].y = int32_t(peach_at.y);
	}
	ppu.sprites[index].x = int32_t(peach_at.x);
	ppu.sprites[index].index = tile_index;
	ppu.sprites[index].attributes = 4;
	ppu.sprites[index+1].x = ppu.sprites[index].x;
	ppu.sprites[index+1].y = ppu.sprites[index].y + 8;
	ppu.sprites[index+1].index = tile_index + 1;
	ppu.sprites[index+1].attributes = 4;
	ppu.sprites[index+2].x = ppu.sprites[index].x + 8;
	ppu.sprites[index+2].y = ppu.sprites[index].y;
	ppu.sprites[index+2].index = tile_index + 2;
	ppu.sprites[index+2].attributes = 4;
	ppu.sprites[index+3].x = ppu.sprites[index].x + 8;
	ppu.sprites[index+3].y = ppu.sprites[index].y + 8;
	ppu.sprites[index+3].index = tile_index + 3;
	ppu.sprites[index+3].attributes = 4;

	// draw apples
	if (apple_facing_left) {
		tile_index = 72;
	} else {
		tile_index = 76;
	}
	index = 12;
	if (player_color_scheme == 2 && apple_up) {
		ppu.sprites[index].y = int32_t(apple_at.y) + 2;
	} else {
		ppu.sprites[index].y = int32_t(apple_at.y);
	}
	ppu.sprites[index].x = int32_t(apple_at.x);
	ppu.sprites[index].index = tile_index;
	ppu.sprites[index].attributes = 5;
	ppu.sprites[index+1].x = ppu.sprites[index].x;
	ppu.sprites[index+1].y = ppu.sprites[index].y + 8;
	ppu.sprites[index+1].index = tile_index + 1;
	ppu.sprites[index+1].attributes = 5;
	ppu.sprites[index+2].x = ppu.sprites[index].x + 8;
	ppu.sprites[index+2].y = ppu.sprites[index].y;
	ppu.sprites[index+2].index = tile_index + 2;
	ppu.sprites[index+2].attributes = 5;
	ppu.sprites[index+3].x = ppu.sprites[index].x + 8;
	ppu.sprites[index+3].y = ppu.sprites[index].y + 8;
	ppu.sprites[index+3].index = tile_index + 3;
	ppu.sprites[index+3].attributes = 5;

	for (int i=0; i < 16; i++) {
		int index = 36 + i;
		ppu.sprites[index].x = int32_t(color_dots_at[i].x);
		ppu.sprites[index].y = int32_t(color_dots_at[i].y);
		ppu.sprites[index].index = 84;
		ppu.sprites[index].attributes = (((collected >> i) & 1) << 7) | 7;
	}

	//--- actually draw ---
	ppu.draw(drawable_size);
}
