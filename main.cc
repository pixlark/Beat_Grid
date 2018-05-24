#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>

struct RGB {
	char r;
	char g;
	char b;
	RGB() { }
	RGB(char r, char g, char b) : r(r), g(g), b(b) { }
};

struct RGBA {
	char r;
	char g;
	char b;
	char a;
	RGBA() { }
	RGBA(char r, char g, char b, char a) : r(r), g(g), b(b), a(a) { }
};

struct v2 {
	int x;
	int y;
	v2() { }
	v2(int x, int y) : x(x), y(y) { }
};

struct SDL_Context {
	v2 window_size;
	SDL_Window   * window;
	SDL_Renderer * renderer;
};

enum Event {
	EVENT_NONE,
	EVENT_QUIT,
	EVENT_CLICK,
	EVENT_SWITCH,
};

Event next_event(SDL_Context context)
{
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			return EVENT_QUIT;
		case SDL_MOUSEBUTTONDOWN:
			return EVENT_CLICK;
		case SDL_KEYDOWN:
			if (event.key.repeat) break;
			switch(event.key.keysym.scancode) {
			case SDL_SCANCODE_SPACE:
				return EVENT_SWITCH;
			}
		}
	}
	return EVENT_NONE;
}

void set_render_rgb(SDL_Context context, RGB color)
{
	SDL_SetRenderDrawColor(context.renderer,
		color.r, color.g, color.b, 0xff);
}

void set_render_rgba(SDL_Context context, RGBA color)
{
	SDL_SetRenderDrawColor(context.renderer,
		color.r, color.g, color.b, color.a);
}

void render_fill_rect(SDL_Context context, v2 pos, v2 size, RGBA color)
{
	set_render_rgba(context, color);
	SDL_Rect rect = {pos.x, pos.y, size.x, size.y};
	SDL_RenderFillRect(context.renderer, &rect);
}

void render_clear(SDL_Context context, RGB color)
{
	set_render_rgb(context, color);
	SDL_RenderClear(context.renderer);
}

struct Song_Info {
	int bpm;
	float s_per_beat;
	float timer;
	uint64_t last_timestamp;
	bool beat;
	void init(int bpm)
	{
		bpm = 100;
		s_per_beat = 60.0 / bpm;
		timer = 0.0;
		last_timestamp = SDL_GetPerformanceCounter();
		beat = true;
	}
	bool tick()
	{
		// Increase timer by time passed since last frame
		uint64_t timestamp = SDL_GetPerformanceCounter();
		float delta = (float) (timestamp - last_timestamp) /
			SDL_GetPerformanceFrequency();
		timer += delta;
		last_timestamp = timestamp;
		
		// Check if beat has passed
		beat = false;
		if (timer >= (s_per_beat / 4.0)) {
			timer = 0.0;
			beat = true;
		}
	}
};

struct Beat {
	bool beat;
	bool enabled;
};

struct Beat_Grid {
	v2 pad_size;
	int size;
	int padding;
	Beat * grid;
	int beat;
	v2 render_size()
	{
		v2 render_size;
		render_size.x = (pad_size.x * size) + (padding * (size - 1));
		render_size.y = (pad_size.y * size) + (padding * (size - 1));
		return render_size;
	}
	void init(v2 pad_size, int size, int padding)
	{
		this->pad_size = pad_size;
		this->size = size;
		this->padding = padding;
		grid = (Beat*) malloc(sizeof(Beat) * size * size);
		for (int i = 0; i < size*size; i++) grid[i] = {false, false};
		this->beat = (size * size) - 1;
	}
	void click(v2 pos, v2 mouse_pos)
	{
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				SDL_Point point = {mouse_pos.x, mouse_pos.y};
				SDL_Rect rect = {
					pos.x + x * (pad_size.x + padding),
					pos.y + y * (pad_size.y + padding),
					pad_size.x, pad_size.y,
				};
				if (SDL_PointInRect(&point, &rect)) {
					grid[x + y * size].enabled = !grid[x + y * size].enabled;
				}
			}
		}
	}
	bool update(Song_Info song_info)
	{
		if (song_info.beat) {
			grid[beat].beat = false;
			beat = (beat + 1) % (size * size);
			grid[beat].beat = true;
		}
		return song_info.beat && grid[beat].enabled;
	}
	void render(SDL_Context context, v2 pos)
	{
		for (int y = 0; y < size; y++) {
			for (int x = 0; x < size; x++) {
				RGBA color;
				if (grid[x + y * size].enabled) {
					color = RGBA(0xff, 0xff, 0xff, 0xff);
				} else {
					color = RGBA(0xaa, 0xaa, 0xaa, 0xff);
				}
				if (grid[x + y * size].beat) {
					color.a = 0xaa;
				} else {
					color.a = 0x88;
				}
				render_fill_rect(context,
					v2(pos.x + x * (pad_size.x + padding),
					   pos.y + y * (pad_size.y + padding)),
					pad_size, color);
			}
		}
	}
};

