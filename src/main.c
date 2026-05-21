#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include "queue.h"
#include "scanner.h"
#include "worker.h"
#include "log.h"

TaskQueue file_queue;

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Kullanim: %s <thread_sayisi> <kaynak_dizin> <hedef_dizin>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int thread_count = atoi(argv[1]);
    if (thread_count <= 0)
    {
        fprintf(stderr, "Hata: thread_sayisi pozitif bir tamsayi olmali.\n");
        return EXIT_FAILURE;
    }

    const char *src_dir = argv[2];
    const char *dest_dir = argv[3];

    log_init("copy_tool.log");
    log_event("INFO", "Arac baslatildi: src=%s dst=%s thread_sayisi=%d",
              src_dir, dest_dir, thread_count);

    queue_init(&file_queue);

    if (mkdir(dest_dir, 0755) == -1 && errno != EEXIST)
    {
        perror("Hedef dizin olusturulamadi");
        log_event("ERROR", "Hedef dizin olusturulamadi: %s", dest_dir);
    }

    pthread_t *workers = malloc(thread_count * sizeof(pthread_t));
    if (!workers)
    {
        perror("Bellek tahsis hatasi");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < thread_count; i++)
    {
        if (pthread_create(&workers[i], NULL, worker_thread, &file_queue) != 0)
        {
            perror("Worker thread olusturulamadi");
            log_event("ERROR", "Worker thread %d olusturulamadi", i);
            free(workers);
            return EXIT_FAILURE;
        }
    }

    printf("--- Tarama Basliyor (%d worker thread) ---\n", thread_count);

    scan_directory(src_dir, dest_dir, &file_queue);

    pthread_mutex_lock(&file_queue.lock);
    file_queue.shutdown = 1;
    pthread_cond_broadcast(&file_queue.not_empty);
    pthread_mutex_unlock(&file_queue.lock);

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(workers[i], NULL);
    }

    pthread_mutex_destroy(&file_queue.lock);
    pthread_cond_destroy(&file_queue.not_empty);
    pthread_cond_destroy(&file_queue.not_full);

    free(workers);

    printf("--- Tum Islemler Basariyla Tamamlandi ---\n");
    log_event("INFO", "Tum islemler tamamlandi");
    log_close();

    return 0;
}
