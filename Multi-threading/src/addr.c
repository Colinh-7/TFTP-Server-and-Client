#include "tftp/addr.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netdb.h>


Addr* ADDR_create()
{
    // Allocation de la struture de donnees
    Addr* addr= (Addr*)malloc( sizeof( Addr ) );
    memset( addr, 0, sizeof( Addr ) );

    return( addr );
}


Addr* ADDR_createLocal( uint16_t port )
{
    // Allocation de la struture de donnees
    Addr* addr= (Addr*)malloc( sizeof( Addr ) );
    memset( addr, 0, sizeof( Addr ) );

    // Recopie du host et du port
    strcpy( addr->host, "localhost" );
    addr->port = port;

    // Adresse Internet
    addr->inAddr.sin_family = AF_INET;

    // Initialisation du port
    addr->inAddr.sin_port = htons( port );

    // Initialisation de l'adresse IP
    addr->inAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" );

    return( addr );
}


Addr* ADDR_createRemote( const char* host, uint16_t port )
{
    // Conversion du port en string
    char strPort[32];
    sprintf( strPort, "%u", port );

    // Recuperation des informations sur l'adresse du serveur
    struct addrinfo hints;
    hints.ai_flags = 0;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = 0;
    hints.ai_addrlen = 0;
    hints.ai_addr = NULL;
    hints.ai_canonname = NULL;
    hints.ai_next = NULL;
    struct addrinfo* info = NULL;
    //int error = getaddrinfo( host, strPort, &hints, &info );
    int error = getaddrinfo( host, strPort, &hints, &info );
    if( error != 0 )
    {
        fprintf( stderr, "ERREUR - Echec lors de l'appel à getaddrinfo():\n%s\n", gai_strerror( error ) );
        return( NULL );
    }

    // Allocation de la struture de donnees
    Addr* addr= (Addr*)malloc( sizeof( Addr ) );
    memset( addr, 0, sizeof( Addr ) );

    // Recopie du host et du port
    const struct sockaddr_in* inAddr = (const struct sockaddr_in*)info->ai_addr;
    strcpy( addr->host, host );
    addr->port = ntohs( inAddr->sin_port );

    // Recopie de l'adresse native
    memcpy( &( addr->inAddr ), inAddr, sizeof( struct sockaddr_in ) );

    // Liberation memoire
    freeaddrinfo( info );

    return( addr );
}


void ADDR_update( Addr* addr, const struct sockaddr_in* inAddr )
{
    // Copy de l'adresse native
    memcpy( &( addr->inAddr ), inAddr, sizeof( struct sockaddr_in ) );

    // Mise a jour du port (on laisse le host vide acr pas utilise)
    addr->port = ntohs( inAddr->sin_port );
}


void ADDR_destroy( Addr* addr )
{
    // Si adresse valide
    if( addr != NULL )
    {
        // Liberation memoire
        free( addr );
    }
}
