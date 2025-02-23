#include "connection.h"

/* !!! HIER MUSS NICHTS MEHR GEÄNDERT WERDEN !!! */
// Nur DEBUG Funktionen fehlen noch


/**
 * Struktur: queue
 * ---------------------
 * Speichert die Pakete in der Warteschlange und notiert ob ein TIMEOUT registriert wurde.
 */
struct queue 
{
    bool timeout;
    struct request req;
};


/**
 * Funktion: shift_queue
 * ---------------------
 * Verschiebt die Elemente einer Warteschlange (`queue`) um eine Position nach vorne.
 * Das erste Element wird entfernt, und alle nachfolgenden Elemente werden einen Schritt 
 * nach vorne verschoben.
 *
 * Parameter:
 * - queue: Ein Pointer auf einen Pointer zur Warteschlange (Array von `queue`-Elementen).
 * - packages_in_queue: Anzahl der Elemente in der Warteschlange.
 *
 * Rückgabewert:
 * - Keiner (void).
 *
 * Beschreibung:
 * - Die Funktion überprüft die Gültigkeit der Eingaben und verschiebt die Elemente 
 *   der Warteschlange um eine Position nach vorne.
 * - Das erste Element der Warteschlange wird dabei "überschrieben", sodass Platz für ein neues Element frei wird.
 * - Die Funktion geht davon aus, dass die Warteschlange als Array von Strukturen implementiert ist.
 *
 * Einschränkungen:
 * - Die Funktion bearbeitet nur die ersten `packages_in_queue` Elemente im Array.
 * - Es wird nicht überprüft, ob genügend Speicherplatz im Array vorhanden ist.
 */
void shift_queue(struct queue** queue, int packages_in_queue)
{
    if(packages_in_queue == 1)
    {
        memset((*queue), 0, sizeof(struct queue)); // Lösche das einzige Element
        return;
    }

    // Elemente verschieben
    for(int i = 1; i < packages_in_queue; i++)
    {
        (*queue)[i - 1] = (*queue)[i]; // Verschiebe jedes Element um eine Position nach vorne
    }
}

/**
 * Funktion: open_file
 * --------------------
 * Öffnet eine Datei im Lesemodus anhand des Dateipfads, der in der `properties`-Struktur gespeichert ist,
 * und speichert den Datei-Zeiger in der Struktur. Überprüft, ob die Datei erfolgreich geöffnet wurde.
 *
 * Parameter:
 * - props: Ein Pointer auf die Struktur `properties`, die den Pfad zur Datei (`file_path`) 
 *          und den Datei-Zeiger (`file`) enthält.
 *
 * Rückgabewert:
 * - 0, wenn die Datei erfolgreich geöffnet wurde.
 * - -1, wenn die Datei nicht geöffnet werden konnte.
 */
int open_file(struct properties* props)
{
    // Datei im Lesemodus ("r") öffnen
    props->file = fopen(props->file_path, "r");
    if (props->file == NULL) 
    {
        print_timestamp();
        printf(RED "Datei konnte nicht geöffnet werden\n" RESET); // Fehlerausgabe
        return -1;
    }

    print_timestamp();
    printf(GREEN "Datei geöffnet\n" RESET); // Erfolgsmeldung
    return 0;
}



/**
 * Funktion: get_file_line
 * ------------------------
 * Liest eine Zeile aus der Datei, die in `props->file` geöffnet ist, 
 * und speichert sie im bereitgestellten Puffer `line`.
 *
 * Parameter:
 * - props: Ein Pointer auf die Struktur `properties`, die den Datei-Zeiger enthält.
 * - line: Ein Puffer, in dem die gelesene Zeile gespeichert wird.
 */
int get_file_line(struct properties* props, char* line)
{
    if(fgets(line, DEFAULT_DATA_BUFFER_SIZE, props->file) == NULL)
    {
        return -1;
    }

    return 0;
}


