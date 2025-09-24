# Schéma de Câblage - Contrôleur Autoclave Sécurisé

### Illustrations du Schéma (dossier `schemas`)

![Schéma électronique 1](schemas/elctronic.jpg)
![Schéma électronique 2](schemas/elctronic.png)
![Explication 1](schemas/explication.jpg)
![Explication 2](schemas/explication.png)

## ⚠️ SÉCURITÉ CRITIQUE ⚠️

**ATTENTION: Ce système contrôle une résistance 220V dans un autoclave sous pression. Une erreur de câblage peut être MORTELLE. Faites vérifier par un électricien qualifié.**

## Composants Principaux

- **Mega32U4** (Arduino compatible)
- **DS3231** (Horloge temps réel)
- **Afficheur 4 digits TM1637**
- **Capteur pression 0-30PSI** (0.5-4.5V analogique)
- **Relais JQC3F-03VDC-C** (bobine 3V, contacts 250V/10A)
- **Interrupteur 220V** (marche/arrêt général)
- **3 micro-switches** (+ / - / Mode)
- **2 vis inox** (détection d'eau)
- **Condensateurs 104 (100nF)**

## Schéma de Câblage

### Alimentation et Sécurité 220V (Option 1 - Transformateur)

```
SECTEUR 220V ──┬── INTERRUPTEUR GÉNÉRAL ──┬── TRANSFORMATEUR 220V→5V ── Arduino + circuits
               │                          │   (Isolation galvanique parfaite)
               │                          │
               └─────────── NEUTRE ───────┴── NEUTRE vers résistance
                            │
                         RELAIS ────────── PHASE vers résistance
                      (contacts NO)
```

### Connections Arduino Mega32U4

```
Mega32U4                     Composant
========                     =========
+5V ────────────────────────── VCC DS3231, Afficheur, Capteurs
+3.3V ──────────────────────── VCC Relais (bobine)
GND ────────────────────────── GND commun tous composants

Pin 2 ──────────────────────── Vis inox #1 (détection eau)
Pin 3 ──────────────────────── Vis inox #2 (détection eau)
Pin 4 ──────────────────────── Signal relais (avec résistance 1kΩ)
Pin 5 ──────────────────────── Switch + (avec condo 104)
Pin 6 ──────────────────────── Switch - (avec condo 104)  
Pin 7 ──────────────────────── Switch Mode (avec condo 104)
(Pin 8 non utilisé - interrupteur coupe l'alimentation directement)
Pin 9 ──────────────────────── CLK Afficheur TM1637
Pin 10 ─────────────────────── DIO Afficheur TM1637
Pin A0 ─────────────────────── Signal capteur pression

SDA (Pin 20) ───────────────── SDA DS3231
SCL (Pin 21) ───────────────── SCL DS3231
```

### Détail Circuit de Sécurité Eau

```
     +5V
      │
      ├── 10kΩ ──── Pin 2 (Mega32U4)
      │
   Vis INOX #1
      │
    EAU (conductrice)
      │
   Vis INOX #2  
      │
      ├── 10kΩ ──── Pin 3 (Mega32U4)
      │
     GND
```

### Circuit Capteur Pression

```
Capteur Pression (0-30PSI)
========================
Rouge (+5V) ──────────── +5V Mega32U4
Noir (GND) ───────────── GND Mega32U4
Jaune (Signal) ──────── Pin A0 Mega32U4
                        (0.5V-4.5V = 0-30PSI)
```

### Circuit Relais de Sécurité

```
Mega32U4 Pin 4 ──── 1kΩ ──── Base Transistor NPN (2N2222)
                              │
+3.3V ──── Bobine Relais ──── Collecteur
           │
           └── Diode 1N4007 (cathode vers +3.3V)
           
GND ─────────────────────────── Émetteur Transistor

CONTACTS RELAIS (ISOLÉS):
NO (Normalement Ouvert) ──── PHASE 220V vers résistance
COM (Commun) ──────────────── PHASE 220V secteur  
NC (Normalement Fermé) ──── Non utilisé
```

### Switches avec Anti-Rebond

```
Pour chaque switch (+ / - / Mode):

+5V ──── 10kΩ ──── Pin Arduino
          │
      Switch ──── GND
          │
    Condensateur 104 ──── GND
```

### Interrupteur Principal 220V (SOLUTION TRANSFORMATEUR)

```
COUPURE TOTALE - Plus d'optocoupleur nécessaire!

220V ──── INTERRUPTEUR ──── TRANSFORMATEUR 220V/5V ──── Arduino
                            (Isolation galvanique intégrée)

Quand interrupteur OFF = Arduino éteint = Sécurité maximale
Quand interrupteur ON  = Arduino démarre automatiquement avec timer 45min
```

## Liste des Sécurités Implémentées

### 1. **Sécurité Eau (Double détection)**

- Deux sondes inox indépendantes
- Arrêt immédiat si pas d'eau
- Résistance ne peut démarrer sans eau

### 2. **Sécurité Pression (Triple niveau)**

- **Niveau 1**: Coupure configurable (5-20 PSI)
- **Niveau 2**: Coupure forcée à 15 PSI
- **Niveau 3**: ARRÊT D'URGENCE à 21 PSI

### 3. **Sécurité Timer**

- Arrêt automatique à 0 minute
- Impossible de démarrer avec timer négatif
- Timer max 999 minutes

### 4. **Sécurité Électrique**

- Relais NO (normalement ouvert)
- Fusible sur phase 220V (recommandé 16A)
- Isolation galvanique Arduino/220V

## Mode de Fonctionnement

### Démarrage Automatique (Option 1)

1. **Interrupteur ON** → Arduino alimenté, démarre automatiquement
2. **Timer = 45min** par défaut au boot
3. **Vérifications automatiques**:
   - Présence d'eau (2 sondes)
   - Pression < seuil configuré
   - Timer > 0
4. **Si OK** → Relais activé, chauffage ON
5. **Décompte automatique** toutes les minutes

### Contrôles Utilisateur

- **Switch +** : +5 minutes (max 999)
- **Switch -** : -5 minutes (min 0)
- **Switch Mode** : Bascule Timer ↔ Config Pression
- **En mode Config** : +/- ajuste le seuil de coupure (0.1-10.0 PSI) par pas de 0.1

### Arrêts de Sécurité

- **Manque d'eau** → Arrêt immédiat
- **Pression > seuil** → Arrêt immédiat  
- **Pression > 21 PSI** → ARRÊT D'URGENCE (affichage "8888" clignotant)
- **Timer = 0** → Arrêt automatique
- **Interrupteur OFF** → Coupure totale

## Recommandations Transformateur

### Alimentations Recommandées

1. **HLK-PM01** (220V → 5V, 3W, compact)
2. **Alimentation rail DIN** 220V → 12V + régulateur 7805
3. **Alimentation à découpage** Mean Well (plus robuste)

### Avantages de cette solution

- ✅ **Isolation galvanique parfaite** (norme CE)
- ✅ **Simplicité de câblage** (pas d'optocoupleur)
- ✅ **Sécurité maximale** (coupure totale)
- ✅ **Démarrage automatique** à chaque mise sous tension
- ✅ **Moins cher** qu'une solution optocoupleur

## Test de Validation

Avant utilisation, tester obligatoirement:

- [ ] Coupure sur manque d'eau
- [ ] Coupure sur surpression (simuler)
- [ ] Arrêt d'urgence à 21 PSI
- [ ] Fonctionnement interrupteur principal
- [ ] Décompte timer correct
- [ ] Affichage modes timer/config

**EN CAS DE DOUTE, NE PAS UTILISER. CONSULTER UN PROFESSIONNEL.**
