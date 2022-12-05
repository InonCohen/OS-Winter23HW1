#ifndef SMASH_COMMAND_H
#define SMASH_COMMAND_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <list>
#include <algorithm>
#include <memory>
#include <iomanip>
#include <cstring>
#include <ctime>
#include <csignal>
#include <unistd.h>
#include <utime.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/resource.h>


#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_PROCESSES (100)
#define MAX_COMMAND_LENGTH (80)


//TODO: change all perror into a function with an argument


const std::string WHITESPACE = " \n\r\t\f\v";

class SmallShell;
class JobsList;
class JobEntry;


class Command {
protected:
    int job_entry_id;
    int command_words;
    bool bg_cmd;
    char *original_command;
    std::string *command;
    bool timed;
    time_t cmd_alarm_time;
    char* timeout_command;
    char **parsed;
    int timed_job_id;
public:
    explicit Command(const char* cmd_line);
    Command(const char* cmd_line, const char* timeout_command_r, time_t cmd_alarm_time);

    virtual ~Command() = 0;

    virtual void execute() = 0;

    std::string getCommand() const;
    char **getParsed() const;

    bool isBG() const;

    void setBG(bool bg_status);

    int getParsedLength() const;

    int getJobEntryId() const;

    void setJobEntryId(int job_id);

    void trimAmpersand();

    static bool isPiped(const char *cmd_line);

    static bool isRedirection(const char *cmd_line);
    bool isTimed();
    virtual int getTime();
    void setTimed(bool timed_status);
    void setTimedJobId(int id);
    char* getTimedCommand(const char* cmd_line);
    void setTime(int time_to_set);
    int getTimedJobId() const;
    const char* getTimeoutCommand() const;
    char *getOriginalCommand() const;
};

class BuiltInCommand : public Command { // left 4 commands: fg, bg
        public:
        explicit BuiltInCommand(const char* cmd_line): Command(cmd_line){setBG(false);};
        virtual ~BuiltInCommand() = 0 ;
};

class ExternalCommand : public Command {
        public:
        explicit ExternalCommand(const char* cmd_line): Command(cmd_line){};
        ExternalCommand(const char* cmd_line, const char* timeout_command, time_t cmd_alarm_time = 0): Command(cmd_line, timeout_command, cmd_alarm_time){};
        virtual ~ExternalCommand()=default;
        void execute() override;
};

class PipeCommand : public Command {
        // TODO: Add your data members
        public:
        explicit PipeCommand(const char* cmd_line): Command(cmd_line){};
        PipeCommand(const char* cmd_line, char* timeout_command, time_t cmd_alarm_time = 0): Command(cmd_line, timeout_command, cmd_alarm_time){};
        virtual ~PipeCommand() = default;
        void execute() override;
};

class RedirectionCommand : public Command {
        // TODO: Add your data members
        public:
        explicit RedirectionCommand(const char* cmd_line): Command(cmd_line){};
        RedirectionCommand(const char* cmd_line, char* timeout_command, time_t cmd_alarm_time = 0): Command(cmd_line, timeout_command, cmd_alarm_time){};
        virtual ~RedirectionCommand() {};
        void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
        public:
        explicit ChangeDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~ChangeDirCommand() {}
        void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
        public:
        explicit GetCurrDirCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~GetCurrDirCommand() {}
        void execute() override;
};

class FindAndReplaceCommand : public BuiltInCommand {
        public:
        explicit FindAndReplaceCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~FindAndReplaceCommand() {}
        void execute() override;
};

class SetCoreCommand : public BuiltInCommand {
public:
    explicit SetCoreCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
    virtual ~SetCoreCommand() {}
    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
        public:
        explicit ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~ShowPidCommand()= default;
        void execute() override;
};

class ChangePromptCommand : public BuiltInCommand {
        public:
        explicit ChangePromptCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~ChangePromptCommand() = default;
        void execute() override;
};

class JobsCommand : public BuiltInCommand {
        public:
        explicit JobsCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~JobsCommand() = default;
        void execute() override;
};

class KillCommand : public BuiltInCommand {
        // TODO: Add your data members
        public:
        explicit KillCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~KillCommand() {}
        void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
        // TODO: Add your data members
        public:
        explicit ForegroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~ForegroundCommand() {}
        void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
        // TODO: Add your data members
        public:
        explicit BackgroundCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~BackgroundCommand() {}
        void execute() override;
};

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
        public:
        explicit QuitCommand(const char* cmd_line): BuiltInCommand(cmd_line){};
        virtual ~QuitCommand() {};
        void execute() override;
};

class TimeoutCommand : public Command {
    int alarm_time;
    Command* timed_cmd;
public:
    explicit TimeoutCommand(const char* cmd_line): Command(cmd_line){};
    virtual ~TimeoutCommand() {};
    void execute() override;
};

class JobEntry {
    int id;
    time_t creation_time;
    Command &created_command;
    pid_t pid;
    bool stopped;
    bool finished;
public:
    JobEntry(Command *command = nullptr, pid_t pid = getpid(), bool stopped = false);

    ~JobEntry() { delete &created_command; };

