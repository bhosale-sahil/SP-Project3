/*
 * network.c
 * Sahil Bhosale: HTTP Networking and Download Operations
 * Responsibilities: HTTP requests, file size detection, chunk downloading, retry logic
 */

#include "download_manager.h"

// Callback to write downloaded data to file
size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
    DownloadChunk *chunk = (DownloadChunk *)userdata;
    size_t total = size * nmemb;

    // Thread-safe file writing with mutex
    pthread_mutex_lock(chunk->mutex);

    // Seek to correct position in file
    fseek(chunk->file, chunk->start_byte + chunk->bytes_downloaded, SEEK_SET);

    // Write data to file
    size_t written = fwrite(ptr, 1, total, chunk->file);
    fflush(chunk->file);

    // Update bytes downloaded counter
    chunk->bytes_downloaded += written;

    pthread_mutex_unlock(chunk->mutex);

    return written;
}

// Get file size using HTTP HEAD request
long get_file_size(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "ERROR: Failed to initialize CURL\n");
        return -1;
    }

    double content_length = 0;

    // Configure CURL for HEAD request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);           // HEAD request only
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);   // Follow redirects
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);  // 30 sec connection timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);         // 60 sec total timeout

    // Perform the request
    CURLcode res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        // Get content length from response
        curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &content_length);
    } else {
        fprintf(stderr, "ERROR: Failed to get file size: %s\n", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_cleanup(curl);

    if (content_length <= 0) {
        fprintf(stderr, "ERROR: Invalid file size\n");
        return -1;
    }

    return (long)content_length;
}

// Download a specific chunk with retry logic
int download_chunk(DownloadChunk *chunk) {
    int retry_count = 0;
    int max_retries = RETRY_LIMIT;

    while (retry_count < max_retries) {
        CURL *curl = curl_easy_init();
        if (!curl) {
            fprintf(stderr, "ERROR: Thread %d failed to initialize CURL\n", chunk->thread_id);
            return -1;
        }

        // Prepare byte range string (e.g., "0-1048575")
        char range[256];
        snprintf(range, sizeof(range), "%ld-%ld", chunk->start_byte, chunk->end_byte);

        // Configure CURL for range request
        curl_easy_setopt(curl, CURLOPT_URL, chunk->url);
        curl_easy_setopt(curl, CURLOPT_RANGE, range);              // Byte range
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // Data callback
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, chunk);          // Pass chunk info
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);        // Follow redirects
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);       // Connection timeout
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);             // Total timeout
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);              // Thread safety

        // Perform the download
        CURLcode res = curl_easy_perform(curl);

        // Check if successful
        if (res == CURLE_OK) {
            curl_easy_cleanup(curl);
            return 0;  // Success
        }

        // Download failed - retry logic
        retry_count++;
        fprintf(stderr, "WARNING: Thread %d retry %d/%d (Error: %s)\n",
                chunk->thread_id, retry_count, max_retries, curl_easy_strerror(res));

        curl_easy_cleanup(curl);

        // Wait before retry (exponential backoff)
        if (retry_count < max_retries) {
            sleep(retry_count);  // Wait 1, 2, 3 seconds
        }
    }

    // All retries failed
    fprintf(stderr, "ERROR: Thread %d failed after %d retries\n", 
            chunk->thread_id, max_retries);
    return -1;
}
