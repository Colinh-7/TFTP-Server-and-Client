#include "tftp/tftp.h"

// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <arpa/inet.h>


// Terminaison possible lors de l'envoie de fichier
enum
{
    SEND_FILE_COMPLETE = 0,     // Envoie terminee
    SEND_FILE_IN_PROGRESS,      // Envoie en cours
    SEND_FILE_ERROR             // Erreur lors de l'envoie
};

// Terminaison possible d'une reception de fichier
enum
{
    RECV_FILE_COMPLETE,         // Reception terminee
    RECV_FILE_IN_PROGRESS,      // Reception en cours
    RECV_FILE_ERROR             // Erreur lors de la reception
};


//--- Declaration des fonctions locales ------------------------------------------------------------------------

/** Envoi d'un paquet
 *
 */
static int sendPacket( Sock* sock, Packet* packet, const Addr* to );


//--- Fonctions publiques --------------------------------------------------------------------------------------

int TFTP_sendXrqPacket( Sock* sock, uint16_t code, const char* fileName, const Addr* to )
{
    // Verification code
    assert( ( code == TFTP_RRQ || code == TFTP_WRQ ) && "Code RRQ/WRQ invalide !" );

    // Construction du paquet WRQ/RRQ
    Packet* packet = PACKET_create( code );
    if( packet == NULL ) return( 1 );
    strcpy( ( (XrqPacket*)packet->data )->fileName, fileName );

    // Envoi du paquet
    if( sendPacket( sock, packet, to ) != 0 ) return( 2 );

    return( 0 );
}


int TFTP_sendAckPacket( Sock* sock, uint16_t blockNum, const Addr* to )
{
    // Construction du paquet ACK
    Packet* packet = PACKET_create( TFTP_ACK );
    if( packet == NULL ) return( 1 );
    ( (AckPacket*)packet->data )->blockNum = blockNum;

    // Envoi du paquet
    if( sendPacket( sock, packet, to ) != 0 ) return( 2 );

    return( 0 );
}


int TFTP_sendDataPacket(
                    Sock* sock,
                    uint16_t blockNum,
                    const unsigned char* bytes,
                    uint16_t bytesCount,
                    const Addr* to )
{
    // Construction du paquet DATA
    Packet* packet = PACKET_create( TFTP_DATA );
    if( packet == NULL ) return( 1 );
    ( (DataPacket*)packet->data )->blockNum = blockNum;
    memcpy( ( (DataPacket*)packet->data )->bytes, bytes, bytesCount );
    ( (DataPacket*)packet->data )->bytesCount = bytesCount;

    // Envoi du paquet
    if( sendPacket( sock, packet, to ) != 0 ) return( 2 );

    return( 0 );
}


int TFTP_sendErrorPacket( Sock* sock, uint16_t error, const char* msg, const Addr* to )
{
    // Construction du paquet ERROR
    Packet* packet = PACKET_create( TFTP_ERROR );
    if( packet == NULL ) return( 1 );
    ( (ErrorPacket*)packet->data )->errorCode = error;
    strcpy( ( (ErrorPacket*)packet->data )->errorMsg, msg );

    // Envoi du paquet
    if( sendPacket( sock, packet, to ) != 0 ) return( 2 );

    return( 0 );
}


Packet* TFTP_recvPacket( Sock* sock, Addr* from )
{
    // Attente du paquet dans un buffer en reception et recuperation du nouveau port
    unsigned char buff[PACKET_MAX_SIZE];
    size_t size = PACKET_MAX_SIZE;

    int response = SOCK_recvData( sock, buff, &size, from );
    if( response > 0) return( NULL );
    else if (response == -1) return TIMEOUT;

    // Position courante dans le buffer lors du decodage
    size_t offset = 0;

    // Verification taille des donnees recues
    if( size < sizeof( uint16_t ) )
    {
        fprintf( stderr, "ERREUR - Taille des donnees recues insuffisante\n" );
        return( NULL );
    }

    // Decodage du code du paquet
    uint16_t netCode = 0;
    memcpy( &netCode, buff, sizeof( uint16_t ) );
    const uint16_t code = ntohs( netCode );
    offset += sizeof( uint16_t );
    size -= offset;

    // Creation du paquet
    Packet* packet = PACKET_create( code );
    if( packet == NULL ) return( NULL );

    // Decodage des donnees du paquet
    if( PACKET_decode( packet, buff + offset, size ) != 0 )
    {
        // Echec du decodage
        PACKET_destroy( packet );
        return( NULL );
    }

    return( packet );
}


