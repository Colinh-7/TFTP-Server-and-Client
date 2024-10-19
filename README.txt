------------------------------------------------------
			Select
------------------------------------------------------
Tous les changements liés au select sont dans le fichier sock.c
Pour lancer le serveur taper ces commandes :

- Compiler : "make"
- Exécuter : "./bin/tftp --mode SRV --port 6999"

Vous pouvez lancer plusieurs commandes en même temps en lançant deux instances de clients (dans un répertoire différent sinon problème de concurrences sur les fichiers).

Commandes pour lancer un client lançant des requetes sur le port 6999 : "./bin/tftp --mode CLT --port 6999"


------------------------------------------------------
		Multi-threading
------------------------------------------------------

Pour cette étape nous avons rajouter une structure "Service" qui sont les différents threads auxquels sont donnés les requêtes à gérer. 

Le "Server" possède une liste de service. Quand celui-ci reçoit une requête, il la transmet à un de ses services disponibles. Le "Service" choisit prend donc le relais et traite la requête. Cela permet à notre serveur de traiter des requêtes en parallèle.

Pour éviter les problèmes de concurrences, nous avons assigner un mutex à chaque fichier disponible à la racine du serveur. Tous ces mutex sont stockés dans un AVL pour une recherche plus rapide.

Voici une schématisation de notre AVL.

                Node
          +-------------+
          |   Mutex     |
          |             |
          |   File      |
          +-------------+
          /           \
      Node             Node
+-------------+     +-------------+
|   Mutex     |     |   Mutex     |
|             |     |             |
|   File      |     |   File      |
+-------------+     +-------------+
   /      \               /     \
Node     Node         Node      Node

Lorsqu'un service traite une requête, il prend le mutex associé au nom de fichier correspondant, une fois la requête terminée il le relâche.

Commandes :
-----------

- "make"
- "./bin/tftp --mode SRV --port 6999"


------------------------------------------------------
		Spam requêtes clients
------------------------------------------------------

Pour tester notre serveur, nous avons fait un script bash permettant de lancer en arrière plan des requêtes aléatoires.

Il est à votre disposition si vous voulez l'utiliser. (run.sh)

Il s'exécute de la manière suivante :
- "./run.sh [nombre de clients envoyant des requêtes aléatoires en même temps] [PORT]"

Exemple : "./run.sh 3 6999"
	
Trois clients enverront donc en même temps des requêtes RRQ ou WRQ aléatoirement.
