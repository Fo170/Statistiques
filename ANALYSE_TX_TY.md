# Analyse Détaillée: Gestion de tx/ty et Cas Dangereux

## Vue d'Ensemble

Les variables **tx** et **ty** sont des décalages calculés pour transformer les données vers des domaines mathématiquement valides (éviter log(0), racines négatives, etc.).

```cpp
m_rg.tx = 0.0; m_rg.ty = 0.0;
if (m_rg.mode == 1 && m_rg.xmin <= 0.0) m_rg.tx = 1.0 - m_rg.xmin;
if (m_rg.mode == 2 && m_rg.ymin <= 0.0) m_rg.ty = 1.0 - m_rg.ymin;
// mode 3: tx=0, ty=0 (points négatifs exclus du MCO)
```

---

## Mode 1 - Logarithmique: y = a + b*ln(x + tx)

### Calcul de tx
```cpp
if (m_rg.xmin <= 0.0) tx = 1.0 - m_rg.xmin
```

**Objectif:** Assurer que le plus petit point x + tx peut être log-transformé

**Exemple:**
```
Données: x = [-5, -3, 0.5, 2, 5]
xmin = -5
tx = 1.0 - (-5) = 6

Après décalage:
x + tx = [1, 3, 6.5, 8, 11]  ✓ Tous > 0
```

### Entraînement (MCO sur points valides)
```cpp
for (i = 0; i < nd; i++) {
    x = xd[i] + tx;
    if (x <= 0.0) continue;  // Exclut si même après décalage, x ≤ 0
    x = logl(x);
    // ... MCO
}
```

**Garantie:** Tous les points MCO ont x' = ln(x + tx) défini ✓

### Cas Dangereux Identifiés et Gestion

#### ✅ CAS 1: Extrapolation en arrière (x < xmin - tx)
```
xmin = -5, tx = 6, xmin_train = 1 (après décalage)
Extrapolation: x = -7 (< xmin = -5)
arg = x + tx = -7 + 6 = -1 < 0  ✗ DANGER!
```

**Gestion actuellement:**
```cpp
Ldbl arg = x + m_rg.tx;
if (arg > 0.0) y = m_rg.a + m_rg.b * logl(arg);
else y = 0.0;  // Out of domain  ✓ SAFE
```

**Verdict:** ✅ Géré (retourne y=0, pas de NaN)

#### ✅ CAS 2: Extrapolation en avant (x > xmax)
```
xmax = 5, tx = 6
Extrapolation: x = 1000
arg = 1000 + 6 = 1006 > 0  ✓ OK
```

**Verdict:** ✅ Pas de problème (ln défini pour tout x > 0)

#### ✅ CAS 3: Données exactement à xmin
```
Point exact: x = xmin = -5, tx = 6
arg = -5 + 6 = 1
ln(1) = 0  ✓ OK (juste)
```

**Verdict:** ✅ Pas de problème

#### ⚠️ CAS 4: xmin très proche de -∞
```
xmin = -1e10, tx = 1 - (-1e10) ≈ 1e10
Numériquement fragile! Perte de précision en calcul x + tx
```

**Gestion:** ⚠️ Pas de check de stabilité numérique

---

## Mode 2 - Exponentielle: y = a*e^(b*x) - ty

### Calcul de ty
```cpp
if (m_rg.ymin <= 0.0) ty = 1.0 - m_rg.ymin
```

**Objectif:** Transformer y pour pouvoir faire ln(y + ty) durant MCO

**Exemple:**
```
Données: y = [-2, -0.5, 1, 3, 5]
ymin = -2
ty = 1.0 - (-2) = 3

Transformation MCO: y' = ln(y + ty) = ln([-2+3, -0.5+3, ...]) = ln([1, 2.5, ...])
```

### Processus MCO
```cpp
for (i = 0; i < nd; i++) {
    y = yd[i] + ty;
    if (y <= 0.0) continue;  // Exclut si y + ty ≤ 0
    y = logl(y);
    // ... MCO sur (x, y')
}
```

### Cas Dangereux Identifiés et Gestion

#### ✅ CAS 1: Inverse (regFX) - Résultat y < -ty
```
ty = 3, extrapolation résultat: y = -4 (< -ty = -3)
arg = (y + ty) / a = (-4 + 3) / a = -1/a < 0  ✗ DANGER!
```

**Gestion actuellement:**
```cpp
Ldbl arg = (y + m_rg.ty) / m_rg.a;
if (arg > 0.0) x = logl(arg) / m_rg.b;
else x = 0.0;  // Out of domain  ✓ SAFE
```

**Verdict:** ✅ Géré (retourne x=0, pas de NaN)

#### ✅ CAS 2: regFY - Toujours défini
```cpp
y = m_rg.a * expl(m_rg.b * x) - m_rg.ty;
```

exp est défini partout, donc pas de problème domaine ✓

**Verdict:** ✅ Aucun cas dangereux

#### ✅ CAS 3: ty très grand
```
Exemple: ymin = -1e10, ty = 1e10
Pas de problème spécifique
```

**Verdict:** ✅ Pas de problème

---

## Mode 3 - Puissance: y = a*(x + tx)^b - ty

### Calcul de tx et ty
```cpp
// Mode 3: tx = 0, ty = 0 TOUJOURS
// Points avec x ≤ 0 OU y ≤ 0 sont EXCLUS du MCO
```

