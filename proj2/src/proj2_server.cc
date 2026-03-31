// Copyright 2026 Jordan Weinstein
#include "domain_socket.h"
#include "file_reader.h"
#include "sha_solver.h"
#include "thread_log.h"
#include "timings.h"

#include <pthread.h>
#include <csignal>
#include <cstring>
#include <vector>

volatile std::sig_atomic_t term = 1;

void signal_handler(int signal){
    term = 0;
}

void ParseMessage(const std::string& msg,
    std::string* client_addr,
    std::vector<std::string>* file_paths,
    std::vector<std::uint32_t>* rows_per_file) {
        const char* c_ptr = msg.data();
        size_t offset = 0;

        uint32_t reply_len;
        std::memcpy(&reply_len, c_ptr + offset, 4);
        offset += 4;
        *client_addr = std::string(c_ptr + offset, reply_len);
        offset += reply_len;

        uint32_t file_count;
        std::memcpy(&file_count, c_ptr + offset, 4);
        offset += 4;

        for (uint32_t i = 0; i < file_count; i++) {
            uint32_t path_len;
            std::memcpy(&path_len, c_ptr + offset, 4);
            offset += 4;
            file_paths->push_back(std::string(c_ptr + offset, path_len));
            offset += path_len;

            uint32_t row_count;
            std::memcpy(&row_count, c_ptr + offset, 4);
            offset += 4;
            rows_per_file->push_back(row_count);
        }
}

void* StartRoutine(void* arg);

int main(int argc, char* argv[]) {
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    proj2::FileReaders::Init(std::stoul(argv[2]));
    proj2::ShaSolvers::Init(std::stoul(argv[3]));
    proj2::UnixDomainDatagramEndpoint server(argv[1]);
    server.Init();

    while (term) {
        std::string temp_addr;
        std::string request = server.RecvFrom(&temp_addr, 65535);

        if (!request.empty()) {
            std::string* thread = new std::string(std::move(request));
            pthread_t tid;
            if (pthread_create(&tid, nullptr, StartRoutine, thread) == 0) {
                pthread_detach(tid);
            } else {
                delete thread;
            }
        }
    }

    return 0;
}

void* StartRoutine(void* arg){
    std::string* msg = static_cast<std::string*>(arg);
    std::string client_addr;
    std::vector<std::string> file_paths;
    std::vector<std::uint32_t> rows_per_file;

    ParseMessage(*msg, &client_addr, &file_paths, &rows_per_file);
    delete msg;

    uint32_t max_row_count = 0;
    for (uint32_t r : rows_per_file) {
        if (r > max_row_count) {
            max_row_count = r;
        }
    }

    proj2::SolverHandle sha_handler = proj2::ShaSolvers::Checkout(max_row_count);
    proj2::ReaderHandle reader_handler = proj2::FileReaders::Checkout(file_paths.size(), &sha_handler);

    std::vector<std::vector<proj2::ReaderHandle::HashType>> hash;
    hash.resize(file_paths.size());
    reader_handler.Process(file_paths, rows_per_file, &hash);

    std::string payload;
    for (const auto& f : hash) {
        for (const auto & h : f) {
            payload.append(h.data(), 64);
        }
    }

    proj2::UnixDomainStreamClient client(client_addr);
    client.Init();
    client.Write(payload.data(), payload.size());

    proj2::FileReaders::Checkin(std::move(reader_handler));
    proj2::ShaSolvers::Checkin(std::move(sha_handler));

    return nullptr;
}