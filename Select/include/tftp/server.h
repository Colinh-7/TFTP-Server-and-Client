#ifndef _TFTP_SERVER_H_
#define _TFTP_SERVER_H_

// System
#include <stdint.h>

// Local
#include "tftp/sock.h"


//--------------------------------------------------------------------------------------------------------------
// Module: SERVER
// Description:
//      Server TFTP
//--------------------------------------------------------------------------------------------------------------


/** Structure de donnees associee au serer TFTP
 *
 */
typedef struct
{
    Sock* sock;         // Socket du serveur (attente des requetes entrantes)
} Server;


/** Creation d'un serveur sur le port UDP specifie
 *
 */
extern Server* SERVER_create( uint16_t port );

/** Lancement du serveur TFTP
 *
 */
extern void SERVER_run( Server* srv );

/** Destruction d'un serveur
 *
 */
extern void SERVER_destroy( Server* srv );

#endif // _TFTP_SERVER_H_
