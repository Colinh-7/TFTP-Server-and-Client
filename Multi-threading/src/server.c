#include "tftp/server.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Local
#include "tftp/tftp.h"
#include "tftp/packet.h"

//--- Fonctions publiques --------------------------------------------------------------------------------------

Server* SERVER_create( uint16_t port )
{
    // Allocation de la struture de donnees
    Server* srv= (Server*)malloc( sizeof( Server ) );
    srv->sock = NULL;

    for( int i = 0; i < MAX_NB_THREADS; ++i )
        srv->listService[i] = SERVICE_createEmpty();

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
    // Index du thread
    int index = -1;

    // Création de l'AVL
    FileAVL *avl = FILEAVL_create();
    if (!avl) return;

    // Création du mutex spécifique à l'AVL
    pthread_mutex_t avl_mutex;
    pthread_mutex_init(&avl_mutex, NULL);

    // Boucle de traitement des requetes entrantes (soit RRQ, soit WRQ)
    fprintf( stdout, "INFO - Serveur en attente de requêtes sur le port %u\n", srv->sock->addr->port );
    while( 1 )
    {
        // Attente d'une requete sur la socket
        Addr* cltAddr = ADDR_create();
        Packet* request = TFTP_recvPacket( srv->sock, cltAddr );
        fprintf( stdout, "INFO - Requête reçue (code = %u)\n", request->code );

        // Lock des mutex pour les variables flag dans chaque service
        for( int i = 0; i < MAX_NB_THREADS; ++i )
        {
            pthread_mutex_lock( &srv->listService[i]->mutex );
        }
        // On parcour la liste de thread pour trouver un thread disponible
        for( int i = 0; i < MAX_NB_THREADS; ++i )
        {
            if( srv->listService[i]->flag )
            {
                index = i;
                break;
            }
        }
        // Unlock des mutex pour les variables flag dans chaque service
        for( int i = 0; i < MAX_NB_THREADS; ++i )
        {
            pthread_mutex_unlock( &srv->listService[i]->mutex );
        }
        
        // Si aucun thread n'est disponible alors 
        if( index == -1 )
        {
            // Si le nombre maximal de threads est atteint on ignore la requete
            fprintf( stderr, "Nombre maximal de threads atteint. Ignorer la requête.\n" );
            ADDR_destroy( cltAddr );
            PACKET_destroy( request );
        }
        else
        {
            // Creation d'un service
            srv->listService[index] = SERVICE_create( 0 , &avl);

            // Attribution du mutex de l'AVL
            srv->listService[index]->avl_mutex = &avl_mutex;
            
            // Attribution de l'adresse et d'un paquet
            srv->listService[index]->addr = cltAddr;
            srv->listService[index]->packet = request;

            // Creation d'un thread
            pthread_create( &srv->listService[index]->thread, NULL, SERVICE_ProcessRequest, (void*)srv->listService[index] );

            index = -1;
        }
    }
    FILEAVL_destroy(avl);
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
