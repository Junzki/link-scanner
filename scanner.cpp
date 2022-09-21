// -*- coding:utf-8 -*-
#include <dirent.h>

#include "scanner.h"


int 
scanner::stat_file(const char *path) {
    static struct stat buf_;

    if (0 != stat(path, &buf_)) {
        return -errno;
    }

    if (buf_.st_nlink > 1) {
        return 1;
    }

    return 0;
}


void
scanner::iterate_directory(scan_context *context, 
                           const char   *path) {
    DIR           *dir;
    struct dirent *entry;
    char           buf[BUFSIZ];  // 4096 bytes

    dir = opendir(path);
    if (nullptr == dir)
        return;

    printf("Current directory: %s\n", path);

    char* next = nullptr;

    while(entry = readdir(dir)) {
        if (0 == strcmp(entry->d_name, ".") || 0 == strcmp(entry->d_name, ".."))
            continue;

        if (nullptr == next)
            next = new char[BUFSIZ];
        else 
            memset(next, 0, BUFSIZ);

        std::thread::id thread_id = std::this_thread::get_id();

        // join_path(next, path, entry->d_name);
        strcat(next, path);
        strcat(next, "/"); 
        strcat(next, entry->d_name);
        std::cout << "[Thread " << thread_id << "] " << "Next path: " << next << std::endl;

        // Submit task.
        if (DT_DIR == entry->d_type) {
            scan_request req = {
                .path    = next,
                .context = context
            };
            next = nullptr;
            this->submit(req);
            continue;
        }

        if (0 < this->stat_file(next)) {
            // Submit link
            context->mutex.lock();
            context->result.push(next);
            context->mutex.unlock();

            next = nullptr;
            continue;
        }
    }

    printf("Directory: %s cleaned\n", path);
    closedir(dir);

    if (nullptr != next) {
        delete[] next;
        next = nullptr;
    }
}


std::vector<const char *>
scanner::scan(const char *path) {
    std::vector<const char *> result;
    scan_context context;
    this->iterate_directory(&context, path);

    int   retries = 0;
    char *front;
    while (retries < 3) {
        if (context.result.empty()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            retries++;
            continue;
        }

        context.mutex.lock();
        front = (char *)context.result.front();
        context.result.pop();
        context.mutex.unlock();

        result.push_back(front);
    }

    return result;
}

int main(int argc, char *argv[]) {
    scanner scan;

    char *path;
    if (argc > 1) {
        path = argv[1];
    } else {
        path = new char[BUFSIZ];
        getcwd(path, BUFSIZ);
    }

    auto ret = scan.scan(path);

    FILE *out = fopen("out.txt", "w");

    for (auto *p : ret) {
        fputs(p, out);
        fputc('\n', out);
        printf("Collected: %s\n", p);
    }

    fclose(out);

    return 0;
}
