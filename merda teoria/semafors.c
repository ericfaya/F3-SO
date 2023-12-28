/**
 * Solució S9 - Semàfors II, curs 2023-24
 * @author Marc Valsells Niubó
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include "semaphore_v2.h"

#define ERROR_ARGS "Nombre d'arguments incorrecte, s'espera un sol argument amb el fixer de peticions\n"
#define ERROR_FILE "Error obrint el fitxer\n"
#define NUM_VLANS 3
#define BOARDS_ID 0
#define PDI_PAS_ID 1
#define STUDENTS_ID 2

typedef struct
{
    int id;
    char *vlan;
    char *link;
    char *username;
} Packet;

pthread_mutex_t mutexScreen;
semaphore sVlans[NUM_VLANS];
Packet *filePackets;
int numPackets = 0;
pthread_t threads[NUM_VLANS];

void poweroffFirewall()
{
    // Remove resources
    pthread_mutex_destroy(&mutexScreen);
    for (int i = 1; i < NUM_VLANS; i++)
    {
        SEM_destructor(&sVlans[i]);
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
        pthread_detach(threads[i]);
    }

    // Free memory
    for (int i = 0; i < numPackets; i++)
    {
        free(filePackets[i].vlan);
        free(filePackets[i].link);
        free(filePackets[i].username);
    }
    free(filePackets);

    // Exit
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void printText(char *string)
{
    pthread_mutex_lock(&mutexScreen);
    write(1, string, strlen(string));
    pthread_mutex_unlock(&mutexScreen);
}

// If needed for debugging
void printPacket(Packet packet)
{
    char *buffer;
    asprintf(&buffer, "--- Packet %d ---\nVLAN: %s\nLink: %s\nUser: %s\n-----------------\n", packet.id, packet.vlan, packet.link, packet.username);
    printText(buffer);
    free(buffer);
}

char *readUntil(int fd, char delimiter)
{
    char *msg = malloc(sizeof(char));
    char current;
    int i = 0;
    int len = 0;
    while ((len += read(fd, &current, 1)) > 0)
    {
        msg[i] = current;
        msg = (char *)realloc(msg, ++i + 1);
        if (current == delimiter)
            break;
    }
    msg[i - 1] = '\0';

    return msg;
}

void parseFile(char *filePath)
{
    int fileFd;
    char *buffer;

    // Open file and check for errors
    fileFd = open(filePath, O_RDONLY);
    if (fileFd < 3)
    {
        printText(ERROR_FILE);
        exit(-1);
    }
    else
    {
        // Get number of packets
        buffer = readUntil(fileFd, '\n');
        numPackets = atoi(buffer);
        free(buffer);

        // Parse packets
        filePackets = (Packet *)malloc(sizeof(Packet) * numPackets);
        for (int i = 0; i < numPackets; i++)
        {
            buffer = readUntil(fileFd, '>');
            filePackets[i].id = atoi(buffer);
            free(buffer);
            filePackets[i].vlan = readUntil(fileFd, '>');
            filePackets[i].link = readUntil(fileFd, '>');
            filePackets[i].username = readUntil(fileFd, '\n');
        }
        close(fileFd);
    }
}

void *vlanManager(void *arg)
{
    int vlanId = *(int *)arg;
    while (1)
    {
        // Wait for packets in my VLAN
        SEM_wait(&sVlans[vlanId]);
        switch (vlanId)
        {
        case BOARDS_ID:
            printText("Processed a packet in Pissarres VLAN\n");
            break;
        case PDI_PAS_ID:
            printText("Processed a packet in PDI-PAS VLAN, waiting 2 seconds\n");
            sleep(2);
            break;
        case STUDENTS_ID:
            printText("Processed a packet in Alumnes VLAN, waiting 3 seconds\n");
            sleep(3);
            break;
        default:
            printText("Unknown VLAN in vlanManager\n");
            break;
        }
    }
}

int main(int argc, char *argv[])
{
    int ids[NUM_VLANS];
    char *buffer;
    signal(SIGINT, poweroffFirewall);
    if (argc != 2)
    {
        printText(ERROR_ARGS);
        return -1;
    }

    parseFile(argv[1]);

    // Create semaphores
    for (int i = 0; i < NUM_VLANS; i++)
    {
        SEM_constructor(&sVlans[i]);
        SEM_init(&sVlans[i], 0);
    }

    // Create a thread for each VLAN
    for (int i = 0; i < NUM_VLANS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, vlanManager, &ids[i]);
    }

    // Parse packets
    for (int i = 0; i < numPackets; i++)
    {
        if (strcmp(filePackets[i].vlan, "Pissarres") == 0)
        {
            // Packet in Pissarres VLAN
            if (strstr(filePackets[i].link, "zoom.us") == NULL)
            {
                asprintf(&buffer, "Droping packet with id %d which is in the Pissarres VLAN and the link doesn't have \"zoom.us\"\n", filePackets[i].id);
                printText(buffer);
                free(buffer);
            }
            else
            {
                // Add packet to the vlan queue
                SEM_signal(&sVlans[BOARDS_ID]);
                asprintf(&buffer, "Packet with id %d added to Pissarres VLAN queue.\n", filePackets[i].id);
                printText(buffer);
                free(buffer);
            }
        }
        else if (strcmp(filePackets[i].vlan, "PAS-PDI") == 0)
        {
            // Packet in PAS-PDI VLAN
            // Add packet to the vlan queue
            SEM_signal(&sVlans[PDI_PAS_ID]);
            asprintf(&buffer, "Packet with id %d added to PAS-PDI VLAN queue.\n", filePackets[i].id);
            printText(buffer);
            free(buffer);
        }
        else if (strcmp(filePackets[i].vlan, "Alumnes") == 0)
        {
            // Packet in Alumnes VLAN
            if (strstr(filePackets[i].link, "itexamanswers.net") != NULL)
            {
                asprintf(&buffer, "Student %s may be cheating in XAL. Droping packet and notifying teachers.\n", filePackets[i].username);
                printText(buffer);
                free(buffer);
            }
            else
            {
                // Add packet to the vlan queue
                SEM_signal(&sVlans[STUDENTS_ID]);
                asprintf(&buffer, "Packet with id %d added to Alumnes VLAN queue.\n", filePackets[i].id);
                printText(buffer);
                free(buffer);
            }
        }
        else
        {
            // Packet in an unkown VLAN
            asprintf(&buffer, "Droping packet with id %d, unknown VLAN with name %s.\n", filePackets[i].id, filePackets[i].vlan);
            printText(buffer);
            free(buffer);
        }
    }

    while (1)
    {
        pause();
    }
}