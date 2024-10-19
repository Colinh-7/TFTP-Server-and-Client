#include "tftp/client.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Local
#include "tftp/tftp.h"
#include "tftp/packet.h"

// Commandes disponibles
enum { CMD_NONE = -1, CMD_GET = 0, CMD_PUT, CMD_HELP, CMD_EXIT };
static const char* CMDS[] =
{
    "get",
    "put",
    "help",
    "exit"
};
static const size_t CMD_COUNT = sizeof( CMDS ) / sizeof( char* );

// Taille de buffer
#define BUFF_SIZE 1024

// Taille de mot
#define WORD_SIZE 256

// Terminaison possible d'une reception de fichier
enum
{
    RECV_FILE_COMPLETE,         // Reception terminee
    RECV_FILE_IN_PROGRESS,      // Reception en cours
    RECV_FILE_ERROR             // Erreur lors de la reception
};


//--- Declaration des fonctions locales ------------------------------------------------------------------------

/** Envoi d'un fichier au serveur
 *
 */
static int putFile( Client* client, char* filePath );

/** Demande de fichier au serveur
 *
 */
static int getFile( Client* client, char* filePath );

/** Reception d'un morceau de fichier envoye par le serveur, avec renvoi de l'ACK
 *
 */
static int recvNextFileChunk( Client* client, FILE* file, uint16_t* lastBlock, Addr* from );

/** Parsing d'une ligne de commande (controle, extraction du code de commande et d'un eventuel argument)
 *
 */
static int parseCmdLine( char* buff, int* cmd, char* arg );

/** Retourne le code de commande correspondant au mot specifie (ou CMD_NONE si non-reconnu)
 *
 */
static int stringToCmd( const char* sCmd );

/** Aide en ligne
 *
 */
static void printHelp();


//--- Fonctions publiques --------------------------------------------------------------------------------------

Client* CLIENT_create( const char* srvHost, uint16_t srvPort )
{
    // Allocation de la struture de donnees
    Client* client= (Client*)malloc( sizeof( Client ) );
    client->sock = NULL;
    client->toSrv = NULL;

    // Creation de la socket
    client->sock = SOCK_create( 0 );
    if( client->sock == NULL )
    {
        free( client );
        return( NULL );
    }
    // Timeout
    struct timeval timeout;
    timeout.tv_sec = 10;  // 10 secondes
    timeout.tv_usec = 0;

    if (setsockopt(client->sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout)) < 0) {
        perror("Erreur timeout : ");
        return NULL;
    }


    // Creation de l'adresse du serveur
    client->toSrv = ADDR_createRemote( srvHost, srvPort );
    if( client->toSrv == NULL )
    {
        free( client );
        return( NULL );
    }

    return( client );
}


void CLIENT_run( Client* client )
{
    // Boucle d'interactivite avec le client
    int loop = 1;
    while( loop )
    {
        // Prompt
        fprintf( stdout, "\ntftp> " );

        // Attente d'une ligne de commande (et suppression du '\n')
        char buff[BUFF_SIZE];
        fgets( buff, sizeof( buff ), stdin );
        buff[strlen( buff ) - 1] = '\0';
        if( strlen( buff ) == 0 ) continue;

        // Parsing de la ligne de commande
        int cmd = CMD_NONE;
        char arg[WORD_SIZE];
        if( parseCmdLine( buff, &cmd, arg ) != 0 ) continue;

        // Selon la commande
        switch( cmd )
        {
            // Envoi d'un fichier
            case CMD_PUT:
                putFile( client, arg );
                break;

            // Recuperation d'un fichier
            case CMD_GET:
                getFile( client, arg );
                break;

            // Aide en ligne
            case CMD_HELP:
                printHelp();
                break;

            // Fin de session
            case CMD_EXIT:
                loop = 0;
                fprintf( stdout, "bye!\n" );
                break;
        }
    }
}


void CLIENT_destroy( Client* client )
{
    // Si client valide
    if( client != NULL )
    {
        // Destruction de la socket
        if( client->sock ) SOCK_destroy( client->sock );

        // Destruction de l'adresse du serveur
        if( client->toSrv ) ADDR_destroy( client->toSrv );

        // Liberation memoire
        free( client );
    }
}


