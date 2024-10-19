#ifndef _TFTP_TFTP_H_
#define _TFTP_TFTP_H_

// System
#include <stdint.h>

// Local
#include "tftp/sock.h"
#include "tftp/packet.h"
#include "tftp/addr.h"

#define TIMEOUT ((Packet*)1)
#define MAX_TRY_TIMEOUT 5

//--------------------------------------------------------------------------------------------------------------
// Module: TFTP
// Description:
//      Envoi et reception des paquets TFTP
//--------------------------------------------------------------------------------------------------------------

/** Envoi d'un paquet WRQ/RRQ
 *
 */
extern int TFTP_sendXrqPacket( Sock* sock, uint16_t code, const char* fileName, const Addr* to );

/** Envoi d'un paquet ACK
 *
 */
extern int TFTP_sendAckPacket( Sock* sock, uint16_t blockNum, const Addr* to );

/** Envoi d'un paquet DATA
 *
 */
extern int TFTP_sendDataPacket(
        Sock* sock, uint16_t blockNum, const unsigned char* bytes, uint16_t bytesCount, const Addr* to );

/**A Envoi d'un paquet ERROR
 *
 */
extern int TFTP_sendErrorPacket( Sock* sock, uint16_t error, const char* msg, const Addr* to );

/** Reception d'un paquet TFTP
 *
 */
extern Packet* TFTP_recvPacket( Sock* sock, Addr* from );

/** Envoi d'un fichier vers l'adresse specifiee
 *
 */
extern int TFTP_sendFileToEndpoint( Sock* sock, const char* fileName, const Addr* endpoint );

/** Reception d'un fichier et stockage dans le stream specifie
 *
 */
extern int TFTP_recvFileFromEndpoint( Sock* sock, FILE* file, const Addr* endpoint );

#endif // _TFTP_TFTP_H_
