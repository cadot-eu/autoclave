# Contrôleur d'Autoclave - Projet Expérimental

**⚠️ PROJET EXPÉRIMENTAL - À DES FINS ÉDUCATIVES UNIQUEMENT ⚠️**

Ceci est un projet expérimental de contrôleur d'autoclave utilisant Arduino Mega32U4. Ce projet est destiné à des fins éducatives et expérimentales uniquement. **Ne pas utiliser en environnement de production sans certifications de sécurité appropriées et validation professionnelle.**

## Aperçu du Projet

Ce projet implémente un contrôleur d'autoclave critique pour la sécurité avec les fonctionnalités suivantes :

- **Détection d'eau** utilisant des sondes en acier inoxydable
- **Surveillance de pression** avec capteur 0-30 PSI
- **Contrôle de minuterie** avec durée ajustable (1-99 minutes)
- **Arrêt de sécurité** pour absence d'eau, surpression et expiration du timer
- **Régulation de pression** avec contrôle à hystérésis
- **Affichage 4 digits TM1637** pour surveillance de l'état

## ⚠️ AVERTISSEMENT DE SÉCURITÉ CRITIQUE ⚠️

**ATTENTION : Ce système contrôle un élément chauffant 220V dans un autoclave sous pression. Des erreurs de câblage peuvent être FATALES. Faites vérifier le système par un électricien qualifié avant utilisation.**

## Composants Matériels

- **Mega32U4** (microcontrôleur compatible Arduino)
- **DS3231** (horloge temps réel - actuellement non utilisée dans le code)
- **Affichage 4 digits TM1637**
- **Capteur de pression 0-30PSI** (sortie analogique 0.5-4.5V)
- **Relais JQC3F-03VDC-C** (bobine 3V, contacts 250V/10A)
- **Interrupteur principal 220V** (marche/arrêt)
- **3 micro-interrupteurs** (+ / - / Mode)
- **2 vis en acier inoxydable** (sondes de détection d'eau avec joints toriques pour isolation électrique de la cuve de l'autoclave)
- **Condensateurs 104** (100nF)

## Fonctionnalités de Sécurité

### 1. **Sécurité Eau (Double détection)**
- Deux sondes en acier inoxydable indépendantes
- Arrêt immédiat si pas d'eau détectée
- L'élément chauffant ne peut pas démarrer sans eau

### 2. **Sécurité Pression (Triple niveau)**
- **Niveau 1** : Coupure configurable (5-20 PSI)
- **Niveau 2** : Coupure forcée à 15 PSI
- **Niveau 3** : ARRÊT D'URGENCE à 21 PSI

### 3. **Sécurité Minuterie**
- Arrêt automatique à 0 minute
- Impossible de démarrer avec un timer négatif
- Timer maximum 999 minutes

### 4. **Sécurité Électrique**
- Relais NO (Normalement Ouvert)
- Fusible sur phase 220V (recommandé 16A)
- Isolation galvanique Arduino/220V

## Modes de Fonctionnement

### Démarrage Automatique (Option 1)
1. **Interrupteur ON** → Arduino s'alimente, démarre automatiquement
2. **Timer = 45min** par défaut au démarrage
3. **Vérifications automatiques** :
   - Présence d'eau (2 sondes)
   - Pression < seuil configuré
   - Timer > 0
4. **Si OK** → Relais activé, chauffage ON
5. **Décompte automatique** toutes les minutes

### Contrôles Utilisateur
- **Interrupteur +** : +5 minutes (max 999)
- **Interrupteur -** : -5 minutes (min 0)
- **Interrupteur Mode** : Basculer Timer ↔ Configuration Pression
- **En mode Config** : +/- ajuste le seuil de coupure (0.1-10.0 PSI) par pas de 0.1

### Arrêts de Sécurité
- **Pas d'eau** → Arrêt immédiat
- **Pression > seuil** → Arrêt immédiat
- **Pression > 21 PSI** → ARRÊT D'URGENCE (affichage "8888" clignotant)
- **Timer = 0** → Arrêt automatique
- **Interrupteur principal OFF** → Coupure totale de l'alimentation

## Structure du Code

Le programme principal (`programme/programme.ino`) comprend :

- **Définitions des broches** pour tous les composants
- **Seuils de pression** configurables pour modes test/production
- **Détection d'eau** avec seuil configurable
- **Gestion du timer** avec décompte
- **Régulation de pression** avec hystérésis
- **Fonctions d'affichage** pour l'écran 4 digits TM1637
- **Vérifications de sécurité** dans la boucle principale

## Installation

### Prérequis
1. **IDE Arduino** installé sur votre ordinateur
2. **Arduino Mega32U4** (ou compatible) connecté via USB
3. **Bibliothèque TM1637** installée dans l'IDE Arduino

### Installation de la Bibliothèque TM1637
1. Ouvrez l'IDE Arduino
2. Allez dans **Outils → Gérer les bibliothèques...**
3. Recherchez **"TM1637"** par Avishay Orpaz
4. Cliquez sur **Installer**

### Configuration de l'IDE
1. Connectez votre Arduino Mega32U4 via USB
2. Dans l'IDE Arduino :
   - **Outils → Type de carte** : Sélectionnez "Arduino Leonardo"
   - **Outils → Port** : Sélectionnez le port correspondant à votre carte (ex : COM3, /dev/ttyACM0)
   - **Outils → Processeur** : "ATmega32U4"

### Téléversement du Code
1. Ouvrez `programme/programme.ino` dans l'IDE Arduino
2. Vérifiez que toutes les bibliothèques nécessaires sont installées
3. Cliquez sur **"Vérifier"** (✓) pour compiler sans erreurs
4. Cliquez sur **"Téléverser"** (→) pour envoyer le code sur la carte
5. Attendez le message **"Téléversement terminé"**

### Vérification
- L'affichage TM1637 devrait montrer le temps restant (ex : "4500" pour 45 minutes)
- La LED sur la broche 10 devrait s'allumer brièvement au démarrage
- Ouvrez le **Moniteur Série** (Outils → Moniteur Série) à 115200 bauds pour voir les messages de débogage

## Tests

Avant utilisation, tests obligatoires :

- [ ] Arrêt sur absence d'eau
- [ ] Arrêt sur surpression (simuler)
- [ ] Arrêt d'urgence à 21 PSI
- [ ] Fonctionnement de l'interrupteur principal
- [ ] Décompte correct du timer
- [ ] Modes d'affichage timer/config

**EN CAS DE DOUTE, NE PAS UTILISER. CONSULTER UN PROFESSIONNEL.**

## Avertissement

Ce projet est fourni "tel quel" sans garantie d'aucune sorte. Les auteurs ne sont pas responsables des dommages, blessures ou pertes résultant de l'utilisation de ce projet. Ceci est un projet expérimental/éducatif uniquement.

## Licence

Ce projet est à des fins éducatives. Utilisez à vos propres risques.

---

**Rappelez-vous : Ce n'est qu'une expérience !**