//--- Fonctions locales ---------------------------------------------------------------------------------------

int putFile( Client* client, char* filePath )
{
    // Verification que le fichier est accessible en lecture
    if( access( filePath, R_OK ) != 0 )
    {
        fprintf( stderr, "Fichier inconnu : %s\n", filePath );
        return( 1 );
    }

    // Envoi du paquet WRQ
    if( TFTP_sendXrqPacket( client->sock, TFTP_WRQ, filePath, client->toSrv ) != 0 ) return( 1 );

    // Attente de la reponse (ACK ou ERROR), et recuperation de l'adresse qu'il faudra utiliser pour le transfert
    Addr* from = ADDR_create();
    Packet* response = TFTP_recvPacket( client->sock, from );
    if( response == NULL  || response == TIMEOUT)
    {
        ADDR_destroy( from );
        return( 2 );
    }

    // Valeur retournee
    int status = 0;

    // Selon le code de la reponse
    switch( response->code )
    {
        // ACK
        case TFTP_ACK:
            // Envoi du fichier
            status = TFTP_sendFileToEndpoint( client->sock, filePath, from );
            if( status == 0 )
            {
                fprintf( stdout, "OK - Fichier envoyé : %s\n", filePath );
            }
            break;

        // ERROR
        case TFTP_ERROR:
        {
            // Affichage de l'erreur
            ErrorPacket* err = (ErrorPacket*)response->data;
            fprintf( stderr, "ERREUR - code = %u, msg = %s\n", err->errorCode, err->errorMsg );
            status = 4;
            break;
        }
        break;

        // Code imprevu
        default:
            fprintf( stderr, "ERREUR - Réception d'un paquet non-prévu: code = %u\n", response->code );
            status = 5;
            break;
    }

    // Liberation memoire
    PACKET_destroy( response );
    ADDR_destroy( from );

    return( status );
}


int getFile( Client* client, char* filePath )
{
    // Construction du path du fichier local
    char fileName[FILENAME_SIZE];
    char* lastSlash = strrchr( filePath, '/' );
    if( lastSlash != NULL )
    {
        // Copie de la fin du path (apres le dernier '/')
        strcpy( fileName, lastSlash + 1 );
    }
    else
    {
        // Copie du path entier (qui ne contient pas de '/' )
        strcpy( fileName, filePath );
    }

    // Ouverture du fichier local
    FILE* file = fopen( fileName, "wb" );
    if( file == NULL )
    {
        fprintf( stderr, "ERREUR - Impossible d'ouvrir le fichier local: %s\n", fileName );
        return( 1 );
    }

    // Envoi du paquet RRQ
    if( TFTP_sendXrqPacket( client->sock, TFTP_RRQ, filePath, client->toSrv ) != 0 )
    {
        fclose( file );
        unlink( fileName );
        return( 2 );
    }

    // Adresse renvoyee par le serveur pour la duree du transfert
    Addr* from = ADDR_create();

    // Boucle de reception des paquets de reponse
    int status = RECV_FILE_IN_PROGRESS;
    uint16_t lastBlock = 0;
    while( status == RECV_FILE_IN_PROGRESS )
    {
        // Reception du paquet suivant
        status = recvNextFileChunk( client, file, &lastBlock, from );

        // Si transfer termine
        if( status == RECV_FILE_COMPLETE )
        {
            // Fermeture du fichier
            fclose( file );
            fprintf( stdout, "OK - Fichier copié : %s\n", fileName );
            break;
        }

        // Si transfer en erreur
        else if( status == RECV_FILE_ERROR )
        {
            // Fermeture et suppression du fichier
            fclose( file );
            unlink( fileName );
            break;
        }
    }

    // Liberation memoire
    ADDR_destroy( from );

    return( 0 );
}


