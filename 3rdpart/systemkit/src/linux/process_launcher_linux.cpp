#include "process_launcher_linux.hpp"

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace leigod {
namespace manager {

bool ProcessLauncherLinux::startProcess(const std::string& executablePath, const std::string& args,
                                        bool hideWindow) {
    if (!startWithForkExec(executablePath, args, hideWindow)) {
        return startWithShell(executablePath, args, hideWindow);
    }
    return true;
}

bool ProcessLauncherLinux::startWithForkExec(const std::string& executablePath,
                                             const std::string& args, bool hideWindow) {
    pid_t pid = fork();
    if (pid == 0) {
        // 子进程
        if (hideWindow) {
            setsid();  // 创建新会话，脱离终端
        }

        std::vector<const char*> c_args;
        c_args.push_back(executablePath.c_str());
        c_args.push_back(args.c_str());
        c_args.push_back(nullptr);

        execvp(executablePath.c_str(), const_cast<char* const*>(c_args.data()));
        _exit(EXIT_FAILURE);  // exec 失败
    } else if (pid < 0) {
        return false;  // fork 失败
    } else {
        // 父进程
        int status;
        waitpid(pid, &status, 0);
        return WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
}

bool ProcessLauncherLinux::startWithShell(const std::string& executablePath,
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