# Statistiques - Regression Analysis Tool

Application Qt de regression statistique portee depuis un original DOS (Borland C++, 1996).

Offre **9 modeles de regression** (+ selection automatique) avec affichage graphique interactif (zoom, drag), grille, et export des resultats.

## Fonctionnalites

- **Saisie flexible** : points saisis manuellement, depuis fichier texte, ou generation depuis une fonction mathematique predefinie (sin, cos, exp, log, sqrt, x², x³...)
- **9 regressions + Auto** : de la lineaire a la logistique, avec affichage de r, r², covariance, et tous les parametres
- **Graphique interactif** : courbe + points, zoom molette, drag souris, auto-ranging, grille, formule + stats en superposition
- **Export** : resultats sauvegardes dans un fichier texte
- **Exemples inclus** : 6 jeux de donnees prets a l'emploi dans le dossier `exemples/`

## Les regressions en details

| Mode | Nom | Formule | Cas d'usage concret |
|------|-----|---------|-------------------|
| 0 | **Lineaire** | y = a + b*x | Relation proportionnelle : distance/temps, cout/unite, loi d'Ohm U=RI |
| 1 | **Logarithmique** | y = a + b*ln(x+tx) | Loi de Weber-Fechner (perception), decroissance radioactive (log du signal), rendements decroissants |
| 2 | **Exponentielle** | y = a*e^(b*x) - ty | Croissance bacterienne, interets composes, desintegration radioactive, charge/decharge de condensateur |
| 3 | **Puissance** | y = a*(x+tx)^b - ty | Loi de Kleiber (metabolisme/masse), loi de Moore, relation allometrique en biologie |
| 4 | **Puissance NLS** | y = a*x^b | Meme que 3 mais sans decalage, gere x=0 correctement (ex: loi de Stefan-Boltzmann, debit/ pression) |
| 5 | **Reciproque** | y = a + b/x | Loi de Michaelis-Menten (cinetique enzymatique), relation pression/volume, courbe de saturation |
| 6 | **Polynomial deg 2** | y = a + b*x + c*x^2 | Trajectoire parabolique (projectile), cout marginal en economie, acceleration constante |
| 7 | **Sinusoidal** | y = a*sin(bx+c) + d | Oscillations (pendule, ressort), courant alternatif, cycles saisonniers, marees, rythmes circadiens |
| 8 | **Logistique** | y = c/(1+ae^(-bx)) | Croissance de population avec limite, penetration de marche, courbe d'apprentissage, epidemies (modele SIR) |
| Auto | **Meilleur fit** | - | Teste les 9 modeles et selectionne celui avec le meilleur r² |

## Exemples concrets

### Exemple 1 : Decharge de condensateur (exponentielle)
```
# donnees.txt : tension (V) aux bornes d'un condo qui se decharge
# t(s)    V(V)
0        5.00
1        3.32
2        2.21
3        1.47
4        0.98
5        0.65
6        0.43
7        0.29
8        0.19
9        0.13
10       0.08
```
- **Mode 2 (Exponentielle)** trouve V = 5.00*e^(-0.405*t) → constante de temps τ = 1/0.405 ≈ 2.47s
- **Auto** selectionne automatiquement l'exponentielle

### Exemple 2 : Pendule simple (sinusoidal)
```
# pendule.txt : angle en fonction du temps pour un pendule
# t(s)    angle(rad)
0.0      0.000
0.1      0.309
0.2      0.588
0.3      0.809
0.4      0.951
...
```
- **Mode 7 (Sinusoidal)** trouve θ(t) = a*sin(ω*t + φ) + d
- Determine la pulsation ω, l'amplitude a, et le dephasage φ
- **Auto** detecte le mode sinusoidal automatiquement

### Exemple 3 : Croissance bacterienne (logistique)
```
# levure.txt : population de levure dans un milieu limite
# t(h)   population
0       10
2       25
4       65
6       160
8       350
10      600
12      800
14      920
16      970
18      990
20      995
```
- **Mode 8 (Logistique)** : P(t) = c/(1+ae^(-bt))
- c = capacite limite du milieu (~1000), a et b parametres de croissance
- Modele classique en biologie des populations

### Exemple 4 : Cinematique (parabole)
```
# trajectoire.txt : hauteur d'un projectile
# t(s)   h(m)
0.0     0.0
0.2     3.8
0.4     7.2
0.6     10.1
0.8     12.5
1.0     12.8
1.2     12.0
1.4     10.1
1.6     7.5
1.8     4.2
2.0     0.0
```
- **Mode 6 (Polynomial deg 2)** trouve h = a + b*t + c*t²
- c = -g/2 (demi-acceleration gravitationnelle)
- Altitude max au sommet de la parabole

