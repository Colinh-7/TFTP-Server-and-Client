#ifndef _TFTP_PACKET_H_
#define _TFTP_PACKET_H_

// System
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>


//--------------------------------------------------------------------------------------------------------------
// Module: PACKET
// Description:
//      Gestion des different types de packets TFTP
//--------------------------------------------------------------------------------------------------------------

// Taille max des paquets TFTP
#define PACKET_MAX_SIZE 516

// Longueur des tableaux
#define FILENAME_SIZE 64
#define MODE_SIZE 9
#define DATA_SIZE 512
#define ERROR_SIZE 64

// Types de paquets TFTP disponibles
enum 
{
    TFTP_RRQ = 1,            // Demande de lecture 
    TFTP_WRQ,                // Demande d'ecriture
    TFTP_DATA,               // Donnees
    TFTP_ACK,                // Accuse de reception
    TFTP_ERROR               // Erreur
};

// Codes d'erreurs
enum
{
    ERR_UNDEFINED = 0,
    ERR_FILE_NOT_FOUND,
    ERR_NOT_PERMITTED,
    ERR_NOT_ENOUGH_SPACE_ON_DISK,
    ERR_INVALID_OPTION,
    ERR_UNKNOWN_TRANSFER_ID,
    ERR_FILE_ALREADY_EXISTS,
    ERR_UNKOWN_USER
};

/** Structure de donnees associee a un packet TFTP general
 *
 */
typedef struct
{
    uint16_t code;  // Code du paquet (enum precedent)
    void* data;     // Donnees specifique du paquet (en fonction du code)
} Packet;

/** Donnees pour un paquet RRQ ou WRQ
 *
 */
typedef struct
{
    char fileName[FILENAME_SIZE];           // Nom du fichier
    char mode[MODE_SIZE];                   // Mode d'encodage (toujours "octet")
} XrqPacket;

/** Donnees pour un paquet DATA
 *
 */
typedef struct
{
    uint16_t blockNum;                      // Numero du bloc de donnee
    unsigned char bytes[DATA_SIZE];         // Octets du bloc DATA
    size_t bytesCount;                      // Nombre d'octets du bloc DATA
} DataPacket;

/** Donnees pour un paquet ACK
 *
 */
typedef struct
{
    uint16_t blockNum;                      // Numero du bloc de donnee qui est acquitte
} AckPacket;

/** Donnees pour un paquet ERROR
 *
 */
typedef struct
{
    uint16_t errorCode;                     // Code de l'erreur
    char errorMsg[ERROR_SIZE];              // Message d'erreur
} ErrorPacket;


/** Creation d'un packet (donnees initialisee par defaut)
 *
 *  code: code TFTP du paquet
 */
extern Packet* PACKET_create( uint16_t code );

/** Decodage des donnees du paquet (sans le code) depuis le buffer specifie (issu d'une reception))
 *
 */
extern int PACKET_decode( Packet* packet, const unsigned char* buff, size_t buffSize );

/** Encodage d'un paquet (avec le code) dans le buffer specifie (pour un envoi a venir)
 *
 */
extern int PACKET_encode( Packet* packet, unsigned char* buff, size_t* buffSize );

/** Destruction d'un packet
 *
 */
extern void PACKET_destroy( Packet* packet );

#endif // _TFTP_PACKET_H_
