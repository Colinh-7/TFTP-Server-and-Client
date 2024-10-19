#include "tftp/packet.h"

// System
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>


//--- Declaration des fonctions locales ------------------------------------------------------------------------

/** Decodage des donnees specifiques de chaque type de paquet
 *
 */
static int decodeXrq( XrqPacket* packet, const unsigned char* buff, size_t buffSize );
static int decodeData( DataPacket* packet, const unsigned char* buff, size_t buffSize );
static int decodeAck( AckPacket* packet, const unsigned char* buff, size_t buffSize );
static int decodeError( ErrorPacket* packet, const unsigned char* buff, size_t buffSize );

/** Encodage des donnees specifiques de chaque type de paquet
 *
 */
static void encodeXrq( XrqPacket* packet, unsigned char* buff, size_t* buffSize );
static void encodeData( DataPacket* packet, unsigned char* buff, size_t* buffSize );
static void encodeAck( AckPacket* packet, unsigned char* buff, size_t* buffSize );
static void encodeError( ErrorPacket* packet, unsigned char* buff, size_t* buffSize );


//--- Fonctions publiques --------------------------------------------------------------------------------------

Packet* PACKET_create( uint16_t code )
{
    // Allocation de la struture de donnees du packet
    Packet* packet = (Packet*)malloc( sizeof( Packet ) );
    packet->code = code;
    packet->data = NULL;

    // Allocation des donnees specifiques du paquet (en fonction du code)
    switch( code )
    {
        // RRQ/WRQ
        case TFTP_RRQ:
        case TFTP_WRQ:
            packet->data = (XrqPacket*)malloc( sizeof( XrqPacket ) );
            memset( ( (XrqPacket*)packet->data )->fileName, '\0', FILENAME_SIZE );
            strcpy( ( (XrqPacket*)packet->data )->mode, "octet" );
            break;

        // DATA
        case TFTP_DATA:
            packet->data = (DataPacket*)malloc( sizeof( DataPacket ) );
            ( (DataPacket*)packet->data )->blockNum = 0;
            memset( ( (DataPacket*)packet->data )->bytes, '\0', DATA_SIZE );
            break;

        // ACK
        case TFTP_ACK:
            packet->data = (AckPacket*)malloc( sizeof( AckPacket ) );
            ( (AckPacket*)packet->data )->blockNum = 0;
            break;

        // ERROR
        case TFTP_ERROR:
            packet->data = (ErrorPacket*)malloc( sizeof( ErrorPacket ) );
            ( (ErrorPacket*)packet->data )->errorCode = 0;
            memset( ( (ErrorPacket*)packet->data )->errorMsg, '\0', ERROR_SIZE );
            break;

        // Code inconnu
        default:
            fprintf( stderr, "Demande de création d'un paquet TFTP avec un code inconnu: %u", code );
            free( packet );
            return( NULL );
    }

    return( packet );
}


int PACKET_decode( Packet* packet, const unsigned char* buff, size_t buffSize )
{
    // Selon le code du paquet
    switch( packet->code )
    {
        // RRQ et WRQ (memes donnees)
        case TFTP_RRQ:
        case TFTP_WRQ:
            return( decodeXrq( (XrqPacket*)packet->data, buff, buffSize ) );

        // DATA
        case TFTP_DATA:
            return( decodeData( (DataPacket*)packet->data, buff, buffSize ) );

        // ACK
        case TFTP_ACK:
            return( decodeAck( (AckPacket*)packet->data, buff, buffSize ) );

        // ERROR
        case TFTP_ERROR:
            return( decodeError( (ErrorPacket*)packet->data, buff, buffSize ) );

        // Code inconnu
        default:
            fprintf( stderr, "Décodage d'un paquet TFTP avec un code inconnu: %u", packet->code );
            return( 1 );
    }

    return( 0 );
}


int PACKET_encode( Packet* packet, unsigned char* buff, size_t* buffSize )
{
    // Debut de l'encodage
    *buffSize = 0;

    // Encodage du code TFTP
    const uint16_t netValue = htons( packet->code );
    memcpy( buff, &netValue, sizeof( uint16_t ) );
    *buffSize += sizeof( uint16_t );

    // Encodage des donnees specifiques du paquets
    switch( packet->code )
    {
        // RRQ et WRQ (memes donnees)
        case TFTP_RRQ:
        case TFTP_WRQ:
            encodeXrq( (XrqPacket*)packet->data, buff, buffSize );
            break;

        // DATA
        case TFTP_DATA:
            encodeData( (DataPacket*)packet->data, buff, buffSize );
            break;

        // ACK
        case TFTP_ACK:
            encodeAck( (AckPacket*)packet->data, buff, buffSize );
            break;

        // ERROR
        case TFTP_ERROR:
            encodeError( (ErrorPacket*)packet->data, buff, buffSize );
            break;

        // Code inconnu
        default:
            fprintf( stderr, "Encodage d'un paquet TFTP avec un code inconnu: %u", packet->code );
            return( 1 );
    }

    return( 0 );
}