/**
 * Funktion: get_line_length
 * -------------------------
 * Berechnet die Länge einer Zeichenkette bis zum ersten Zeilenumbruch (`\n`).
 *
 * Parameter:
 * - line: Ein Pointer auf die Zeichenkette, deren Länge berechnet werden soll.
 *
 * Rückgabewert:
 * - Die Länge der Zeichenkette bis einschließlich des ersten `\n`.
 *
 * Beschreibung:
 * - Die Funktion zählt die Anzahl der Zeichen in der Zeichenkette `line`, 
 *   bis der erste Zeilenumbruch (`\n`) gefunden wird.
 * - Der Zeilenumbruch (`\n`) wird in die berechnete Länge einbezogen.
 * - Es wird davon ausgegangen, dass die Zeichenkette garantiert einen Zeilenumbruch enthält.
 */
int get_line_length(char* line)
{
    int c = 0; // Zähler für die Länge der Zeichenkette
    while(line[c] != '\n') // Schleife bis zum Zeilenumbruch
    {
        c += 1;
    }

    return c + 1; // Länge inklusive des Zeilenumbruchs zurückgeben
}


/**
 * Funktion: prepare_hello_package
 * -------------------------------
 * Bereitet ein "Hello"-Paket vor, das verwendet wird, um sich z. B. bei anderen 
 * Teilnehmern zu registrieren oder eine Verbindung aufzubauen. Die Funktion 
 * initialisiert die relevanten Felder der `req`-Struktur innerhalb der `communication`-Struktur.
 *
 * Parameter:
 * - props: Ein Pointer auf die `properties`-Struktur, die Server- oder Client-Eigenschaften wie die ID und Fenstergröße speichert.
 * - com: Ein Pointer auf die `communication`-Struktur, in der die Anfragedaten (`req`) gespeichert werden.
 *
 * Rückgabewert:
 * - Keiner (void).
 *
 * Beschreibung:
 * - Setzt den Nachrichtentyp (`REQ_HELLO`) in der Anfragestruktur.
 * - Initialisiert relevante Felder wie `packageId`, `packageLen`, `senderId`, `reciverId`, und `data`.
 * - Überträgt die Fenstergröße (`windows_size`) als Paketlänge.
 * - Löscht den Datenpuffer (`data`), da für ein "Hello"-Paket keine Nutzdaten erforderlich sind.
 */
void prepare_hello_package(struct properties* props, struct communication* com)
{
    com->req.type = REQ_HELLO;            // Nachrichtentyp: "Hello"
    com->req.packageId = 0;               // Paket-ID: 0 für Initialnachrichten
    com->req.packageLen = props->windows_size; // Fenstergröße als Paketlänge
    com->req.senderId = props->id;        // Absender-ID aus den Eigenschaften
    com->req.reciverId = -1;              // Empfänger-ID: Broadcast (-1 bedeutet, an alle senden)
    memset(com->req.data, 0, DEFAULT_DATA_BUFFER_SIZE); // Datenpuffer löschen
}


/**
 * Funktion: prepare_data_package
 * ------------------------------
 * Bereitet ein Datenpaket vor, das an einen Empfänger gesendet werden kann. 
 * Die Funktion initialisiert die relevanten Felder der `req`-Struktur innerhalb
 * der `communication`-Struktur.
 *
 * Parameter:
 * - props: Ein Pointer auf die `properties`-Struktur, die Server- oder Client-Eigenschaften wie die ID speichert.
 * - com: Ein Pointer auf die `communication`-Struktur, in der die Anfragedaten (`req`) gespeichert werden.
 * - package_id: Die ID des Pakets, das gesendet werden soll.
 * - line: Ein Zeiger auf den Datenpuffer (Zeichenkette), der im Paket enthalten sein soll.
 *
 * Rückgabewert:
 * - Keiner (void).
 *
 * Beschreibung:
 * - Setzt den Nachrichtentyp (`REQ_DATA`) in der Anfragestruktur.
 * - Initialisiert relevante Felder wie `packageId`, `packageLen`, `senderId`, `reciverId`, und `data`.
 * - Berechnet die Länge der Daten (`line`) und speichert sie in `packageLen`.
 * - Kopiert die Daten (`line`) in den Datenpuffer der Anfrage.
 */