struct Instrument {
	Mix_Chunk * sample;
	int init(char * path)
	{
		sample = Mix_LoadWAV(path);
		if (!sample) {
			fprintf(stderr, "Error loading instrument sample:\n\t%s\n", Mix_GetError());
			return 1;
		}
		return 0;
	}
	void play()
	{
		Mix_PlayChannel(-1, sample, 0);
	}
};

enum Instr_Type {
	INSTR_KICK = 0,
	INSTR_SNARE,
	INSTR_COUNT,
};

enum LR {
	LEFT  = 0,
	RIGHT = 1,
};

/*
 * This struct manages two beat grids in a window, that are positioned
 * side-by-side and referred to as the left grid and the right grid.
 */
struct Grid_Manager {
	Instr_Type indexes[2];
	v2 positions[2];
	Beat_Grid grids[INSTR_COUNT];
	void init(v2 window_size, Instr_Type left, Instr_Type right, v2 pad_size, int grid_size, int padding)
	{
		indexes[LEFT]  = left;
		indexes[RIGHT] = right;
		for (int i = 0; i < INSTR_COUNT; i++) {
			grids[i].init(pad_size, grid_size, padding);
		}
		v2 render_size = grids[left].render_size();
		int center_y = window_size.y / 2 - render_size.y / 2;
		int ext_padding = (window_size.x - 2 * render_size.x) / 3;
		positions[LEFT]  = v2(ext_padding, center_y);
		positions[RIGHT] = v2(2 * ext_padding + render_size.x, center_y);
	}
	void switch_instr(LR which)
	{
		indexes[which] = (Instr_Type) ((indexes[which] + 1) % INSTR_COUNT);
	}
	void click(v2 mpos)
	{
		grids[indexes[LEFT]] .click(positions[LEFT],  mpos);
		grids[indexes[RIGHT]].click(positions[RIGHT], mpos);
	}
	void render(SDL_Context context)
	{
		grids[indexes[LEFT]] .render(context, positions[LEFT]);
		grids[indexes[RIGHT]].render(context, positions[RIGHT]);
	}
};

int main()
{
	SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO);
	TTF_Init();
	if (Mix_Init(MIX_INIT_OGG)) {
		printf("Error initializing SDL_Mixer:\n\t%s\n", Mix_GetError());
	}
	Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 128);

	SDL_Context context;

	// Halved iphone 6 resolution (1334x750)
	context.window_size = v2(667, 375);
	
	context.window = SDL_CreateWindow("Beatgrid",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		context.window_size.x, context.window_size.y, 0);
	
	context.renderer = SDL_CreateRenderer(context.window, -1, 0);
	SDL_SetRenderDrawBlendMode(context.renderer, SDL_BLENDMODE_BLEND);
	
	Song_Info song_info;
	song_info.init(100);
	
	Instrument instruments[INSTR_COUNT];
	if (instruments[INSTR_KICK] .init("kick.ogg"))  return 1;
	if (instruments[INSTR_SNARE].init("snare.ogg")) return 1;

	Grid_Manager grid_manager;
	grid_manager.init(context.window_size, INSTR_KICK, INSTR_SNARE, v2(50, 50), 4, 5);
	
	bool running = true;
	while (running) {
		Event event;
		v2 mouse_position;
		SDL_GetMouseState(&mouse_position.x, &mouse_position.y);
		while ((event = next_event(context)) != EVENT_NONE) {
			switch (event) {
			case EVENT_QUIT:
				running = false;
				break;
			case EVENT_CLICK:
				grid_manager.click(mouse_position);
				break;
			case EVENT_SWITCH:
				grid_manager.switch_instr(LEFT);
			}
		}
		for (int i = 0; i < INSTR_COUNT; i++) {
			if (grid_manager.grids[i].update(song_info)) {
				instruments[i].play();
			}
		}
		
		render_clear(context, RGB(0x00, 0x00, 0x00));
		grid_manager.render(context);
		SDL_RenderPresent(context.renderer);

		song_info.tick();
	}
}
