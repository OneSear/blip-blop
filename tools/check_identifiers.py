#!/usr/bin/env python3
"""Checks that no French identifier remains in the code (migration.md 4.5).

The French words come from the glossary (migration.md Appendix B, see
FRENCH below; the script checks they are still in it). Every identifier of
src/, tests/ and tools/ (comments and string literals left out: comments
may name the 2002 files a port comes from) is split into its words
(camelCase, snake_case); a word of the French list is reported.

Usage: check_identifiers.py <repository root>
Exit status 0 when nothing is found, 1 otherwise.
"""

import pathlib
import re
import sys

# The French words of the glossary (migration.md Appendix B, "Current"
# column), without the ones that are also English words, abbreviations or
# names the glossary keeps (base, charge, plan, dir, rpg, sbk, Blip...).
FRENCH = set("""
affiche alarme arme ascenceur avance bombe bouge boule bulbizarre bulle cadeau
caisse chaman ciel cigogne cloche coeur colonne controlor costaud couille cree
debouchechiote degats detruire drapeau droite eau ecran enflame ennemi ennemis
epee epine etape etat etoile farceur flameche fleche fond fonte frileux gateau
gauche gicle giclure glisse gouverneur gravite haut heros intensite interupteur
joue joueur liste lumiere lunettes marche mariotapette meteo meure monolite
morceau mortel mur murs neige niveau nuage opaques paresseux partie paysan
personnage plat pluie poid poisson princesse princesseguard raymanpoing rearme
recul rocher saleto sanglant sanglants sauti saut saute sautille sauvage sens
soniceclair statique tailsvolant tete texte tir tombe tornade touche tournesol
tremblement tresor turc vehicule ventilo voiture xpacker xpaker
""".split())

# Identifiers allowed although they contain one: the asset banks keep the
# names of their data files (the format documents name them).
ALLOWED = {"bisouenn", "bisoufond", "snuffond", "snorkfond", "lemfond", "dorkfonda", "videofonda"}


def glossary_words(plan: str) -> set:
    start = plan.index("## Appendix B")
    end = plan.index("## Appendix C")
    words = set()
    for line in plan[start:end].splitlines():
        cells = [c.strip() for c in line.split("|")]
        if len(cells) < 4 or cells[1] in ("Current", "") or set(cells[1]) <= {"-"}:
            continue
        words.update(split_words(" ".join(re.findall(r"`([^`]+)`", cells[1]))))
    return words


def split_words(text: str) -> list:
    words = []
    for token in re.findall(r"[A-Za-z][A-Za-z0-9]*", text):
        for part in re.findall(r"[A-Z]+(?![a-z])|[A-Z]?[a-z]+|[0-9]+", token):
            words.append(part.lower())
    return words


def code_without_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.S)
    text = re.sub(r"//[^\n]*", " ", text)
    text = re.sub(r'R"\((.*?)\)"', '""', text, flags=re.S)
    text = re.sub(r'"(\\.|[^"\\\n])*"', '""', text)
    text = re.sub(r"'(\\.|[^'\\\n])'", "''", text)
    return text


def main() -> int:
    root = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    glossary = glossary_words((root / "migration.md").read_text(encoding="utf-8"))
    missing = FRENCH - glossary
    if missing:
        print("words not in the glossary any more:", " ".join(sorted(missing)))
        return 1
    french = FRENCH
    found = []
    for directory in ("src", "tests", "tools"):
        for path in sorted((root / directory).rglob("*")):
            if path.suffix not in (".cpp", ".hpp", ".h", ".inc") or "bb/assets/" in path.as_posix():
                continue
            code = code_without_comments(path.read_text(encoding="utf-8", errors="replace"))
            for number, line in enumerate(code.splitlines(), 1):
                for identifier in set(re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", line)):
                    hits = [w for w in split_words(identifier) if w in french]
                    if hits and identifier.lower() not in ALLOWED:
                        found.append(f"{path.relative_to(root)}:{number}: {identifier} ({', '.join(hits)})")
    for line in found:
        print(line)
    print(f"{len(french)} French words checked; {len(found)} identifiers found")
    return 1 if found else 0


if __name__ == "__main__":
    sys.exit(main())
