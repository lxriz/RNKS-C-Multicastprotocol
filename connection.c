#include "connection.h"

/* !!! HIER MUSS NICHTS MEHR GEÄNDERT WERDEN !!! */



/* 
** print_timestamp
** ----------------
** Gibt die aktuelle Uhrzeit im Format HH:MM:SS.mmm aus, wobei:
**  - HH: Stunden im 24-Stunden-Format
**  - MM: Minuten
**  - SS: Sekunden
**  - mmm: Millisekunden
**
** Parameter:
**  - Keine
**
** Rückgabewert:
**  - Keiner (void)
**
** Beschreibung:
** Diese Funktion verwendet die Systemfunktionen `gettimeofday` und `localtime`, um die
** aktuelle Zeit bis auf Millisekunden genau zu ermitteln und auf die Konsole auszugeben.
**
** Besonderheiten:
** - `gettimeofday` liefert die aktuelle Zeit in Sekunden und Mikrosekunden.
** - `localtime` wandelt die Sekunden in eine lokale Zeitstruktur um.
*/
void print_timestamp() 
{
    struct timeval tv;         // Struktur, um Zeit mit Sekunden und Mikrosekunden zu speichern
    struct tm *timeinfo;       // Struktur, um die lokale Zeit zu speichern

    // Aktuelle Zeit mit Mikrosekunden abrufen
    gettimeofday(&tv, NULL);

    // Sekundenwert in lokale Zeit umwandeln
    timeinfo = localtime(&tv.tv_sec);

    // Uhrzeit mit Millisekunden formatieren und ausgeben
    printf("%02d:%02d:%02d.%03d\t", 
           timeinfo->tm_hour,    // Stunden (0-23)
           timeinfo->tm_min,     // Minuten (0-59)
           timeinfo->tm_sec,     // Sekunden (0-59)
           tv.tv_usec / 1000);   // Mikrosekunden in Millisekunden umwandeln
}


