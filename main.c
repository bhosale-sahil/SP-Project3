/*
 * main.c
 * Keval Shah: File Operations, Main Program, and Utilities
 * Responsibilities: File I/O, verification, logging, cleanup, main flow
 */

#include "download_manager.h"

void print_usage(char *program) {
    printf("\nUsage: %s -u <URL> -o <output> -t <threads>\n", program);
    printf("Options: -u URL  -o output  -t threads (1-16)\n\n");
}

int create_file(const char *filename, long size) {
    FILE *file = fopen(filename, "wb");
    if (!file) return -1;
    fseek(file, size - 1, SEEK_SET);
    fputc(0, file);
    fclose(file);
    printf("File created: %s (%.2f MB)\n", filename, size / (1024.0 * 1024.0));
    return 0;
}

int verify_file(const char *filename, long expected_size) {
    struct stat st;
    if (stat(filename, &st) != 0) return -1;

    if (st.st_size == expected_size) {
        printf("Verification PASSED: %ld bytes\n", (long)st.st_size);
        return 0;
    }
    fprintf(stderr, "Verification FAILED\n");
    return -1;
}

void cleanup(DownloadManager *manager) {
    if (manager->threads) free(manager->threads);
    if (manager->chunks) free(manager->chunks);
    pthread_mutex_destroy(&manager->mutex);
}

void print_logs(DownloadManager *manager) {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    double total = (end_time.tv_sec - manager->start_time.tv_sec) +
                   (end_time.tv_usec - manager->start_time.tv_usec) / 1000000.0;

    printf("\n========================================\n");
    printf("        DOWNLOAD SUMMARY\n");
    printf("========================================\n");
    printf("File: %s\n", manager->output_file);
    printf("Size: %.2f MB\n", manager->file_size / (1024.0 * 1024.0));
    printf("Threads: %d\n", manager->num_threads);
    printf("Total Time: %.2f sec\n", total);
    printf("Speed: %.2f MB/s\n", (manager->file_size / (1024.0 * 1024.0)) / total);
    printf("========================================\n\n");

    printf("THREAD DETAILS:\n");
    printf("Thread | Start | End   | Duration | Data\n");
    printf("----------------------------------------\n");

    for (int i = 0; i < manager->num_threads; i++) {
        double s = (manager->chunks[i].start_time.tv_sec - manager->start_time.tv_sec) +
                   (manager->chunks[i].start_time.tv_usec - manager->start_time.tv_usec) / 1000000.0;
        double e = (manager->chunks[i].end_time.tv_sec - manager->start_time.tv_sec) +
                   (manager->chunks[i].end_time.tv_usec - manager->start_time.tv_usec) / 1000000.0;

        printf("  %2d   | %5.2fs | %5.2fs | %6.2fs | %.2f MB\n",
               i, s, e, e - s, manager->chunks[i].bytes_downloaded / (1024.0 * 1024.0));
    }
    printf("----------------------------------------\n\n");
}

int main(int argc, char *argv[]) {
    DownloadManager manager = {0};
    manager.num_threads = 4;

    printf("\n║   Multi-threaded Download Manager     ║\n");
    
    int opt;
    while ((opt = getopt(argc, argv, "u:o:t:h")) != -1) {
        switch (opt) {
            case 'u': manager.url = optarg; break;
            case 'o': manager.output_file = optarg; break;
            case 't': manager.num_threads = atoi(optarg); break;
            case 'h': print_usage(argv[0]); return 0;
            default: print_usage(argv[0]); return 1;
        }
    }

    if (!manager.url || !manager.output_file) {
        print_usage(argv[0]);
        return 1;
    }

    printf("URL: %s\n", manager.url);
    printf("Output: %s\n", manager.output_file);
    printf("Threads: %d\n\n", manager.num_threads);

    curl_global_init(CURL_GLOBAL_ALL);
    gettimeofday(&manager.start_time, NULL);

    printf("Getting file size...\n");
    manager.file_size = get_file_size(manager.url);
    if (manager.file_size <= 0) {
        curl_global_cleanup();
        return 1;
    }
    printf("File size: %.2f MB\n\n", manager.file_size / (1024.0 * 1024.0));

    if (create_file(manager.output_file, manager.file_size) != 0) {
        curl_global_cleanup();
        return 1;
    }

    if (start_download_threads(&manager) != 0) {
        cleanup(&manager);
        curl_global_cleanup();
        return 1;
    }

    wait_for_threads(&manager);

    printf("\nVerifying download...\n");
    verify_file(manager.output_file, manager.file_size);

    print_logs(&manager);
    cleanup(&manager);
    curl_global_cleanup();

    printf("Download completed!\n\n");
    return 0;
}
