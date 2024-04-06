/**
 * Soitin
 * ======
 * Tällä ohjelmalla voi käyttää näppäimistöä kosketinsoittimena. Näppäimet
 * voi konfiguroida itse, ja asetukset voi tallentaa tiedostoon.
 *
 * Ohjelman toiminta
 * =================
 * Ohjelmassa on soittotila ja asetustila. Kulloinenkin tila ilmoitetaan
 * otsikkopalkissa. Aluksi ohjelma on soittotilassa, ja tilaa vaihdetaan
 * enter-napista. Asetustilassa voi muokata nappeja painamalla napin pohjaan
 * ja valitsemalla korkeuden nuolilla. Delete nollaa valitun napin.
 * Soittotilassa nuolet transponoivat koko soitinta, asetus ei tallennu.
 * Molemmissa tiloissa escape sulkee ohjelman.
 * 
 * Asetustiedosto
 * ==============
 * Ohjelma tallentaa napit tiedostoon. Asetustiedoston nimen voi antaa
 * komentoriviparametrina. Nyt nimi on haitari.txt. Tiedoston jokainen
 * rivi kertoo yhden napin koodin ja korkeuden puoliaskelina a1:sta laskien.
 * Tiedostoa ei kannata muokata itse! Kun sopivat napit on asetettu, tiedoston
 * voi varmuuden vuoksi asettaa vain luku -tilaan.
 */
#include <SDL.h>
#include <ctime>
#include <cmath>
#include <algorithm>
#include <map>
#include <stdexcept>
#include <limits>
#include <fstream>
#include <iostream>

int global_bpm = 140, global_pitch = 0;
int global_beat_num = 0;

const char* conf_name = "soitin.txt";
const double pi = 3.14159265358979323846;
const double fade_time = 0.01;

SDL_AudioSpec spec;

inline double wave(int pitch, double vol, double t, double fade) {
	const double freq = 440 * std::pow(2, pitch / 12.0);
	const double wave = std::sin(freq * t * 2 * pi) * (0.65 + 0.35 * std::sin(freq * t * 2 * pi * 2)) * std::min(1.0, pow((32 - pitch) / 60.0, 2)) * vol;
	if (fade < fade_time) return std::sin(fade / fade_time * pi / 2) * wave;
	fade = 1 - fade;
	if (fade < fade_time) return std::sin(fade / fade_time * pi / 2) * wave;
	return wave;
}

struct Note {
	int pitch;
	double volume;
	double t, fade;
	bool pressed;
	Note(int _pitch, double _volume = 0): pitch(_pitch), volume(_volume), t(0), fade(0), pressed(true) {}
	Note() { throw std::logic_error("Wrong constructor."); }
	
	double next(double dt) {
		t += dt;
		if (pressed) {
			fade = std::min(fade + dt, fade_time);
		} else {
			fade = std::max(fade - dt, 0.0);
		}
		return wave(pitch, volume, t, fade);
	}
	bool ended() {
		return !pressed && fade <= 0;
	}
};
std::map<int, int> pitches;
std::map<int, Note> notes;

template <typename T>
void real_mix(Uint8* _stream, int _len) {
	T* stream = (T*) _stream;
	int len = _len / sizeof(T);
	for (int i = 0; i < len;) {
		double val = 0;
		for (std::map<int, Note>::iterator j = notes.begin(); j != notes.end(); ++j) {
			val += j->second.next(1.0 / spec.freq);
		}
		
		// Scale to [0, 1].
		val = std::min(std::max(-1.0, val), 1.0) / 2 + 0.5;
		
		// Scale to [0, SIZE].
		val *= std::numeric_limits<T>::max() - (double) std::numeric_limits<T>::min();
		
		// Shift to [MIN, MAX].
		T real_val = (T) (val + std::numeric_limits<T>::min());
		
		// Put to each channel.
		for (int j = 0; j < spec.channels; ++j, ++i) {
			stream[i] = real_val;
		}
	}
	for (std::map<int, Note>::iterator j = notes.begin(); j != notes.end();) {
		if (j->second.ended()) {
			notes.erase(j->first);
			j = notes.begin();
		} else {
			++j;
		}
	}
}

