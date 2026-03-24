// Copyright 2026 Jordan Weinstein
// Imports
#include "domain_socket.h"
#include "file_reader.h"
#include "sha_solver.h"
#include "thread_log.h"
#include "timings.h"

#include <pthread.h>
#include <csignal>

#include <vector>

void ParseMessage(const std::string& msg,
    std::string* client_addr,
    std::vector<std::string>* file_paths,
    std::vector<std::uint32_t>* rows_per_file) {
        const char* c_ptr = msg.data();
}

std::sig_atomic_t term = 1;

void* StartRoutine(void* arg);

int main(int argc, char* argv[]) {
    
}

void* StartRoutine(void* arg){

}