#include "fmt/core.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <ratio>
#include <thread>
#include <vector>
#include <folly/ProducerConsumerQueue.h>
const int QUEUESIZE = 1000;

const int num_threads = 4;       // 线程总数
std::atomic<int> sync_counter(0);// 同步计数器
std::atomic<int> sync_marks(0);
std::atomic<bool> finish(false);
std::mutex cv_mutex;
std::condition_variable cv;

struct Info {
    bool is_sync_point;
    int data;
    int index;
};

template<typename T>
using QueueSp = std::shared_ptr<folly::ProducerConsumerQueue<T>>;

std::function<void(std::thread::id)> callback_func = [](std::thread::id id) {
    std::cout << id << " called" << std::endl;
    sync_marks--;
    //fmt::print("sync_marks {}\n", sync_marks);
};
std::atomic<bool> ready(false);

void process_message(QueueSp<Info> queue_sp) {
    while (true) {
        if (!queue_sp->isEmpty()) {
            Info *info = queue_sp->frontPtr();
            if (info->is_sync_point) {
                std::unique_lock<std::mutex> lock(cv_mutex);
                sync_counter++;
                if (sync_counter == num_threads) {
                    // 所有线程都到达了当前同步节点
                    if (callback_func) {
                        callback_func(std::this_thread::get_id());
                    }
                    ready.store(true);
                    sync_counter--;
                    while (true) {
                        if (sync_counter == 0 && !ready.load()) {
                            lock.unlock();
                            break;
                        }
                        std::this_thread::yield();
                    }
                    std::cout << "pass" << std::endl;
                } else {
                    lock.unlock();
                    while (ready.load() == false) {
                        std::this_thread::yield();
                    }
                    // 减少计数，表示已通过此同步点
                    sync_counter--;
                    fmt::print("sync_counter {}\n", sync_counter);
                    if (sync_counter == 0) {
                        ready.store(false);// Reset for the next sync point
                    }
                }
            }
            queue_sp->popFront();
        }
        if (sync_marks.load() == 0) {
            break;
        }
    }
}


int main() {
    std::vector<Info> messages;
    // 模拟生成消息和同步点
    for (int i = 0; i < 20; ++i) {
        bool is_sync_point = (i % 5 == 0);
        if (is_sync_point) {
            sync_marks++;
            //fmt::print("sync_marks {}\n", sync_marks);
        }
        messages.push_back({
                .is_sync_point =  is_sync_point,
                .data = i,
        });// 每5个消息设一个同步点
    }

    std::vector<QueueSp<Info>> queue_sps;
    for (auto i = 0; i < num_threads; i++) {
        QueueSp<Info> ptr = std::make_shared<folly::ProducerConsumerQueue<Info>>(QUEUESIZE);
        queue_sps.push_back(std::move(ptr));
    }
    for (auto &info: messages) {
        for (auto &sp: queue_sps) {
            sp->write(info);
        }
    }

    std::vector<std::thread> workers;
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(process_message, queue_sps[i]);
    }

    for (auto &t: workers) {
        t.join();
    }

    std::cout << "All messages processed." << std::endl;
    return 0;
}
