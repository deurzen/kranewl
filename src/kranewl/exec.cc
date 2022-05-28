#include <cstdlib>
#include <string>
extern "C" {
#include <unistd.h>
}

void
exec_external(std::string& command) {
    if (!fork()) {
        dup2(STDERR_FILENO, STDOUT_FILENO);
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", ("exec " + command).c_str(), nullptr);
        _exit(EXIT_FAILURE);
    }
}