void mix(void* userdata, Uint8* stream, int len) {
	switch (spec.format) {
		case AUDIO_S16SYS: real_mix<Sint16>(stream, len); return;
		case AUDIO_U16SYS: real_mix<Uint16>(stream, len); return;
		case AUDIO_S8: real_mix<Sint8>(stream, len); return;
		case AUDIO_U8: real_mix<Uint8>(stream, len); return;
	}
}

bool init_audio(int freq, int channels, int format) {
	SDL_AudioSpec spec_try = {0};
	spec_try.freq = freq;
	spec_try.channels = channels;
	spec_try.samples = 1 << 10;
	while (spec_try.samples > freq / 16) {
		spec_try.samples /= 2;
	}
	if (!spec_try.samples) {
		return false;
	}
	spec_try.format = format;
	spec_try.callback = mix;
	if (SDL_OpenAudio(&spec_try, &spec) < 0) {
		fprintf(stderr, "Couldn't open audio (%d, %d, %#02x): %s\n", freq, channels, format, SDL_GetError());
		return false;
	}
	SDL_PauseAudio(0);
	return true;
}

bool init_audio() {
	const int freq[] = {48000, 44100, 32000, 22050, 16000, 11025, 8000, -1};
	const int channels[] = {1, 2, -1};
	const int format[] = {AUDIO_S16SYS, AUDIO_U16SYS, AUDIO_S8, AUDIO_U8, -1};
	for (int i = 0; freq[i] != -1; ++i) {
		for (int j = 0; channels[j] != -1; ++j) {
			for (int k = 0; format[k] != -1; ++k) {
				if (init_audio(freq[i], channels[j], format[k])) {
					return true;
				}
			}
		}
	}
	return false;
}

bool load_pitches() {
	std::ifstream ifs(conf_name);
	if (!ifs.is_open()) {
		return false;
	}
	while (ifs >> std::ws && !ifs.eof()) {
		int key, pitch;
		if (ifs >> key >> pitch) {
			pitches[key] = pitch;
		}
		ifs.clear();
		ifs.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	}
	return pitches.size() > 5;
}

void print_file_help(std::ostream& ost) {
	ost << "# Asetustiedosto\n";
	ost << "# ==============\n";
	ost << "# Ohjelma tallentaa napit tiedostoon. Asetustiedoston nimen voi antaa\n";
	ost << "# komentoriviparametrina. Nyt nimi on " << conf_name << ". Tiedoston jokainen\n";
	ost << "# rivi kertoo yhden napin koodin ja korkeuden puoliaskelina a1:sta laskien.\n";
	ost << "# Tiedostoa ei kannata muokata itse! Kun sopivat napit on asetettu, tiedoston\n";
	ost << "# voi varmuuden vuoksi asettaa vain luku -tilaan.\n";
}

void print_help(std::ostream& ost) {
	ost << "# Ohjelman toiminta\n";
	ost << "# =================\n";
	ost << "# Ohjelmassa on soittotila ja asetustila. Kulloinenkin tila ilmoitetaan\n";
	ost << "# otsikkopalkissa. Aluksi ohjelma on soittotilassa, ja tilaa vaihdetaan\n";
	ost << "# enter-napista. Asetustilassa voi muokata nappeja painamalla napin pohjaan\n";
	ost << "# ja valitsemalla korkeuden nuolilla. Delete nollaa valitun napin.\n";
	ost << "# Soittotilassa nuolet transponoivat koko soitinta, asetus ei tallennu.\n";
	ost << "# Molemmissa tiloissa escape sulkee ohjelman.\n";
	ost << "# \n";
	print_file_help(ost);
}

void store_pitches() {
	std::ofstream ofs(conf_name);
	if (!ofs.is_open()) {
		return;
	}
	print_file_help(ofs);
	ofs << std::endl;
	for (std::map<int, int>::iterator i = pitches.begin(); i != pitches.end(); ++i) {
		ofs << i->first << '\t' << i->second << '\n';
	}
	std::cout << "Asetukset tallennettu tiedostoon " << conf_name << "!" << std::endl;
}

