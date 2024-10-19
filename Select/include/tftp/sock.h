#ifndef _TFTP_SOCK_H_
#define _TFTP_SOCK_H_

// System
#include <stdint.h>

// Local
#include "tftp/addr.h"


//--------------------------------------------------------------------------------------------------------------
// Module: SOCK
// Description:
//      Gestion des sockets UDP
//--------------------------------------------------------------------------------------------------------------

/** Structure de donnees associee a une socket UDP
 *
 */
typedef struct
{
    int fd;         // File descriptor de la socket
    Addr* addr;     // Adresse a laquelle la socket est rattachee
} Sock;


/** Creation d'une socket UDP
 *
 *  La socket est automatiquement rattachee a une adresse locale avec le port UDP specifie (si nul, il est
 *  alloue automatiquement par l'OS)
 */
extern Sock* SOCK_create( uint16_t port );

/** Reception d'un bloc de donnees de taille connue
 *
 */
extern int SOCK_recvSizedData( Sock* sock, void* data, size_t size );

/** Envoi d'un bloc de donnees a l'adresse specifiee
 *
 */
extern int SOCK_sendData( Sock* sock, const void* data, size_t size, const Addr* to );

/** Reception d'un bloc de donnees de taille inconnue
 *
 *  En entree, size specifie la taille max du buffer de reception. En sortie, size contient la taille
 *  des donnees recues
 */
extern int SOCK_recvData( Sock* sock, void* data, size_t* size, Addr* from );

/** Destruction d'une socket
 *
 */
extern void SOCK_destroy( Sock* sock );

#endif // _TFTP_SOCK_H_
