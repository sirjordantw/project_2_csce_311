// Copyright 2026 Jordan Weinstein
// Headers
#include "domain_socket.h"
#include "file_reader.h"
#include "sha_solver.h"
#include "thread_log.h"
#include "timings.h"

// Libraries
#include <pthread.h>
#include <csignal>
#include <cstring>
#include <vector>

/*
 * Parses the message recieved from client.
 * @param msg : The message
 * @param client_addr : Address of client in memory
 * @param file_paths : Path of file
 * @param rows_per_file : Amount of lines in a file
 * (Used to tell the server how many lines need to be
 * processed.)
 */
void ParseMessage(const std::string& msg,
    std::string* client_addr,
    std::vector<std::string>* file_paths,
    std::vector<std::uint32_t>* rows_per_file) {
        // Pointer of msg bytes
        const char* c_ptr = msg.data();
        size_t offset = 0;

        // Gets client's address
        uint32_t reply_len;
        std::memcpy(&reply_len, c_ptr + offset, 4);
        offset += 4;
        *client_addr = std::string(c_ptr + offset, reply_len);
        offset += reply_len;

        // Gets file count
        uint32_t file_count;
        std::memcpy(&file_count, c_ptr + offset, 4);
        offset += 4;

        // Goes through each file from client
        for (uint32_t i = 0; i < file_count; i++) {
            // Gets the file path's length
            uint32_t path_len;
            std::memcpy(&path_len, c_ptr + offset, 4);
            offset += 4;
            // Gets the content within file
            file_paths->push_back(std::string(c_ptr + offset, path_len));
            offset += path_len;

            // Gets number of lines in file
            uint32_t row_count;
            std::memcpy(&row_count, c_ptr + offset, 4);
            offset += 4;
            rows_per_file->push_back(row_count);
        }
}

void* StartRoutine(void* arg);

int main(int argc, char* argv[]) {  
    // Initializes the server and its resources
    // based on arguments given by user.
    proj2::FileReaders::Init(std::stoul(argv[2]));
    proj2::ShaSolvers::Init(std::stoul(argv[3]));
    proj2::UnixDomainDatagramEndpoint server(argv[1]);
    server.Init();

    for (;;) {
        // Gets request from client.
        std::string temp_addr;
        std::string request = server.RecvFrom(&temp_addr, 65535);

        // Handles the request.
        if (!request.empty()) {
            // Makes the request a thread.
            std::string* thread = new std::string(std::move(request));
            pthread_t id;
            if (pthread_create(&id, nullptr, StartRoutine, thread) == 0) {
                // Cleans up the finished thread via its ID.
                pthread_detach(id);
            } else {
                // Handler if thread fails to be created.
                delete thread;
            }
        }
    }

    StopLog();
    return 0;
}

void* StartRoutine(void* arg){
    // Message parsing
    std::string* msg = static_cast<std::string*>(arg);
    std::string client_addr;
    std::vector<std::string> file_paths;
    std::vector<std::uint32_t> rows_per_file;

    ParseMessage(*msg, &client_addr, &file_paths, &rows_per_file);
    delete msg; // Deletes from memory once already parsed

    // Gets total number of rows
    uint32_t max_row_count = 0;
    for (uint32_t r : rows_per_file) {
        if (r > max_row_count) {
            max_row_count = r;
        }
    }

    // Gets a unique 64 hexadecimal string that came from the file's contents.
    proj2::SolverHandle sha_handler = proj2::ShaSolvers::Checkout(max_row_count);
    proj2::ReaderHandle reader_handler = proj2::FileReaders::Checkout(file_paths.size(), &sha_handler);

    // Prepars hash to hold size of results,
    // then fills the hash with 64-character strings.
    std::vector<std::vector<proj2::ReaderHandle::HashType>> hash;
    hash.resize(file_paths.size());
    reader_handler.Process(file_paths, rows_per_file, &hash);

    // Flattens all of hash into a single payload.
    std::string payload;
    for (const auto& f : hash) {
        for (const auto & h : f) {
            payload.append(h.data(), 64);
        }
    }

    // Writes payload back to client.
    proj2::UnixDomainStreamClient client(client_addr);
    client.Init();
    client.Write(payload.data(), payload.size());

    // Returns handlers for other threads in pool to use.
    proj2::FileReaders::Checkin(std::move(reader_handler));
    proj2::ShaSolvers::Checkin(std::move(sha_handler));

    return nullptr;
}