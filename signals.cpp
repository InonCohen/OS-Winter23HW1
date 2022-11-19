#include <iostream>
#include <csignal>
#include <cassert>
#include "signals.h"

using namespace std;

void ctrlZHandler(int sig_num) { //sig_num == SIGSTOP
    std::cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    smash.getJobsList().removeFinishedJobs();
    Command* fg_cmd = smash.getCurrentFGCommand();
    if(!fg_cmd){
//        std::cout << "no fg command" << std::endl;
        return;
    }
    pid_t fg_pid = smash.getCurrentFGpid();
    assert(fg_pid != -1);
    assert(fg_pid != smash.getSmashPid());
    JobEntry* job = smash.getCurrentFGJobPtr();
    if(!job){ //job is null
        if(kill(fg_pid, SIGSTOP) == 0){
            std::cout << "smash: process " << fg_pid << " was stopped" << endl;
            smash.getJobsList().addJob(fg_cmd,fg_pid ,true);
        }
        else{
            perror("smash error: kill failed");
        }
    }
    else {
        if (fg_pid == smash.getSmashPid()) {
            smash.getCurrentFGJobPtr()->stop();
            std::cout << "smash: process " << fg_pid << " was stopped" << endl;
        }
        else {
            if (kill(fg_pid, SIGSTOP) == 0) {
                std::cout << "smash: process " << fg_pid << " was stopped" << endl;
                if (fg_cmd->getJobEntryId() == -1) {
                    smash.getJobsList().addJob(fg_cmd, fg_pid, true);  //TODO: should we add even if the kill failed?
                }
                else {
                    smash.getCurrentFGJobPtr()->stop();
                }
            }
            else {
                perror("smash error: kill failed");
            }
        }
    }
    smash.clearFG();
}




void ctrlCHandler(int sig_num) { //sig_num == SIGINT
    std::cout << "smash: got ctrl-C" << endl;
    pid_t fg_pid = -1;
    Command* fg_cmd = SmallShell::getInstance().getCurrentFGCommand();
    if (fg_cmd) {
        fg_pid = SmallShell::getInstance().getCurrentFGpid();
        if(fg_pid == SmallShell::getInstance().getSmashPid()){
            SmallShell::getInstance().getCurrentFGJobPtr()->finish();
            SmallShell::getInstance().clearFG();
            return;
        }
        else{
            if (kill(fg_pid, SIGKILL)==0){
                std::cout << "smash: process "<< fg_pid << " was killed" << endl;
                if(fg_cmd->isTimed())
                    SmallShell::getInstance().getTimedJobsList().getTimedJobById(fg_cmd->getTimedJobId())->finish();
            }
            else {
                perror("smash error: kill failed");//TODO: check if necessary
            }
        }
    }
    //else: no process is running in the fg so no killing will be done

}



void alarmHandler(int sig_num) {
    // 1. ● print the following information: “smash got an alarm”
    std::cout << "smash: got an alarm" << endl;
    // 2. ● search which command caused the alarm, send a SIGKILL to its process and print:
    SmallShell& smash = SmallShell::getInstance();
    TimedJobsList* timed_jobs = &smash.getTimedJobsList();
    assert(timed_jobs != nullptr);
    TimedJobEntry* job_to_kill = timed_jobs->getSoonest();
    if(job_to_kill != nullptr){
        pid_t pid_to_kill = job_to_kill->getPid();
        const char* cmd_to_print = job_to_kill->getCommand()->getTimeoutCommand();
        if(kill(pid_to_kill, 0)==0) {
            if (kill(pid_to_kill, SIGKILL) == 0) {
                if(!(job_to_kill->isFinished())) {
                    job_to_kill->finish();
                    cout << "smash: " << cmd_to_print << " timed out!" << endl;
                }
            }
            else {
                perror("smash error: kill failed");
            }
        }
        smash.getJobsList().updateMax();
        timed_jobs->removeJobById(job_to_kill->getId());// updates soonest
    }
}
