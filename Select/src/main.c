// System
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Local
#include "tftp/client.h"
#include "tftp/server.h"


// Executions en mode serveur et client
enum { MODE_UNKNOWN = -1, MODE_NONE, MODE_CLT, MODE_SRV };
static void runServer( uint16_t srvPort );
static void runClient( const char* srvHost, uint16_t srvPort );
static int getMode( const char* sMode );

// Utilisation du programme
static const char* USAGE = "tftp --mode CLT|SRV --host HOST --port PORT";


int main( int argc, char* argv[] )
{
    // Mode d'execution : client ou serveur (par defaut)
    int mode = MODE_SRV;

    // Machine du serveur (en mode client) : localhost par defaut
    char srvHost[32];
    strcpy( srvHost, "localhost" );

    // Port utilise par le serveur
    uint16_t srvPort = 0;

    // Parsing de la ligne de commande
    int i = 0;
    while( argv[++i] )
    {
        // Option et valeur associee
        char* option = argv[i];
        char* value = argv[++i];
        if( value == NULL )
        {
            fprintf( stderr, "ERREUR - L'option %s requiert une valeur\n", option );
            fprintf( stderr, "%s\n", USAGE );
            return( 1 );
        }

        // Mode d'execution
        if( strcmp( option, "--mode" ) == 0 )
            mode = getMode( value );

        // Machine du serveur
        else if( strcmp( option, "--host" ) == 0 )
            strcpy( srvHost, value );

        // Port du serveur
        else if( strcmp( option, "--port" ) == 0 )
            srvPort = (uint16_t)atoi( value );

        // Option inconnue
        else
        {
            fprintf( stderr, "ERREUR - Option inconnue : %s\n", option );
            fprintf( stderr, "%s\n", USAGE );
            return( 1 );
        }
    }

    // Selon le mode de lancement
    switch( mode )
    {
        // Mode client
        case MODE_CLT:
            runClient( srvHost, srvPort );
            break;

        // Mode serveur
        case MODE_SRV:
            runServer( srvPort );
            break;

        // Mode inconnu
        default:
            fprintf( stderr, "ERREUR - Mode d'exécution inconnu !\n" );
            fprintf( stderr, "%s\n", USAGE );
            return( 2 );
    }

	return( 0 );
}


static void runServer( uint16_t srvPort )
{
    // Creation d'un serveur
    Server* srv = SERVER_create( srvPort );
    if( srv == NULL )
    {
        fprintf( stderr, "FATAL - Echec d'initialisation du serveur!!!\n" );
        return;
    }

    // Lancement du serveur (traitement des requetes entrantes)
    SERVER_run( srv );

    // Destruction du serveur
    SERVER_destroy( srv );
}


static void runClient( const char *srvHost, uint16_t srvPort )
{
    // Creation d'un client. Si port est nul, on utilise le port 69 (port TFTP standard)
    Client* clt = CLIENT_create( srvHost, srvPort ? srvPort : 69 );
    if( clt == NULL )
    {
        fprintf( stderr, "FATAL - Echec d'initialisation du client!!!\n" );
        return;
    }

    // Lancement de la session utilisateur
    CLIENT_run( clt );

    // Destruction du client
    CLIENT_destroy( clt );
}


static int getMode( const char* sMode )
{
    // Mode client
    if( strcmp( sMode, "CLT" ) == 0 )
        return( MODE_CLT );
    // Mode serveur
    else if( strcmp( sMode, "SRV" ) == 0 )
        return( MODE_SRV );
    // Mode inconnu
    else
        return( MODE_UNKNOWN );
}
