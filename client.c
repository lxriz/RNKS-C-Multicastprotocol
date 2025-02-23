#include "connection.h"



/**
 * Struktur: queue
 * ---------------------
 * Speichert die Pakete in der Warteschlange und notiert ob ein TIMEOUT registriert wurde.
 */
struct queue 
{
    bool timeout;
    bool recived;
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
void shift_queue(struct queue** queue, int queue_length)
{
    // Elemente verschieben
    for(int i = 1; i < queue_length; i++)
    {
        (*queue)[i - 1] = (*queue)[i]; // Verschiebe jedes Element um eine Position nach vorne
    }

    memset(&((*queue)[queue_length-1]), 0, sizeof(struct queue));
}

/**
 * Funktion: open_file
 * --------------------
 * Überprüft, ob eine Datei existiert, und erstellt sie, wenn sie nicht existiert.
 *
 * Parameter:
 * - props: Ein Pointer auf die Struktur `properties`, die den Dateipfad und 
 *          den Datei-Zeiger enthält.
 *
 * Rückgabewert:
 * - 0: Datei wurde erfolgreich erstellt.
 * - -1: Datei existiert bereits oder konnte nicht erstellt werden.
 */
int open_file(struct properties* props)
{
    // Überprüfen, ob die Datei existiert
    props->file = fopen(props->file_path, "r");
    if (props->file != NULL) 
    {
        print_timestamp();
        printf(RED "Die Datei existiert schon\n" RESET);

        fclose(props->file); // Sicherstellen, dass die Datei geschlossen wird

        return -1;
    }

    // Datei im Schreibmodus erstellen
    props->file = fopen(props->file_path, "w");
    if (props->file == NULL) 
    {
        print_timestamp();
        printf(RED "Fehler beim Anlegen der Datei\n" RESET);

        return -1;
    }

    print_timestamp();
    printf(GREEN "Datei angelegt\n" RESET);
    return 0;
}


void write_to_file(struct properties* props, char* line)
{
    fprintf(props->file, "%s", line);
}


/**
 * Funktion: prepare_hello_package
 * -------------------------------
 * Erstellt ein "Hello"-Antwortpaket (`ANS_HELLO`) und speichert es 
 * in der `communication`-Struktur.
 *
 * Parameter:
 * - props: Ein Pointer auf die Struktur `properties`, die die ID des Senders enthält.
 * - com: Ein Pointer auf die Struktur `communication`, in der die Antwort gespeichert wird.
 */
void prepare_hello_package(struct properties* props, struct communication* com)
{
    struct answer ans;         // Lokale Antwortstruktur erstellen
    ans.senderId = props->id;  // Sender-ID setzen
    ans.reciverId = com->req.senderId;  // Empfänger-ID setzen
    ans.type = ANS_HELLO;      // Pakettyp auf "Hello" setzen
    ans.packageId = 0;         // PacketId auf 0 setzen

    com->ans = ans;            // Antwort in die Kommunikationsstruktur kopieren
}



/**
 * Funktion: prepare_nack_package
 * ------------------------------
 * Erstellt ein "Negative Acknowledgment"-Antwortpaket (`ANS_NACK`) und speichert 
 * es in der `communication`-Struktur. Zusätzlich wird die Paket-ID (`packageId`) 
 * angegeben, auf die sich das NACK bezieht.
 *
 * Parameter:
 * - props: Ein Pointer auf die Struktur `properties`, die die ID des Senders enthält.
 * - com: Ein Pointer auf die Struktur `communication`, in der die Antwort gespeichert wird.
 * - packageId: Die ID des Pakets, das nicht erfolgreich verarbeitet wurde.
 */
void prepare_nack_package(struct properties* props, struct communication* com, int packageId)
{
    struct answer ans;            // Lokale Antwortstruktur erstellen
    ans.senderId = props->id;     // Sender-ID setzen
    ans.reciverId = com->req.senderId; // Setze EmpfängerID
    ans.type = ANS_NACK;          // Pakettyp auf "NACK" setzen
    ans.packageId = packageId;    // ID des betroffenen Pakets setzen

    com->ans = ans;               // Antwort in die Kommunikationsstruktur kopieren
}



/**
 * Funktion: prepare_close_package
 * -------------------------------
 * Erstellt ein "Close"-Antwortpaket (`ANS_CLOSE`) und speichert es 
 * in der `communication`-Struktur.
 *
 * Parameter:
 * - props: Ein Pointer auf die Struktur `properties`, die die ID des Senders enthält.
 * - com: Ein Pointer auf die Struktur `communication`, in der die Antwort gespeichert wird.
 */
void prepare_close_package(struct properties* props, struct communication* com)
{
    struct answer ans;         // Lokale Antwortstruktur erstellen
    ans.reciverId = com->req.senderId;  // Empfänger-ID setzen
    ans.senderId = props->id;  // Sender-ID setzen
    ans.type = ANS_CLOSE;      // Pakettyp auf "Close" setzen

    com->ans = ans;            // Antwort in die Kommunikationsstruktur kopieren
}



void run_state_machine(struct properties* props) 
{
    // Lokale Variablen initialisieren
    struct communication com;               // Kommunikation: Anfragen und Antworten
        
    struct queue* queue = NULL;             // Warteschlange für zu sendende Pakete
    int packages_in_queue;                  // Anzahl der Pakete in der Warteschlange
    int base;                               // Basis-ID des aktuellen Fensters

    struct linked_list_timer* timer_list;   // Timer-Liste zur Verwaltung von Timeouts
    
    // Startzustand setzen
    connection_state state = STATE_INIT;   // Initialzustand
    bool running = true;                   // Steuerung der Hauptschleife


    // Hauptschleife der Zustandsmaschine
    while(running) 
    {
        switch (state) 
        {
            case STATE_INIT:
            {                
                // Kommunikationsstruktur initialisieren
                com.req.type = '0';

                // Variablen initialisieren
                packages_in_queue = 0;
                base = 1;

                timer_list = NULL; // Timer-Liste initialisieren

                // Zustand wechseln
                print_timestamp();
                printf("Wechsel zu STATE_IDLE\n");
                state = STATE_IDLE;
                break;
            }

            case STATE_IDLE:
            {
                if(receive_cast(props, &com, NULL)) // Paket empfangen
                {
                    if(com.req.type == REQ_HELLO)
                    {
                        props->windows_size = com.req.packageLen;
                        queue = malloc(sizeof(struct queue) * props->windows_size);
                        memset(queue, 0, sizeof(struct queue)*props->windows_size);

                        print_timestamp();
                        printf("Wechsel zu STATE_PREPARE\n");
                        state = STATE_PREPARE;
                    }
                }
                
                break;
            }

            case STATE_PREPARE:
            {
                prepare_hello_package(props, &com);
                if(send_unicast(props, &com)<0)
                {
                    running = false;
                }

                print_timestamp();
                printf("Wechsel zu STATE_ESTABLISHED\n");
                state = STATE_ESTABLISHED;
                break;
            }

            // Bisschen Kacke geschrieben alles ngl, könnte mit Funktionen besser werden
            case STATE_ESTABLISHED:
            {                  
                int timeout_package_id = tick_timer_linked_list_timer(&timer_list);
              
                if(com.req.type == REQ_DATA)
                {
                    print_timestamp();
                    printf(GREEN "Erwarte Paket %d, erhalten %d\n" RESET, base, com.req.packageId);

                    if(com.req.packageId == base)
                    {
                        if(!queue[0].recived)
                        {
                            queue[0].req = com.req;
                            queue[0].recived = true;
                        }

                        del_timer_linked_list_timer(&timer_list, base);

                        // Fenster verschieben
                        while(queue[0].recived)
                        {
                            write_to_file(props, queue[0].req.data);
                            shift_queue(&queue, props->windows_size);
                            base += 1;
                        }

                        del_timer_linked_list_timer(&timer_list, base);
                        add_timer_linked_list_timer(&timer_list, base, MAX_ALLOWED_CLIENTS);
                    }
                    else if(com.req.packageId > base)
                    {   
                        // Puffern wenn es ins  Fenster passt
                        if(com.req.packageId < base + props->windows_size && com.req.packageId >= base)
                        {
                            queue[com.req.packageId - base].req = com.req;
                            queue[com.req.packageId - base].timeout = false;
                            queue[com.req.packageId - base].recived = true;
                        }

                        // Wenn erster Timeout vorliegt dann NACK senden
                        if(!queue[0].timeout)
                        {
                            queue[0].timeout = true;
                            prepare_nack_package(props, &com, base);

                            del_timer_linked_list_timer(&timer_list, base);
                            add_timer_linked_list_timer(&timer_list, base, MAX_ALLOWED_CLIENTS);

                            if(send_unicast(props, &com)<0)
                            {
                                running = false;
                            }

                            print_timestamp();
                            printf(RED "Sende NACK für Paket %d\n" RESET, com.ans.packageId);
                        }
                        else
                        {
                            print_timestamp();
                            printf(RED "Paket %d wird ausgelassen!\n" RESET, base);

                            // Hier wird ein Paket auf desen Nack nicht reagiert wurde als verloren markiert
                            queue[0].req.type = REQ_DATA;
                            queue[0].req.data[0] = '\n';
                            queue[0].req.packageLen = 0;
                            queue[0].recived = true;

                            del_timer_linked_list_timer(&timer_list, base);

                            // Fenster verschieben
                            while(queue[0].recived)
                            {
                                write_to_file(props, queue[0].req.data);
                                shift_queue(&queue, props->windows_size);
                                base += 1;
                            }
                            
                            // Stimmt immer noch nicht
                            if(com.req.packageId > base)
                            {
                                queue[0].timeout = true;
                                prepare_nack_package(props, &com, base);
                                del_timer_linked_list_timer(&timer_list, base);
                                add_timer_linked_list_timer(&timer_list, base, MAX_ALLOWED_CLIENTS);

                                if(send_unicast(props, &com)<0)
                                {
                                    running = false;
                                }

                                print_timestamp();
                                printf(RED "Sende NACK für Paket %d\n" RESET, com.ans.packageId);
                            }
                            else
                            {
                                del_timer_linked_list_timer(&timer_list, base);
                                add_timer_linked_list_timer(&timer_list, base, MAX_ALLOWED_CLIENTS);
                            }
                        }                        
                    }
                    else
                    {
                        printf("Paket %d kleiner Base wird ignoriert.\n", com.ans.packageId);
                    }
                }
                else if(timeout_package_id > 0)
                {
                    print_timestamp();
                    printf(RED "Paket %d TIMEOUT\n" RESET, base);
                    if(!queue[0].timeout)
                    {
                        queue[0].timeout = true;
                        prepare_nack_package(props, &com, timeout_package_id);
                        del_timer_linked_list_timer(&timer_list, timeout_package_id);
                        add_timer_linked_list_timer(&timer_list, timeout_package_id, MAX_ALLOWED_CLIENTS);

                        if(send_unicast(props, &com)<0)
                        {
                            running = false;
                        }

                        print_timestamp();
                        printf(RED "Sende NACK für Paket %d\n" RESET, com.ans.packageId);
                    }
                    else
                    {
                        print_timestamp();
                        printf(RED "Paket %d wird ausgelassen!\n" RESET, timeout_package_id);

                        // Hier wird ein Paket auf desen Nack nicht reagiert wurde als verloren markiert
                        queue[0].req.type = REQ_DATA;
                        queue[0].req.data[0] = '\n';
                        queue[0].req.packageLen = 0;
                        queue[0].recived = true;

                        del_timer_linked_list_timer(&timer_list, base);

                        // Fenster verschieben
                        while(queue[0].recived)
                        {
                            write_to_file(props, queue[0].req.data);
                            shift_queue(&queue, props->windows_size);
                            base += 1;
                        }
                        
                        // Stimmt immer noch nicht
                        if(com.req.packageId > base)
                        {
                            queue[0].timeout = true;
                            prepare_nack_package(props, &com, base);
                            del_timer_linked_list_timer(&timer_list, base);
                            add_timer_linked_list_timer(&timer_list, base, MAX_ALLOWED_CLIENTS);

                            if(send_unicast(props, &com)<0)
                            {
                                running = false;
                            }

                            print_timestamp();
                            printf(RED "Sende NACK für Paket %d\n" RESET, com.ans.packageId);
                        }
                        else
                        {
                            del_timer_linked_list_timer(&timer_list, base);
                            add_timer_linked_list_timer(&timer_list, base, MAX_ALLOWED_CLIENTS);
                        }
                    }
                
                    
                }
                else
                {
                    print_timestamp();
                    printf(RED "Kein TIMEOUT\n" RESET);
                }

                print_timestamp();
                print_timer_linked_list_timer(&timer_list); // Timer anzeigen

                com.ans.type = '0';
                com.req.type = '0';
            
                // Empfang von Paketen
                if(receive_cast(props, &com, NULL) < 0)
                {
                    running = false;
                }

                if(com.req.type == REQ_CLOSE)
                {
                    print_timestamp();
                    printf("Wechsel zu STATE_CLOSE\n");
                    state = STATE_CLOSE;
                    break;
                }

                break;
            }

            case STATE_CLOSE:
            {
                if(com.req.packageId > base)
                {
                    print_timestamp();
                    printf("Wechsel zu STATE_ESTABLISHED\n");
                    state = STATE_ESTABLISHED;
                    break; 
                }

                prepare_close_package(props, &com);
                send_unicast(props, &com);

                running = false;

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




int main(int argc, char* argv[]) 
{       
    struct properties props; 
    props.is_server = false;

    if(setup_properties(argc, argv, &props)<0)
    {
        print_timestamp();
        printf("Programm wird beendet\n");

        return -1;
    }

    if(start_socket(&props)<0)
    {  
        print_timestamp();
        printf("Programm wird beendet\n");

        close_socket(&props);

        return -1;
    }


    if(open_file(&props)<0)
    {
        print_timestamp();
        printf("Programm wird beendet\n");

        close_socket(&props);

        return -1;
    }


    run_state_machine(&props);

    
    fclose(props.file);
    close_socket(&props);
    
    return 0;
}
