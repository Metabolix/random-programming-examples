# Matopeli

25.2.2006, vähän korjattu 24.9.2009

Ohjelmointifoorumilla käytiin keskustelua matopelin tekemisestä. Koska en ollut ennen tehnyt matopeliä, päätin kokeilla antamiani neuvoja käytännössä. Lopputulos on tässä. Tavoitteena oli siis tehdä jotenkuten toimiva matopeli mahdollisimman nopeasti, ja se onnistuikin: aikaa kului yhteensä vain pari tuntia. Koodi ei missään nimessä ole tasoltaan mallikelpoista, joten siitä ei kannata ottaa liikaa mallia.

Matopeli tarvitsee toimiakseen kirjastot SDL ja SDL_gfx.

Käännä itse:

```
g++ matopeli.cpp $(sdl-config --cflags --libs) -lSDL_gfx -o matopeli.bin
```
