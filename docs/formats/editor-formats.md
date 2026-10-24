# Editor-side formats

- `.pbk` (GFX Editor source list): `count`, then per image a line `hotspotX hotspotY compression` and a line with the BMP path (absolute Windows paths in the shipped `smurf-sprites.pbk`).
- Level editor `bbe_*.ini`: `bbe_decors.ini` (count + screen BMP names), `bbe_ndecors.ini` (count + screen index sequence), `bbe_ennemis.ini` / `bbe_fondsanimes.ini` / `bbe_mifonds.ini` / `bbe_1plan.ini` (`count`, then `id Name*bitmap.bmp`).
