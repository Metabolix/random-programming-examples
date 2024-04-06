#include <cmath>
#include <ctime>
#include <cstdlib>
#include <SDL.h>
#include <SDL_rotozoom.h>

const Uint32 hwsurf = SDL_HWSURFACE;
//const Uint32 hwsurf = SDL_SWSURFACE;
//const Uint32 hwsurf = 0;

const int koko_x = 20;
const int koko_y = 15;

const int max_arpa = 0xffffff;
int arpa() {
	static int i;
	if (!i) {
		i = (int) time(0);
		i *= i + 5;
	}
	i = i * 2089 + 536870911;
	int j = i % (max_arpa + 1);
	return j < 0 ? -j : j;
}

enum state_t {
	S_MENU = 0x01,
	S_PELI = 0x02,
	S_DEAD = 0x04,
	SFORCE = 0x7fff
};
Uint16 state = S_MENU, prev_state;

// Löydämme pikselistä eri värit näiden avulla.
// Määritellään täällä, koska ovat arkkitehtuuririippuvaisia
Uint32
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000, rshift = 24,
	gmask = 0x00ff0000, gshift = 16,
	bmask = 0x0000ff00, bshift = 8,
	amask = 0x000000ff, ashift = 0;
#else
	rmask = 0x000000ff, rshift = 0,
	gmask = 0x0000ff00, gshift = 8,
	bmask = 0x00ff0000, bshift = 16,
	amask = 0xff000000, ashift = 24;
#endif

// Funktiot, joilla saadaan helpommin pinnan bpp ja tietty piste
inline Uint32 bpp(SDL_Surface *s) {
	return s->format->BytesPerPixel;
}
inline Uint32& piste(SDL_Surface *s, int x, int y) {
	return *(Uint32*)((Uint8*) s->pixels + y * s->pitch + x * bpp(s));
}

template <typename T> T pow2(T x) {
	return x*x;
}

// Rakenne, jossa pidetään sijaintia
struct koord_t {
	float x, y, r;
};