    JobEntry(const JobEntry &to_copy);//=delete?
    Command *getCommand() const;                        //ready
    int getId() const;                         //ready
    pid_t getPid() const;

    time_t getCreationTime() const;

    void resetTime();

    bool isStopped() const;                             //ready
    bool isFinished() const;                            //ready
    void stop(); //change stopped to true         //ready
    void cont(); //change stopped to false        //ready
    bool operator<(const JobEntry &other) const;

    bool operator==(const JobEntry &other) const;

    friend std::ostream &operator<<(std::ostream &os, const JobEntry &job);

    void finish(); //TODO: delete after debugging of JobsList
};

std::ostream& operator<<(std::ostream& os, const std::list<JobEntry*>& jobs);
std::ostream& operator<<(std::ostream& os, const JobEntry& job);

class TimedJobEntry : public JobEntry {
    time_t alarm_time;
    int timed_job_id;
public:
    TimedJobEntry(time_t alarm_time, Command *command = nullptr, pid_t pid = getpid(), bool stopped = false);
    ~TimedJobEntry() = default;
    TimedJobEntry(const TimedJobEntry &to_copy);//=delete?
    time_t getAlarmTime();

};

class JobsList {
    std::list<JobEntry *> jobs;

    std::list<JobEntry *> sortJobsById() const;

    friend class JobEntry;

    int max_job_id;
public:
    JobsList();

    ~JobsList() {};

    JobsList(const JobsList &list) = delete;

    void operator=(const JobsList &list) = delete;

    int getJobsSize() const;

    int getMaxJobId() const;

    std::list<JobEntry *> &getJobsList();

    void addJob(Command *cmd, pid_t pid = getpid(), bool stopped = false);

    void printJobsList() const;

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int job_id) const;
    bool isTimed();
    void removeJobById(int job_id);

    JobEntry *getLastJob() const;

    JobEntry *getLastStoppedJob() const;

    friend std::ostream &operator<<(std::ostream &os, const JobsList &jobs);

    // TODO: Add extra methods or modify existing ones as needed
    void updateMax();
};

class TimedJobsList {
    std::list<TimedJobEntry*> jobs;
    friend class TimedJobEntry;
    time_t soonest_alarm;
    TimedJobEntry* soonest_job;
    int max_timed_id;
public:
    void updateSoonest();
    TimedJobsList() = default;
    ~TimedJobsList() {};
    TimedJobsList(const TimedJobsList &list) = delete;
    void operator=(const TimedJobsList &list) = delete;
    int getTimedJobsSize() const;
    TimedJobEntry * getSoonest();
    std::list<TimedJobEntry*>& getJobsList();
    void addJob(Command *cmd, pid_t pid = getpid(), bool stopped = false);
    void printTimedJobsList() const;
    void killAllJobs();
    TimedJobEntry *getTimedJobById(int timed_id) const;
    void removeJobById(int job_id);
    TimedJobEntry *getLastJob() const;
    TimedJobEntry *getLastStoppedJob() const;
    friend std::ostream &operator<<(std::ostream &os, const TimedJobsList& timed_jobs);
    void updateMax();
    void removeFinishedJobs();

};

class SmallShell {
private:
    std::string prompt;

    SmallShell() : prompt("smash> "), jobs_list(nullptr), current_fg_command(nullptr), current_fg_job(nullptr),
            current_fg_pid(-1), previous_directory(nullptr), pid(getpid()), timed_jobs_list(nullptr) { jobs_list = new JobsList(); timed_jobs_list = new TimedJobsList();};
    JobsList *jobs_list;
    Command *current_fg_command;
    JobEntry *current_fg_job;
    pid_t current_fg_pid;
    char *previous_directory;
    pid_t pid;
    TimedJobsList* timed_jobs_list;
    std::list<TimeoutCommand*> timeouts;
public:
    JobsList &getJobsList() const;           //ready
//    std::list<int>& getBGList() const;      //ready
    std::string getPrompt() const;      // ready
    char *getPreviousDirectory() const;  // ready
    void setPreviousDirectory(char *new_directory);    // ready
    int getMaxJobID() const; // ready
    pid_t getSmashPid() const; //ready //TODO: maybe static?
    int moveToBG(JobEntry *new_bg_job); //ready
    int moveToFG(JobEntry *new_fg_job = nullptr);  //ready
    JobEntry *getMaxStoppedJob() const; //ready
    Command* CreateCommand(const char *cmd_line);
    Command *CreateCommand(const char *cmd_line, const char* timeout_command, time_t alarm_time);

    JobEntry *getCurrentFGJobPtr() const; // ready
    Command *getCurrentFGCommand() const; // ready
    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    void clearFG(); // ready
    ~SmallShell(); // ready
    void executeCommand(const char *cmd_line);

    static int changePrompt(const std::string new_prompt);//  ready    returns 0 if success, else -1
    pid_t getCurrentFGpid() const;
    void setFG(Command *cmd, pid_t pid, JobEntry *job = nullptr);
    std::list<TimeoutCommand*> getTimeoutCommandsList();
    TimedJobsList &getTimedJobsList() const;
};


#endif //SMASH_COMMAND_H