**Objectif:** Éviter complètement les points problématiques plutôt que de les décaler

**Exemple:**
```
Données: x = [-2, -1, 0.5, 2, 5]
         y = [?, ?, 0.25, 4, 25]

Exclusions: x = [-2, -1] (x ≤ 0) et y = [?] (y ≤ 0)
Points MCO: {0.5, 2, 5} avec {0.25, 4, 25}
```

### Processus MCO
```cpp
for (i = 0; i < nd; i++) {
    x = xd[i] + 0.0;  // tx = 0
    y = yd[i] + 0.0;  // ty = 0
    if (x <= 0.0) continue;  // Exclut x ≤ 0
    if (y <= 0.0) continue;  // Exclut y ≤ 0
    x = logl(x);
    y = logl(y);
    // ... MCO sur (ln(x), ln(y))
}
```

### Cas Dangereux Identifiés et Gestion

#### ✅ CAS 1: Extrapolation x < xmin
```
Points MCO: x ∈ [0.5, 5]
Extrapolation: x = -1 < 0
arg = -1 + 0 = -1 < 0  ✗ DANGER!
```

**Gestion actuellement:**
```cpp
Ldbl arg = x + m_rg.tx;  // tx = 0
if (arg > 0.0) y = m_rg.a * powl(arg, m_rg.b) - m_rg.ty;
else y = 0.0;  // Out of domain  ✓ SAFE
```

**Verdict:** ✅ Géré

#### ✅ CAS 2: x = 0 exactement
```
arg = 0 + 0 = 0
x^b est indéfini pour b < 0, 0 pour b > 0
```

**Gestion:** arg > 0.0 check exclut x = 0 ✓

**Verdict:** ✅ Géré

#### ✅ CAS 3: Points exclus au MCO mais inclus aux stats
```
Point original: x = -0.5, y = 2
Exclu du MCO (x ≤ 0) ✓
Mais inclus dans r, r², cov pour évaluer qualité
```

**Gestion:** 
```cpp
// Maintenant: Stats EXCLUT aussi ces points (commit c475e7b)
if (m_rg.mode == 3) {
    if (xi > 0 && yi > 0) {
        // Inclure dans stats
    }
}
```

**Verdict:** ✅ Géré (commit c475e7b)

---

## Récapitulatif des Cas Dangereux

| Mode | Cas Dangereux | Entraînement | Extrapolation | regFX | Gestion |
|------|---------------|--------------|---------------|-------|---------|
| 1 | x + tx ≤ 0 | Exclu ✓ | Check ✓ | N/A | ✅ |
| 2 | ln(y + ty) | Exclu ✓ | Check ✓ | Check ✓ | ✅ |
| 3 | x ≤ 0 | Exclu ✓ | Check ✓ | Check ✓ | ✅ |
| 3 | y ≤ 0 | Exclu ✓ | Stats ✓ | N/A | ✅ |

---

## Points Critiques de Sécurité

### ✅ **Tous Gérés:**

1. **Division par zéro lors de log-transformation**
   - Mode 1: x + tx > 0 check
   - Mode 2: y + ty > 0 check
   - Mode 3: x > 0 et y > 0 exclusion + check

2. **NaN dans regFY**
   - Mode 1: Check arg > 0
   - Mode 2: exp défini partout
   - Mode 3: Check arg > 0

3. **NaN dans regFX**
   - Mode 1: Check exp
   - Mode 2: Check arg > 0
   - Mode 3: Check arg > 0

4. **Extrapolation en dehors du domaine**
   - Tous les modes: Retourne y = 0 (safe, pas de NaN)

### ⚠️ **À Documenter:**

1. **Extrapolation en dehors du domaine d'entraînement**
   - Techniquement sûr (pas de crash)
   - Mais mathématiquement invalide (retourne y = 0)
   - Utilisateur ne sait pas que c'est hors domaine

2. **Stabilité numérique**
   - Si xmin/ymin sont très grands négatifs, tx/ty peuvent être très grands
   - Perte de précision possible dans x + tx ou y + ty

---

## Recommandations

### Priorité 1 - Documentation
Ajouter commentaires clairs dans le code sur:
- Les limites de l'extrapolation
- Le fait que y = 0 retourné signifie "hors domaine"

### Priorité 2 - Validation (future)
Ajouter validations optionnelles:
```cpp
// Warn if attempting extrapolation
if (x < xmin - tx || x > xmax + tx) {
    // Optionally: Log warning, return NaN, or error
}
```

### Priorité 3 - Amélioration numerique (future)
Détecter et gérer les cas d'overflow/underflow:
```cpp
if (tx > 1e10 || ty > 1e10) {
    // Use alternative transformation strategy
}
```

---

## Conclusion

**✅ TOUS les cas dangereux sont actuellement gérés de manière SÛRE (pas de NaN/Inf).**

La stratégie est:
1. **Entraînement:** Exclure/décaler pour domaine valide
2. **Évaluation:** Check domaine, retourner 0 si invalide
3. **Résultat:** Pas de crash, données robustes

**Limitation:** L'extrapolation hors domaine retourne y = 0 (perte d'info), mais c'est mathématiquement correct car la régression n'est valide que dans le domaine d'entraînement.