float manhattan(const koord_t &a, const koord_t &b) {
	return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

bool osuu(const koord_t &a, const koord_t &b) {
	return pow2(a.x - b.x) + pow2(a.y - b.y) < pow2(a.r + b.r);
}

struct omena_t {
	int aika, alkuaika;
	koord_t sij;
	omena_t *aiempi;
};

int omppu_alpha(const omena_t &o) {
	return (((128 * o.aika) / o.alkuaika) + 128);
}

int alkuomenia = 1;
omena_t uusi_omena;

int syoty_jo = 0;
int omenatiheys;

// mato linkitettynä listana
struct matopala_t {
	koord_t sij;
	matopala_t *nenampi, *hannampi;
};

namespace mato {
	float suunta;
	float nopeus_f;
	int nopeus;
	int nenavaihe, hantavaihe;
	matopala_t *nena, *hanta;
	koord_t tuleva_sij;
}

// Ja grafiikat
namespace pinnat {
	SDL_Surface *ruutu = 0;
	SDL_Surface *tausta = 0, *rot_tausta = 0;
	SDL_Surface *omena = 0, *alfaomena = 0;
	SDL_Surface *matopallo = 0;
}

namespace pinnat {
	int alusta();
	void tuhoa();
	int laske_bitit(Uint32 x);
	int laske_shift(Uint32 x);
	SDL_Surface *tee_pinta(Uint32 w, Uint32 h, Uint32 c = 0);
	void blittaa(SDL_Surface * src, float x, float y, SDL_Surface * dest);
	void rotozoom_blittaa(SDL_Surface * src, int x, int y, SDL_Surface * dest, double kulma, double skaalaus, int smooth);
}

void lopeta_peli();
int aloita_peli();
void vaihtokohta();
void tormasi_itseensa();
void tormasi_seinaan();
void syo_omena();
void tee_omena();
void lopetusfunktio();

void piirra_mato();
void piirra_nena();
void piirra_hanta();
void piirra_omenat();
void piirra_omena(const omena_t &omppu);

int pinnat::laske_bitit(Uint32 x) {
	Uint32 i;
	int j = 0;
	for (i = 1; i; i <<= 1) {
		if (x & i) {
			++j;
		}
	}
	return j;
}

int pinnat::laske_shift(Uint32 x) {
	int i;
	if (!x) return 0;
	for (i = 0; ((x >> i) << i) == x; ++i);
	return i - 1;
}

// Funktio, jolla luomme pintoja, kun emme halua kirjoittaa samoja uudestaan
SDL_Surface *pinnat::tee_pinta(Uint32 w, Uint32 h, Uint32 c) {
	SDL_Surface * res;
	res = SDL_CreateRGBSurface(SDL_SRCALPHA, w, h, 32, rmask, gmask, bmask, amask);

	if (!res) {
		fprintf(stderr, "Virhe: %s\n", SDL_GetError());
		exit(1);
	}

	if (((c & 0xff) != (c >> 24)) || ((c & 0xffff) != (c >> 16))) {
		Uint32 i, j;
		for (i = 0; i < w; ++i) for (j = 0; j < h; ++j) {
			piste(res, i, j) = c;
		}
	} else {
		memset(res->pixels, (c & 0xff), res->pitch * (h - 1) + w * bpp(res));
	}

	return res;
}

// blittaa kuvan niin, että sen keskusta on annetuissa koordinaateissa
void pinnat::blittaa(SDL_Surface *src, float x, float y, SDL_Surface * dest) {
	if (!dest) dest = ruutu;
	SDL_Rect alue = {
		(int)(x - (src->w >> 1) + 0.5),
		(int)(y - (src->h >> 1) + 0.5),
		src->w, src->h};
	SDL_BlitSurface(src, 0, dest, &alue);
}

// Pyörittää ja kutsuu ylempää
void pinnat::rotozoom_blittaa(SDL_Surface *src, int x, int y, SDL_Surface * dest, double kulma, double skaalaus, int smooth) {
	SDL_Surface * temp = rotozoomSurface(src, kulma, skaalaus, smooth);
	blittaa(temp, x, y, dest);
	SDL_FreeSurface(temp);
}

void piirra_omena(const omena_t &omppu) {
	//SDL_BlitSurface(omena, 0, alfaomena, 0);
	Uint32 alpha, color, ake = omppu_alpha(omppu);
	int i, j;
	for (i = 0; i < 32; ++i) {
		for (j = 0; j < 32; ++j) {
			//color = piste(alfaomena, j, i);
			color = piste(pinnat::omena, j, i);
			alpha = (color & amask) >> ashift;
			//color &= (rmask | gmask | bmask);
			color ^= ((((ake * alpha) >> 8) ^ alpha) << ashift) & amask;
			piste(pinnat::alfaomena, j, i) = color;
		}
	}
	pinnat::blittaa(pinnat::alfaomena, (0.5+(omppu.sij.x*32)), (0.5+(omppu.sij.y*32)), 0);
}

void piirra_omenat() {
	for (omena_t *ptr = uusi_omena.aiempi; ptr != 0; ptr = ptr->aiempi) {
		piirra_omena(*ptr);
	}
}

void piirra_hanta() {
	if (mato::hanta == mato::nena) {
		return;
	}

	float x, y;

	x = (mato::hanta->sij.x * (32 - mato::hantavaihe)) + (mato::hantavaihe * mato::hanta->nenampi->sij.x);
	y = (mato::hanta->sij.y * (32 - mato::hantavaihe)) + (mato::hantavaihe * mato::hanta->nenampi->sij.y);
	pinnat::blittaa(pinnat::matopallo, x, y, 0);
}

void piirra_nena() {
	if (!mato::nena) return;
	float x, y;

	// Nenän koordinaatit
	x = (mato::nena->sij.x * 32);
	y = (mato::nena->sij.y * 32);
	pinnat::blittaa(pinnat::matopallo, x, y, 0);

	x = (32 * mato::tuleva_sij.x);
	y = (32 * mato::tuleva_sij.y);
	pinnat::blittaa(pinnat::matopallo, x, y, 0);
}

void piirra_mato() {
	if (!mato::nena || !mato::hanta) return;
	float x, y;

	piirra_hanta();
	matopala_t *mato_ptr = mato::hanta;
	while (mato_ptr->nenampi != mato::nena) {
		mato_ptr = mato_ptr->nenampi;
		x = (mato_ptr->sij.x * 32);
		y = (mato_ptr->sij.y * 32);
		pinnat::blittaa(pinnat::matopallo, x, y, 0);
	}
	piirra_nena();
}

void piirra_tausta() {
	SDL_BlitSurface(pinnat::tausta, 0, pinnat::ruutu, 0);
}

int pinnat::alusta() {
	int i, j;
	Sint32 Sp;
	Uint32 Up;
	SDL_Surface *temp[3];
	ruutu = SDL_SetVideoMode(640, 480, 32, SDL_DOUBLEBUF | SDL_SRCALPHA | hwsurf);
	if (!ruutu) {
		fprintf(stderr, "Virhe: %s\n", SDL_GetError());
		exit(1);
	}
	SDL_WM_SetCaption("Matopeli", "Matopeli");

	// Taustakuva
	tausta = tee_pinta(640, 480, amask);
	temp[0] = tee_pinta(640, 480);
	temp[1] = tee_pinta(640, 480);
	temp[2] = tee_pinta(640, 480);

	for (i = 192; i < 256+192; ++i) for (j = 112; j < 256+112; ++j) {
		Up = (i - 192) ^ (j - 112);
		// (amask & (((arpa() & 0x3f) + 0xc0) << ashift))
		piste(temp[0], i, j) = (Up << rshift) | amask;
		piste(temp[1], i, j) = (Up << gshift) | ((Up << (rshift-2)) & rmask) | amask;
		piste(temp[2], i, j) = (Up << bshift) | amask;
	}
	int rotot[] = {
		526121365, 220430271, 1073938780, 491330759, 321531514, 918478021,
		371236158, 880707739, 1103731301, 1940070973, 1649048506, 1851354483,
		1407652329, 2138084399, 1697992471, 1743989528, 834647656, 1166862771,
		1360455451, 1492489322, 1519601816, 2112003681, 234859702, 951868152,
		85366167, 1375457466, 1301940351, 1601211896, 1016807447, 1102611807,
		418923025, 1542928813, 1323042078, 1492861805, 2034259572, 1644573593,
		263856179, 258012082, 377797684, 1367587480, 50599407, 2026846190,
		1071458316, 1458251736, 2017446941, 621967139, 1054757617, 704610950,
		1788829911, 267729420, 49616624, 1160948079, 232249453, 284476326,
		2112816231, 317615621, 1659933792
	};
	double scale = 0.40;
	for (i = j = 0; j < 19; ++j, i += 3) {
		if (scale < 1.0) scale += 0.04;
		rotozoom_blittaa(temp[j % 3], rotot[i+2] % 640, rotot[i+1] % 480, tausta, rotot[i+0] % 180, scale, 1);
	}
	SDL_FreeSurface(temp[0]);
	SDL_FreeSurface(temp[1]);
	SDL_FreeSurface(temp[2]);
	
	// omena
	omena = tee_pinta(32, 32);
	alfaomena = tee_pinta(32, 32);
	matopallo = tee_pinta(32, 32);
	const double k = (32.0 - 1) / 2;
	for (i = 0; i < 32; ++i) for (j = 0; j < 32; ++j) {
		// x² + y² + z² = r => z = sqrt(r - x² - y²)
		// dot(normaali, -valonsuunta) = cos
		// valonsuunta = (0.2, 0.2, 0.7); Ei ole normalisoitu, mutta ihan sama.
		Sp = (int)(225 - (pow2(k - i) + pow2(k - j)));
		if (Sp < 0) {
			piste(omena, i, j) = 0;
		} else {
			Sp = 32 + 20*(int)((sqrt(Sp) * 0.7) + ((k - i) * 0.2) + ((k - j) * 0.2));
			Up = Sp < 0 ? 0 : Sp;
			if (Sp < 16) {
				piste(omena, i, j) = ((((Up << 2) + 64) << ashift) & amask) | (Up << gshift);
			} else {
				piste(omena, i, j) = amask | (Up << gshift);
			}
		}
	}

	// Ja väritetään uudestaan
	Uint32 a, c;
	for (i = 0; i < 32; ++i) for (j = 0; j < 32; ++j) {
		a = (piste(omena, i, j) & amask);
		c = (piste(omena, i, j) & gmask) >> gshift;
		c = ((c << rshift) & rmask) | ((c << gshift >> 2) & gmask) | ((c << bshift >> 1) & bmask);
		piste(matopallo, i, j) = c | a;
	}

	rot_tausta = tausta;
	tausta = SDL_DisplayFormat(tausta);
	return 0;
}

void pinnat::tuhoa() {
	if (rot_tausta) SDL_FreeSurface(rot_tausta);
	if (tausta) SDL_FreeSurface(tausta);
	if (omena) SDL_FreeSurface(omena);
	if (alfaomena) SDL_FreeSurface(alfaomena);
	if (matopallo) SDL_FreeSurface(matopallo);
	rot_tausta = tausta = omena = alfaomena = matopallo = 0;
}

void alkualustus() {
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
		fprintf(stderr, "Virhe: %s\n", SDL_GetError());
		exit(1);
	}
	pinnat::alusta();
}

