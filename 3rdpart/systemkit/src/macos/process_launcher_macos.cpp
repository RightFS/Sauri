#include "process_launcher_macos.hpp"

#include <cstdio>
#include <cstdlib>
#include <spawn.h>
#include <sstream>
#include <sys/wait.h>

extern char** environ;

namespace leigod {
namespace manager {

bool ProcessLauncherMacOS::startProcess(const std::string& executablePath, const std::string& args,
                                        bool hideWindow) {
    if (!startWithPosixSpawn(executablePath, args, hideWindow)) {
        return startWithShell(executablePath, args, hideWindow);
    }
    return true;
}

bool ProcessLauncherMacOS::startWithPosixSpawn(const std::string& executablePath,
                                               const std::string& args, bool hideWindow) {
    pid_t pid;
    std::vector<const char*> c_args;
    c_args.push_back(executablePath.c_str());
    c_args.push_back(args.c_str());
    c_args.push_back(nullptr);

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    if (hideWindow) {
        posix_spawnattr_setflags(&attr, POSIX_SPAWN_SETSID);
    }

    if (posix_spawn(&pid, executablePath.c_str(), nullptr, &attr,
                    const_cast<char* const*>(c_args.data()), environ) != 0) {
        posix_spawnattr_destroy(&attr);
        return false;
    }

    posix_spawnattr_destroy(&attr);

    int status;
    if (waitpid(pid, &status, 0) != -1) {
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    return false;
}

bool ProcessLauncherMacOS::startWithShell(const std::string& executablePath,
                                          const std::string& args, bool hideWindow) {
    std::ostringstream command;
    command << "\"" << executablePath << "\"";
    command << " \"" << args << "\"";
    if (hideWindow) {
        command << " & disown";
    }

    FILE* pipe = popen(command.str().c_str(), "r");
    if (!pipe) {
        return false;
    }
    int status = pclose(pipe);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

}  // namespace manager
}  // namespace leigod