/* 
** setup_properties
** -----------------
** Initialisiert die Eigenschaften einer `properties`-Struktur mit Standardwerten 
** und verarbeitet die Kommandozeilenargumente, um spezifische Werte zu setzen.
**
** Parameter:
**  - int argc: Anzahl der Kommandozeilenargumente
**  - char** argv: Array der Kommandozeilenargumente
**  - struct properties* props: Zeiger auf die zu initialisierende properties-Struktur
**
** Rückgabewerte:
**  - 0 bei Erfolg
**  - -1 bei Fehler (z. B. bei unbekannten Argumenten)
**
** Beschreibung:
** Diese Funktion setzt initiale Standardwerte für die übergebene Struktur `props` und
** überschreibt diese Werte basierend auf den vom Benutzer übergebenen Optionen. 
** Sie unterstützt Argumente wie Ports, Dateipfade, Multicast-Adressen, Fenstergrößen usw.
** Unbekannte Optionen führen zu einem Fehler und die Funktion gibt -1 zurück.
*/
int setup_properties(int argc, char** argv, struct properties* props)
{
    // Initialisieren von Standardwerten für die properties-Struktur
    props->sockfd = -1;                 // Dateideskriptor initialisieren, -1 bedeutet "nicht gesetzt"
    props->local = 0;                   // Standardwert für "local" ist false (0)
    props->id = time(NULL) % 2147483647; // Generiert eine eindeutige ID basierend auf der aktuellen Zeit (Modulo, um Überlauf zu vermeiden)

    props->debug_code = 0;              // Standard-Debug-Code ist 0
    props->network_interface[0] = '\0'; // Netzwerkschnittstelle nicht gesetzt initialisieren

    if(props->is_server) // Konfiguration, wenn die Anwendung als Server läuft
    {
        props->port_server = DEFAULT_PORT_SERVER;         // Standardport für den Server
        props->windows_size = DEFAULT_WINDOW_SIZE;        // Standardfenstergröße
        props->port_client = DEFAULT_PORT_CLIENT;         // Standardport für den Client
    }
    else // Konfiguration, wenn die Anwendung als Client läuft
    {
        props->port_client = DEFAULT_PORT_CLIENT;         // Standardport für den Client
    }
    
    // Standardwerte für Multicast-Adresse und Dateipfad setzen
    strncpy(props->multi_address, DEFAULT_MULTI_ADRESS, INET6_ADDRSTRLEN); // Standard-Multicast-Adresse
    strncpy(props->file_path, DEFAULT_FILE_PATH, 254); // Standard-Dateipfad

    // Verarbeitung der Kommandozeilenargumente
    for (int shift = 1; shift < argc; shift++)
    {
        // Verarbeiten des Arguments --portserver und Zuweisung eines Server-Ports
        if (strcmp(argv[shift], "--portserver") == 0 && shift + 1 < argc)
        {
            shift += 1;
            props->port_server = atoi(argv[shift]); // Konvertiert den String in eine Ganzzahl
            continue;
        }
        // Verarbeiten des Arguments --portclient und Zuweisung eines Client-Ports
        else if(strcmp(argv[shift], "--portclient") == 0 && shift + 1 < argc)
        {
            shift += 1;
            props->port_client = atoi(argv[shift]);
            continue;
        }
        // Verarbeiten des Arguments --filepath und Festlegen des Dateipfads
        else if (strcmp(argv[shift], "--filepath") == 0 && shift + 1 < argc)
        {
            shift += 1;
            strncpy(props->file_path, argv[shift], 254); // Pfad kopieren
            continue;
        }
        // Verarbeiten des Arguments --multicastaddress und Zuweisung einer Multicast-Adresse
        else if (strcmp(argv[shift], "--multicastaddress") == 0 && shift + 1 < argc)
        {
            shift += 1;
            strncpy(props->multi_address, argv[shift], INET6_ADDRSTRLEN); // Adresse kopieren
            continue;
        }
        // Verarbeiten des Arguments --windowsize (nur gültig für den Server)
        else if (strcmp(argv[shift], "--windowsize") == 0 && shift + 1 < argc && props->is_server)
        {
            shift += 1;
            props->windows_size = atoi(argv[shift]); // Fenstergröße setzen

            if(props->windows_size < 1 || props->windows_size>10)
            {
                printf(RED "Fenstergröße muss zwischen 1 und 10 sein.\n" RESET);
                return -1;
            }

            continue;
        }
        // Verarbeiten des Arguments --local und Aktivieren der lokalen Port-Wiederverwendung
        else if (strcmp(argv[shift], "--local") == 0)
        {
            props->local = true; // Lokale Wiederverwendung aktivieren
            strncpy(props->multi_address, DEFAULT_MULTI_ADRESS_LOCAL, INET6_ADDRSTRLEN); // Standard-Multicast-Adresse-Local
            continue;
        }
        // Verarbeiten des Arguments --loop und Aktivieren der Loopback-Option
        else if(strcmp(argv[shift], "--loop") == 0)
        {
            props->loop = true; // Loopback aktivieren
            continue;
        }
        // Verarbeiten des Arguments --id und Setzen einer benutzerdefinierten ID
        else if(strcmp(argv[shift], "--id") == 0 && shift + 1 < argc)
        {
            shift += 1;
            props->id = atoi(argv[shift]);
            continue;
        }
        // Verarbeiten des Arguments --debug und Setzen eines Debug-Codes
        else if(strcmp(argv[shift], "--debug") == 0 && shift + 1 < argc)
        {
            shift += 1;
            props->debug_code = atoi(argv[shift]);

            if(props->debug_code < -1 || props->debug_code>100)
            {
                printf(RED "Debug muss zwischen -1 und 100 sein!\n" RESET);
                return -1;
            }

            continue;
        }
        else if(strcmp(argv[shift], "--interface") == 0 && shift + 1 < argc)
        {
            shift += 1;
            strcpy(props->network_interface, argv[shift]);
            continue;
        }
        if(strcmp(argv[shift], "--help") == 0)
        {
            printf(
            "Verwendung: ./programmname [OPTIONEN]\n"
            "\nOptionen:\n"
            "  --portserver <Portnummer>\n"
            "    Legt den Server-Port auf die angegebene Portnummer fest.\n"
            "    Standard: %d\n\n"
            "  --portclient <Portnummer>\n"
            "    Legt den Client-Port auf die angegebene Portnummer fest.\n"
            "    Standard: %d\n\n"
            "  --filepath <Pfad>\n"
            "    Gibt den Pfad zur Datei an, die verwendet werden soll.\n"
            "    Standard: %s\n\n"
            "  --multicastaddress <Adresse>\n"
            "    Legt die Multicast-Adresse für die Kommunikation fest.\n"
            "    Überschreibt den Standardwert, auch bei Verwendung von --local.\n"
            "    Standard: %s\n\n"
            "  --windowsize <Größe>\n"
            "    Legt die Fenstergröße (1-10) für den Server fest.\n"
            "    Gilt nur, wenn die Anwendung als Server läuft.\n"
            "    Standard: %d\n\n"
            "  --local\n"
            "    Aktiviert die Wiederverwendung lokaler Ports (lokale Bindung).\n"
            "    Wenn gesetzt, wird die lokale Multicast-Adresse (%s) und das Loopback-Interface verwendet.\n"
            "    Standard: deaktiviert.\n\n"
            "  --loop\n"
            "    Lässt Server nach dem CLOSE State wieder in den IDLE State wechseln.\n"
            "    Standard: deaktiviert.\n\n"
            "  --id <ID>\n"
            "    Setzt eine benutzerdefinierte ID (Ganzzahl) für die Anwendung.\n"
            "    Standard: Automatisch generierte ID basierend auf der aktuellen Zeit.\n\n"
            "  --debug <Wahrscheinlichkeit>\n"
            "    Aktiviert Debugging und gibt eine Wahrscheinlichkeit (in Prozent) an,\n"
            "    dass beim Server ein Paketverlust simuliert wird.\n"
            "    Bei -1 wird das CLOSE Paket verloren.\n"
            "    Standard: 0 (Debugging deaktiviert).\n\n"
            "  --interface <Schnittstellenname>\n"
            "    Legt die Netzwerkschnittstelle für Multicast-Kommunikation fest.\n"
            "    Überschreibt die Standardwerte von --local.\n"
            "    Standard: nicht gesetzt.\n\n"
            "  --help\n"
            "    Zeigt diese Hilfe an.\n\n"
            "Beispiel-Aufruf:\n"
            "  ./programmname --portserver 8080 --filepath /path/to/file --local --debug 10\n\n"
            "Erläuterung:\n"
            "  - Setzt den Server-Port auf 8080.\n"
            "  - Verwendet /path/to/file als Dateipfad.\n"
            "  - Aktiviert die lokale Port-Wiederverwendung und setzt Loopback-Interface.\n"
            "  - Simuliert eine Wahrscheinlichkeit von 10%% für Paketverlust beim Server.\n\n",
            DEFAULT_PORT_SERVER, DEFAULT_PORT_CLIENT, DEFAULT_FILE_PATH, DEFAULT_MULTI_ADRESS, DEFAULT_WINDOW_SIZE, DEFAULT_MULTI_ADRESS_LOCAL);

            return -1;
        }


        // Fehlerbehandlung für unbekannte Optionen
        print_timestamp();
        printf(RED "Unbekannte Option benutze --help für Hilfe\n" RESET);
        return -1; // Rückgabewert -1 signalisiert einen Fehler
    }

    return 0; // Rückgabewert 0 signalisiert Erfolg
}


