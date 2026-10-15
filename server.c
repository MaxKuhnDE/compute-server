#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

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

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    FILE *logfile = fopen("log.txt", "w");
    if (logfile == NULL) {
        perror("Fehler: log.txt konnte nicht geöffnet werden");
        return 1;
    }

    printf("Starte Server auf Port 8080...\n");
    fprintf(logfile, "Starte Server auf Port 8080...\n");
    fflush(logfile);

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

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Fehler bei accept()");
            continue; //Wenn 1 failt das es tritz. weiter geht
        }

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
            
            printf("[Job] Berechne Primzahlen bis: %d\n", limit);
            fprintf(logfile, "[Job] Berechne Primzahlen bis: %d\n", limit);
            fflush(logfile);

            int primes = count_primes(limit); //zählen anwenden

            printf("[Ergebnis] %d\n", primes);
            fprintf(logfile, "[Ergebnis] %d\n", primes);
            fflush(logfile);

            char antwort[256]; //senden
            snprintf(antwort, sizeof(antwort), "%d\n", primes);
            
            if (send(client_sock, antwort, strlen(antwort), 0) < 0) {
                perror("Fehler beim Senden");
            }
        }

        close(client_sock);
    }

    close(server_sock);
    fclose(logfile);
    return 0;
}