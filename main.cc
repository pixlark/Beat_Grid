#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

struct SDL_Context {
	SDL_Window   * window;
	SDL_Renderer * renderer;
};

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

enum Event {
	EVENT_NONE,
	EVENT_QUIT,
	EVENT_CLICK,
};

Event next_event(SDL_Context context)
{
	SDL_Event event;
	if (!SDL_PollEvent(&event)) return EVENT_NONE;
	switch (event.type) {
	case SDL_QUIT:
		return EVENT_QUIT;
	case SDL_MOUSEBUTTONDOWN:
		return EVENT_CLICK;
	}
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
	static const int padding = 5;
	static const int size = 4;
	v2 pos;
	v2 pad_size;
	Beat grid [size*size];
	int beat;
	void init(v2 pos, v2 pad_size)
	{
		this->pos = pos;
		this->pad_size = pad_size;
		for (int i = 0; i < size*size; i++) grid[i] = {false, false};
		this->beat = (size * size) - 1;
	}
	void click(v2 mouse_pos)
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
	void update(Song_Info song_info)
	{
		if (song_info.beat) {
			grid[beat].beat = false;
			beat = (beat + 1) % (size * size);
			grid[beat].beat = true;
		}
	}
	void render(SDL_Context context)
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

int main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	Mix_Init(MIX_INIT_OGG);

	SDL_Context context;
	
	context.window = SDL_CreateWindow("DAW",
		SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		// 1134 x 750
		567, 375, 0);
	
	context.renderer = SDL_CreateRenderer(context.window, -1, 0);
	SDL_SetRenderDrawBlendMode(context.renderer, SDL_BLENDMODE_BLEND);
	
	Song_Info song_info;
	song_info.init(100);
	
	Beat_Grid beat_grid;
	beat_grid.init(v2(0, 0), v2(50, 50));
	
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
				beat_grid.click(mouse_position);
				break;
			}
		}
		beat_grid.update(song_info);
		
		render_clear(context, RGB(0x00, 0x00, 0x00));
		beat_grid.render(context);
		SDL_RenderPresent(context.renderer);

		song_info.tick();
	}
}