void lopetusfunktio() {
	lopeta_peli();
	pinnat::tuhoa();
	SDL_Quit();
}

void tee_omena() {
	uusi_omena.sij.x = 0.6 + (arpa() / (float)max_arpa) * (koko_x - 1.2);
	uusi_omena.sij.y = 0.6 + (arpa() / (float)max_arpa) * (koko_y - 1.2);
	uusi_omena.sij.r = 0.5;
	if (mato::nena) {
		uusi_omena.aika =
		uusi_omena.alkuaika = (int)(mato::nopeus * 5.0 * manhattan(uusi_omena.sij, mato::nena->sij));
	} else {
		uusi_omena.aika = uusi_omena.alkuaika = 25 * mato::nopeus;
	}
	omena_t *ptr = new omena_t;
	*ptr = uusi_omena;
	uusi_omena.aiempi = ptr;
}

void syo_omena() {
	if (syoty_jo || !mato::nena) {
		return;
	}
	syoty_jo = 1;

	for (omena_t *omena_ptr = uusi_omena.aiempi; omena_ptr != 0; omena_ptr = omena_ptr->aiempi) {
		if (!osuu(mato::tuleva_sij, omena_ptr->sij)) {
			continue;
		}
		omena_ptr->aika = 0;
		
		matopala_t *mato_ptr = new matopala_t;
		mato_ptr->sij = mato::nena->sij;
		mato_ptr->hannampi = mato::nena->hannampi;
		mato_ptr->nenampi = mato::nena;
		if (mato::nena->hannampi) {
			mato::nena->hannampi->nenampi = mato_ptr;
		}
		mato::nena->hannampi = mato_ptr;
	}
}

