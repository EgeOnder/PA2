#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include "webserver.h"

#define MAX_REQUEST 100
#define BUFFER_SIZE 20

int port, numThread;
int buffer[BUFFER_SIZE];
int in = 0, out = 0;

sem_t sem_full, sem_empty;
phread_mutex_t mutex;

void *listener()
{
	int r;
	struct sockaddr_in sin;
	struct sockaddr_in peer;
	int peer_len = sizeof(peer);
	int sock;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);
	r = bind(sock, (struct sockaddr *) &sin, sizeof(sin));
	if(r < 0) {
		perror("Error binding socket:");
		return;
	}

	r = listen(sock, 5);
	if(r < 0) {
		perror("Error listening socket:");
		return;
	}

	printf("HTTP server listening on port %d\n", port);
	while (1)
	{
		int s;
		s = accept(sock, NULL, NULL);
		if (s < 0) break;

		sem_wait(&sem_empty);
		pthread_mutex_lock(&mutex);

		buffer[in] = s;
		in = (in + 1) % BUFFER_SIZE;

		pthread_mutex_unlock(&mutex);
		sem_post(&sem_full);
	}

	close(sock);
	return NULL;
}

void *worker(void *arg) {
	while (1) {
		int s;

		sem_wait(&sem_full);
		pthread_mutex_lock(&mutex);

		s = buffer[out];
		out = (out + 1) % BUFFER_SIZE;

		pthread_mutex_unlock(&mutex);
		sem_post(&sem_empty);

		process(s);
	}

	return NULL;
}

void thread_control()
{
	pthread_t listener_thread;
	pthread_t worker_threads[MAX_REQUESTS];

	sem_init(&sem_full, 0, 0);
	sem_init(&sem_empty, 0, BUFFER_SIZE);
	pthread_mutex_init(&mutex, NULL);

	pthread_create(&listener_thread, NULL, listener, NULL);

	for (int i = 0; i < numThread; i++) {
		pthread_create(&worker_threads[i], NULL, worker, NULL);
	}

	pthread_join(listener_thread, NULL);

	for (int i = 0; i < numThread; i++) {
		pthread_join(worker_threads[i], NULL);
	}

	sem_destroy(&sem_full);
	sem_destroy(&sem_empty);
	pthread_mutex_destroy(&mutex);
}

int main(int argc, char *argv[])
{
	if(argc != 3 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
	{
		fprintf(stderr, "./webserver_multi PORT(2001 ~ 49999) #_of_threads(1-100)\n");
		return 0;
	}

	port = atoi(argv[1]);
	numThread = atoi(argv[2]);
	thread_control();
	return 0;
}