int TFTP_sendFileToEndpoint( Sock* sock, const char* fileName, const Addr* endpoint )
{
    // Code de retour
    int status = SEND_FILE_IN_PROGRESS;

    // Open file
    FILE* file = fopen( fileName, "rb" );
    if( file == NULL )
    {
        // Message d'erreur
        fprintf( stderr, "ERREUR - Fichier inexistant: %s\n", fileName );

        // Envoi paquet ERROR
        if( TFTP_sendErrorPacket( sock, ERR_FILE_NOT_FOUND, "Fichier inexistant", endpoint ) != 0 ) return( 1 );
        return 3;
    }

    // Recuperation de la taille du fichier
    int fd = fileno( file );
    struct stat fileInfo;
    fstat( fd, &fileInfo );
    const off_t fileSize = fileInfo.st_size;

    // Nombre de paquets DATA necessaires (y-compris le dernier)
    const uint16_t nbDataPacket = fileSize / DATA_SIZE + 1;

    // Taille du dernier paquet. Si cette taille est nulle, le dernier paquet ne contient pas de donnees,
    // mais doit quand meme etre envoye
    uint16_t lastPacketSize = fileSize % DATA_SIZE;

    // Boucle d'envoi
    for( uint16_t blockNum = 1; blockNum <= nbDataPacket; ++blockNum )
    {
        // Taille des donnees (differente pour le dernier paquet)
        const uint16_t bytesCount = ( blockNum == nbDataPacket ? lastPacketSize : DATA_SIZE );

        // Lecture des donnees
        unsigned char bytes[DATA_SIZE];
        if( fread( bytes, bytesCount, 1, file ) != 1 )
        {
            fprintf( stderr, "ERREUR - Echec de lecture\n");
            status = SEND_FILE_ERROR;
            break;
        }

        Packet *response = TIMEOUT;
        int nb_try = 0;

        // Envoie du paquet DATA
        while (response == TIMEOUT) {
            if( TFTP_sendDataPacket( sock, blockNum, bytes, bytesCount, endpoint ) != 0 )
            {
                status = SEND_FILE_ERROR;
                break;
            }

            // Attente de la reponse (ACK ou ERROR)
            response = TFTP_recvPacket( sock, NULL );
            if (response == TIMEOUT) {
                if (nb_try++ == MAX_TRY_TIMEOUT) {
                    // On abandonne l'envoie de paquet
                    // Envoie d'un paquet erreur
                    if( TFTP_sendErrorPacket( sock, ERR_UNDEFINED, "Timeout", endpoint ) != 0 )return( 1 );
                    response = NULL;
                }
                else printf("Timeout. Nouvel envoi paquet data. (%d)\n", nb_try);
            }
        }

        if( response == NULL )
        {
            status = SEND_FILE_ERROR;
            break;
        }

        // Selon le code de la reponse
        switch( response->code )
        {
            // ACK
            case TFTP_ACK:
            {
                // Controle du numero de bloc
                AckPacket* ack = (AckPacket*)response->data;
                if( ack->blockNum != blockNum )
                {
                    fprintf( stderr,
                             "ERREUR - ACK incohérent (num bloc = %u, attendu = %u\n",
                             ack->blockNum,
                             blockNum );
                    status = SEND_FILE_ERROR;
                }

            }
            break;

            // ERROR
            case TFTP_ERROR:
            {
                // Affichage de l'erreur
                ErrorPacket* err = (ErrorPacket*)response->data;
                fprintf( stderr, "ERREUR - code = %u, msg = %s\n", err->errorCode, err->errorMsg );
                status = SEND_FILE_ERROR;
            }
            break;

            // Code imprevu, on renvoie une erreur
            default:
                if( TFTP_sendErrorPacket( sock, ERR_UNDEFINED, "Code paquet inattendu", endpoint ) != 0 )
                    return( 1 );
                status = SEND_FILE_ERROR;
                break;
        }

        // Liberation memoire
        PACKET_destroy( response );

        // Controle erreur
        if( status != SEND_FILE_IN_PROGRESS ) return( status );
    }

    // Fermeture du fichier
    fclose( file );

    return( SEND_FILE_COMPLETE );
}