void PACKET_destroy( Packet* packet )
{
    // Si packet valide
    if( packet != NULL )
    {
        // Destruction des donnees specifique
        if( packet->data ) free( packet->data );

        // Liberation memoire
        free( packet );
    }
}


//--- Fonctions locales ---------------------------------------------------------------------------------------

static int decodeXrq( XrqPacket* packet, const unsigned char* buff, size_t buffSize )
{
    // Position courante dans le buffer
    size_t offset = 0;

    // Decodage du nom du fichier (jusqu'au caractere '\0')
    size_t index = 0;
    while( offset < buffSize && buff[offset] != '\0' )
    {
        packet->fileName[index++] = buff[offset++];
    }
    packet->fileName[index] = '\0';
    ++offset;

    // Decodage du mode d'encodage (jusqu'au caractere '\0')
    index = 0;
    while( offset < buffSize && buff[offset] != '\0' )
    {
        packet->mode[index++] = buff[offset++];
    }
    packet->mode[index] = '\0';

    return( 0 );
}


static int decodeData( DataPacket* packet, const unsigned char* buff, size_t buffSize )
{
    // Verification taille buffer
    if( buffSize < sizeof( uint16_t ) )
    {
        fprintf( stderr, "Taille buffer insuffisante pour décodage d'un paquet ACK\n" );
        return( 1 );
    }

    // Position courante dans le buffer
    size_t offset = 0;

    // Decodage du numero de bloc
    uint16_t netValue = 0;
    memcpy( &netValue, buff, sizeof( uint16_t ) );
    packet->blockNum = ntohs( netValue );
    offset += sizeof( uint16_t );

    // Copie des octets (taille = reste du buffer moins le numero de bloc)
    packet->bytesCount = buffSize - offset;
    memcpy( packet->bytes, buff + offset, packet->bytesCount );

    return( 0 );
}


static int decodeAck( AckPacket* packet, const unsigned char* buff, size_t buffSize )
{
    // Verification taille buffer
    if( buffSize < sizeof( uint16_t ) )
    {
        fprintf( stderr, "Taille buffer insuffisante pour décodage d'un paquet ACK\n" );
        return( 1 );
    }

    // Decodage du numero de bloc
    uint16_t netValue = 0;
    memcpy( &netValue, buff, sizeof( uint16_t ) );
    packet->blockNum = ntohs( netValue );

    return( 0 );
}

static int decodeError( ErrorPacket* packet, const unsigned char* buff, size_t buffSize )
{
    // Verification taille buffer (min = code erreur + caractere '\0' pour chaine vide)
    if( buffSize < sizeof( uint16_t ) + 1 )
    {
        fprintf( stderr, "Taille buffer insuffisante pour décodage d'un paquet ERROR\n" );
    }

    // Position courante dans le buffer
    size_t offset = 0;

    // Decodage du code d'erreur
    uint16_t netValue = 0;
    memcpy( &netValue, buff, sizeof( uint16_t ) );
    packet->errorCode = ntohs( netValue );
    offset += sizeof( uint16_t );

    // Decodage du message d'erreur (jusqu'au caractere '\0')
    size_t index = 0;
    while( offset < buffSize && buff[offset] != '\0' )
    {
        packet->errorMsg[index++] = buff[offset++];
    }
    packet->errorMsg[index] = '\0';

    return( 0 );
}


static void encodeXrq( XrqPacket* packet, unsigned char* buff, size_t* buffSize )
{
    // Encodage du chemin du fichier
    const size_t filePathLength = strlen( packet->fileName ) + 1;
    memcpy( buff + *buffSize, packet->fileName, filePathLength );
    *buffSize += filePathLength;

    // Encodage du mode
    const size_t modeLength = strlen( packet->mode ) + 1;
    memcpy( buff + *buffSize, packet->mode, modeLength );
    *buffSize += modeLength;
}


static void encodeData( DataPacket* packet, unsigned char* buff, size_t* buffSize )
{
    // Encodage du numero de block
    const uint16_t netValue = htons( packet->blockNum );
    memcpy( buff + *buffSize, &netValue, sizeof( uint16_t ) );
    *buffSize += sizeof( uint16_t );

    // Encodage des donnees
    memcpy( buff + *buffSize, packet->bytes, packet->bytesCount );
    *buffSize += packet->bytesCount;
}


static void encodeAck( AckPacket* packet, unsigned char* buff, size_t* buffSize )
{
    // Encodage du numero de block
    const uint16_t netValue = htons( packet->blockNum );
    memcpy( buff + *buffSize, &netValue, sizeof( uint16_t ) );
    *buffSize += sizeof( uint16_t );
}


static void encodeError( ErrorPacket* packet, unsigned char* buff, size_t* buffSize )
{
    // Encodage du code d'erreur
    const uint16_t netValue = htons( packet->errorCode );
    memcpy( buff + *buffSize, &netValue, sizeof( uint16_t ) );
    *buffSize += sizeof( uint16_t );

    // Encodage du message d'erreur
    const size_t msgLength = strlen( packet->errorMsg ) + 1;
    memcpy( buff + *buffSize, packet->errorMsg, msgLength );
    *buffSize += msgLength;
}
