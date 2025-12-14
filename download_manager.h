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

// Sahil Bhosale: HTTP and Network Functions
long get_file_size(const char *url);
int download_chunk(DownloadChunk *chunk);

// Jyotsna Pathak: Thread Management Functions
int start_download_threads(DownloadManager *manager);
void wait_for_threads(DownloadManager *manager);

// Keval Shah: File and Utility Functions
int create_file(const char *filename, long size);
int verify_file(const char *filename, long expected_size);
void cleanup(DownloadManager *manager);
void print_logs(DownloadManager *manager);

#endif