void prepare_data_package(struct properties* props, struct communication* com, int package_id, char* line)
{
    com->req.type = REQ_DATA;                      // Nachrichtentyp: Datenpaket
    com->req.packageId = package_id;              // ID des Pakets
    com->req.packageLen = get_line_length(line);  // Länge der Daten berechnen
    com->req.senderId = props->id;                // Absender-ID aus den Eigenschaften
    com->req.reciverId = -1;                      // Empfänger-ID: Broadcast (-1 bedeutet, an alle senden)
    strcpy(com->req.data, line);                  // Kopiere die Daten in den Puffer
}


/**
 * Funktion: prepare_close_package
 * -------------------------------
 * Bereitet ein "Close"-Paket vor, das verwendet werden kann, um eine Verbindung oder Sitzung 
 * zu schließen. Die Funktion initialisiert die relevanten Felder der `req`-Struktur innerhalb
 * der `communication`-Struktur.
 *
 * Parameter:
 * - props: Ein Pointer auf die `properties`-Struktur, die Server- oder Client-Eigenschaften wie die ID speichert.
 * - com: Ein Pointer auf die `communication`-Struktur, in der die Anfragedaten (`req`) gespeichert werden.
 * - package_id: Die ID des Pakets, das geschlossen werden soll.
 *
 * Rückgabewert:
 * - Keiner (void).
 *
 * Beschreibung:
 * - Setzt den Nachrichtentyp (`REQ_CLOSE`) in der Anfragestruktur.
 * - Initialisiert relevante Felder wie `packageId`, `senderId`, `reciverId`, und `data` 
 *   mit standardmäßigen oder angegebenen Werten.
 * - Löscht den Datenpuffer (`data`) mit `memset`, da keine Nutzdaten erforderlich sind.
 */
void prepare_close_package(struct properties* props, struct communication* com, int package_id)
{
    com->req.type = REQ_CLOSE;          // Nachrichtentyp: Schließen
    com->req.packageId = package_id;   // ID des Pakets, das geschlossen wird
    com->req.packageLen = 0;           // Keine Nutzdaten
    com->req.senderId = props->id;     // Absender-ID wird aus den Eigenschaften übernommen
    com->req.reciverId = -1;           // Empfänger-ID: Broadcast (-1 bedeutet, an alle senden)
    memset(com->req.data, 0, DEFAULT_DATA_BUFFER_SIZE); // Datenpuffer löschen
}


/**
 * Funktion: run_state_machine
 * ----------------------------------
 * Implementiert die Zustandsmaschine für den Server, beginnend mit dem Init-Zustand.
 * Die Zustandsmaschine steuert den Ablauf des Servers durch verschiedene Zustände,
 * von der Initialisierung über die Kommunikation bis zur Beendigung.
 *
 * Parameter:
 * - props: Pointer auf die `properties`-Struktur mit Server-Informationen.
 */