int TFTP_recvFileFromEndpoint( Sock* sock, FILE* file, const Addr* endpoint )
{
    // Code de retour
    int status = RECV_FILE_IN_PROGRESS;

    // Numero de bloc attendu
    uint16_t blockNum = 1;

    // Boucle de reception
    Packet* packet = NULL;
    while( 1 )
    {
        // Attente du prochain paquet DATA
        if( packet != NULL ) PACKET_destroy( packet );
        packet = TFTP_recvPacket( sock, NULL );
        if( packet == NULL || packet == TIMEOUT) return( 1 );

        // Si ce n'est pas un paquet DATA
        if( packet->code != TFTP_DATA )
        {
            // Renvoi d'une erreur
            fprintf( stderr, "ERREUR - Réception d'un paquet non prévu (code = %u)\n", packet->code );
            if( TFTP_sendErrorPacket( sock, ERR_UNDEFINED, "Code paquet inattendu", endpoint ) != 0 )
                return( 1 );
            status = RECV_FILE_ERROR;
            break;
        }
        else
        {
            // Controle du numero de bloc
            const DataPacket* data = (const DataPacket*)packet->data;
            if( data->blockNum != blockNum )
            {
                // Renvoi d'une erreur
                fprintf( stderr, "ERREUR - Mauvais numero de bloc (attendu = %u, reçu = %u)\n",
                         blockNum, data->blockNum );
                if( TFTP_sendErrorPacket( sock, ERR_UNDEFINED, "Mauvais numero de bloc", endpoint ) != 0 )
                    return( 1 );
                status = RECV_FILE_ERROR;
                break;
            }

            // Ecriture des donnees dans le fichier
            if( fwrite( data->bytes, data->bytesCount, 1, file ) != 1 )
            {
                fprintf( stderr, "ERREUR - Echec d'écriture\n");
                status = RECV_FILE_ERROR;
                break;
            }

            // Envoi de l'ACK
            if( TFTP_sendAckPacket( sock, blockNum++, endpoint ) != 0 ) return( 1 );
            status = RECV_FILE_ERROR;

            // Si taille des donnees inferieure a 512
            if( data->bytesCount < DATA_SIZE )
            {
                // Reception terminee
                status = RECV_FILE_COMPLETE;
                break;
            }
        }
    }

    // Liberation memoire
    if( packet != NULL ) PACKET_destroy( packet );

    return( status );
}


//--- Fonctions locales ----------------------------------------------------------------------------------------

static int sendPacket( Sock* sock, Packet* packet, const Addr* to )
{
    // Encodage du paquet dans un buffer en emission
    unsigned char buff[PACKET_MAX_SIZE];
    size_t size = 0;
    if( PACKET_encode( packet, buff, &size ) != 0 )
    {
        PACKET_destroy( packet );
        return( 1 );
    }
    // Envoi du paquet a l'adresse specifiee
    if( SOCK_sendData( sock, buff, size, to ) != 0 )
    {
        PACKET_destroy( packet );
        return( 2 );
    }

    // Destruction du paquet
    PACKET_destroy( packet );

    return( 0 );
}
