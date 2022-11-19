#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#include "signals.h"

SmallShell &smash = SmallShell::getInstance();
int main() {
    if (signal(SIGTSTP, ctrlZHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if (signal(SIGINT, ctrlCHandler) == SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    struct sigaction alarm;
    alarm.sa_handler = alarmHandler;
    alarm.sa_flags = SA_RESTART;
    if (sigaction(SIGALRM, &alarm, nullptr) == -1) {
        perror("smash error: failed to set alarm handler");
    }
    std::string cmd_line;
    while (true) {
        std::cout << smash.getPrompt();
        getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}
