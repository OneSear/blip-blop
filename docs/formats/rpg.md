# `.rpg` — dialogue script (text, `rpg_player.cpp`)

Sections start at `rpg=N` and end at `stop`. Commands: `focus=haut|bas` (top/bottom box), `id=joueur|ennemi` (portrait bank: players or level), `img=N` (player portraits are offset +6 for Blop), `txt=N`, `affiche=ms` (show for N ms, skippable after 750 ms), `flag=F.V` (set flag F to V), `ifnbj=N … endif` (only if N players), `bbswap` (switch to the other player), `;` comments. The world is frozen while a dialogue plays.
