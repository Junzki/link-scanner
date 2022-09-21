// -*- coding:utf-8 -*-
#pragma once

#include <iostream>
#include <vector>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>

#ifndef LINK_SCANNER_SCANNER_H
#define LINK_SCANNER_SCANNER_H

typedef struct scan_context {
    std::queue<const char *> result;
    std::mutex               mutex;
} scan_context;

typedef struct scan_request {
    char           *path;
    scan_context   *context = nullptr;
} scan_request;


class scanner {

public:
    bool      alive;
    const int worker_limit = 4;

    scanner() {
        this->alive = true;

        for (int i = 0; i < worker_limit; ++i) {
            auto thread = this->init_worker();
            this->pool.push_back(std::move(thread));
        }
    }

    ~scanner() {
        this->alive = false;
        for (auto iter = this->pool.begin(); iter != this->pool.end(); ++iter) {
            iter->join();
        }
    }

    std::thread init_worker() {
        std::thread thread([this]() {
            while (this->alive) {
                if (this->queue.empty()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                }

                this->queue_lock.lock();
                auto front = this->queue.front();
                this->queue.pop();
                this->queue_lock.unlock();

                this->iterate_directory(front.context, front.path);
            }
        });

        return thread;
    }

    int stat_file(const char *);
    void iterate_directory(scan_context *, const char *);

    std::vector<const char *> scan(const char *);
    

    inline void submit(scan_request req) {
        this->queue_lock.lock();
        this->queue.push(req);
        this->queue_lock.unlock();
    }

protected:
    std::mutex               queue_lock;
    std::queue<scan_request> queue;
    std::vector<std::thread> pool;
};

#endif  // LINK_SCANNER_SCANNER_H
