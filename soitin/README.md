# Soitin / Keyboard instrument

Tällä ohjelmalla voi käyttää näppäimistöä kosketinsoittimena. Näppäimet
voi konfiguroida itse, ja asetukset voi tallentaa tiedostoon.

## Ohjelman toiminta

Ohjelmassa on soittotila ja asetustila. Kulloinenkin tila ilmoitetaan
otsikkopalkissa. Aluksi ohjelma on soittotilassa, ja tilaa vaihdetaan
enter-napista. Asetustilassa voi muokata nappeja painamalla napin pohjaan
ja valitsemalla korkeuden nuolilla. Delete nollaa valitun napin.
Soittotilassa nuolet transponoivat koko soitinta, asetus ei tallennu.
Molemmissa tiloissa escape sulkee ohjelman.

## Asetustiedosto

Ohjelma tallentaa napit tiedostoon. Asetustiedoston nimen voi antaa
komentoriviparametrina. Nyt nimi on haitari.txt. Tiedoston jokainen
rivi kertoo yhden napin koodin ja korkeuden puoliaskelina a1:sta laskien.
Tiedostoa ei kannata muokata itse! Kun sopivat napit on asetettu, tiedoston
voi varmuuden vuoksi asettaa vain luku -tilaan.

## English summary

This is an electronical instrument which keeps key bindings from a file.
'Enter' switches between configuration and playing mode.
In configuration mode, press a key to select it and use arrow keys to
change its pitch. 'Delete' resets the selected key.
In playing mode, arrow keys transpose the whole instrument.
'Escape' closes the program.

## Käännös / Compiling

```
g++ soitin.cpp $(sdl-config --cflags --libs)
```