void kuolema() {
	state |= S_DEAD;
}

void tormasi_seinaan() {
	kuolema();
}

void tormasi_itseensa() {
	kuolema();
}

void tormaa() {
	int i;

	matopala_t *mato_ptr = mato::nena;
	// Pari ensimmäistä voi ja pitää jättää tarkistamatta.
	for (i = 0; i < 4 && mato_ptr; ++i) {
		mato_ptr = mato_ptr->hannampi;
	}

	for (; mato_ptr; mato_ptr = mato_ptr->hannampi) {
		if (osuu(mato::tuleva_sij, mato_ptr->sij)) {
			return tormasi_itseensa();
		}
	}

	for (mato_ptr = mato::nena; mato_ptr; mato_ptr = mato_ptr->hannampi) {
		if ((mato_ptr->sij.x - mato_ptr->sij.r < 0)
		|| (mato_ptr->sij.x + mato_ptr->sij.r > koko_x)
		|| (mato_ptr->sij.y - mato_ptr->sij.r < 0)
		|| (mato_ptr->sij.y + mato_ptr->sij.r > koko_y)) {
			return tormasi_seinaan();
		}
	}
}

void vaihtokohta() {
	if (!mato::nena) return;

	syoty_jo = 0;

	// Uusi nenä paikalleen
	matopala_t *mato_ptr = mato::nena;
	mato::nena = new matopala_t;
	mato_ptr->nenampi = mato::nena;
	mato::nena->hannampi = mato_ptr;
	mato::nena->sij = mato::tuleva_sij;

	mato::hanta = mato::hanta->nenampi;
	delete mato::hanta->hannampi;
	mato::hanta->hannampi = 0;
}

