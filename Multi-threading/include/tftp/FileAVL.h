#ifndef _FILEALV_H_
#define _FILEAVL_H_

#include <pthread.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief AVL de fichier disponible sur le serveur
*/
typedef struct FileAVL {
    char filename[256];
    pthread_mutex_t mutex;
    struct FileAVL *left, *right;
} FileAVL;

/**
 * @brief Instancie l'AVL de départ avec tous les fichiers présents à la racine de l'exécutable du serveur.
 * @return Renvoie l'avl de départ. L'avl doit être free avec FILEAVL_destroy(FileAVL * ).
*/
extern FileAVL *FILEAVL_create();
/**
 * @brief Ajoute un élément dans l'AVL. Nécessaire après un WRQ reçu par un client si le fichier n'existe pas.
 * @param filepath : nom du fichier à ajouter.
 * @param avl : l'avl dans lequel on ajoute filepath
 * @return Renvoie l'avl avec le fichier ajouté.
*/
extern FileAVL *FILEAVL_addInAVL(const char *filepath, FileAVL *avl, pthread_mutex_t *avl_mutex);
/**
 * @brief Permet de trouver un élément dans l'AVL. Cela permet de récupérer le noeud correspondant au fichier et donc de prendre le mutex associé.
 * @param avl : l'AVL dans lequel on cherche l'élément.
 * @param filename : le nom du fichier que l'on cherche.
 * @return Renvoie le noeud cherché.
*/
extern FileAVL *FILEAVL_findInAVL(FileAVL *avl, const char *filename, pthread_mutex_t *avl_mutex);
/**
 * @brief Libère la mémoire de l'AVL.
*/
extern void FILEAVL_destroy(FileAVL *avl);


#endif