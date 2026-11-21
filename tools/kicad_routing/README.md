# Scripts de routage KiCad – Eeloo Pad

Outils Python (pcbnew de KiCad 10) utilisés pour router `Kicad/Eeloo_Pad.kicad_pcb`.
À lancer avec `python3 -I script.py ...` (KiCad fermé ; travailler sur une copie, puis DRC avant d'écraser le projet).

| Fichier | Rôle |
|---|---|
| `router.py` | Routeur sur grille 2 couches (A*, 45°, coût via/virage, préférence H/V par couche, obstacles retirables) |
| `clusters.py` | Îlots de connexion d'un réseau (pads/pistes/vias déjà reliés) |
| `trim.py` | Supprime les bouts de piste pendants et les vias utilisés sur une seule face |
| `viaspot.py` | Teste si un via / une piste tient à une position donnée (clearances) |
| `gndfix.py` | Recoud les îlots de plan GND par des vias (`gndfix.py in.kicad_pcb out.kicad_pcb [REF.pad,...]`) |
| `clean.py` | Re-routage réseau par réseau pour réduire les vias (bloc A) |
| `drc_sum.py` | Résumé d'un rapport `kicad-cli pcb drc` |
| `render.py` | Image PNG d'une zone du PCB (`render.py pcb out.png NET x0,y0,x1,y1 échelle`) |
| `nets.py`, `sexp.py` | Lecture netlist KiCad / symboles de bibliothèque |
| `addjack.py`, `placejack.py`, `editjack.py`, `routejack.py` | Ajout de l'entrée jack 12 V (schéma, placement, retouches, routage) – historique |

`backups/` : états avant les dernières modifications (PCB complet avant jack, schémas avant jack).
| `silkfix.py`, `silkfix2.py`, `silkfix3.py`, `silk_common.py` | Passe sérigraphie : bord commun des afficheurs 14 seg, contour du jack coupé au bord, références déplacées vers la place libre la plus proche (`silkfix.py in out rapport_drc.txt`) |
