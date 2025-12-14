/*
 * threads.c
 * Jyotsna Pathak: Thread Management and Parallel Execution
 * Responsibilities: Thread creation, synchronization, coordination, logging
 */

#include "download_manager.h"

// Thread worker function - runs for each download thread
void* download_worker(void *arg) {
    DownloadChunk *chunk = (DownloadChunk *)arg;

    // Record start time
    gettimeofday(&chunk->start_time, NULL);

    // Calculate chunk size in MB
    double chunk_size_mb = (chunk->end_byte - chunk->start_byte + 1) / (1024.0 * 1024.0);

    // Print thread start log
    printf("\n[THREAD %d START] Time: 0.000 sec | Bytes: %ld-%ld (%.2f MB)\n",
           chunk->thread_id,
           chunk->start_byte, 
           chunk->end_byte,
           chunk_size_mb);

    // Download the chunk
    int result = download_chunk(chunk);

    // Record end time
    gettimeofday(&chunk->end_time, NULL);

    // Calculate duration
    double duration = (chunk->end_time.tv_sec - chunk->start_time.tv_sec) +
                     (chunk->end_time.tv_usec - chunk->start_time.tv_usec) / 1000000.0;

    // Print thread end log
    if (result == 0) {
        printf("[THREAD %d END  ] Time: %.3f sec | Downloaded: %.2f MB | Duration: %.2f sec\n",
               chunk->thread_id,
               duration,
               chunk->bytes_downloaded / (1024.0 * 1024.0),
               duration);
    } else {
        printf("[THREAD %d FAILED] Duration: %.2f sec | Status: Error\n", 
               chunk->thread_id, duration);
    }

    return NULL;
}

// Initialize and start all download threads
int start_download_threads(DownloadManager *manager) {
    long chunk_size = manager->file_size / manager->num_threads;
    long remaining = manager->file_size % manager->num_threads;

    // Print download initialization header
    printf("\n========================================\n");
    printf("STARTING PARALLEL DOWNLOAD\n");
    printf("========================================\n");
    printf("Threads: %d\n", manager->num_threads);
    printf("File Size: %.2f MB\n", manager->file_size / (1024.0 * 1024.0));
    printf("Chunk Size: %.2f MB per thread\n", chunk_size / (1024.0 * 1024.0));
    printf("========================================\n");

    // Allocate memory for threads array
    manager->threads = malloc(sizeof(pthread_t) * manager->num_threads);
    if (!manager->threads) {
        fprintf(stderr, "ERROR: Failed to allocate memory for threads\n");
        return -1;
    }

    // Allocate memory for chunks array
    manager->chunks = malloc(sizeof(DownloadChunk) * manager->num_threads);
    if (!manager->chunks) {
        fprintf(stderr, "ERROR: Failed to allocate memory for chunks\n");
        free(manager->threads);
        return -1;
    }

    // Initialize mutex for thread synchronization
    if (pthread_mutex_init(&manager->mutex, NULL) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize mutex\n");
        free(manager->threads);
        free(manager->chunks);
        return -1;
    }

    // Open file for writing (shared by all threads)
    FILE *file = fopen(manager->output_file, "r+b");
    if (!file) {
        fprintf(stderr, "ERROR: Cannot open file for writing\n");
        pthread_mutex_destroy(&manager->mutex);
        free(manager->threads);
        free(manager->chunks);
        return -1;
    }

    // Create and configure each thread
    long offset = 0;
    for (int i = 0; i < manager->num_threads; i++) {
        // Initialize chunk data
        manager->chunks[i].thread_id = i;
        manager->chunks[i].url = manager->url;
        manager->chunks[i].start_byte = offset;

        // Calculate end byte (distribute remaining bytes to first threads)
        long this_chunk_size = chunk_size + (i < remaining ? 1 : 0);
        manager->chunks[i].end_byte = offset + this_chunk_size - 1;

        manager->chunks[i].bytes_downloaded = 0;
        manager->chunks[i].file = file;
        manager->chunks[i].mutex = &manager->mutex;

        // Create the thread
        if (pthread_create(&manager->threads[i], NULL, download_worker, 
                          &manager->chunks[i]) != 0) {
            fprintf(stderr, "ERROR: Failed to create thread %d\n", i);
            fclose(file);
            pthread_mutex_destroy(&manager->mutex);
            free(manager->threads);
            free(manager->chunks);
            return -1;
        }

        offset += this_chunk_size;
    }

    return 0;
}

// Wait for all threads to complete
void wait_for_threads(DownloadManager *manager) {
    printf("\n----------------------------------------\n");
    printf("Waiting for threads to complete...\n");
    printf("----------------------------------------\n\n");

    // Join all threads (wait for completion)
    for (int i = 0; i < manager->num_threads; i++) {
        pthread_join(manager->threads[i], NULL);
    }

    // Close the shared file handle
    if (manager->chunks && manager->chunks[0].file) {
        fclose(manager->chunks[0].file);
    }

    printf("\n----------------------------------------\n");
    printf("All threads completed\n");
    printf("----------------------------------------\n");
}
