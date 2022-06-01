#include <kranewl/exec.hh>

extern "C" {
#include <unistd.h>
}

#include <cstdlib>
#include <string>

void
exec_external(std::string const& command)
{
    if (!fork()) {
        dup2(STDERR_FILENO, STDOUT_FILENO);
        setsid();
        execl("/bin/sh", "/bin/sh", "-c", ("exec " + command).c_str(), nullptr);
        _exit(EXIT_FAILURE);
    }
}
