
# _ESP_IDF_WEB_SERVER_

Ce code est un exemple de serveur web pour l'ESP32, une carte de développement basée sur un microcontrôleur ESP32 de la société Espressif. Le serveur utilise les bibliothèques ESP-IDF (Espressif IoT Development Framework) et LWIP (Lightweight IP), qui sont des bibliothèques logicielles pour développer des applications IoT (Internet des objets) pour les microcontrôleurs ESP32.

Le code inclut des bibliothèques nécessaires pour utiliser les fonctionnalités du serveur Web, telles que l'envoi de fichiers HTML et la gestion des requêtes HTTP. Il utilise également la bibliothèque GPIO pour la gestion des broches, ainsi que la bibliothèque wifi pour se connecter à un réseau sans fil.

Le code définit deux chaînes de caractères pour les réponses du serveur lors de la gestion de deux routes, '/led2on' et '/led2off', qui contrôlent l'état d'une broche GPIO. Les chaînes de caractères contiennent du code HTML pour la génération de pages web qui montrent l'état des broches et permettent de changer l'état via des boutons.

La fonction principale du code crée une connexion wifi, initialise les bibliothèques nécessaires, configure la broche GPIO, crée une structure pour la configuration du serveur et démarre le serveur. Le serveur écoute les requêtes HTTP entrantes et gère les requêtes correspondantes en fonction de l'URL de la requête. Si la requête est '/led2on', le serveur change l'état de la broche GPIO et renvoie la page web correspondante via la chaîne de caractères 'on_resp'. Si la requête est '/led2off', le serveur change l'état de la broche GPIO et renvoie la page web correspondante via la chaîne de caractères 'off_resp'.



## Comment utilisé le code
Ce code nécessite l'utilisation d'un point d'accès (PA) externe. Utilisez le fichier [main/Kconfig.projbuild](main/Kconfig.projbuild) qui se trouve dans le main. Vous devez entrer le SSID du PA ainsi que son mot de passe dans ce document. Une fois connectée, l'ESP-32 affichera dans le terminal son adresse IP et vous allez pouvoir y accéder grâce à votre navigateur web.
