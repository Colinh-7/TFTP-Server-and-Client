#include "tftp/service.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Local
#include "tftp/tftp.h"
#include "tftp/packet.h"
#include "tftp/server.h"


//--- Fonctions publiques --------------------------------------------------------------------------------------

Service* SERVICE_create( uint16_t port , FileAVL **avl)
{
    // Allocation de la struture de donnees
    Service* service = (Service*)malloc( sizeof( Service ) );
    service->sock = NULL;
    service->addr = NULL;
    service->packet = NULL;
    service->flag = 0;
    service->avl = avl;
    pthread_mutex_init( &service->mutex, NULL );

    // Creation de la socket (attachee sur le port specifie)
    service->sock = SOCK_create( port );
    if( service->sock == NULL )
    {
        free( service );
        return( NULL );
    }

    return( service );
}


Service* SERVICE_createEmpty()
{
    // Allocation de la struture de donnees
    Service* service = (Service*)malloc( sizeof( Service ) );
    service->sock = NULL;
    service->addr = NULL;
    service->packet = NULL;
    service->flag = 1;

    return( service );
}


void* SERVICE_ProcessRequest( void* arg )
{
    // Cast en Service*
    Service* service = (Service*)arg;

    // Creation de la socket qu'on va utiliser pour les echanges avec le client
    Sock* sock = SOCK_create( 0 );
    if( sock == NULL ) return NULL;

    // Gestion du timeout
    struct timeval timeout;
    timeout.tv_sec = 10;    // 10 secondes
    timeout.tv_usec = 0;

    if (setsockopt(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) 
    {
        perror("Erreur timeout : ");
        SOCK_destroy( sock);
        return NULL;
    }

    // Va servir à la recherche dans l'AVL
    FileAVL *node = NULL;

    // Suivant la nature du paquet recu
    switch( service->packet->code )
    {
        // RRQ
        case TFTP_RRQ:
            // On cherche le noeud associé au fichier
            node = FILEAVL_findInAVL(*(service->avl), ((XrqPacket*)service->packet->data )->fileName, service->avl_mutex);
            if (node) {
                pthread_mutex_lock(&(node->mutex));
                TFTP_sendFileToEndpoint( sock, ( (XrqPacket*)service->packet->data )->fileName, service->addr );
                pthread_mutex_unlock(&(node->mutex));
            }
            else {
                TFTP_sendErrorPacket( sock, ERR_FILE_NOT_FOUND, "Fichier introuvable", service->addr );
            }
            break;

        // RRQ
        case TFTP_WRQ:
            // On cherche le noeud associé au fichier
            node = FILEAVL_findInAVL(*(service->avl), ((XrqPacket*)service->packet->data )->fileName, service->avl_mutex);

            if (node) { // Si le noeud existe déjà, pas besoin de le recréer
                pthread_mutex_lock(&(node->mutex));
                SERVICE_RecvFile( sock, service->addr, (XrqPacket*)service->packet->data );
                pthread_mutex_unlock(&(node->mutex));
            }
            else {  // Si le noeud n'existe pas cela veut dire qu'on doit le créer car on reçoit un nouveau fichier
                FILEAVL_addInAVL(((XrqPacket*)service->packet->data )->fileName, *(service->avl), service->avl_mutex);
                node = FILEAVL_findInAVL(*(service->avl), ((XrqPacket*)service->packet->data )->fileName, service->avl_mutex);
                if (node) {
                    pthread_mutex_lock(&(node->mutex));
                    SERVICE_RecvFile( sock, service->addr, (XrqPacket*)service->packet->data );
                    pthread_mutex_unlock(&(node->mutex));
                }
                else {
                    perror("Erreur création du fichier dans l'AVL.");
                    TFTP_sendErrorPacket( sock, ERR_UNDEFINED, "Erreur AVL", service->addr );
                }
            }
            break;

        // Requete hors protocole
        default:
            fprintf( stderr, "ERREUR - Requête inattendue (code = %u)\n", service->packet->code );
            break;
    }

    // Liberation memoire
    PACKET_destroy( service->packet );
    ADDR_destroy( service->addr );
    SOCK_destroy( service->sock );
    SOCK_destroy( sock );

    // Liberation des ressources associe au thread
    pthread_detach( service->thread );

    // Lock du mutex pour la variable flag
    pthread_mutex_lock( &service->mutex );
    // thread de nouveau disponible
    service->flag = 1;
    // Unlock du mutex pour la variable flag
    pthread_mutex_unlock( &service->mutex );

    return NULL;
}


int SERVICE_RecvFile( Sock* sock, Addr* cltAddr, XrqPacket* wrq )
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

    // Fermeture du fichier
    fclose( file );

    return( 0 );
}


void SERVICE_destroy( Service* service )
{
    // Si serveur valide
    if( service != NULL )
    {
        // Destruction de la socket
        if( service->sock ) SOCK_destroy( service->sock );

        // Destruction de l'adresse
        if( service->addr ) ADDR_destroy( service->addr );

        // Destruction du paquet
        if( service->packet ) PACKET_destroy( service->packet );

        // Destruction du mutex
        pthread_mutex_destroy( &service->mutex );

        // Liberation memoire
        free( service );
    }
}