void init_pitches() {
	const int pitch0 = -18; // 'H' = C4
	pitches[SDLK_F1] = 0 + pitch0 + 0;
	pitches[SDLK_F2] = 0 + pitch0 + 3;
	pitches[SDLK_F3] = 0 + pitch0 + 6;
	pitches[SDLK_F4] = 0 + pitch0 + 9;
	pitches[SDLK_F5] = 0 + pitch0 + 12;
	pitches[SDLK_F6] = 0 + pitch0 + 15;
	pitches[SDLK_F7] = 0 + pitch0 + 18;
	pitches[SDLK_F8] = 0 + pitch0 + 21;
	pitches[SDLK_F9] = 0 + pitch0 + 24;
	pitches[SDLK_F10] = 0 + pitch0 + 27;
	
	pitches[SDLK_1] = 2 + pitch0 + 0;
	pitches[SDLK_2] = 2 + pitch0 + 3;
	pitches[SDLK_3] = 2 + pitch0 + 6;
	pitches[SDLK_4] = 2 + pitch0 + 9;
	pitches[SDLK_5] = 2 + pitch0 + 12;
	pitches[SDLK_6] = 2 + pitch0 + 15;
	pitches[SDLK_7] = 2 + pitch0 + 18;
	pitches[SDLK_8] = 2 + pitch0 + 21;
	pitches[SDLK_9] = 2 + pitch0 + 24;
	pitches[SDLK_0] = 2 + pitch0 + 27;
	
	pitches[SDLK_q] = 4 + pitch0 + 0;
	pitches[SDLK_w] = 4 + pitch0 + 3;
	pitches[SDLK_e] = 4 + pitch0 + 6;
	pitches[SDLK_r] = 4 + pitch0 + 9;
	pitches[SDLK_t] = 4 + pitch0 + 12;
	pitches[SDLK_y] = 4 + pitch0 + 15;
	pitches[SDLK_u] = 4 + pitch0 + 18;
	pitches[SDLK_i] = 4 + pitch0 + 21;
	pitches[SDLK_o] = 4 + pitch0 + 24;
	pitches[SDLK_p] = 4 + pitch0 + 27;
	
	pitches[SDLK_a] = 6 + pitch0 + 0;
	pitches[SDLK_s] = 6 + pitch0 + 3;
	pitches[SDLK_d] = 6 + pitch0 + 6;
	pitches[SDLK_f] = 6 + pitch0 + 9;
	pitches[SDLK_g] = 6 + pitch0 + 12;
	pitches[SDLK_h] = 6 + pitch0 + 15;
	pitches[SDLK_j] = 6 + pitch0 + 18;
	pitches[SDLK_k] = 6 + pitch0 + 21;
	pitches[SDLK_l] = 6 + pitch0 + 24;
	// pitches[SDLK_ö] = 6 + pitch0 + 27;
	
	pitches[SDLK_z] = 8 + pitch0 + 0;
	pitches[SDLK_x] = 8 + pitch0 + 3;
	pitches[SDLK_c] = 8 + pitch0 + 6;
	pitches[SDLK_v] = 8 + pitch0 + 9;
	pitches[SDLK_b] = 8 + pitch0 + 12;
	pitches[SDLK_n] = 8 + pitch0 + 15;
	pitches[SDLK_m] = 8 + pitch0 + 18;
	pitches[SDLK_COMMA] = 8 + pitch0 + 21;
	pitches[SDLK_PERIOD] = 8 + pitch0 + 24;
	pitches[SDLK_MINUS] = 8 + pitch0 + 27;
}

bool change_pitch(int& pitch, int key) {
	switch (key) {
		case SDLK_UP: ++pitch; return true;
		case SDLK_DOWN: --pitch; return true;
		case SDLK_LEFT: pitch -= 12; return true;
		case SDLK_RIGHT: pitch += 12; return true;
		default: return false;
	}
}

