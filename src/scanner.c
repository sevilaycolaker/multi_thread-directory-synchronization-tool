#include "queue.h"
#include "log.h"

void scan_directory(const char *root_src, const char *root_dst, TaskQueue *task_q)
{
    DirQueue dq;
    dir_queue_init(&dq);
    dir_queue_push(&dq, root_src, root_dst);

    char current_src[PATH_MAX];
    char current_dst[PATH_MAX];

    while (dir_queue_pop(&dq, current_src, current_dst))
    {
        DIR *dir = opendir(current_src);
        if (!dir)
        {
            perror("Dizin acilamadi");
            log_event("ERROR", "Dizin acilamadi: %s", current_src);
            continue;
        }

        struct dirent *entry;
        struct stat src_stat, dest_stat;

        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char src_path[PATH_MAX];
            char dest_path[PATH_MAX];

            if (snprintf(src_path, PATH_MAX, "%s/%s", current_src, entry->d_name) >= PATH_MAX)
                continue;
            if (snprintf(dest_path, PATH_MAX, "%s/%s", current_dst, entry->d_name) >= PATH_MAX)
                continue;

            if (lstat(src_path, &src_stat) == -1)
                continue;

            if (S_ISLNK(src_stat.st_mode))
                continue;

            if (S_ISDIR(src_stat.st_mode))
            {
                mkdir(dest_path, src_stat.st_mode & 0777);
                dir_queue_push(&dq, src_path, dest_path);
            }
            else if (S_ISREG(src_stat.st_mode))
            {
                int need_copy = 0;
                int is_update = 0;

                if (stat(dest_path, &dest_stat) == -1)
                {
                    need_copy = 1;
                    is_update = 0;
                }
                else if (src_stat.st_mtime > dest_stat.st_mtime ||
                         src_stat.st_size != dest_stat.st_size)
                {
                    need_copy = 1;
                    is_update = 1;
                }

                if (need_copy)
                {
                    CopyTask task;
                    strncpy(task.source_path, src_path, PATH_MAX - 1);
                    task.source_path[PATH_MAX - 1] = '\0';
                    strncpy(task.dest_path, dest_path, PATH_MAX - 1);
                    task.dest_path[PATH_MAX - 1] = '\0';
                    task.is_update = is_update;

                    printf("[Scanner] Kuyruga alindi (%s): %s\n",
                           is_update ? "UPDATE" : "COPY", src_path);
                    queue_push(task_q, task);
                }
            }
        }
        closedir(dir);
    }
}
