# Écou-Terre

**Déscription du projet:** Nous créons un module qui captera le bruit des vers de terre et donnera ainsi une idée sur la biodiversité et la santé des sols. Ceci est le répertoire git de tous les fichiers techniques pour notre projet.

![Schéma Synoptique](https://github.com/EricGingras/Ecou-Terre/blob/main/Documentation_session5/Sch%C3%A9maSynoptiqueDT_V2.jpg)

**Déscription technique:** Nous utilisons un microphone électret pour capter le bruit des vers de terre. Ce signal est ensuite amplifié et numérisé pour pouvoir le transmettre dans un ESP-32 en format I2S. Par la suite, ces données I2S sont enregistrées dans une carte SD en format WAV, ce qui permet ensuite de les écouter sur un ordinateur. Il y a aussi un connecteur jack 3,5mm pour l'écoute en temps réelle des vers de terre. Le module au complet est alimenté par une pile 4.2 volts. Pour le contrôler, nous allons avoir un page web avec un interface pour le démarrage, l'ârret et d'autre fonctions du module.

# Fichiers
> Déscription des fichiers dans le répertoire git.
- **3d_print:** Contient les fichiers pour la seule impression 3d que nous avons fait pour notre projet à date: une base pour le boîtier.
- **Documentation_pieces:** Contient tous les fichiers de documentation pour les pièces principales qui se retrouvent sur le module.
- **Documentation_session5:** Contient des fichiers de la session passée qui servent encore pour notre projet.
- **Fichier_PCB:** Contient tous les fichiers KiCad pour les PCB.
- **Prog_Arduino:** Contient le code Arduino qui permet au module d'enregistrer les données I2S dans une carte SD.
- **Prog_ESP-32:** Contient le vieux code pour l'enregistrement des données I2S qui utilisait ESP-IDF avant le passage à Arduino ainsi que le code pour le serveur web qui servira à contrôler le module.
- **Prog_InterfaceOrdi:** Contiendra le code d'une interface pour écouté les échantillons de son sur la carte SD ainsi que pour affiché l'onde sonore et la position GPS de celle-ci. Cela ne servira peut-être pas dépendamment de si nous avons assez de temps pour faire le module GPS. Sinon, nous allons utiliser une application telle que "Audacity".