void run_state_machine(struct properties* props) 
{
    // Lokale Variablen initialisieren
    struct communication com;               // Kommunikation: Anfragen und Antworten
    struct memberlist list_members;         // Liste der Mitglieder im Netzwerk
        
    struct queue* queue = NULL;             // Warteschlange für zu sendende Pakete
    int packages_in_queue;                  // Anzahl der Pakete in der Warteschlange
    int base;                               // Basis-ID des aktuellen Fensters
    int current;                            // Aktuelle Paket ID

    struct linked_list_timer* timer_list;   // Timer-Liste zur Verwaltung von Timeouts
    
    bool closed = false;                    // Speichert ob close Paket gepuffert wurde

    // Startzustand setzen
    connection_state state = STATE_INIT;   // Initialzustand
    bool running = true;                   // Steuerung der Hauptschleife

    srand(time(NULL)); // Zufallsgenerator initialisieren

    // Hauptschleife der Zustandsmaschine
    while(running) 
    {
        switch (state) 
        {
            case STATE_INIT:
            {
                // Initialisierung des Servers
                list_members.number_members = 0; // Leere Mitgliederliste
                
                // Warteschlange freigeben, falls vorhanden
                if(queue != NULL)
                {
                    free(queue);
                }
                
                // Warteschlange erstellen
                queue = malloc(sizeof(struct queue) * props->windows_size);

                // Datei auf Anfang zurücksetzen
                rewind(props->file);
                
                // Kommunikationsstruktur initialisieren
                com.ans.type = '0';

                // Variablen initialisieren
                packages_in_queue = 0;
                base = 1;
                current = 1;

                timer_list = NULL; // Timer-Liste initialisieren

                closed = false;     // Pufferendemarkierung zurücksetzen

                // Zustand wechseln
                print_timestamp();
                printf("Wechsel zu STATE_IDLE\n");
                state = STATE_IDLE;
                break;
            }

            case STATE_IDLE:
            {
                // Wartezeit
                print_timestamp();
                printf(BLUE "Warte... %is\n" RESET, DEFAULT_IDLE_TIME);
                sleep(DEFAULT_IDLE_TIME); // Warten

                // Hello-Paket vorbereiten und senden
                prepare_hello_package(props, &com);
                if(send_multicast(props, &com)<0)
                {
                    running = false; // Fehler beim Senden
                }
                
                // Zustand wechseln
                print_timestamp();
                printf("Wechsel zu STATE_PREPARE\n");
                state = STATE_PREPARE;
                break;
            }

            case STATE_PREPARE:
            {
                // Wartezeit für Mitgliederregistrierung
                for(int interval=0; interval<MAX_ALLOWED_CLIENTS; interval++)
                {
                    if(receive_cast(props, &com, NULL)) // Paket empfangen
                    {
                        if(com.ans.type == ANS_HELLO) // Hello-Paket erkannt
                        {
                            // Mitglied registrieren
                            list_members.member[list_members.number_members].member_id = com.ans.senderId;
                            list_members.member[list_members.number_members].member = com.partner;
                            list_members.number_members += 1;
                            
                            print_timestamp();
                            printf(GREEN "Mitglied mit ID:%i registriert\n" RESET, com.ans.senderId);
                        }
                    }
                }

                // Überprüfen, ob Mitglieder registriert wurden
                if(list_members.number_members>0)
                {
                    print_timestamp();
                    printf("Wechsel zu STATE_ESTABLISHED\n");
                    state = STATE_ESTABLISHED;
                }
                else
                {
                    print_timestamp();
                    printf(RED "Keine Teilnehmer gefunden.\n" RESET);
                    print_timestamp();
                    printf("Wechsel zu STATE_IDLE\n");
                    state = STATE_IDLE;
                }

                break;
            }

            // Bisschen Kacke geschrieben alles ngl, könnte mit Funktionen besser werden
            case STATE_ESTABLISHED:
            {
                int timeout_package_id = tick_timer_linked_list_timer(&timer_list);

                // Timer verwalten
                if(timeout_package_id > 0)
                {
                    print_timestamp();
                    printf(RED "Paket %d TIMEOUT\n" RESET, timeout_package_id);
                    queue[timeout_package_id - base].timeout = true;

                }
                else
                {
                    print_timestamp();
                    printf(RED "Kein TIMEOUT\n" RESET);
                }

                // NACK behandeln
                bool nack_recived = false;
                if(com.ans.type == ANS_NACK)
                {   
                    // NACK außerhalb des Fensters
                    if(com.ans.packageId < base)
                    {
                        print_timestamp();
                        printf(RED "NACK von Empänger mit ID %d für Paket %d, außerhalb des Sendefensters!\n" RESET, com.ans.senderId, com.ans.packageId);

                    }
                    else if(com.ans.packageId > current)
                    {
                        print_timestamp();
                        printf(RED "NACK von Empänger mit ID %d für Paket %d, wurde noch nicht gesendet!\n" RESET, com.ans.senderId, com.ans.packageId);
                    }
                    else if(queue[com.req.packageId-base].req.type == REQ_CLOSE)
                    {
                        print_timestamp();
                        printf(RED "NACK für CLOSE wird ignoriert!\n" RESET);
                    }
                    else
                    {
                        nack_recived = true;

                        print_timestamp();
                        printf(RED "NACK von Empänger mit ID %d für Paket %d erhalten\n" RESET, com.ans.senderId, com.ans.packageId);
                        
                        // Timer neu setzen
                        del_timer_linked_list_timer(&timer_list, com.ans.packageId);
                        add_timer_linked_list_timer(&timer_list, com.ans.packageId, MAX_ALLOWED_CLIENTS);
                        queue[com.ans.packageId - base].timeout = false;
                        
                        // Paket erneut senden
                        com.req = queue[com.ans.packageId - base].req;
                        com.req.reciverId = com.ans.senderId;
                        
                        if(!props->local)
                        {
                            if(props->debug_code>0)
                            {
                                if((rand()%100)+1<=props->debug_code)
                                {
                                    print_timestamp();
                                    printf(RED "Paket %d verloren\n" RESET, com.req.packageId);
                                }
                                else
                                {
                                    if(send_unicast(props, &com)<0)
                                    {
                                        running = false;
                                    }

                                    print_timestamp();
                                    printf(GREEN "Paket %d gesendet an Empfänger mit Id %d\n" RESET, com.req.packageId, com.req.reciverId);
                                }
                            }
                            else
                            {
                                if(send_unicast(props, &com)<0)
                                {
                                    running = false;
                                }

                                print_timestamp();
                                printf(GREEN "Paket %d gesendet an Empfänger mit Id %d\n" RESET, com.req.packageId, com.req.reciverId);
                            }
                        }
                        else
                        {
                            if(props->debug_code>0)
                            {
                                if((rand()%100)+1<=props->debug_code)
                                {
                                    print_timestamp();
                                    printf(RED "Paket %d verloren\n" RESET, com.req.packageId);
                                }
                                else
                                {
                                    if(send_multicast(props, &com)<0)
                                    {
                                        running = false;
                                    }

                                    print_timestamp();
                                    printf(GREEN "Paket %d gesendet\n" RESET, com.req.packageId);
                                }
                            }
                            else
                            {
                                if(send_multicast(props, &com)<0)
                                {
                                    running = false;
                                }

                                print_timestamp();
                                printf(GREEN "Paket %d gesendet\n" RESET, com.req.packageId);
                            }
                        }
                        
                        
                        // Setzen des Pakets auf 0, damit wird das Paket bei der nächsten Itteration überschrieben und nicht als NACK
                        com.ans.type = '0';
                    }
                }
                

                // Fenster verschieben
                while(queue[0].timeout == true && packages_in_queue > 0)
                {
                    shift_queue(&queue, packages_in_queue);
                    base += 1;
                    packages_in_queue -= 1;
                }


                // Füllen des Fensters bis es voll ist
                while(packages_in_queue<props->windows_size)
                {
                    // Im Nachhinhein eine etwas hässliche Lösung mit den Prepare Paclage ngl
                    // Habe übersehen das man vorpuffern soll warum auch immer

                    struct communication com_temp;

                    char line[DEFAULT_DATA_BUFFER_SIZE];
                    if(get_file_line(props, line)>=0)
                    {   
                        print_timestamp();
                        printf(GREEN "Paket %d wird gepackt\n" RESET, base+packages_in_queue);
                        prepare_data_package(props, &com_temp, base+packages_in_queue, line);
                        queue[packages_in_queue].req = com_temp.req;
                        queue[packages_in_queue].timeout = false;
                        packages_in_queue += 1;
                    }
                    else
                    {
                        // Wenn CLOSE noch nicht erstellt wurde, wird CLOSE Paket erstellt
                        if(!closed)
                        {
                            print_timestamp();
                            printf(GREEN "Paket %d CLOSE wird gepackt\n" RESET, base+packages_in_queue);
                            prepare_close_package(props, &com_temp, base+packages_in_queue);
                            queue[packages_in_queue].req = com_temp.req;
                            queue[packages_in_queue].timeout = false;
                            packages_in_queue += 1;
                            closed = true;
                        }

                        break;
                    }
                }
                

                // Daten senden, wenn kein NACK empfangen wurde
                if(!nack_recived)
                {   
                    if(base+props->windows_size > current)
                    {
                        // Laden des Pakets aus dem Fenster und starten eines Timers
                        com.req = queue[current-base].req;    

                        // Wenn CLOSE Paket aus dem Fenster geladen wird, wird zu close state gewechselt    
                        if(com.req.type != REQ_CLOSE)
                        {
                            del_timer_linked_list_timer(&timer_list, current);
                            add_timer_linked_list_timer(&timer_list, current, MAX_ALLOWED_CLIENTS);
                            current += 1;
                        }
                        else
                        {
                            // Doppelte Timerlänge CLOSE Paket um CLOSE NACK Problem zu lösen.
                            del_timer_linked_list_timer(&timer_list, current);
                            add_timer_linked_list_timer(&timer_list, current, 2 * MAX_ALLOWED_CLIENTS);                            
                            print_timestamp();
                            printf("Wechsel zu STATE_CLOSE\n");

                            state = STATE_CLOSE;
                        }

                        // Multicast senden
                        if(props->debug_code > 0)
                        {
                            if((rand()%100)+1<=props->debug_code)
                            {
                                print_timestamp();
                                printf(RED "Paket %d verloren\n" RESET, com.req.packageId);
                            }
                            else
                            {
                                if(send_multicast(props, &com)<0)
                                {
                                    running = false;
                                }

                                print_timestamp();
                                printf(GREEN "Paket %d gesendet\n" RESET, com.req.packageId);
                            }                            
                        }
                        else if(props->debug_code == -1 && com.req.type == REQ_CLOSE)
                        {
                            props->debug_code = 0;
                            print_timestamp();
                            printf(RED "CLOSE Paket verloren\n" RESET);
                        }
                        else
                        {
                            if(send_multicast(props, &com)<0)
                            {
                                running = false;
                            }
                        }

                    }
                    else
                    {
                        print_timestamp();
                        printf(RED "Fensterende erreicht, kein Paket gesendet\n" RESET);
                    }
                }

                print_timestamp();
                print_timer_linked_list_timer(&timer_list); // Timer anzeigen
                print_timestamp();
                printf("Pakete in %d Queue\n", packages_in_queue);
                print_timestamp();
                printf("Base %d\n", base);


                com.ans.type = '0'; // Antwort zurücksetzen

                // Empfang von Paketen
                if(receive_cast(props, &com, &list_members) < 0)
                {
                    running = false;
                }

                break;
            }

            case STATE_CLOSE:
            {
                // Empfang von Antworten
                while(true)
                {   
                    if(com.ans.type == ANS_CLOSE)
                    {
                        print_timestamp();
                        printf(GREEN "CLOSE erhalten von Id %d\n" RESET, com.ans.senderId);
                    }

                    if(com.ans.type == ANS_NACK)
                    {
                        del_timer_linked_list_timer(&timer_list, current);
                        print_timestamp();
                        printf("Wechsel zu STATE_ESTABLISHED\n");
                        state = STATE_ESTABLISHED;
                        break;
                    }

                    int timeout_package_id = tick_timer_linked_list_timer(&timer_list);


                    com.ans.type = '0';

                    if(timeout_package_id > 0)
                    {
                        print_timestamp();
                        printf(RED "Paket %d TIMEOUT\n" RESET, timeout_package_id);
                        queue[timeout_package_id - base].timeout = true;
                    }
                    else if(timeout_package_id == 0)
                    {
                        print_timestamp();
                        printf(RED "Kein TIMEOUT\n" RESET);
                    }

                    // Fenster verschieben
                    while(queue[0].timeout == true && packages_in_queue > 0)
                    {
                        shift_queue(&queue, packages_in_queue);
                        base += 1;
                        packages_in_queue -= 1;
                    }

                    print_timestamp();
                    print_timer_linked_list_timer(&timer_list); // Timer anzeigen


                    // Beenden wenn keine Pakete mehr in Liste
                    if(packages_in_queue <= 0)
                    {
                        break;
                    }
                    else
                    {
                        print_timestamp();
                        printf("Pakete in %d Queue\n", packages_in_queue);
                        print_timestamp();
                        printf("Base %d\n", base);
                    }
                    
                    if(receive_cast(props, &com, &list_members)<0)
                    {
                        running = false;
                    }
                }


                if(state == STATE_CLOSE)
                {
                    if(props->loop)
                    {
                        print_timestamp();
                        printf("Wechsel zu STATE_INIT\n");
                        state = STATE_INIT;
                        break;
                    }

                    running = false; // Beenden der Schleife
                }         

                break;
            }
        }
    }

    // Ressourcen freigeben
    free(queue);
    close_socket(props);
    print_timestamp();
    printf("Programm wird beendet\n");
}


