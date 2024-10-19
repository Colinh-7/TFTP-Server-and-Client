#include "tftp/server.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Local
#include "tftp/tftp.h"
#include "tftp/packet.h"


//--- Declaration des fonctions locales ------------------------------------------------------------------------

/** Traitement d'une requette entrante
 *
 */
static int processRequest( Server* srv, Addr* cltAddr, Packet* request );

/** Traitement d'une requette WRQ
 *
 */
static int recvFile( Sock* sock, Addr* cltAddr, XrqPacket* wrq );


//--- Fonctions publiques --------------------------------------------------------------------------------------

Server* SERVER_create( uint16_t port )
{
    // Allocation de la struture de donnees
    Server* srv= (Server*)malloc( sizeof( Server ) );
    srv->sock = NULL;

    // Creation de la socket (attachee sur le port specifie)
    srv->sock = SOCK_create( port );
    if( srv->sock == NULL )
    {
        free( srv );
        return( NULL );
    }

    return( srv );
}


void SERVER_run( Server* srv )
{
    // Boucle de traitement des requetes entrantes (soit RRQ, soit WRQ)
    fprintf( stdout, "INFO - Serveur en attente de requêtes sur le port %u\n", srv->sock->addr->port );

	Packet* request = NULL;

    while( 1 )
    {
		Addr* cltAddr = ADDR_create();
		request = TFTP_recvPacket( srv->sock, cltAddr );
		fprintf( stdout, "INFO - Requête reçue (code = %u)\n", request->code );

		processRequest( srv, cltAddr, request );

        // Liberation memoire
        PACKET_destroy( request );
    }
}


void SERVER_destroy( Server* srv )
{
    // Si serveur valide
    if( srv != NULL )
    {
        // Destruction de la socket
        if( srv->sock ) SOCK_destroy( srv->sock );

        // Liberation memoire
        free( srv );
    }
}


//--- Fonctions locales ---------------------------------------------------------------------------------------

static int processRequest( Server* srv, Addr* cltAddr, Packet* request )
{
    // Valeur retournee
    int status = 0;

    // Creation de la socket qu'on va utiliser pour les echanges avec le client
    Sock* sock = SOCK_create( 0 );
    if( sock == NULL ) return( 1 );

    // Gestion du timeout
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 secondes
    timeout.tv_usec = 0;

    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("Erreur timeout : ");
        return 3;
    }

    // Suivant la nature du paquet recu
    switch( request->code )
    {
        // RRQ
        case TFTP_RRQ:
            status = TFTP_sendFileToEndpoint( sock, ( (XrqPacket*)request->data )->fileName, cltAddr );
            break;

        // RRQ
        case TFTP_WRQ:
            status = recvFile( sock, cltAddr, (XrqPacket*)request->data );
            break;

        // Requete hors protocole
        default:
            fprintf( stderr, "ERREUR - Requête inattendue (code = %u)\n", request->code );
            status = 2;
            break;
    }

    // Liberation memoire
    SOCK_destroy( sock );

    return( status );
}


static int recvFile( Sock* sock, Addr* cltAddr, XrqPacket* wrq )
{
    // Envoi ACK (numero de bloc = 0)
    if( TFTP_sendAckPacket( sock, 0, cltAddr ) != 0 ) return( 1 );

    // Ouverture du fichier
    FILE* file = fopen( wrq->fileName, "wb" );
    if( file == NULL )
    {
        // Fichier non trouve, envoi d'une erreur
        fprintf( stderr, "ERREUR - Fichier inexistant : %s\n", wrq->fileName );
        if( TFTP_sendErrorPacket( sock, ERR_FILE_NOT_FOUND, "Fichier inexistant", cltAddr ) != 0 )
        {
            fclose( file );
            unlink( wrq->fileName );
            return( 1 );
        }
    }

    // Reception du fichier
    if( TFTP_recvFileFromEndpoint( sock, file, cltAddr ) == 0 )
    {
        fprintf( stdout, "INFO - Fichier reçu : %s\n", wrq->fileName );
    }
    else {
    	fprintf( stdout, "INFO - Fichier mal reçu : %s\n", wrq->fileName );
    }

    // Fermeture du fichier
    fclose( file );

    return( 0 );
}
