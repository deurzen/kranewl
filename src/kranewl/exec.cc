#include <unistd.h>
#include <cstdlib>
#include <string>

void
exec_external(std::string& command) {
    if (!fork()) {
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", ("exec " + command).c_str(), NULL);
        exit(EXIT_SUCCESS);
    }
}