### Exemple 5 : Michaelis-Menten (reciproque)
```
# enzyme.txt : vitesse de reaction en fonction de la concentration
# [S](mM)   V(umol/min)
0.5        0.33
1.0        0.50
2.0        0.67
4.0        0.80
8.0        0.89
16.0       0.94
```
- **Mode 5 (Reciproque)** : V = a + b/[S] → parametres de cinetique enzymatique
- Equivalent a Lineweaver-Burk sans linearisation

### Exemple 6 : Loi de puissance (allometrie)
```
# metabolism.txt : taux metabolique en fonction de la masse
# masse(kg)   met(W)
0.01         0.02
0.10         0.15
1.00         1.20
10.0         9.50
100          75.0
1000         500
```
- **Mode 3 ou 4 (Puissance)** : M = a * masse^b
- b ≈ 0.75 (loi de Kleiber) : le metabolisme croit moins vite que la masse
- **Mode 4** gere mieux les donnees pres de zero

## Compilation

### Windows (MinGW)
```batch
set QTDIR=C:\Qt\6.11.0\mingw_64
.\Windows\build.bat
```

### Linux (GCC)
```bash
./linux/build.sh
```

### Manuel
```bash
mkdir build && cd build
qmake ../Statistiques.pro
make -j$(nproc)
```

## Utilisation

Lancez l'executable :

1. **Saisir des donnees** :
   - Menu `Donnees > Saisie manuelle` : entrez les paires X Y
   - Menu `Donnees > Fonction mathematique` : genere des points depuis une fonction (sin, cos, exp...)
   - Menu `Fichier > Ouvrir` : charge un fichier texte (des exemples sont dans le dossier `exemples/`)

2. **Choisir une regression** :
   - Menu `Regression > Mode X` : selectionne un mode specifique
   - Menu `Regression > Auto (best fit)` : teste les 9 modeles et choisit le meilleur (r² max)

3. **Explorer le graphique** :
   - Molette : zoom avant/arriere
   - Clic + drag : deplacer le graphique
   - Bouton `Reinitialiser le zoom` : revient a l'echelle automatique

4. **Sauvegarder** :
   - Menu `Fichier > Sauvegarder` : exporte les resultats (parametres + stats) dans un fichier texte

La regression se lance automatiquement apres chaque ajout ou modification des donnees.

## Format du fichier de donnees

```
5                           <- nombre de points (1ere ligne)
1.0     2.3                 <- X Y (un point par ligne, separes par espace ou tabulation)
2.0     4.1
3.0     5.9
4.0     8.2
5.0     9.8
```

## Notes techniques

- **Puissance (mode 3)** : points x<=0 ou y<=0 exclus du calcul des parametres a,b (ils n'apportent aucune information sur la loi de puissance). r, r², cov calcules sur TOUS les points.
- **Puissance NLS (mode 4)** : utilise Gauss-Newton pour resoudre y = a*x^b sans decalage, gere naturellement x=0.
- **Sinusoidal (mode 7)** : initialisation par denombrement des passages par zero (frequence) + correlation lineaire (phase). NLS Gauss-Newton 4 parametres.
- **Logistique (mode 8)** : NLS Gauss-Newton 3 parametres. Si les donnees ne sont pas sigmoidales, peut diverger → utiliser manuellement.
- **Auto** : teste les 9 modeles sequentiellement. Les modes NLS (4, 7, 8) peuvent ralentir l'auto pour de grands jeux de donnees.

## Corrections Mathématiques (v1.0)

Une audit mathématique complet a identifié et corrigé **20+ bugs** de cohérence :

### Bugs Critiques Éliminés
- **Mode 3 (Puissance)**: Incohérence majeure - statistiques calculées sur points exclus du MCO ✓
- **Mode 7 (Sinusoidale)**: NaN possible dans asin si y hors domaine [-d-a, d+a] ✓
- **Tous modes**: Coefficient r invalide pour régressions non-linéaires (utilise R² maintenant) ✓

### Améliorations par Mode
- **Mode 1,2**: Statistiques cohérentes (exclude points exclus du MCO) ✓
- **Mode 4**: Validation paramètres a>0, b≠0 ✓
- **Mode 5**: Gestion robuste du cas y≈a ✓
- **Mode 6**: Support DEUX racines du polynôme (avant: seulement +) ✓
- **Mode 6**: Gestion c≈0 (devient linéaire) ✓
- **Mode 8**: Validation sigmoid post-NLS ✓

**Voir:** [CORRECTIONS_APPLIQUEES.md](CORRECTIONS_APPLIQUEES.md) pour la liste complète

---

## Origine

Portage d'un programme original ecrit en Borland C++ pour DOS (1996).
