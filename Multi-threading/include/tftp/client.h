#ifndef _TFTP_CLIENT_H_
#define _TFTP_CLIENT_H_

// System
#include <stdint.h>

// Local
#include "tftp/sock.h"


//--------------------------------------------------------------------------------------------------------------
// Module: CLIENT
// Description:
//      Client TFTP
//--------------------------------------------------------------------------------------------------------------


/** Structure de donnees associee au client TFTP
 *
 */
typedef struct
{
    Sock* sock;     // Socket du client
    Addr* toSrv;    // Adresse du serveur
} Client;


/** Creation d'un client
 *
 *  port: port UDP local du client
 *  srvHost: machine ou IP du serveur
 *  srvName: nom du service ('tftp')
 */
extern Client* CLIENT_create( const char* srvHost, uint16_t srvPort );

/** Lancement du client TFTP (On tape nos commandes dans le terminal)
 *
 */
extern void CLIENT_run( Client* client );

/** Lancement du client TFTP (On spam des commandes dans le terminal)
 *
 */
extern void CLIENT_MultiRun( Client* client );

/** Destruction d'un client
 *
 */
extern void CLIENT_destroy( Client* client );

#endif // _TFTP_CLIENT_H_