static int recvNextFileChunk( Client* client, FILE* file, uint16_t* lastBlock, Addr* from )
{
    // Code de retour
    int status = RECV_FILE_IN_PROGRESS;

    // Attente de la reponse (DATA ou ERROR)
    Packet* response = TFTP_recvPacket( client->sock, from );
    if( response == NULL || response == TIMEOUT) return( RECV_FILE_ERROR );

    // Selon le code de la reponse
    switch( response->code )
    {
        // DATA
        case TFTP_DATA:
        {
            // Paquet DATA
            DataPacket* packet = (DataPacket*)response->data;

            // Envoi de l'ACK (a l'adresse d'ou provient le paquet DATA)
            if( TFTP_sendAckPacket( client->sock, packet->blockNum, from ) != 0 )
            {
                status = RECV_FILE_ERROR;
                break;
            }

            // Si code bloc different du precedent
            if( packet->blockNum  != *lastBlock )
            {
                // Copie des donnees du paquet dans le fichier local
                fwrite( packet->bytes, packet->bytesCount, 1, file );

                // Mise a jour dernier bloc recu
                *lastBlock = packet->blockNum;
            }

            // Si le paquet a une taille inferieure a DATA_SIZE
            if( packet->bytesCount < DATA_SIZE )
            {
                // Il s'agit du dernier paquet DATA
                status = RECV_FILE_COMPLETE;
            }
        }
        break;

        // ERROR
        case TFTP_ERROR:
        {
            // Affichage de l'erreur
            ErrorPacket* err = (ErrorPacket*)response->data;
            fprintf( stderr, "ERREUR - code = %u, msg = %s\n", err->errorCode, err->errorMsg );
            status = RECV_FILE_ERROR;
        }
        break;

        // Code imprevu, fin de la reception
        default:
            fprintf( stderr, "ERREUR - Réception d'un paquet non-prévu: code = %u\n", response->code );
            status = RECV_FILE_ERROR;
            break;
    }

    // Liberation memoire
    PACKET_destroy( response );

    return( status );
}


static int parseCmdLine( char* buff, int* cmd, char* arg )
{
    // Initialisation du parsing
    char* token = strtok( buff, " " );

    // Recuperation de la commande
    if( token == NULL )
    {
        // Erreur : nom de commande manquant
        fprintf( stderr, "ERREUR - Pas de commande spécifiée\n" );
        fprintf( stderr, "Taper 'help' pour l'aide en ligne\n" );
        return( 1 );
    }

    // Verification que la commande est valide
    *cmd = stringToCmd( token );
    if( *cmd == CMD_NONE )
    {
        // Commande inconnue
        fprintf( stderr, "ERREUR - '%s': commande inconnue\n", token );
        fprintf( stderr, "Taper 'help' pour l'aide en ligne\n" );
        return( 2 );
    }
    char sCmd[WORD_SIZE];
    strcpy( sCmd, token );

    // Selon la commande
    switch( *cmd )
    {
        // Argument attendu pour les commandes ci-dessous
        case CMD_PUT:
        case CMD_GET:
            // Extraction et controle de l'argument
            token = strtok( NULL, " " );
            if( token == NULL )
            {
                fprintf( stderr, "ERREUR - La commande '%s' requiert un argument\n", sCmd );
                fprintf( stderr, "Taper 'help' pour l'aide en ligne\n" );
                return( 3 );
            }
            else
            {
                // Copie de l'argument
                strcpy( arg, token );
            }
            break;

        // Parsing termine
        default:
            break;
    }

    // Controle arguments superflus
    if( strtok( NULL, " " ) != NULL )
    {
        fprintf( stderr, "ERREUR - Trop d'arguments pour la commande '%s'\n", sCmd );
        fprintf( stderr, "Taper 'help' pour l'aide en ligne\n" );
        return( 4 );
    }

    return( 0 );
}


static int stringToCmd( const char* sCmd )
{
    // Comparaison aux mots de commandes connus
    for( size_t i = 0; i < CMD_COUNT; ++i )
    {
        // Si correspondance
        if( strcmp( sCmd, CMDS[i] ) == 0 )
        {
            // Commande identifiee
            return( (int)i );
        }
    }

    return( CMD_NONE );
}


static void printHelp()
{
    fprintf( stdout, "Utilisation : CMD [ARG]\n\n" );
    fprintf( stdout, "Commandes supportées :\n" );
    fprintf( stdout, "- put FILE: upload d'un fichier vers le serveur\n" );
    fprintf( stdout, "- get FILE: download d'un fichier depuis le serveur\n" );
    fprintf( stdout, "- help: affiche ce message\n" );
    fprintf( stdout, "- exit: termine la session TFTP\n" );
}
