#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h> //w. TH

#define MAX_JOBS 64
#define NUM_THREADS 10

int job_queue[MAX_JOBS];
int q_count = 0;
int q_head = 0; //hier entnehmen die Worker
int q_tail = 0; //hier fügt main hinzu

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_not_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_not_full = PTHREAD_COND_INITIALIZER;

FILE *logfile; //Global, damit die Worker reinkommen

int is_prime(int n) {
    if (n <= 1) return 0;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) return 0; //Teilbar, also kP
    }
    return 1;
}

//Counter
int count_primes(int limit) {
    int count = 0;
    for (int i = 0; i <= limit; i++) {
        if (is_prime(i)) {
            count++;
        }
    }
    return count;
}

//Worker-Funktion
//quasi was vorher in der main-Schleife stand
void *worker_thread(void *arg) {
    int thread_id = *(int*)arg; //ID merken für Logging

    while (1) {
        int client_sock;

        //Lock holen für Queue
        pthread_mutex_lock(&mutex);

        //Warten bis Job da ist
        while (q_count == 0) {
            pthread_cond_wait(&cond_not_empty, &mutex);
        }

        //Job aus der Queue nehmen (FIFO)
        client_sock = job_queue[q_head];
        q_head = (q_head + 1) % MAX_JOBS;
        q_count--;

        //Main bescheid geben, dass wieder ein Platz im Puffer frei ist
        pthread_cond_signal(&cond_not_full);

        //Queue-Zugriff fertig -> Lock freigeben
        pthread_mutex_unlock(&mutex);


        char buffer[256];
        memset(buffer, 0, sizeof(buffer)); //Puffer leerern
        int bytes_read = 0;

        while (1) {
            int result = recv(client_sock, buffer + bytes_read, 1, 0);
            if (result <= 0) break;
            
            if (buffer[bytes_read] == '\n') {
                buffer[bytes_read] = '\0';
                break;
            }
            bytes_read++;
        }

        //wenn true geht weiter
        if (bytes_read > 0) {
            int limit = atoi(buffer); //aci to int
            
            //RC verhindern
            pthread_mutex_lock(&mutex);
            printf("[Thread %d] Berechne Primzahlen bis: %d\n", thread_id, limit);
            fprintf(logfile, "[Thread %d] Berechne Primzahlen bis: %d\n", thread_id, limit);
            fflush(logfile);
            pthread_mutex_unlock(&mutex);

            int primes = count_primes(limit); //zählen anwenden

            pthread_mutex_lock(&mutex);
            printf("[Thread %d] Ergebnis für %d -> %d\n", thread_id, limit, primes);
            fprintf(logfile, "[Thread %d] Ergebnis für %d -> %d\n", thread_id, limit, primes);
            fflush(logfile);
            pthread_mutex_unlock(&mutex);

            char antwort[256]; //senden
            snprintf(antwort, sizeof(antwort), "%d\n", primes);
            
            if (send(client_sock, antwort, strlen(antwort), 0) < 0) {
                perror("Fehler beim Senden");
            }
        }
        close(client_sock);
    }
    return NULL;
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    logfile = fopen("log.txt", "w");
    if (logfile == NULL) {
        perror("Fehler: log.txt konnte nicht geöffnet werden");
        return 1;
    }

    printf("Starte Server auf Port 8080...\n");
    fprintf(logfile, "Starte Server auf Port 8080...\n");
    fflush(logfile);

    //die WT erzeugen
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]) != 0) {
            perror("Fehler beim Erstellen der Threads");
            return 1;
        }
    }

    //Socket machen
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Fehler bei socket()");
        return 1;
    }

    int opt = 1; //damit Port gecleared wird
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Fehler bei bind()");
        return 1;
    }

    if (listen(server_sock, 10) < 0) {
        perror("Fehler bei listen()");
        return 1;
    }

    //Main-Schleife: Nimmt nur noch an und packt in die Queue
    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Fehler bei accept()");
            continue; //Wenn 1 failt das es tritz. weiter geht
        }

        //Queue locken zum Einfügen
        pthread_mutex_lock(&mutex);

        //Warten falls  Queue voll ist
        while (q_count == MAX_JOBS) {
            pthread_cond_wait(&cond_not_full, &mutex);
        }

        //In die Queue einreihen (FIFO)
        job_queue[q_tail] = client_sock;
        q_tail = (q_tail + 1) % MAX_JOBS;
        q_count++;

        //Einen wartenden Worker aufwecken
        pthread_cond_signal(&cond_not_empty);

        //Lock wieder freigeben
        pthread_mutex_unlock(&mutex);
    }

    close(server_sock);
    fclose(logfile);
    return 0;
}