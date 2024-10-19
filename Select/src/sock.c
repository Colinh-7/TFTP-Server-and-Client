#include "tftp/sock.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>


Sock* SOCK_create( uint16_t port )
{
    // Allocation de la struture de donnees
    Sock* sock= (Sock*)malloc( sizeof( Sock ) );
    sock->fd = 0;
    sock->addr = NULL;

    // Creation d une socket UDP/IP
    // - AF_INET = domaine IPV4
    // - SOCK_DGRAM = UDP
    sock->fd= socket( AF_INET, SOCK_DGRAM, 0 );

    // Si erreur lors de la creation de la socket alors
    if( sock->fd == -1 )
    {
        fprintf( stderr, "ERREUR - Echec de création de la socket:\n%s\n", strerror( errno ) );
        free( sock );
        return( NULL );
    }

    // Creation d'une adresse locale pour la socket
    sock->addr = ADDR_createLocal( port );
    if( ! sock->addr )
    {
        free( sock );
        return( NULL );
    }

    // Bind de la socket sur l'adresse
    if( bind( sock->fd, (const struct sockaddr*)&( sock->addr->inAddr ), sizeof( struct sockaddr ) ) != 0 )
    {
        fprintf( stderr, "ERREUR - Echec du bind de la socket:\n%s\n", strerror( errno ) );
        free( sock );
        return( NULL );
    }

    // Si le port specifie est nul
    if( port == 0 )
    {
        // Recuperation du port effectif de la socket
        struct sockaddr_in addr;
        socklen_t addrLen = 0;
        getsockname( sock->fd, (struct sockaddr*)&addr, &addrLen );
        sock->addr->port = ntohs( addr.sin_port );
    }

    return( sock );
}


int SOCK_sendData( Sock* sock, const void* data, size_t size, const Addr* to )
{
    // Envoi des donnees
    if( sendto( sock->fd, data, size, 0, (const struct sockaddr*)&( to->inAddr ), sizeof( to->inAddr ) ) == -1 )
    {
        fprintf( stderr, "ERREUR - Echec de l'envoi:\n%s\n", strerror( errno ) );
        return( -1 );
    }

    return( 0 );
}


int SOCK_recvSizedData( Sock* sock, void* data, size_t size )
{
    // Taille recue
    size_t receivedSize = 0;

    // Tant que on n'a pas recu la taille demandee, on attend des octets
    while( receivedSize < size )
    {
        // Attente et lecture de donnees
        ssize_t status = recvfrom( sock->fd, data + receivedSize, size - receivedSize, 0, NULL, NULL );
        if( status == -1 )
        {
            // Erreur de reception
            return( 1 );
        }

        // Mise a jour de la taille des donnees recues
        receivedSize += (size_t)status;
    }

    return( 0 );
}


int SOCK_recvData( Sock* sock, void* data, size_t* size, Addr* from )
{
    // Addresse de l'emetteur du datagramme recu
    struct sockaddr_in senderAddr;
    socklen_t addrLen = sizeof( senderAddr );
    ssize_t status;
    fd_set read_fd;                 // cet variable va nous servir à récupérer
    int max_fd = sock->fd + 1; // +1 car select requiert le plus grand descripteur + 1

    FD_ZERO(&read_fd);                  // On met le set read_fd à zéro
    FD_SET(sock->fd, &read_fd);    // On ajoute la socket du serveur au set d'écoute
        
    // Select va gérer les requêtes entrantes
    if (select(max_fd, &read_fd, NULL, NULL, NULL) < 0) {
        perror("Erreur select. ");
        return 2;
    }

    if (FD_ISSET(sock->fd, &read_fd)) {
        status = recvfrom( sock->fd, data, *size, 0, (struct sockaddr*)&senderAddr, &addrLen );
    }

    if (status == -1) {
        // Timeout
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Timeout catched\n");
            return -1; // On renvoie -1 lors des timeout
        } else {
            // Erreur de réception
            perror("Erreur de réception");
            return 1;
        }
    }

    // Si une adresse a ete fournie
    if( from != NULL )
    {
        // Mise a jour de l'adresse de l'emetteur
        ADDR_update( from, &senderAddr );
    }

    // Mise a jour de la taille des donnees recues
    *size = (size_t)status;

    return( 0 );
}


void SOCK_destroy( Sock* sock )
{
    // Si socket valide
    if( sock != NULL )
    {
        // Fermeture de la socket
        if( sock->fd != 0 ) close( sock->fd );

        // Destruction de l'adresse
        if( sock->addr ) ADDR_destroy( sock->addr );

        // Liberation memoire
        free( sock );
    }
}
