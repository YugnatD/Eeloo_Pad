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
| `smooth.py` | Lissage des routes du routeur (supprime les escaliers A*, segments 0/45/90° vérifiés contre les obstacles) |
| `keepouts.py` | Ajoute les zones interdites (rule areas du PCB et des empreintes) comme obstacles du routeur |
| `ledswap.py` | Passage des SK6812MINI-E en montage inversé (B.Cu, découpe, centrées à +5,08 mm sous le switch MX) |
| `ledroute.py` | Dégagement des découpes LED et reroutage réseau par réseau (+ lissage) |
| `safetrim.py` | Nettoyage des bouts pendants sans casser de jonction (vérifie le nombre d'îlots après chaque suppression ; jamais sur GND) |
| `finish.py` | Finition après reroutage : micro-trous signalés par le DRC (connection_width), pendants DRC, clearance pads LED, réseaux encore coupés |
| `shorten.py` | Raccourcit les pistes pendantes (DRC) jusqu'au dernier contact cuivre du même réseau |
| `ripup.py` | Arracher-et-refaire : `RIP=net1,net2 ORDER=first ripup.py in out NET REF.pad REF.pad [marge]` |

Notes : le routeur doit utiliser `clr=0.2` (classe Default = 0,2 mm) ; règles perso dans `Kicad/Eeloo_Pad.kicad_dru` (valeurs AVEC unité, ex. `0.25mm`).
| `busscan.py` | Cherche une hauteur y où un bus droit passe (trous sur B, sauts possibles par F) |
| `vbusspine.py` | Bus VBUS droit sur B (y=40,45) sous la rangée de LED du haut + piquages LED/condo → bus |
| `cellfix.py` | Refait le piquage VBUS d'une cellule LED (options env : FRIP, KEEP, NORIP, ANYV, SR, RYMIN) |
| `endtrim.py` | Raccourcit les extrémités libres jusqu'au dernier contact (sans rapport DRC) |
| `vipfix.py` | Sort les vias des pads CMS (liaison courte + piste de l'autre face raccordée ; annule si la connectivité change ; ignore les pads thermiques ≥ 1,5 mm) |
| `stitch.py` | Ajoute des vias de couture GND dans les îlots de plan reliés par 0–1 via (ou grands avec < 4) |
| `localredo.py` | Reprise locale : `localredo.py in out 'NET@x,y,r[@coût_via]' ...` (arrache le réseau dans le cercle, reconnecte, lisse, annule si échec) |
| `tidy.py` | Supprime les segments en double, coupe les segments aux jonctions en T, sérigraphie < 1 mm → 1 mm |
| `gndtie.py` | Relie un pad GND isolé au plan de l'autre face (piste courte + via, plan > 200 mm²) |
| `padfree.py` | Reroute un réseau autour d'un pad GND en lui laissant un dégagement pour le plan |

Notes : `router.py` interdit maintenant les vias dans les pads CMS (`no_via_in_pad=True`). Le remplissage des zones par Python diffère de celui de KiCad (faux îlots) → pour livrer, faire `kicad-cli pcb drc --refill-zones --save-board` avec le .kicad_pro/.kicad_dru à côté.
| `localredo.py` | Reprise locale : `localredo.py in out 'NET@x,y,r[@coût_via]' ...` (arrache le réseau dans le cercle, reconnecte, lisse, annule si échec) |
| `tidy.py` | Supprime les segments en double, coupe les segments aux jonctions en T, sérigraphie < 1 mm → 1 mm |
| `gndtie.py` | Relie un pad GND isolé au plan de l'autre face (piste courte + via, plan > 200 mm²) |
| `padfree.py` | Reroute un réseau autour d'un pad GND en lui laissant un dégagement pour le plan |

Notes : `router.py` interdit maintenant les vias dans les pads CMS (`no_via_in_pad=True`). Le remplissage des zones par Python diffère de celui de KiCad (faux îlots) → pour livrer, faire `kicad-cli pcb drc --refill-zones --save-board` avec le .kicad_pro/.kicad_dru à côté.
