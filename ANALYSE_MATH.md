# Analyse de Cohérence Mathématique - Statistiques

## Résumé Exécutif
**Problèmes critiques trouvés: 5**
**Problèmes graves trouvés: 7**
**Problèmes mineurs/design: 8+**

---

## Mode 0 - Linéaire: y = a + b*x
✅ **COHÉRENT**
- Formule: MCO direct
- regFY: y = a + b*x ✓
- regFX: x = (y - a) / b ✓
- Statistiques: Tous les points, espace original ✓
- Cov = sy2 / (n - 2) ✓

---

## Mode 1 - Logarithmique: y = a + b*ln(x + tx)
⚠️ **PARTIELLEMENT COHÉRENT - Problème avec l'exclusion de points**

**Processus:**
1. Calcul de tx: Si xmin ≤ 0, alors tx = 1 - xmin (décalage minimal)
2. Transformation: x' = ln(x + tx)
3. MCO sur (x', y): Mais certains points avec x ≤ 0 sont exclus si x + tx ≤ 0
4. Statistiques: Calculées sur TOUS les points (y compris exclus)

**Problème #1: Incohérence cov**
```
- cov = sy2 / (n - 2)  <- Utilise n TOTAL
- sy2 est calculé sur TOUS les points
- MAIS a, b sont estimés sur nn < n points
- Devrait être: cov = sy2 / (nn - 2)
```

Cela rend cov faussement optimiste si beaucoup de points sont exclus!

**Problème #2: Domaine de regFX**
```cpp
regFX: x = exp((y - a) / b) - tx
```
- Si b < 0, cette formule peut produire des résultats inattendus
- Pas de vérification que exp(...) / b est dans le domaine d'estimation

**Problème #3: Pas de gestion d'erreur**
- Si b = 0 exactement, division par zéro! 
- Pas de vérification après MCO que b ≠ 0

---

## Mode 2 - Exponentielle: y = a*e^(b*x) - ty
⚠️ **PARTIELLEMENT COHÉRENT - Même problème que Mode 1**

**Processus:**
1. Calcul de ty: Si ymin ≤ 0, alors ty = 1 - ymin
2. Transformation: y' = ln(y + ty)
3. MCO sur (x, y'): Certains points avec y ≤ 0 sont exclus
4. Conversion: a = e^(ln(a)) > 0 toujours
5. Statistiques: Sur TOUS les points

**Problème #1: Même incohérence de cov que Mode 1**

**Problème #2: a toujours > 0, mais peut-il être < 0 en pratique?**
Après conversion a = exp(...)  > 0 toujours.
Cela forcerait a > 0 même si les données suggèrent une décroissance exponentielle!

Attendez: Si b < 0, alors y = a*e^(b*x) = a*e^(-|b|*x) est une décroissance.
C'est correct - a > 0 est nécessaire.

**Problème #3: Formule regFX nécessite a > 0**
```cpp
if (m_rg.mode == 2) x = logl((y + m_rg.ty) / m_rg.a) / m_rg.b;
```
- Si (y + ty) / a ≤ 0, alors logl retourne NaN!
- Si a ≤ 0, alors logl((y + ty) / a) est problématique pour y + ty > 0

---

## Mode 3 - Puissance: y = a*(x + tx)^b - ty
🔴 **INCOHÉRENCE MAJEURE**

**Processus:**
1. tx = 0, ty = 0 toujours
2. Exclusion de points: x ≤ 0 OU y ≤ 0 sont EXCLUS du MCO (lignes 58, 62)
3. MCO sur nn points (nn < n généralement)
4. Conversion: a = e^(ln(a)) > 0
5. Statistiques: Calculées sur TOUS les n points

**PROBLÈME CRITIQUE #1: Incohérence majeure des statistiques**
```
- Paramètres a, b estimés sur nn points (ceux avec x > 0 ET y > 0)
- Statistiques r, r², cov calculées sur TOUS les n points
- Cela viole le principe statistique fondamental!

Exemple: Si on a 10 points, dont 3 avec x ≤ 0 ou y ≤ 0:
- MCO effectué sur 7 points
- r, r², cov calculés sur 10 points
- Résultat: La qualité de la régression inclut des points 
  qui n'ont PAS participé à son estimation!
```

**PROBLÈME CRITIQUE #2: cov fortement biaisé**
```cpp
m_rg.cov = sy2 / (m_rg.n - 2.0);  // Ligne 111
```
- sy2 inclut les résidus de TOUS les points
- Mais a, b ont été estimés sur un SOUS-ENSEMBLE
- Devrait être: cov = sy2 / (nn - 2.0)

Si 30% des points sont exclus:
- cov est calculé avec n = 100
- Mais estimation avec nn = 70
- Cov est donc sous-estimé d'un facteur ~100/70 ≈ 1.4x

**PROBLÈME #3: Points exclus contribuent "gratuitement" à r, r², cov**

Supposons:
- Données: x = [1, 2, 3, -1, -2], y = [1, 4, 9, 2, 3]
- Régression y = a*x^b estimée sur 3 premiers points
- Statistiques incluent les 2 derniers points

Les points avec x < 0 n'ont pas aidé à estimer a, b, mais utilisent le modèle pour calculer les résidus!

---

## Mode 4 - Puissance NLS: y = a*x^b
🟡 **COMPORTEMENT ARBITRAIRE**

**Processus:**
1. Initialisation par MCO sur points > 0
2. Gauss-Newton 2 paramètres
3. Line search
4. Statistiques sur TOUS les points

**PROBLÈME #1: Gestion arbitraire de x ≤ 0 dans regFY**
```cpp
if (x <= 0.0 && m_rg.b > 0.0) y = 0.0;
else y = m_rg.a * powl(x, m_rg.b);
```
- Pour x < 0 et b non-entier, x^b est mathématiquement indéfini en réels!
- Le code retourne y = 0 pour x ≤ 0, ce qui:
  - N'est pas une approximation mathématique
  - Crée une discontinuité à x = 0
  - Invalide la formule pour l'extrapolation

Exemple: Si on fit y = 2*x^0.5 sur données positives, puis on évalue à x = -1:
- Code retourne y = 0
- Mais x^0.5 n'est pas défini en réels pour x < 0!

**PROBLÈME #2: Même problème dans regFX**
```cpp
if (y <= 0.0 && m_rg.b > 0.0) x = 0.0;
```

**PROBLÈME #3: Pas de vérification que NLS a convergé**
- Le code effectue 30 itérations mais n'indique pas la convergence
- Les résultats peuvent être suboptimaux
- Pas de diagnostic d'erreur

---

## Mode 5 - Réciproque: y = a + b/x
🟡 **PROBLÈMES MINEURS**

**Processus:**
1. Transformation: X' = 1/x (x = 0 exclu)
2. MCO sur (X', y)
3. Statistiques sur TOUS les points

**PROBLÈME #1: regFX retourne 1e100 si y = a**
```cpp
if (m_rg.mode == 5) {
    Ldbl tmp = y - m_rg.a;
    x = (tmp != 0.0) ? m_rg.b / tmp : 1e100;
}
```
- Mathématiquement: Si y = a, alors a + b/x = a ⟹ b/x = 0 ⟹ x = ∞
- Retourner 1e100 au lieu de inf est un hack
- Pire: Cela introduit une fausse solution finie!

**PROBLÈME #2: Pas de gestion du domaine**
- Si y - a = 0, on retourne 1e100
- Si y - a est très proche de 0, x explose (1/ε)

---

## Mode 6 - Polynomial deg 2: y = a + b*x + c*x²
🟡 **PROBLÈMES DE RÉCIPROQUE**

**Processus:**
1. MCO 3x3 via Cramer
2. regFX résout quadratique, utilise racine +

**PROBLÈME #1: Utilise seulement une racine**
```cpp
if (disc >= 0) x = (-m_rg.b + sqrtl(disc)) / (2.0 * m_rg.c);
```
- Formule quadratique: x = (-b ± √disc) / (2c)
- Code utilise seulement le +
- Potentiellement la mauvaise racine!

Si c < 0 (parabole inversée) et on cherche y > sommet:
- Les deux racines sont complexes
- Le code retourne x = 0 (initialisation par défaut)

**PROBLÈME #2: Retour par défaut x = 0 si disc < 0**
```cpp
Ldbl x = 0;  // Initialisation ligne 662
...
if (disc >= 0) x = ...;
// Sinon x reste 0
```
Aucune indication que la solution n'existe pas!

**PROBLÈME #3: Pas de gestion si c = 0**
- Si c = 0 exactement, ce n'est plus une parabole, c'est une droite
- Mais le code calcule 2c au dénominateur
- Division par zéro implicite!

---

## Mode 7 - Sinusoidal: y = a*sin(b*x + c) + d
🔴 **PROBLÈMES CRITIQUES DANS regFX**

**Processus:**
1. Initialisation: zéro-crossing pour b, corrélation pour c/a
2. Gauss-Newton 4 paramètres
3. Statistiques sur TOUS les points

**PROBLÈME #1: Domain invalide dans regFX**
```cpp
if (m_rg.mode == 7) x = (asinl((y - m_rg.d) / m_rg.a) - m_rg.c) / m_rg.b;
```
- asin(z) est défini seulement pour z ∈ [-1, 1]
- Ici: z = (y - d) / a
- Si |y - d| > |a|, asin produit NaN!

Exemple: a = 1, d = 0, mais y = 2 (en dehors du range)
- (2 - 0) / 1 = 2
- asin(2) = NaN

**PROBLÈME #2: Division par zéro si a = 0 ou b = 0**
```cpp
x = (asinl(...) - c) / m_rg.b;
```
- Si b = 0, division par zéro!
- Si a = 0, asin(∞) = NaN

**PROBLÈME #3: Pas de validation post-NLS**
- NLS peut converger vers des paramètres dégénérés
- Pas de vérification que a ≠ 0, b ≠ 0
- Pas d'indication que la solution est invalide

---

## Mode 8 - Logistique: y = c/(1 + a*e^(-b*x))
🔴 **PROBLÈMES CRITIQUES**

**Processus:**
1. Initialisation par linéarisation (c = ymax * 1.1)
2. Gauss-Newton 3 paramètres
3. Statistiques sur TOUS les points

**PROBLÈME #1: Initialisation arbitraire de c**
```cpp
Ldbl a = 1, b = 1, c = m_rg.ymax * 1.1;
```
- c est censé être la limite asymptote
- Mais il est initialisé à ymax * 1.1
- Et ymax peut être une simple fluke dans les données!
- Si les données ne sont pas vraiment sigmoidales, c sera complètement faux

**PROBLÈME #2: Linéarisation suppose y ∈ (0, c)**
```cpp
if (y > 0 && y < c) {
    Ldbl z = logl(c / y - 1.0);
    ...
}
```
- Si aucun point satisfait cette condition, pas d'initialisation!
- Paramètres restent à valeurs par défaut (1, 1, ymax*1.1)
- NLS peut diverger ou converger vers une mauvaise solution

**PROBLÈME #3: regFX suppose a > 0 et c > 0**
```cpp
Ldbl tmp = m_rg.c / y - 1.0;
x = (tmp > 0) ? -logl(tmp / m_rg.a) / m_rg.b : 0;
```
- Si a ≤ 0 ou c ≤ 0, comportement imprévisible
- Si tmp ≤ 0 (c'est-à-dire y ≥ c ou y ≤ 0), retourne x = 0

**PROBLÈME #4: Pas de validation que la courbe est sigmoidale**
- NLS peut converger vers n'importe quel ensemble de paramètres
- Pas de vérification que c est une limite asymptote valide
- Pas de vérification que la courbe est monotone

---

## Problèmes Transversaux

### 1️⃣ **COEFFICIENT r vs rcrit**
```cpp
m_rg.rcrit = (sy_tot > 0.0) ? 1.0 - sy2 / sy_tot : 0.0;  // R² = coefficient de détermination
m_rg.r = r_num / sqrtl(r_den_x * r_den_y);                // r = coefficient de corrélation
```

**Problème:** Ce ne sont PAS la même chose!
- rcrit = 1 - SS_res / SS_tot = Coefficient de détermination (R²)
- r = Cov(X, Y) / (σx * σy) = Coefficient de corrélation

Mathématiquement:
- Pour régression linéaire: r² = rcrit ✓
- Pour régression non-linéaire: r² ≠ rcrit généralement ✗

Le code calcule r comme corrélation de Pearson entre (X, Y_pred), mais ce n'est pas valide pour les régressions non-linéaires!

Pour une régression sinusoidale par exemple, la corrélation de Pearson n'a pas de sens.

### 2️⃣ **Degrés de liberté pour cov**
- Mode 0: cov = sy2 / (n - 2) ✓ (2 paramètres)
- Mode 5: cov = sy2 / (n - 2) ✓
- Mode 6: cov = sy2 / (n - 3) ✓ (3 paramètres)
- Mode 7: cov = sy2 / (n - 4) ✓ (4 paramètres)
- Mode 8: cov = sy2 / (n - 3) ✓

**Mais modes 1, 2, 3:** Tous utilisent (n - 2) 
- Mode 1: Points exclus, nn peut << n, mais cov utilise n ✗
- Mode 2: Points exclus, nn peut << n, mais cov utilise n ✗
- Mode 3: Points exclus, nn peut << n, mais cov utilise n ✗

---

## Tableau de Synthèse

| Mode | Problème | Sévérité | Impact |
|------|----------|----------|--------|
| 1 | cov biaisé si points exclus | 🔴 Critique | Statistiques faussées |
| 2 | cov biaisé si points exclus | 🔴 Critique | Statistiques faussées |
| 3 | Paramètres sur sous-ensemble, stats sur tous | 🔴 CRITIQUE | Incohérence mathématique majeure |
| 3 | cov fortement biaisé | 🔴 Critique | Cov sous-estimée ~1.4x |
| 4 | y = 0 pour x ≤ 0 arbitraire | 🟠 Grave | Discontinuité à x = 0 |
| 5 | Retourne 1e100 au lieu de ∞ | 🟡 Mineur | Solutions fausses si y ≈ a |
| 6 | Utilise seulement une racine | 🟠 Grave | Potentiellement mauvaise réponse |
| 6 | Retourne x = 0 si disc < 0 | 🟡 Mineur | Pas d'indication d'erreur |
| 7 | asin domaine invalide | 🔴 Critique | Produit NaN si y hors range |
| 7 | Division par zéro si a = 0 | 🔴 Critique | Crash potentiel |
| 8 | Pas de validation que courbe est sigmoidale | 🔴 Critique | Solutions dégénérées possibles |
| Tous | r n'a pas de sens pour non-linéaire | 🔴 Critique | Statistiques incorrectes |

---

## Recommandations

### Priorité 1 (Critiques)
1. **Mode 3**: Utiliser nn au lieu de n pour cov
2. **Mode 3**: Exclure les points exclus du calcul de r, r², cov
3. **Mode 7**: Vérifier que (y - d) / a ∈ [-1, 1] avant asin
4. **Tous**: Redéfinir ce qu'est "r" pour les régressions non-linéaires

### Priorité 2 (Graves)
1. **Mode 4**: Documenter le comportement x ≤ 0
2. **Mode 6**: Permettre les deux racines ou documenter le choix
3. **Mode 8**: Valider que la solution est sigmoidale

### Priorité 3 (Mineurs)
1. **Mode 5**: Utiliser inf au lieu de 1e100
2. **Mode 6**: Gérer le cas c = 0
3. Tous: Ajouter des messages d'erreur pour les cas dégénérés