/**
 * Funktion: start_socket
 * -----------------------
 * Diese Funktion erstellt und konfiguriert einen IPv6-Datagramm-Socket 
 * (UDP), der an die angegebene Adresse und den Port gebunden wird. 
 * Zusätzliche Einstellungen wie Multicast und Puffergrößen werden ebenfalls 
 * vorgenommen.
 *
 * Parameter:
 * - props: Ein Pointer auf eine Struktur `properties`, die die Eigenschaften 
 *          für den Socket speichert, wie z. B. Port, Multicast-Adresse und Optionen.
 *
 * Rückgabewert:
 * - 0 bei Erfolg.
 * - -1 bei Fehlern.
 *
 * Beschreibung:
 * Die Funktion initialisiert und konfiguriert einen UDP-Socket. Sie erlaubt:
 * - Aktivieren von SO_REUSEADDR und optional SO_REUSEPORT.
 * - Setzen des Sockets in den nicht-blockierenden Modus.
 * - Binden des Sockets an eine Adresse und einen Port.
 * - Hinzufügen des Sockets zu einem Multicastkanal, falls erforderlich.
 * - Festlegen eines Empfangspuffers für den Socket.
 */
int start_socket(struct properties* props) 
{
    // Erstellen des Sockets (IPv6, UDP)
    props->sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if(props->sockfd < 0) 
    {
        print_timestamp();
        printf(RED "Socket konnte nicht erstellt werden\n" RESET);
        return -1;
    }

    print_timestamp();
    printf(GREEN "Socket erstellt\n" RESET);

    // Aktivieren der Reuse-Port-Option, falls erforderlich
    if(props->local && !props->is_server) 
    {
        // Aktivieren der Reuse-Address-Option
        int optval = 1;
        if(setsockopt(props->sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) 
        {
            print_timestamp();
            printf(RED "Reuse-Address-Option konnte nicht gesetzt werden\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        print_timestamp();
        printf(GREEN "Reuse-Address-Option aktiviert\n" RESET);


        if (setsockopt(props->sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) 
        {
            print_timestamp();
            printf(RED "Reuse-Port-Option konnte nicht gesetzt werden\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        print_timestamp();
        printf(GREEN "Reuse-Port-Option aktiviert\n" RESET);
    }

    // Setzen des Sockets auf nicht-blockierenden Modus
    int flags = fcntl(props->sockfd, F_GETFL, 0);
    if(flags < 0) 
    {
        print_timestamp();
        printf(RED "Konnte Socket-Flags nicht abrufen\n" RESET);
        perror("\t\t");
        close(props->sockfd);
        return -1;
    }

    if(fcntl(props->sockfd, F_SETFL, flags | O_NONBLOCK) < 0) 
    {
        print_timestamp();
        printf(RED "Konnte Socket nicht auf nicht-blockierenden Modus setzen\n" RESET);
        perror("\t\t");
        close(props->sockfd);
        return -1;
    }
    print_timestamp();
    printf(GREEN "Socket ist jetzt nicht-blockierend\n" RESET);

    // Schnittstellenindex setzen, falls angegeben oder Loopback verwenden, wenn local gesetzt ist
    if(props->network_interface[0] != '\0')
    {
        unsigned int ifindex = if_nametoindex(props->network_interface);
        if(ifindex == 0) 
        {
            print_timestamp();
            printf(RED "Konnte Schnittstellenindex nicht abrufen\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        if(setsockopt(props->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) 
        {
            print_timestamp();
            printf(RED "Konnte Schnittstelle nicht setzen\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        props->my_addr.sin6_scope_id = ifindex;
        props->mreq.ipv6mr_interface = ifindex;

        print_timestamp();
        printf(GREEN "Schnittstelle auf %s gesetzt\n" RESET, props->network_interface);
    }
    else if(props->local)
    {
        unsigned int ifindex = if_nametoindex("lo0");
        if(ifindex == 0) 
        {
            ifindex = if_nametoindex("lo");
        }
        
        if(ifindex == 0) 
        {
            print_timestamp();
            printf(RED "Konnte Schnittstellenindex nicht abrufen\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        if(setsockopt(props->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) 
        {
            print_timestamp();
            printf(RED "Konnte Schnittstelle nicht setzen\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        props->my_addr.sin6_scope_id = ifindex;
        props->mreq.ipv6mr_interface = ifindex;

        print_timestamp();
        printf(GREEN "Schnittstelle auf Loopback gesetzt\n" RESET);
    }
    else
    {
        unsigned int ifindex = 0;
        int i = 0;
        const char* default_interfaces[] = {"eno0", "eno1", "eno2", "eth0", "eth1", "eth2", "en0", "en1", "en2", "wlan0", "wlan1", "wlan2", "bond0", "bond1", "bond3"};
        
        while(ifindex == 0)
        {
            ifindex = if_nametoindex(default_interfaces[i]);
            
            i+=1;
            
            if(i>=15)
            {
                printf(RED "Es konnte kein DEFAULT INTERFACE gefunden werden\n" RESET);
                return -1;
            }
        }
        i-=1;

        if(setsockopt(props->sockfd, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex)) < 0) 
        {
            print_timestamp();
            printf(RED "Konnte Schnittstelle nicht setzen\n" RESET);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }

        props->my_addr.sin6_scope_id = ifindex;
        props->mreq.ipv6mr_interface = ifindex;

        print_timestamp();
        printf(GREEN "Schnittstelle auf %s gesetzt\n" RESET, default_interfaces[i]);
    }

    // Konfiguration der Adresse für den Server oder Client
    memset(&props->my_addr, 0, sizeof(props->my_addr));
    props->my_addr.sin6_family = AF_INET6;         // IPv6
    props->my_addr.sin6_addr = in6addr_any;        // Akzeptiere Verbindungen von jeder Adresse
    
    // Konvertiere Port zu Netzwerk-Byte-Reihenfolge
    if(props->is_server)
    {
        props->my_addr.sin6_port = htons(props->port_server);
    }
    else
    {
        props->my_addr.sin6_port = htons(props->port_client); 
    }

    // Binden des Sockets an die Adresse und den Port
    if(bind(props->sockfd, (struct sockaddr *)&props->my_addr, sizeof(props->my_addr)) < 0) 
    {
        print_timestamp();
        printf(RED "Socket konnte nicht auf Port %d gebunden werden\n" RESET, htons(props->my_addr.sin6_port));
        perror("\t\t");
        close(props->sockfd);
        return -1;
    }
    print_timestamp();
    printf(GREEN "Socket auf Port %d gebunden\n" RESET, htons(props->my_addr.sin6_port));

    // Multicast-Registrierung für Clients
    if(!props->is_server) 
    {
        inet_pton(AF_INET6, props->multi_address, &props->mreq.ipv6mr_multiaddr); // Multicast-Adresse konvertieren

        if (setsockopt(props->sockfd, IPPROTO_IPV6, IPV6_JOIN_GROUP, &props->mreq, sizeof(props->mreq)) < 0) 
        {
            print_timestamp();
            printf(RED "Konnte nicht auf Multicastkanal %s registriert werden\n" RESET, props->multi_address);
            perror("\t\t");
            close(props->sockfd);
            return -1;
        }
        print_timestamp();
        printf(GREEN "Auf Multicastkanal %s registriert\n" RESET, props->multi_address);
    }


    // Empfangspuffergröße festlegen
    int buffer_size = sizeof(struct communication) * MAX_ALLOWED_CLIENTS; // Beispielgröße
    if(setsockopt(props->sockfd, SOL_SOCKET, SO_RCVBUF, &buffer_size, sizeof(buffer_size)) < 0) 
    {
        print_timestamp();
        printf(RED "Empfangspuffer konnte nicht auf %d Bytes gesetzt werden\n" RESET, buffer_size);
        perror("\t\t");
        close(props->sockfd);
        return -1;
    }

    print_timestamp();
    printf(GREEN "Empfangspuffer auf %d Bytes gesetzt\n" RESET, buffer_size);

    return 0;
}


/**
 * Funktion: close_socket
 * ----------------------
 * Diese Funktion schließt den Socket, der in der `properties`-Struktur 
 * gespeichert ist. Sie stellt sicher, dass der Socket freigegeben wird, 
 * um Ressourcenlecks zu vermeiden.
 *
 * Parameter:
 * - props: Ein Pointer auf eine Struktur `properties`, die den zu 
 *          schließenden Socket (`sockfd`) enthält.
 */
void close_socket(struct properties* props)
{
    close(props->sockfd); // Schließt den Socket und gibt Ressourcen frei
}



/**
 * Funktion: send_unicast
 * ----------------------
 * Diese Funktion sendet eine Unicast-Nachricht an einen spezifischen Partner
 * unter Verwendung eines definierten Sockets. Die Nachricht wird direkt an die
 * in der `communication`-Struktur angegebene Zieladresse gesendet.
 *
 * Parameter:
 * - props: Ein Pointer auf eine Struktur `properties`, die den Socket 
 *          enthält, über den die Nachricht gesendet wird.
 * - com: Ein Pointer auf eine Struktur `communication`, die die Nachricht 
 *        (`ans`) und die Zieladresseninformationen (`partner`) enthält.
 *
 * Rückgabewert:
 * - 0 bei erfolgreichem Senden der Unicast-Nachricht.
 * - -1 bei Fehlern, z. B. wenn das Senden fehlschlägt.
 */
int send_unicast(struct properties *props, struct communication* com) 
{    
    // Nachricht über den Socket an den angegebenen Partner senden
    if(props->is_server)
    {
        if(sendto(props->sockfd, &com->req, sizeof(com->req), 0, (struct sockaddr *)&com->partner, sizeof(com->partner)) < 0) 
        {   
            print_timestamp();
            printf(RED "Unicast konnte nicht gesendet werden" RESET); // Fehler beim Senden
            print_timestamp();        
            perror(RED "\t\t" RESET);
    
            return -1;
        }
    }
    else
    {
        //MSG_NOSIGNAL WICHTIG!
        // Bei MAC Pro Socket PIPE error idk why?
        if(sendto(props->sockfd, &com->ans, sizeof(com->ans), MSG_NOSIGNAL, (struct sockaddr *)&com->partner, sizeof(com->partner)) < 0) 
        {
            print_timestamp();
            printf(RED "Unicast konnte nicht gesendet werden" RESET); // Fehler beim Senden
            print_timestamp();
            perror("\t\t");
            
            return -1;
        }
    }

    // Erfolgreiches Senden der Nachricht
    print_timestamp();
    printf(GREEN "Unicast gesendet\n" RESET);

    return 0;
}


/**
 * Funktion: send_multicast
 * ------------------------
 * Diese Funktion sendet eine Multicast-Nachricht an eine bestimmte Adresse 
 * und Port unter Verwendung von IPv6. Sie überprüft die Gültigkeit der 
 * Multicast-Adresse und sendet die Nachricht über den angegebenen Socket.
 *
 * Parameter:
 * - props: Ein Pointer auf eine Struktur `properties`, die die notwendigen 
 *          Eigenschaften wie den Socket und die Multicast-Adresse enthält.
 * - com: Ein Pointer auf eine Struktur `communication`, die die Nachricht 
 *        und die Zielportinformationen enthält.
 *
 * Rückgabewert:
 * - 0 bei erfolgreichem Senden der Multicast-Nachricht.
 * - -1 bei Fehlern, z. B. bei ungültiger Multicast-Adresse oder Problemen 
 *   beim Senden.
 */
int send_multicast(struct properties* props, struct communication* com) 
{       
    // Zieladresse initialisieren und auf Null setzen
    struct sockaddr_in6 dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    
    // Zieladresse als IPv6 und Zielport konfigurieren
    dest_addr.sin6_family = AF_INET6; // Adresse ist IPv6
    dest_addr.sin6_port = htons(props->port_client); // Port konvertieren zu Netzwerk-Byte-Reihenfolge

    // Multicast-Adresse in binäre Form konvertieren und überprüfen
    if (inet_pton(AF_INET6, props->multi_address, &dest_addr.sin6_addr) <= 0) 
    {
        print_timestamp();
        printf(RED "Ungültige Multicast-Adresse\n" RESET);
    
        return -1; // Fehler bei ungültiger Adresse
    }

    // Nachricht über den Socket senden
    if(sendto(props->sockfd, &com->req, sizeof(com->req), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) 
    {
        print_timestamp();
        printf(RED "Multicast konnte nicht gesendet werden\n" RESET);
        perror("\t\t");
        
        return -1; // Fehler beim Senden
    }

    // Erfolgreiches Senden der Nachricht
    print_timestamp();
    printf(GREEN "Multicast gesendet\n" RESET);

    return 0;
}


/**
 * Funktion: receive_cast
 * ----------------------
 * Wartet auf ein UDP-Paket und verarbeitet die empfangenen Daten. 
 * Die Funktion unterstützt Timeout-Mechanismen, überprüft den Absender und Empfänger, 
 * und ignoriert ungültige oder nicht relevante Nachrichten.
 *
 * Parameter:
 * - props: Pointer auf eine Struktur `properties`, die Sockets und IDs speichert.
 * - com: Pointer auf eine Struktur `communication`, die die Kommunikationsdaten speichert.
 * - list: Pointer auf eine Struktur `memberlist`, die bekannte Mitglieder speichert (nur relevant für Server).
 *
 * Rückgabewert:
 * - 1: Paket erfolgreich empfangen und verarbeitet.
 * - 0: Timeout abgelaufen, ohne dass ein Paket empfangen wurde.
 * - -1: Fehler bei `select` oder `recvfrom`.
 *
 * Beschreibung:
 * - Wenn memberlist NULL ist wird keine Prüfung vorgenommen.
 * - Die Funktion verwendet `select`, um innerhalb eines definierten Zeitfensters (DEFAULT_SLOT_TIME) 
 *   auf eingehende UDP-Pakete zu warten.
 * - Prüft den Absender (Sender-ID) und Empfänger (Receiver-ID) auf Gültigkeit.
 * - Ignoriert eigene Nachrichten oder Pakete, die für andere Empfänger bestimmt sind.
 * - Kopiert die empfangenen Daten in die `communication`-Struktur für die weitere Verarbeitung.
 */
int receive_cast(struct properties* props, struct communication* com, struct memberlist* list) 
{
    char buffer[sizeof(struct request)]; // Puffer für die empfangenen Daten

    struct timeval timeout;          // Timeout-Einstellung für `select`
    fd_set read_fds;                 // Datei-Deskriptor-Set für `select`

    // Timeout in Sekunden und Mikrosekunden berechnen
    timeout.tv_sec = DEFAULT_SLOT_TIME / 1000;
    timeout.tv_usec = (DEFAULT_SLOT_TIME % 1000) * 1000;

    // Startzeit erfassen
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long timer_start = (long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000);

    while(1) 
    {
        FD_ZERO(&read_fds);                 // Datei-Deskriptor-Set zurücksetzen
        FD_SET(props->sockfd, &read_fds);   // Socket hinzufügen

        print_timestamp();
        printf("Warte auf Paket\n");        // Nachricht vor `select`
        print_timestamp();
        printf(BLUE "Warte... %dms\n" RESET, DEFAULT_SLOT_TIME);
        
        // Warten auf eingehende Daten
        int result = select(props->sockfd + 1, &read_fds, NULL, NULL, &timeout);

        // Verbleibende Zeit berechnen
        gettimeofday(&tv, NULL);
        long long timer_left = DEFAULT_SLOT_TIME - (((long long)(tv.tv_sec) * 1000 + (tv.tv_usec / 1000)) - timer_start);

        if(result <= 0) 
        {
            if(result == 0) 
            {
                print_timestamp();
                printf(RED "Kein Paket empfangen\n" RESET); // Timeout ohne Empfang
                return 0; // Keine Nachricht empfangen
            } 
            else 
            {
                print_timestamp();
                printf(RED "Fehler bei select()\n" RESET); // Fehler bei `select`
                perror("\t\t");
                return -1; // Fehler
            }
        }

        // Empfangene Daten lesen
        socklen_t partner_len = sizeof(com->partner); // Größe der Partneradresse
        ssize_t bytes_received = recvfrom(props->sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&com->partner, &partner_len);
        if(bytes_received < 0) 
        {
            print_timestamp();
            printf(RED "Fehler bei recvfrom\n" RESET); // Fehler beim Empfang
            perror("\t\t");
            return -1;
        }

        // ID des Senders extrahieren
        int sender_id = *(int *)(buffer);
        if(sender_id == props->id) 
        {
            print_timestamp();
            printf("Nachricht von eigener ID ignoriert\n"); // Eigene Nachricht ignorieren
            continue;
        }

        // Für Server: Prüfen, ob Sender bekannt ist
        if(props->is_server && list != NULL)
        {   
            bool knows_sender = false;

            for(int i = 0; i < list->number_members; i++)
            {
                if(list->member[i].member_id == sender_id &&
                   IN6_ARE_ADDR_EQUAL(&list->member[i].member.sin6_addr, &com->partner.sin6_addr))
                {
                    knows_sender = true;
                    break;
                }
            }

            if(!knows_sender)
            {
                print_timestamp();
                printf("Paket von unbekanntem Sender ignoriert\n"); 
                continue;
            }
        }

        // ID des Empfängers extrahieren
        int receiver_id = *(int *)(buffer + sizeof(int));
        if(receiver_id != props->id && receiver_id != -1) 
        {
            print_timestamp();
            printf("Paket für anderen Empfänger ignoriert\n");
            continue;
        }

        print_timestamp();
        printf(GREEN "Paket empfangen\n" RESET);
        print_timestamp();
        printf("Sender ID: %d\n", sender_id);

        // Nachricht verarbeiten
        if(props->is_server) 
        {
            memcpy(&com->ans, buffer, sizeof(struct answer)); // Für Server
        } 
        else 
        {
            memcpy(&com->req, buffer, sizeof(struct request)); // Für Client
        }

        print_timestamp();
        printf(BLUE "Warte... %lldms\n" RESET, timer_left);

        if(timer_left > 0)
        {
            usleep(timer_left * 1000); // Verzögerung einfügen
        }
        
        return 1; // Erfolgreich empfangen
    }
}



/**
 * Funktion: add_timer_linked_list_timer
 * --------------------------------
 * Fügt ein neues Element (`new_link`) zur bestehenden verketteten Liste hinzu. Die
 * Funktion passt dabei die `ticksToGo`-Werte an, sodass jedes Element die relative
 * Zeit bis zum nächsten Element repräsentiert.
 *
 * Parameter:
 * - head: Ein Pointer auf das erste Element der verketteten Liste.
 * - new_link: Ein Pointer auf das neue Element, das hinzugefügt werden soll.
 *
 * Rückgabewert:
 * - Keiner (void).
 */
void add_timer_linked_list_timer(struct linked_list_timer** head, int package_id, int ticks)
{
    struct linked_list_timer* new_timer = (struct linked_list_timer*)malloc(sizeof(struct linked_list_timer));
    new_timer->packageId = package_id;
    new_timer->ticksToGo = ticks;
    
    // DIESE SCHEISS ZEILE HAT MICH 5 STUNDEN MEINES LEBENS GEKOSTET !!! 
    // Auf einem Mac erkennt er das der Wert keine gültige Speicheraddresse ist und bricht ab, ein ThinkPad erkennt das nicht
    // Die Sau versucht auf unbekannten Speicher zuzugreifen was zu einem Absturz bei hinzufügen oder lesen eines Paketes. 
    new_timer->next = NULL;

    
    if(*head == NULL)
    {
        *head = new_timer;
        return;
    }

    // Solange wir nicht am Ende der Liste sind, reduzieren wir die `ticksToGo` des neuen Elements
    // um die `ticksToGo` des aktuellen Elements.
    while((*head)->next != NULL)
    {
        new_timer->ticksToGo -= (*head)->ticksToGo;
        head = &((*head)->next); // Weiter zum nächsten Element in der Liste.
    }

    new_timer->ticksToGo -= (*head)->ticksToGo;


    // Wenn wir das Ende der Liste erreicht haben, verknüpfen wir das neue Element.
    (*head)->next = new_timer;
}


/**
 * Funktion: del_timer_linked_list_timer
 * --------------------------------
 * Entfernt das erste Element aus der verketteten Liste. Wenn die Liste nur ein Element enthält,
 * wird der Kopf der Liste auf NULL gesetzt.
 *
 * Parameter:
 * - head: Ein Pointer auf das erste Element der verketteten Liste.
 *
 * Rückgabewert:
 * - Keiner (void).
 */
void del_timer_linked_list_timer(struct linked_list_timer** head, int package_id) 
{
    if(*head == NULL) 
    {
        // Liste ist leer
        return;
    }

    // Prüfen, ob der Kopfknoten gelöscht werden soll
    struct linked_list_timer* current = *head;
    if(current->packageId == package_id) 
    {
        if(current->next != NULL)
        {
            current->next->ticksToGo += current->ticksToGo; // Nächster Knoten erhält die verbleibenden Ticks
        }
        
        *head = current->next; // Kopf auf das nächste Element setzen
        free(current);
        return;
    }

    // Iterieren und nach dem zu löschenden Element suchen
    struct linked_list_timer* previous = NULL;

    while(current != NULL)
    {
        if(current->packageId == package_id) 
        {
            // Knoten gefunden, entfernen
            if(current->next != NULL) // Wenn es der letzte Knoten ist
            {
                current->next->ticksToGo += current->ticksToGo; // Nächster Knoten erhält die verbleibenden Ticks
            }
            previous->next = current->next;
            free(current);
            return;
        }

        // Pointer aktualisieren
        previous = current;
        current = current->next;
    }
}




/**
 * Funktion: tick_timer_linked_list_timer
 * ---------------------------------
 * Reduziert den `ticksToGo`-Wert des ersten Elements in der verketteten Liste um 1.
 * Wenn `ticksToGo` 0 erreicht, wird das erste Element entfernt und dessen `packageId`
 * zurückgegeben.
 *
 * Parameter:
 * - head: Ein Pointer auf das erste Element der verketteten Liste.
 *
 * Rückgabewert:
 * - -1: Die Liste ist leer.
 * - 0: Die Liste wurde aktualisiert, aber kein Timeout ist aufgetreten.
 * - timeout_package_id: Die `packageId` des Elements, das entfernt wurde.
 */
int tick_timer_linked_list_timer(struct linked_list_timer** head)
{
    // Wenn die Liste leer ist, geben wir -1 zurück.
    if(*head == NULL)
    {
        return -1;
    }

    // Reduziere den `ticksToGo`-Wert des ersten Elements um 1.
    (*head)->ticksToGo -= 1;

    // Wenn der `ticksToGo`-Wert des ersten Elements 0 erreicht:
    if((*head)->ticksToGo <=0)
    {
        // Speichere die `packageId` des Elements, das entfernt wird.
        int timeout_package_id = (*head)->packageId;

        // Entferne das erste Element aus der Liste.
        del_timer_linked_list_timer(head, (*head)->packageId);

        // Gib die `packageId` des entfernten Elements zurück.
        return timeout_package_id;
    }

    // Kein Timeout, Rückgabewert 0.
    return 0;
}



/**
 * Funktion: print_timer_linked_list_timer
 * ---------------------------------------
 * Gibt die Inhalte einer verketteten Liste von Timern auf der Konsole aus.
 * Jedes Element der Liste enthält eine ID (`packageId`) und eine verbleibende Zeit (`ticksToGo`).
 * 
 * Parameter:
 * - head: Ein Pointer auf einen Pointer zum Kopf der verketteten Liste (Struktur `linked_list_timer`).
 *
 * Rückgabewert:
 * - Keiner (void).
 *
 * Beschreibung:
 * - Diese Funktion iteriert über die verkettete Liste, beginnt beim Kopf und gibt die Informationen
 *   jedes Elements in der Liste in der Reihenfolge aus.
 * - Wenn die Liste leer ist (der Kopf zeigt auf `NULL`), wird dies entsprechend gemeldet.
 */
void print_timer_linked_list_timer(struct linked_list_timer** head)
{
    // Überprüfen, ob die Liste leer ist
    if(*head == NULL)
    {
        printf(BLUE "Timer Liste leer\n" RESET);
        return;
    }

    // Iterieren durch die verkettete Liste
    while((*head)->next != NULL)
    {
        // Aktuelles Element ausgeben
        printf(BLUE "[ID %d ToGo %d] -> ", (*head)->packageId, (*head)->ticksToGo);
        // Zum nächsten Element wechseln
        head = &((*head)->next);
    }
    
    // Letztes Element ausgeben (ohne "->")
    printf(BLUE "[ID %d ToGo %d]\n" RESET, (*head)->packageId, (*head)->ticksToGo);
}

