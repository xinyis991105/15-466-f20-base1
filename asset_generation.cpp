#include "GL.hpp"
#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "PPU466.hpp"
#include "data_path.hpp"

#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <stdint.h>

int main() {
	// Referenced ideas proposed on Discord Channels!
	std::vector<PPU466::Palette> palettes;
	std::vector<PPU466::Tile> tiles;
	std::ifstream metafile;
	glm::uvec2 size;
	std::vector<glm::u8vec4> data;
	metafile.open(data_path("../tmp.txt"));
	std::string line;
	if (metafile.is_open()) {
		while (std::getline(metafile, line)) {
			load_png(line, &size, &data, LowerLeftOrigin);
			int w = size[0] / 8;
			int h = size[1] / 8;
			PPU466::Palette p;
			int color_num = 0;
			printf("%d %d\n", w, h);
			for (int i=0; i < w; i++) {
				for (int j=0; j < h; j++) {
					// block i, j
					// one tile per block
					PPU466::Tile t = {
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 }
					};
					for (int a=0; a < 8; a++) {
						for (int b=0; b < 8; b++) {
							// pixel (a,b) in the block
							// a is the number of rows; so the corresponding bit currently is (t.bitx[b] >> a) & 1
							int index = (j * 8 + b) * w * 8 + i * 8 + a;
							glm::u8vec4 cur_color = data[index];
							int there = -1;
							for (int m=0; m < color_num; m++) {
								if (p[m][0] == cur_color[0] && p[m][1] == cur_color[1] &&
									p[m][2] == cur_color[2] && p[m][3] == cur_color[3]) {
									there = m;
									break;
								}
							}
							if (there >= 0) {
								// set tile bits
								t.bit0[b] = t.bit0[b] | ((there & 1) << a);
								t.bit1[b] = t.bit1[b] | (((there >> 1) & 1) << a);
							} else {
								// add color to palette
								p[color_num] = cur_color;
								// set tile bits
								t.bit0[b] = t.bit0[b] | ((color_num & 1) << a);
								t.bit1[b] = t.bit1[b] | (((color_num >> 1) & 1) << a);
								color_num++;
							}
						}
					}
					tiles.push_back(t);
				}
			}
			palettes.push_back(p);
		}
		metafile.close();
		std::ofstream palettes_stream;
		std::ofstream tiles_stream;
		palettes_stream.open(data_path("../palettes.asset"));
		tiles_stream.open(data_path("../tiles.asset"));
		write_chunk(std::string("pale"), palettes, &palettes_stream);
		write_chunk(std::string("tile"), tiles, &tiles_stream);
		palettes_stream.close();
		tiles_stream.close();
	}

}
