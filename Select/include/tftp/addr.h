#ifndef _TFTP_ADDR_H_
#define _TFTP_ADDR_H_

// System
#include <stdint.h>
#include <arpa/inet.h>


//--------------------------------------------------------------------------------------------------------------
// Module: ADDR
// Description:
//      Gestion des adresses reseau (adresse IP + port TCP/UDP)
//--------------------------------------------------------------------------------------------------------------

// Longueur des noms
#define ADDR_SIZE 32

/** Structure de donnees associee a une addresse
 *
 */
typedef struct
{
    char host[ADDR_SIZE];       // Nom de machine ou adresse IP
    uint16_t port;              // Numero de port (si zero, alloue automatiquement)
    struct sockaddr_in inAddr;  // Addresse reseau (API socket)
} Addr;


/** Creation d'une adresse vide
 *
 */
extern Addr* ADDR_create();

/** Creation d'une adresse locale
 *
 *  Permet de creer une adresse sur la machine locale (127.0.0.1) et avec le port specifie.
 *  Si ce dernier est nul, il sera alloue automatiquement lors du bind d'une socket a cette adresse
 */
extern Addr* ADDR_createLocal( uint16_t port );

/** Creation d'un addresse avec un nom de machine et un nom de service
 *  
 *  Permet de creer l'adresse d'un serveur distant a partir de la machine et du port du service
 */
extern Addr* ADDR_createRemote( const char* host, uint16_t port );

/** Mise a jour de l'adresse a partir d'une adresse INET au format natif
 *
 */
extern void ADDR_update( Addr* addr, const struct sockaddr_in* inAddr );

/** Destruction d'une addresse
 *
 */
extern void ADDR_destroy( Addr* addr );

#endif // _TFTP_ADDR_H_
