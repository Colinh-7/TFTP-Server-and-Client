#include "tftp/FileAVL.h"


/*-----------------------------------
    Prototypes
------------------------------------*/
/**
 * @return Renvoie le plus grand entier entre a et b (renvoi a si a = b)
*/
int max (int a, int b);
/**
 * @return Renvoie la valeur absolue de a
*/
int absolute_value(int a);
/**
 * @brief Parcours profondeur gauche (sert à debug)
*/
void travers(FileAVL *avl);
/**
 * @brief Rotation gauche avl
*/
FileAVL *lr(FileAVL *avl);
/**
 * @brief Rotation droite avl
*/
FileAVL *rr(FileAVL *avl);
/**
 * @brief Rotation gauche droite
*/
FileAVL *lrr(FileAVL *avl);
/**
 * @brief Rotation droite gauche
*/
FileAVL *rlr(FileAVL *avl);
/**
 * @return Renvoie la hauteur de l'AVL (-1 si existe pas). L'AVL commence hauteur 1.
*/
int height(FileAVL *avl);
/**
 * @return Renvoie 1 si avl est un AVL, 0 sinon
*/
int isAVL(FileAVL *avl);
/**
 * @brief Rééquilibre avl
*/
FileAVL *rebalance(FileAVL *avl);
/**
 * @brief Ajoute elt à avl
*/
FileAVL *add_AVL(FileAVL *avl, FileAVL *elt);
/**
 * @brief Créé l'AVL initial, c'est-à-dire tous les fichiers disponibles sur le serveur dés le lancement du serveur.
*/
FileAVL *fillAVL(const char *path, FileAVL *avl);


FileAVL *findRec(FileAVL *avl, const char *filename);

/*-----------------------------------
    Fonctions locales
------------------------------------*/
int max (int a, int b) {
    return (a >= b) ? a : b;
}

int absolute_value(int a) {
    return (a > -1) ? a : -a;
}

void travers(FileAVL *avl) {
    if (avl) {
        printf("File : %s\n", avl->filename);
        printf("LEFT : \n");
        travers(avl->left);
        printf("RIGHT : \n");
        travers(avl->right);
    }
}


FileAVL *lr(FileAVL *avl) {
    FileAVL *right = avl;
    if (avl) {
        if (avl->right) {
            right = avl->right;
            if (avl->right->left) avl->right = right->left;
            else avl->right = NULL;
            right->left = avl;
        }
    }
    return right;
}


FileAVL *rr(FileAVL *avl) {
    FileAVL *left = avl;
    if (avl) {
        if (avl->left) {
            left = avl->left;
            if (avl->left->right) avl->left = left->right;
            else avl->left = NULL;
            left->right = avl;
        }
    }
    return left;
}

FileAVL *lrr(FileAVL *avl) {
    avl->left = lr(avl->left);
    return rr(avl);
}

FileAVL *rlr(FileAVL *avl) {
    avl->right = rr(avl->right);
    return lr(avl);
}

int height(FileAVL *avl) {
    if (avl) {
        return 1 + max(height(avl->left), height(avl->right));
    }
    else return -1;
}

int isAVL(FileAVL *avl) {
    if (avl) {
        if (absolute_value(height(avl->left)-height(avl->right)) < 2) {
            return (isAVL(avl->left) && isAVL(avl->right));
        }
        else return 0;
    }
    return 1;
}

FileAVL *rebalance(FileAVL *avl) {
    int balance, balance2;
    if (!isAVL(avl)) {
        balance = height(avl->left) - height(avl->right);

        switch (balance) {
            case 2:
                balance2 = height(avl->left->left)-height(avl->left->right);
                if (balance2 >= 0) avl = rr(avl);
                else avl = lrr(avl);
            break;
            case -2:
                balance2 = height(avl->right->left) - height(avl->right->right);
                if (balance2 > 0) avl = rlr(avl);
                else avl = lr(avl);
            break;
        }
    }
    return avl;
}

FileAVL *add_AVL(FileAVL *avl, FileAVL *elt){
    if (avl) {
        if (strcmp(elt->filename, avl->filename) < 0) {
            avl->left = rebalance(add_AVL(avl->left, elt));
            return rebalance(avl);
        }
        else {
            avl->right = rebalance(add_AVL(avl->right, elt));
            return rebalance(avl);
        }
    }
    return elt;
}

FileAVL *fillAVL(const char *path, FileAVL *avl) {
    struct dirent *entry;
    char fullpath[1024];
    struct stat statbuf;
    DIR *dir = opendir(path);

    if (dir != NULL) {

        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(path, "."))
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
            else
                snprintf(fullpath, sizeof(fullpath), "%s", entry->d_name);

            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            if (stat(fullpath, &statbuf) == -1) {
                perror("Erreur stat : ");
                continue;
            }

            if (S_ISDIR(statbuf.st_mode)) {
                avl = fillAVL(fullpath, avl);
            } else {
                FileAVL *node = (FileAVL*)malloc(sizeof(FileAVL));
                if (node) {
                    strcpy(node->filename, fullpath);
                    pthread_mutex_init(&node->mutex, NULL);
                    node->left = NULL;
                    node->right = NULL;

                    avl = add_AVL(avl, node);
                }
            }
        }
        closedir(dir);
    }
    return avl;
}

FileAVL *findRec(FileAVL *avl, const char *filename) {
    if (avl) {
        if (strcmp(filename, avl->filename) < 0) {
            return findRec(avl->left, filename);
        }
        else if (strcmp(filename, avl->filename) > 0) {
            return findRec(avl->right, filename);
        }
    }    
    return avl;
}

/*-----------------------------------
    Fonctions publiques
------------------------------------*/
FileAVL *FILEAVL_create() {
    return fillAVL(".", NULL);
}

FileAVL *FILEAVL_addInAVL(const char *filepath, FileAVL *avl, pthread_mutex_t *avl_mutex) {
    if (filepath) {
        if (*filepath == '/') filepath++;    // Enleve le '/' si y'en un au début
        FileAVL *node = (FileAVL*)malloc(sizeof(FileAVL));
        if (node) {
            node->left = NULL;
            node->right = NULL;
            strcpy(node->filename, filepath);
            pthread_mutex_init(&node->mutex, NULL);
            pthread_mutex_lock(avl_mutex);
            FileAVL *temp = add_AVL(avl, node);
            pthread_mutex_unlock(avl_mutex);
            return temp;
        }
    }
    return NULL;
}

FileAVL *FILEAVL_findInAVL(FileAVL *avl, const char *filename, pthread_mutex_t *avl_mutex) {
    FileAVL *result = NULL;
    if (*filename == '/') filename++;
    pthread_mutex_lock(avl_mutex);
    result = findRec(avl, filename);
    pthread_mutex_unlock(avl_mutex);
    return result;
}

void FILEAVL_destroy(FileAVL *avl) {
    if (avl) {
        FILEAVL_destroy(avl->left);
        FILEAVL_destroy(avl->right);
        pthread_mutex_destroy(&avl->mutex);
        free(avl);
    }
}
