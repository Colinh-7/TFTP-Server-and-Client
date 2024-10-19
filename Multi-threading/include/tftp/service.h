#ifndef _TFTP_SERVICE_H_
#define _TFTP_SERVICE_H_

// System
#include <stdint.h>
#include <pthread.h>

// Local
#include "tftp/sock.h"
#include "tftp/addr.h"
#include "tftp/packet.h"
#include "tftp/FileAVL.h"


//--------------------------------------------------------------------------------------------------------------
// Module: SERVICE
// Description:
//      Un service du serveur tftp
//--------------------------------------------------------------------------------------------------------------

/** Structure de donnees associee a un service
 *
 */
typedef struct
{
    Sock* sock;                 // Socket du service
    Addr* addr;                 // Adresse du client
    Packet* packet;             // Requete du service 
    pthread_t thread;           // Thread associer a un service
    int flag;                   // Flag pour savoir si le thread est disponible
    pthread_mutex_t mutex;      // Mutex du service 
    FileAVL **avl;              // AVL de mutex de fichier
    pthread_mutex_t *avl_mutex; // Mutex de l'AVL
} Service;


/** Creation d'un service sur le port UDP specifie
 *  
 */
extern Service* SERVICE_create( uint16_t port , FileAVL **avl);

/** Creation d'un service vide
 *  
 */
extern Service* SERVICE_createEmpty();

/** Traitement d'une requette entrante
 * 
 */
extern void* SERVICE_ProcessRequest( void* arg );

/** Traitement d'une requette WRQ
 * 
 */
extern int SERVICE_RecvFile( Sock* sock, Addr* cltAddr, XrqPacket* wrq );

/** Destruction d'un service
 *
 */
extern void SERVICE_destroy( Service* service );

#endif // _TFTP_SERVICE_H_
