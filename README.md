# Statistiques - Regression Analysis Tool

Application Qt de regression statistique portee depuis un original DOS (Borland C++, 1996).

## Fonctionnalites

- **Saisie** : points (X, Y) saisis manuellement, depuis fichier texte, ou generation depuis une fonction mathematique predefinie
- **Regressions** :
  - Mode 0 : Lineaire (`y = a + b*x`)
  - Mode 1 : Logarithmique (`y = a + b*ln(x+tx)`)
  - Mode 2 : Exponentielle (`y = a*e^(b*x) - ty`)
  - Mode 3 : Puissance (`y = a*(x+tx)^b - ty`)
   - Mode 4 : Puissance NLS (`y = a*x^b` par Gauss-Newton)
   - Mode 5 : Reciproque (`y = a + b/x`)
   - Mode 6 : Polynomial deg 2 (`y = a + bx + cx²`)
   - Mode 7 : Sinusoidal (`y = a*sin(bx+c) + d` par Gauss-Newton)
   - Mode 8 : Logistique (`y = c/(1+ae^(-bx))` par Gauss-Newton)
   - Auto : selection automatique du meilleur modele (r^2 maximum, modes 0-8)
- **Graphique** : courbe + points avec zoom (molette), drag (souris), auto-ranging, grille, info de regression en superposition
- **Export** : resultats sauvegardes dans un fichier texte

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

Lancez l'executable. Utilisez les menus de la barre en haut :
- **Fichier** : Ouvrir / Sauvegarder / Quitter
- **Donnees** : Saisie manuelle / Fonction mathematique / Effacer
- **Regression** : Choix du mode ou Auto
- **Reinitialiser le zoom** : remet le graphique a l'echelle

## Format du fichier de donnees

```
5                           <- nombre de points
1.0     2.3                 <- X Y (un point par ligne)
2.0     4.1
3.0     5.9
4.0     8.2
5.0     9.8
```

## Origine

Portage d'un programme original ecrit en Borland C++ pour DOS (1996).