bool conf_pitches() {
	std::cout << "Asetustila." << std::endl;
	SDL_WM_SetCaption("Soitin: asetustila", "Soitin");
	int pitch = 0;
	int key = 0;
	SDL_LockAudio();
	notes.insert(std::pair<int, Note>(0, Note(pitch, 0.2)));
	SDL_UnlockAudio();
	for (SDL_Event e; SDL_WaitEvent(&e);) {
		if (e.type == SDL_QUIT) {
			return false;
		}
		if (e.type == SDL_KEYDOWN) {
			// Esc => quit.
			if (e.key.keysym.sym == SDLK_ESCAPE) {
				return false;
			} else
			// Return => end conf.
			if (e.key.keysym.sym == SDLK_RETURN) {
				SDL_LockAudio();
				notes[0].pressed = false;
				SDL_UnlockAudio();
				return true;
			} else
			// Delete => remove mapping.
			if (e.key.keysym.sym == SDLK_DELETE) {
				if (key) {
					pitches.erase(key);
					key = 0;
				} else {
					pitches.clear();
				}
			} else
			// Arrows => change pitch, store new mapping.
			if (change_pitch(pitch, e.key.keysym.sym)) {
				if (key) {
					pitches[key] = pitch;
				}
			} else
			// New key down => restore old mapping or set to current.
			if (!key) {
				key = e.key.keysym.sym;
				if (pitches.find(key) == pitches.end()) {
					pitches[key] = pitch;
				}
				pitch = pitches[key];
			}
		}
		if (e.type == SDL_KEYUP) {
			if (e.key.keysym.sym == key) {
				key = 0;
			}
		}
		SDL_LockAudio();
		notes[0].pitch = pitch;
		notes[0].volume = key ? 1 : 0.3;
		SDL_UnlockAudio();
	}
	return false;
}

bool play() {
	std::cout << "Soittotila." << std::endl;
	SDL_WM_SetCaption("Soitin: soittotila", "Soitin");
	int pitch = 0;
	for (SDL_Event e; SDL_WaitEvent(&e);) {
		if (e.type == SDL_QUIT) {
			return false;
		}
		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_ESCAPE) {
				return false;
			}
			if (e.key.keysym.sym == SDLK_RETURN) {
				return true;
			}
			if (change_pitch(pitch, e.key.keysym.sym)) {
				continue;
			}
			int n = e.key.keysym.sym;
			if (pitches.find(n) != pitches.end()) {
				SDL_LockAudio();
				notes.insert(std::pair<int, Note>(n, Note(pitches[n] + pitch, 0.2)));
				SDL_UnlockAudio();
			}
		}
		if (e.type == SDL_KEYUP) {
			int n = e.key.keysym.sym;
			SDL_LockAudio();
			if (notes.find(n) != notes.end()) {
				notes[n].pressed = false;
			}
			SDL_UnlockAudio();
		}
	}
	return false;
}

#ifdef WIN32
	#ifdef main
		#undef main
	#endif
#endif
int main(int argc, char **argv) {
	if (argc > 1) {
		conf_name = argv[1];
		std::cout << "Asetustiedosto: " << conf_name << std::endl << std::endl;
	}
	print_help(std::cout);
	std::cout << std::endl;
	if (!load_pitches()) {
		init_pitches();
	}
	
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		fprintf(stderr, "Couldn't init: %s\n", SDL_GetError());
		return 1;
	}
	atexit(SDL_Quit);
	if (!SDL_SetVideoMode(640, 8, 0, 0)) {
		fprintf(stderr, "Couldn't set video mode: %s\n", SDL_GetError());
		return 2;
	}
	if (!init_audio()) {
		fprintf(stderr, "Couldn't open audio: %s\n", SDL_GetError());
		return 4;
	}
	while (true) {
		if (!play()) {
			break;
		}
		if (!conf_pitches()) {
			break;
		}
		store_pitches();
	}
	return 0;
}