/**
 * Funktion: main
 * --------------
 * Startpunkt des Programms. Konfiguriert die Eigenschaften, erstellt den Socket, 
 * öffnet die Datei und startet die Server-Zustandsmaschine.
 *
 * Parameter:
 * - argc: Anzahl der Kommandozeilenargumente.
 * - argv: Array der Kommandozeilenargumente.
 *
 * Rückgabewert:
 * - 0: Erfolgreiche Ausführung.
 * - -1: Fehler bei der Initialisierung oder während der Ausführung.
 *
 * Beschreibung:
 * - Diese Funktion übernimmt die Initialisierung des Servers, einschließlich:
 *   1. Konfiguration der Servereigenschaften.
 *   2. Erstellung und Konfiguration des Sockets.
 *   3. Öffnen der erforderlichen Datei.
 *   4. Start der Zustandsmaschine des Servers.
 * - Bei Fehlern während der Initialisierung wird der Socket geschlossen, und das Programm 
 *   beendet sich mit einer Fehlermeldung.
 */
int main(int argc, char* argv[]) 
{       
    struct properties props; 
    props.is_server = true; // Legt fest, dass der Server-Modus verwendet wird

    // Server-Eigenschaften konfigurieren
    if(setup_properties(argc, argv, &props) < 0)
    {
        print_timestamp();
        printf("Programm wird beendet\n"); // Fehler bei der Konfiguration
        return -1;
    }    
    
    // Socket erstellen    
    if(start_socket(&props) < 0)
    {  
        close_socket(&props); // Socket freigeben

        print_timestamp();
        printf("Programm wird beendet\n"); // Fehler bei der Socket-Erstellung
        return -1;
    }

    // Datei öffnen
    if(open_file(&props))
    {
        close_socket(&props); // Socket freigeben

        print_timestamp();
        printf("Programm wird beendet\n"); // Fehler beim Öffnen der Datei
        return -1;
    }

    // Start der Zustandsmaschine
    run_state_machine(&props);

    return 0;
}