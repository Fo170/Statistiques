# AGENTS.md - Projet Statistiques

## Structure du projet

```
Statistiques/
├── Statistiques.cpp        # Programme principal (GUI Qt)
├── statsengine.h           # Moteur de regression (ported from STATS.HL)
├── statsengine.cpp         # Implementation du moteur de regression
├── chartwidget.h           # Widget graphique Qt (courbes, zoom, drag)
├── chartwidget.cpp         # Implementation du widget graphique
├── Statistiques.pro        # Fichier projet qmake (multiplateforme)
├── includes/               # Headers heritage
│   ├── def.hcl             # Types de base et macros (DEF_HCL.h)
│   ├── fct.hl              # Fonctions utilitaires (FCT.HL)
│   ├── rgm.hl              # Tris et ranges (RGM.HL)
│   └── stats.hl            # Regression originale (STATS.HL)
├── Windows/
│   ├── Statistiques.pro    # .pro Windows (chemins relatifs ../)
│   └── build.bat           # Script de compilation Windows
├── linux/
│   └── build.sh            # Script de compilation Linux
├── AGENTS.md
├── README.md
└── plus_necessaire/        # (supprime) Anciens fichiers DOS/BC++
```

## Commandes de build

### Windows (Qt 6 MinGW)
```batch
set QTDIR=C:\Qt\6.11.0\mingw_64
.\Windows\build.bat
```
L'executable est dans `Windows/build/release/Statistiques.exe`.
Deployer avec : `windeployqt Windows/build/release/Statistiques.exe`

### Linux
```bash
chmod +x linux/build.sh
./linux/build.sh
```

### Build manuel (n'importe quelle plateforme)
```bash
mkdir build && cd build
qmake ../Statistiques.pro
make -j$(nproc)
```

## Architecture du code

### StatsEngine (`statsengine.h/.cpp`)
- Classe encapsulant le moteur de regression
- `regTpl()` : regression par log-linearisation (modes 0-3)
  - Decalage tx/ty pour eviter ln(0) (modes 1-3)
  - Mode 3 (puissance) : tx=0, ty=0, points x<=0/y<=0 exclus du calcul de a,b
  - r, r², cov calcules dans l'espace original sur TOUS les points
- `regNls()` : regression puissance par Gauss-Newton (mode 4)
  - Initialisation par log-linearisation, puis iterations NLS
  - Points x=0 geres naturellement (Jacobienne nulle)
- `autoMode()` : teste les 5 modes, retourne le meilleur (r² max)
- `regFY()` / `regFX()` : evaluation de la courbe de regression

### ChartWidget (`chartwidget.h/.cpp`)
- Affichage courbe (tiree de regFY) + points de donnees
- Auto-ranging, zoom molette, drag souris
- Grille, axes, info de regression (2 lignes)

### Statistiques.cpp (GUI)
- QMainWindow avec menus : Fichier, Donnees, Regression
- QSplitter vertical : graphique + zone de resultats (permanente)
- Auto-regression lancee automatiquement apres chaque saisie
- Fonctions libres : inputManual, inputFile, inputFunction, runRegression, runAutoRegression, saveResults

## Regressions disponibles

| Mode | Nom | Formule | Algorithme |
|------|-----|---------|------------|
| 0 | Lineaire | y = a + b*x | MCO direct |
| 1 | Logarithmique | y = a + b*ln(x+tx) | Log-linearise (tx si xmin<=0) |
| 2 | Exponentielle | y = a*e^(b*x) - ty | Log-linearise (ty si ymin<=0) |
| 3 | Puissance | y = a*(x+tx)^b - ty | Log-linearise (tx=ty=0, points non-positifs exclus) |
| 4 | Puissance NLS | y = a*x^b | Gauss-Newton (initialise par log-lin, gere x=0) |
| Auto | Meilleur fit | - | Teste modes 0-4, garde r² max |

## Portage depuis l'original (BC++ / DOS)

Le code original utilisait un systeme d'inclusion par `#define`/`#undef`:
- `#define stats` puis `#include<stats.hl>` pour activer les fonctions
- `def.hcl` definissait les types (`Ldbl`, `byte`, etc.) et macros (`_()`, `_L()`, `pw2()`)

Le nouveau code Qt modernise l'approche:
- Classes C++ avec encapsulation
- `StatsEngine` remplace la structure globale `RG` et les templates
- `std::vector<double>` remplace les tableaux C
- QPainter pour le graphique (vs BGI/DOS)
- `includes/stats.hl` conserve a titre d'archive

## Notes sur la regression puissance

Le mode 3 original (STATS.HL) decale les donnees avec tx=1-xmin, ty=1-ymin pour
eviter ln(0). Ce decalage deforme la relation de puissance quand xmin=0 ou ymin=0.
La correction appliquee : tx=0, ty=0, points avec x<=0/y<=0 exclus du calcul de a,b
(ils n'apportent aucune information sur les parametres de la loi de puissance).
Les statistiques r, r², cov sont calculees dans l'espace original sur tous les points.

Le mode 4 (NLS) resout le probleme sans compromis en utilisant Gauss-Newton,
ce qui gere naturellement les points x=0.
