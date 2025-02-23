#ifndef CONNECTION_H
#define CONNECTION_H

/* !!! HIER MUSS NICHTS MEHR GEÄNDERT WERDEN !!! */


#include <stdio.h>  // Standard-Ein-/Ausgabefunktionen
#include <stdlib.h> // Standardbibliothek für Speicherverwaltung und mehr
#include <string.h> // Funktionen für die Zeichenkettenverarbeitung
#include <unistd.h> // Zugriff auf POSIX-API-Funktionen
#include <time.h>   // Funktionen zur Zeitmessung und -manipulation
#include <stdbool.h> // Definition von booleschen Datentypen
#include <arpa/inet.h> // Funktionen zur Umwandlung von Internetadressen
#include <sys/socket.h> // Definition von Socketfunktionen
#include <netinet/in.h> // Definition von Internetadressen und Protokollen
#include <net/if.h> // Definition von Netzwerkinterfaces
#include <fcntl.h> // Funktionen zur Steuerung von Dateideskriptoren


// Standard-Dateipfad für Daten
#define DEFAULT_FILE_PATH "data.txt"

// Standard-Portnummer für den Client
#define DEFAULT_PORT_CLIENT 50000

// Standard-Portnummer für den Server
#define DEFAULT_PORT_SERVER 51000

// Standard-Multicast-Adresse
#define DEFAULT_MULTI_ADRESS "FF12::10"

// Standard-Multicast-Adresse für lokale Tests
#define DEFAULT_MULTI_ADRESS_LOCAL "FF01::10"

// Standard-Zeitschlitzdauer in Millisekunden
#define DEFAULT_SLOT_TIME 300

// Standard-Leerlaufzeit in Sekunden
#define DEFAULT_IDLE_TIME 2

// Maximale Anzahl erlaubter Clients
#define MAX_ALLOWED_CLIENTS 3

// Standard-Fenstergröße für die Datenübertragung
#define DEFAULT_WINDOW_SIZE 1

// Maximale Datengröße pro Paket
#define DEFAULT_DATA_BUFFER_SIZE 256

// ANSI-Farbcodes für die Konsolenausgabe
#define RED "\033[31m"   // Rot für Fehler oder Warnungen
#define GREEN "\033[32m" // Grün für erfolgreiche Operationen
#define BLUE "\033[34m"  // Blau für zeitbezogene Nachrichten
#define RESET "\033[0m"  // Zurücksetzen der Farbformatierung


/**
 * Enum: connection_state
 * -----------------------
 * Beschreibt die möglichen Zustände einer Verbindung.
 */
typedef enum 
{
    STATE_INIT,        // Initialisieren von Variablen
    STATE_IDLE,        // Verbindung ist inaktiv
    STATE_PREPARE,     // Verbindung wird vorbereitet
    STATE_ESTABLISHED, // Verbindung ist hergestellt
    STATE_CLOSE        // Verbindung wird geschlossen
} connection_state;


/**
 * Struktur: properties
 * ---------------------
 * Speichert die Eigenschaften einer Verbindung, sowohl für Client als auch Server.
 */
struct properties 
{
    int id;                  // ID des Clients oder Servers
    bool is_server;          // Gibt an, ob es sich um einen Server handelt

    int sockfd;              // Socket-Deskriptor

    bool local;              // Gibt an, ob lokal gearbeitet wird
    bool loop;               // Gibt an, ob Multicast-Nachrichten zurückgeschickt werden

    int debug_code;          // Debug-Code zur Fehleranalyse

    int windows_size;        // Fenstergröße für die Datenübertragung
    struct sockaddr_in6 my_addr; // Eigene IPv6-Adresse
    int port_server;         // Portnummer Server
    int port_client;         // Protnummer Client
    struct ipv6_mreq mreq;   // Multicast-Einstellungen
    char multi_address[INET6_ADDRSTRLEN]; // Multicast-Adresse
    char network_interface[IFNAMSIZ];     // Netzwerkschnittstelle

    char file_path[512];     // Pfad zur Datei
    FILE* file;              // Dateizeiger
};


/**
 * Struktur: request
 * ------------------
 * Definiert eine Anfrage, die zwischen Client und Server ausgetauscht wird.
 */
struct request 
{
    int senderId;          // ID des Senders
    int reciverId;         // ID des Empfängers
    char type;             // Typ der Anfrage
    #define REQ_HELLO 'H'  // Begrüßungsnachricht
    #define REQ_DATA  'D'  // Datenanforderung
    #define REQ_CLOSE 'C'  // Schließanforderung
    long packageLen;       // Länge des Pakets
    int packageId;         // Paket-ID
    char data[DEFAULT_DATA_BUFFER_SIZE];  // Nutzdaten
};


/**
 * Struktur: answer
 * -----------------
 * Definiert eine Antwort auf eine Anfrage.
 */
struct answer 
{
    int senderId;          // ID des Senders
    int reciverId;         // ID des Empfängers
    char type;             // Typ der Antwort
    #define ANS_HELLO 'H'  // Begrüßungsantwort
    #define ANS_NACK  'N'  // Negative Bestätigung
    #define ANS_CLOSE 'C'  // Schließantwort
    int packageId;         // Paket-ID
};


/**
 * Struktur: communication
 * ------------------------
 * Speichert eine Anfrage und eine Antwort sowie die zugehörige Partneradresse.
 */
struct communication
{
    struct request req;    // Anfrage
    struct answer ans;     // Antwort
    struct sockaddr_in6 partner; // Partneradresse
};


/**
 * Struktur: linked_list_timer
 * ----------------------------
 * Definiert einen Eintrag in einer verketteten Liste, die Timer für Pakete speichert.
 */
struct linked_list_timer
{
    struct linked_list_timer* next; // Zeiger auf den nächsten Timer
    int packageId;                  // Paket-ID
    int ticksToGo;                  // Verbleibende Zeit in Ticks
};


/**
 * Struktur: member
 * -----------------
 * Speichert Informationen über ein Mitglied (Client oder Server).
 */
struct member
{
    int member_id;              // ID des Mitglieds
    struct sockaddr_in6 member; // Adresse des Mitglieds
};


/**
 * Struktur: memberlist
 * ---------------------
 * Speichert eine Liste der Mitglieder der Übertragung.
 */
struct memberlist
{
    int number_members;         // Anzahl der Mitglieder
    struct member member[MAX_ALLOWED_CLIENTS]; // Array von Mitgliedern
};


/* Die Kommentare und Erklärung der Funktionen sind connection.c zu entnehmen! */

void print_timestamp();
int setup_properties(int argc, char** argv, struct properties* props);
int start_socket(struct properties* props);
void close_socket(struct properties* props);
int send_unicast(struct properties *props, struct communication* com);
int send_multicast(struct properties* props, struct communication* com);
int receive_cast(struct properties* props, struct communication* com, struct memberlist* list);
void add_timer_linked_list_timer(struct linked_list_timer** head, int package_id, int tick);
void del_timer_linked_list_timer(struct linked_list_timer** head, int package_id);
int tick_timer_linked_list_timer(struct linked_list_timer** head);
void print_timer_linked_list_timer(struct linked_list_timer** head);

#endif
