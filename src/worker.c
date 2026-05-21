#include "common.h"
#include "queue.h"
#include "log.h"

#define BUFFER_SIZE 4096

void *worker_thread(void *arg)
{
    TaskQueue *q = (TaskQueue *)arg;
    CopyTask task;

    while (queue_pop(q, &task))
    {
        printf("[Worker] Kopyalaniyor: %s\n", task.source_path);

        int src_fd = open(task.source_path, O_RDONLY);
        if (src_fd < 0)
        {
            perror("Kaynak dosya acilamadi");
            log_event("ERROR", "Kaynak dosya acilamadi: %s", task.source_path);
            continue;
        }

        int dest_fd = open(task.dest_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (dest_fd < 0)
        {
            perror("Hedef dosya acilamadi");
            log_event("ERROR", "Hedef dosya acilamadi: %s", task.dest_path);
            close(src_fd);
            continue;
        }

        char buffer[BUFFER_SIZE];
        ssize_t bytes_read, bytes_written;
        long total_bytes = 0;

        while ((bytes_read = read(src_fd, buffer, BUFFER_SIZE)) > 0)
        {
            char *out_ptr = buffer;
            ssize_t bytes_to_write = bytes_read;

            while (bytes_to_write > 0)
            {
                bytes_written = write(dest_fd, out_ptr, bytes_to_write);

                if (bytes_written < 0)
                {
                    perror("Yazma hatasi");
                    log_event("ERROR", "Yazma hatasi: %s", task.dest_path);
                    goto cleanup;
                }

                bytes_to_write -= bytes_written;
                out_ptr += bytes_written;
                total_bytes += bytes_written;
            }
        }

        if (bytes_read < 0)
        {
            perror("Okuma hatasi");
            log_event("ERROR", "Okuma hatasi: %s", task.source_path);
        }
        else
        {
            if (task.is_update)
                log_event("UPDATE", "%s yenilendi (%ld bayt)", task.dest_path, total_bytes);
            else
                log_event("COPY", "%s -> %s (%ld bayt)",
                          task.source_path, task.dest_path, total_bytes);

            printf("[Worker] Tamamlandi: %s\n", task.dest_path);
        }

    cleanup:
        close(src_fd);
        close(dest_fd);
    }
    return NULL;
}
