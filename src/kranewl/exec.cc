#include <cstdlib>
#include <string>
extern "C" {
#include <unistd.h>
}

void
exec_external(std::string& command) {
    if (!fork()) {
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", ("exec " + command).c_str(), NULL);
        std::exit(EXIT_SUCCESS);
    }
}
