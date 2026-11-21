#ifndef PERFORMANCE_RUN_MEASUREMENT_HPP
#define PERFORMANCE_RUN_MEASUREMENT_HPP

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cerrno>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

namespace performance{

struct RunMeasurement{
    long long elapsed_nanoseconds;
    size_t peak_rss_delta_bytes;
    bool limit_exceeded;
};

inline size_t read_rss_bytes(std::ifstream& statm){
    size_t total_pages, resident_pages;
    statm.clear();
    statm.seekg(0);
    if(!(statm >> total_pages >> resident_pages)){
        throw std::runtime_error("Nao foi possivel ler /proc/self/statm.");
    }

    const long page_size = sysconf(_SC_PAGESIZE);
    if(page_size <= 0){
        throw std::runtime_error("Nao foi possivel obter o tamanho da pagina.");
    }
    return resident_pages * static_cast<size_t>(page_size);
}

template<typename Function>
RunMeasurement measure_run_in_process(Function&& function){
    std::atomic<bool> ready(false);
    std::atomic<bool> started(false);
    std::atomic<bool> stop(false);
    std::atomic<bool> sampling_failed(false);
    std::atomic<size_t> peak_delta(0);
    size_t baseline = 0;

    std::thread sampler([&]{
        try{
            std::ifstream statm("/proc/self/statm");
            if(!statm){
                throw std::runtime_error("Nao foi possivel abrir /proc/self/statm.");
            }
            baseline = read_rss_bytes(statm);
            ready.store(true, std::memory_order_release);

            while(!started.load(std::memory_order_acquire)){
                std::this_thread::yield();
            }

            const auto sample = [&]{
                const size_t current = read_rss_bytes(statm);
                if(current <= baseline){
                    return;
                }
                const size_t delta = current - baseline;
                size_t observed = peak_delta.load(std::memory_order_relaxed);
                while(observed < delta &&
                      !peak_delta.compare_exchange_weak(
                          observed, delta, std::memory_order_relaxed
                      )){}
            };

            while(!stop.load(std::memory_order_acquire)){
                sample();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            sample();
        } catch(...){
            sampling_failed.store(true, std::memory_order_release);
            ready.store(true, std::memory_order_release);
        }
    });

    while(!ready.load(std::memory_order_acquire)){
        std::this_thread::yield();
    }
    if(sampling_failed.load(std::memory_order_acquire)){
        sampler.join();
        throw std::runtime_error("Falha ao amostrar a memoria residente.");
    }

    started.store(true, std::memory_order_release);
    const auto start = std::chrono::steady_clock::now();
    std::exception_ptr run_error;
    bool limit_exceeded = false;
    try{
        if constexpr(std::is_void_v<std::invoke_result_t<Function&>>){
            function();
        } else{
            limit_exceeded = function();
        }
    } catch(...){
        run_error = std::current_exception();
    }
    const auto end = std::chrono::steady_clock::now();
    stop.store(true, std::memory_order_release);
    sampler.join();

    if(run_error){
        std::rethrow_exception(run_error);
    }
    if(sampling_failed.load(std::memory_order_acquire)){
        throw std::runtime_error("Falha ao amostrar a memoria residente.");
    }

    return {
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count(),
        peak_delta.load(std::memory_order_relaxed),
        limit_exceeded
    };
}

template<typename Function>
RunMeasurement measure_run(Function&& function){
    int pipe_fds[2];
    if(pipe(pipe_fds) != 0){
        throw std::runtime_error("Nao foi possivel criar pipe para a medicao.");
    }

    const pid_t child = fork();
    if(child < 0){
        close(pipe_fds[0]);
        close(pipe_fds[1]);
        throw std::runtime_error("Nao foi possivel criar processo de medicao.");
    }

    if(child == 0){
        close(pipe_fds[0]);
        try{
            const RunMeasurement result =
                measure_run_in_process(std::forward<Function>(function));
            const char* data = reinterpret_cast<const char*>(&result);
            size_t remaining = sizeof(result);
            while(remaining > 0){
                const ssize_t written = write(pipe_fds[1], data, remaining);
                if(written < 0 && errno == EINTR){
                    continue;
                }
                if(written <= 0){
                    _exit(1);
                }
                data += written;
                remaining -= static_cast<size_t>(written);
            }
            close(pipe_fds[1]);
            _exit(0);
        } catch(...){
            close(pipe_fds[1]);
            _exit(1);
        }
    }

    close(pipe_fds[1]);
    RunMeasurement result{};
    char* data = reinterpret_cast<char*>(&result);
    size_t received = 0;
    bool read_failed = false;
    while(received < sizeof(result)){
        const ssize_t count =
            read(pipe_fds[0], data + received, sizeof(result) - received);
        if(count < 0 && errno == EINTR){
            continue;
        }
        if(count < 0){
            read_failed = true;
            break;
        }
        if(count == 0){
            break;
        }
        received += static_cast<size_t>(count);
    }
    close(pipe_fds[0]);

    int status = 0;
    while(waitpid(child, &status, 0) < 0){
        if(errno != EINTR){
            throw std::runtime_error("Falha ao aguardar processo de medicao.");
        }
    }
    if(read_failed || received != sizeof(result) ||
       !WIFEXITED(status) || WEXITSTATUS(status) != 0){
        throw std::runtime_error("Processo de medicao falhou.");
    }
    return result;
}

}

#endif
