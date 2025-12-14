#ifndef DOWNLOAD_MANAGER_H
#define DOWNLOAD_MANAGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_THREADS 16
#define RETRY_LIMIT 3

typedef struct {
    int thread_id;
    char *url;
    long start_byte;
    long end_byte;
    long bytes_downloaded;
    FILE *file;
    pthread_mutex_t *mutex;
    struct timeval start_time;
    struct timeval end_time;
} DownloadChunk;

typedef struct {
    char *url;
    char *output_file;
    long file_size;
    int num_threads;
    pthread_t *threads;
    DownloadChunk *chunks;
    pthread_mutex_t mutex;
    struct timeval start_time;
} DownloadManager;

#endif