// Pelin aloitus
int aloita_peli() {
	mato::nopeus_f = 500.0f;
	mato::nopeus = (int) mato::nopeus_f;

	// Olkoon mato keskellä ja kahden mittainen
	mato::nena = new matopala_t;
	mato::hanta = new matopala_t;
	mato::nena->hannampi = mato::hanta;
	mato::nena->nenampi = 0;
	mato::hanta->nenampi = mato::nena;
	mato::hanta->hannampi = 0;
	mato::hanta->sij.x = mato::nena->sij.x = (koko_x - 1) / 2;
	mato::hanta->sij.y = mato::nena->sij.y = (koko_y - 1) / 2;
	mato::nena->sij.r = mato::hanta->sij.r = mato::tuleva_sij.r = 0.5;
	mato::suunta = 0;
	for (int i = 0; i < alkuomenia; ++i) {
		tee_omena();
	}
	++alkuomenia;
	state = S_PELI;

	return 0;
}

void lopeta_peli() {
	while (mato::nena) {
		mato::hanta = mato::nena->hannampi;
		delete mato::nena;
		mato::nena = mato::hanta;
	}
	while (omena_t *omena_ptr = uusi_omena.aiempi) {
		uusi_omena.aiempi = omena_ptr->aiempi;
		delete omena_ptr;
	}
	state &= ~S_PELI;
}

int hoida_viestit() {
	SDL_Event Event;

	while (SDL_PollEvent(&Event)) {
		if (Event.type == SDL_QUIT) {
			return -1;
		}
	}
	return 0;
}

Uint32 hae_aika(void) {
	return SDL_GetTicks();
}

int main(int argc, char ** argv) {
	int game_time;
	Uint32 frame_time, old_time;

	std::atexit(lopetusfunktio);

	alkualustus();
	old_time = hae_aika();
	game_time = 0;

	while (hoida_viestit() == 0) {
		prev_state = state;
		do {
			frame_time = hae_aika();
		} while (frame_time == old_time);
		frame_time -= old_time;
		old_time += frame_time;
		
		const Uint8 * const keys = SDL_GetKeyState(0);
		
		piirra_tausta();
		if (keys[SDLK_LEFT]) mato::suunta += 0.002 * frame_time;
		if (keys[SDLK_RIGHT]) mato::suunta -= 0.002 * frame_time;
		if (state & S_PELI) {
			game_time += frame_time;
			if (!(state & S_DEAD)) {
				mato::nenavaihe = mato::hantavaihe = ((48 * game_time) / mato::nopeus);
				mato::nenavaihe = mato::nenavaihe >= 32 ? 32 : (mato::nenavaihe & 31);
				mato::hantavaihe = mato::hantavaihe > 16 ? ((mato::hantavaihe - 16) & 31) : 0;

				mato::tuleva_sij.x = mato::nena->sij.x + (mato::nenavaihe * std::sin(mato::suunta) / 64);
				mato::tuleva_sij.y = mato::nena->sij.y + (mato::nenavaihe * std::cos(mato::suunta) / 64);

				syo_omena();
				if (game_time >= mato::nopeus) {
					game_time -= mato::nopeus;
					vaihtokohta();
					mato::nopeus_f *= 0.995f;
					mato::nopeus = (int) mato::nopeus_f;
				}
				tormaa();
			}

			for (omena_t *ptr = &uusi_omena; ptr->aiempi != 0;) {
				omena_t *omena_ptr = ptr->aiempi;
				omena_ptr->aika -= frame_time;
				if (omena_ptr->aika < 0) {
					ptr->aiempi = omena_ptr->aiempi;
					delete omena_ptr;
					tee_omena();
				} else {
					ptr = ptr->aiempi;
				}
			}

			piirra_omenat();
			piirra_mato();

			if (!(state & S_DEAD)) {
				// ...
			} else if (!(prev_state & S_DEAD)) {
				game_time = 0;
			} else if (game_time >= 2160 + 4320)  {
				if (game_time > 2160 + 4320 + 1296) {
					game_time = 2160 + 4320 + 1296;
				}
				pinnat::rotozoom_blittaa(
					pinnat::rot_tausta, 320, 240, 0,
					0.0,
					(2160 + 4320 + 1296) / (float)game_time, 1
				);
				if (game_time >= (2160 + 4320 + 1296)) {
					game_time = 0;
					lopeta_peli();
					state = S_MENU;
				}
			} else if (game_time > 2160) {
				pinnat::rotozoom_blittaa(
					pinnat::rot_tausta, 320, 240, 0,
					((game_time - 2160) / 4) % 360,
					(game_time - 2160) / 3600.0, 1
				);
			}
		} else if (state & S_MENU) {
			aloita_peli();
			game_time = 0;
		}
		SDL_Flip(pinnat::ruutu);
	}

	return 0;
}
