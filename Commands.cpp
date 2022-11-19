#include "Commands.h"

using namespace std;

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif



/////////////// UTILITIES ///////////////


string _ltrim(const string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == string::npos) ? "" : s.substr(start);
}

string _rtrim(const string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    istringstream iss(_trim(string(cmd_line)).c_str());
    for(string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    if(!args[1]){
        args[1]=(char*)"";
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}


/////////////// COMMAND FUNCTIONS ///////////////

Command::Command(const char* cmd_line): job_entry_id(-1), command_words(0), bg_cmd(false), original_command(nullptr),
                                        command(new string(cmd_line)), timed(false),cmd_alarm_time(0), timeout_command(
                nullptr) {
    parsed = (char**)malloc(COMMAND_MAX_ARGS*sizeof(char*));
    original_command = new char[strlen(cmd_line)+1];
    strcpy(original_command, cmd_line);
    command_words = _parseCommandLine(cmd_line, parsed);
    if (_isBackgroundCommand(cmd_line)) { //bg command
        bg_cmd = true;
        trimAmpersand();
    }
}

Command::Command(const char* cmd_line, const char* timeout_command_r, time_t cmd_alarm_time): job_entry_id(-1),
        command_words(0), bg_cmd(false), original_command(nullptr), command(new string(cmd_line)),
        timed(true), cmd_alarm_time(cmd_alarm_time), timeout_command(nullptr){
    parsed = (char**)malloc(COMMAND_MAX_ARGS*sizeof(char*));
    command_words = _parseCommandLine(cmd_line, parsed);
    original_command = new char[strlen(cmd_line)+1];
    strcpy(original_command, cmd_line);
    timeout_command = new char[strlen(timeout_command_r) + 1];
    strcpy(timeout_command, timeout_command_r);
    if (_isBackgroundCommand(cmd_line)) { //bg command
        bg_cmd = true;
        trimAmpersand();
    }
}

Command::~Command(){
    if(SmallShell::getInstance().getCurrentFGCommand() != this && job_entry_id>0){
        SmallShell::getInstance().getJobsList().removeJobById(job_entry_id);
        if(isTimed()) {
            SmallShell::getInstance().getTimedJobsList().removeJobById(timed_job_id);
        }
    }
    if(SmallShell::getInstance().getCurrentFGCommand() == this){
        SmallShell::getInstance().clearFG();
    }
    delete[] original_command;
    delete[] timeout_command;
    delete command;
    for(int i=0;i<getParsedLength();i++){
        if(parsed[i]){
            free(parsed[i]);
        }
    }
    free(parsed);
}

bool Command::isBG() const{
    return bg_cmd;
}

string Command::getCommand() const{
    return *command;
}

char** Command::getParsed() const{
    return parsed;
}

int Command::getParsedLength() const{
    return command_words;
}

int Command::getJobEntryId() const{
    return job_entry_id;
}

void Command::setJobEntryId(int job_id){
    job_entry_id = job_id;
}

void Command::trimAmpersand(){
    if(command->find_last_of("&") == string::npos){
        return;
    }
    command->erase(command->find_last_of("&"));//TODO: DEBUG
    int parsed_len = getParsedLength();
    char* last_arg = parsed[parsed_len-1];
    int arg_len = strlen(last_arg);
    last_arg[arg_len-1] = '\0';
}

void Command::setBG(bool bg_status){
    bg_cmd = bg_status;
}

bool Command::isPiped(const char* cmd_line){
    string str = cmd_line;
    string trimmed = _trim(str);
    auto pos1 = trimmed.find_first_of("|");
    return (pos1 != string::npos && pos1!=0 && pos1!= strlen(cmd_line)-1);
}

bool Command::isRedirection(const char* cmd_line){
    string str = cmd_line;
    string trimmed = _trim(str);
    auto pos1 = trimmed.find_first_of(">");
    auto pos2 = trimmed.find_last_of(">");
    return (pos1 != string::npos && pos1!=0 && pos1!= strlen(cmd_line)-1 && pos1!= strlen(cmd_line)-2 && pos2!=string::npos);
}

char* Command::getOriginalCommand() const{
    return original_command;
}



/////////////// BUILT_IN COMMANDS FUNCTIONS ///////////////

BuiltInCommand::~BuiltInCommand() noexcept {};

void ChangeDirCommand::execute() {
    if(getParsedLength()==1){
        return;
    }
    if(getParsedLength()>=3){
        cerr << "smash error: cd: too many arguments" << endl;
        return;
    }
    else{
        char* new_pwd = getParsed()[1];
        char* old_pwd = SmallShell::getInstance().getPreviousDirectory();
        char* curr_pwd = get_current_dir_name();
        if(strcmp(new_pwd,"-") == 0){
            if(!old_pwd){
                cerr << "smash error: cd: OLDPWD not set" << endl;
                free(curr_pwd);
                return;
            }
            else{
                if(chdir(old_pwd)==0){
                    SmallShell::getInstance().setPreviousDirectory(curr_pwd);
                }
                else{
                    perror("smash error: chdir failed");
                    free(curr_pwd);
                    return;
                }
            }
        }
        else{
            if(chdir(new_pwd)==0){
                SmallShell::getInstance().setPreviousDirectory(curr_pwd);
            }
            else{
                perror("smash error: chdir failed");
                free(curr_pwd);
                return;
            }
        }
        free(curr_pwd);
    }
}

void GetCurrDirCommand::execute() {
    char* curr_dir = get_current_dir_name();
    cout << curr_dir << endl;
    free(curr_dir);
}

void FindAndReplaceCommand::execute(){
    if(getParsedLength()!=4){
        cerr << "smash error: fare: invalid arguments" << endl;
        return;
    }
    const char* file_name = getParsed()[1];
    fstream file = fstream(file_name);
    if(!file){
        cerr << "smash error: fare: invalid arguments" << endl;
        return;
    }
    char* old_word = getParsed()[2];
    int old_word_length = strlen(old_word);
    //string old_word_str = old_word;
    char* new_word = getParsed()[3];
    string file_string;
    for (char ch; file.get(ch); file_string.push_back(ch));
    auto pos = file_string.find(old_word);
    int replacements_counter = 0;
    while(pos != std::string::npos)
    {
        replacements_counter++;
        file_string.replace(pos, old_word_length, new_word);
        pos = file_string.find(old_word, 0);
    }
    ofstream new_file = ofstream("new_file");
    new_file << file_string;
    remove(file_name);
    rename("new_file", file_name);
    string instance_ending_string = (replacements_counter==1)? "" : "s";
    cout <<  "replaced " << replacements_counter << " instance" << instance_ending_string << " of the string \""<< old_word <<"\"" << endl;
    }

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

void ChangePromptCommand::execute(){
    string new_prompt;
    if(getParsedLength()==1){
        new_prompt = "smash";
    }
    else{
        new_prompt = (!strcmp(getParsed()[1],"")) ? "smash" : getParsed()[1];
    }
    SmallShell::changePrompt(new_prompt);
}

void JobsCommand::execute() {
    JobsList& jobs = SmallShell::getInstance().getJobsList();
    jobs.printJobsList();
}

void KillCommand::execute() {
    if (getParsedLength() != 3 || getParsed()[1][0] != '-') {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    } else {
        string job_str = getParsed()[2];
        int job_id = 0;
        try {
            job_id = stoi(job_str);
        }
        catch (const exception &e) {// check if valid
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        string signum_str = getParsed()[1];
        signum_str.erase(0, 1);
        int signum = 0;
        try {
            signum = stoi(signum_str);
        }
        catch (const exception &e) {// check if valid
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        JobEntry *job_to_kill = SmallShell::getInstance().getJobsList().getJobById(job_id);
        if (job_to_kill == nullptr) {
            cerr << "smash error: kill: job-id " << job_id << " does not exist" << endl;
            return;
        }
        pid_t pid_to_kill = job_to_kill->getPid();
//        if (pid_to_kill != SmallShell::getInstance().getSmashPid()) {
        if (kill(pid_to_kill, signum) == -1) {
            cerr << "smash error: kill: invalid arguments" << endl;
            return;
        }
        cout << "signal number " << signum << " was sent to pid " << pid_to_kill << endl;
    }
}

void BackgroundCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    auto& jobs_list = smash.getJobsList();
    if(getParsedLength()>2){
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    else{
        if(getParsedLength()==1){ //Command is: bg
            JobEntry* new_bg = smash.getMaxStoppedJob();
            if(new_bg == nullptr){
                cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
                return;
            }
            else{
                const char* cmd_to_print = (new_bg->getCommand()->isTimed()) ? new_bg->getCommand()->getTimeoutCommand() : new_bg->getCommand()->getOriginalCommand();
                cout << cmd_to_print << " : " << new_bg->getPid() << endl;
                smash.moveToBG(new_bg); //assuming MoveToBG does SIGCONT+waitpid
                    //TODO: error message? - depends on the implementation of moveToBG
                }
        }
        else{ //Command is: bg [job-id]
            int job_id = 0;
            try{
                job_id = stoi(getParsed()[1]);
            }
            catch(const exception& e) {// check if valid
                cerr << "smash error: bg: invalid arguments" << endl;
                return;
            }
            JobEntry* new_bg = jobs_list.getJobById(job_id);
            if(new_bg == nullptr){
                cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
                return;
            }
            else{
                if(!(new_bg->isStopped())){
                    cerr << "smash error: bg: job-id " << job_id << " is already running in the background" << endl;
                    return;
                }
                else{ //job is stopped
                    const char* cmd_to_print = (new_bg->getCommand()->isTimed()) ? new_bg->getCommand()->getTimeoutCommand() : new_bg->getCommand()->getOriginalCommand();
                    cout << cmd_to_print << " : " << new_bg->getPid() << endl;
                    smash.moveToBG(new_bg); //assuming MoveToBG does SIGCONT+waitpid
                }
            }
        }
    }
}

void ForegroundCommand::execute(){
    SmallShell& smash = SmallShell::getInstance();
    auto& jobs_list = smash.getJobsList();
    if(getParsedLength()>2){
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    else{
        if(getParsedLength()==2){//command is: fg [job-id]
            int job_id=0;
            try{
                job_id = stoi(getParsed()[1]);
            }
            catch(const exception& e) {// check if valid
                cerr << "smash error: fg: invalid arguments" << endl;
                return;
            }
            if(jobs_list.getJobById(job_id)== nullptr){
                cerr << "smash error: fg: job-id " << job_id << " does not exist" << endl;
            }
            else{//TODO: eliminate duplicated code
                JobEntry* new_fg = jobs_list.getJobById(job_id);
                const char* cmd_to_print = (new_fg->getCommand()->isTimed()) ? new_fg->getCommand()->getTimeoutCommand() : new_fg->getCommand()->getOriginalCommand();
                cout << cmd_to_print << " : " << new_fg->getPid() << endl;
                smash.moveToFG(new_fg); //assuming: SIGCONT + waitpid
                //TODO: error message? - depends on the implementation of moveToBG
            }
        }
        else{
            if(jobs_list.getJobsSize()==0){
                cerr << "smash error: fg: jobs list is empty" << endl;
                return;
            }
            else{ //command is: fg
                JobEntry* new_fg = jobs_list.getJobById(smash.getMaxJobID());
                const char* cmd_to_print = (new_fg->getCommand()->isTimed()) ? new_fg->getCommand()->getTimeoutCommand() : new_fg->getCommand()->getOriginalCommand();
                cout << cmd_to_print << " : " << new_fg->getPid() << endl;
                smash.moveToFG(new_fg); //assuming: SIGCONT + waitpid
                //TODO: error message? - depends on the implementation of moveToBG
            }
        }
    }
}

void QuitCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    auto& jobs_list = smash.getJobsList();
//    auto& timed_jobs = smash.getTimedJobsList();
    if (getParsedLength() > 1) {
        if (strcmp(getParsed()[1], "kill") == 0) { //quit kill command
            jobs_list.removeFinishedJobs();
//            timed_jobs.removeFinishedJobs();
            int unfinished_jobs_num = jobs_list.getJobsSize();
            cout << "smash: sending SIGKILL signal to " << unfinished_jobs_num << " jobs:" << endl;
            jobs_list.killAllJobs();
        }
    }
    exit(0);//TODO: if got time, run valgrind
}


/////////////// EXTERNAL COMMAND FUNCTION ///////////////

void ExternalCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string cmd_str = getCommand();
    const char *const cmd_line = cmd_str.c_str();
    char *const argv[4] = {(char *const) "/bin/bash", (char *const) "-c", (char *const) cmd_line, NULL};
    const pid_t f_pid = fork();
    if (f_pid == -1) {
        perror("smash error: fork failed");
//        delete cmd_str;
        return;
    } else { // f_pid != -1
        if (f_pid == 0) { //is Child
            if (setpgrp() == -1) {
                perror("smash error: setpgrp failed");
//                delete cmd_str;
                exit(1);//TODO: Check if we shall return?
            } else {
                execv(argv[0], argv);
                perror("smash error: execv failed");
//                delete cmd_str;
                exit(1);
            }
        } else { //is parent
            if (timed) {
                smash.getTimedJobsList().addJob(this, f_pid);
            }
            if (!isBG()) { //not bg_Command
                int status = 0;
                JobEntry *cmd_job = smash.getJobsList().getJobById(this->getJobEntryId());
                smash.setFG(this, f_pid, cmd_job);
//                pid_t ret_pid =
                waitpid(f_pid, &status, WUNTRACED);
                if (WIFSTOPPED((status))) {
                    if (this->getJobEntryId() == -1) {
                        SmallShell::getInstance().getJobsList().addJob(this, f_pid, true); //is_stopped = true
                    } else {
                        (smash.getJobsList().getJobById((this->getJobEntryId())))->stop();
                    }
                }
                else {
                    if (isTimed()) {
                        TimedJobEntry *timed_job = smash.getTimedJobsList().getTimedJobById(this->getTimedJobId());
                        if (timed_job) {
                            timed_job->finish();
                            //                    smash.getTimedJobsList().removeJobById(this->getTimedJobId());
                            smash.getTimedJobsList().updateSoonest();
                        }
                    }
                }
                smash.clearFG();
            } else { //bg_cmd
                if (this->getJobEntryId() == -1) {
                    SmallShell::getInstance().getJobsList().addJob(this, f_pid, false);
                } else {
                    (smash.getJobsList().getJobById(this->getJobEntryId()))->finish();
                    if (isTimed()) {
                        smash.getTimedJobsList().getTimedJobById(this->getTimedJobId())->finish();
//                        smash.getTimedJobsList().removeJobById(this->getTimedJobId());
                        smash.getTimedJobsList().updateSoonest();
                    }
                }
            }
        }
    }
}

/////////////// SPECIAL COMMANDS FUNCTIONS ///////////////
/**
 * We know there is a "|" in the cmd_line
 * check if pipe is "|" or "|&"
 * separate string before the pipe and after
 * make sure each of the parts is a one word string
 *
 */
void PipeCommand::execute(){
    int my_pipe[2];
    if(pipe(my_pipe) == -1){
        perror("smash error: pipe failed");
        return;
    }
    SmallShell& smash = SmallShell::getInstance();
    int pipe_index = command->find_first_of("|");
    string first_part_str = command->substr(0, pipe_index);
    const char* first_part = first_part_str.c_str();
    string second_part_str;
    char after_pipe = command->at(pipe_index+1);
    int pipe_channel = (after_pipe == '&') ? 2 : 1; //channel=2 if |&, channel=1 if |   ///!!!!!!!!!!
    if(after_pipe == '&') { // the pipe operator is |&
        second_part_str = command->substr(pipe_index + 2);
    }
    else{ // the pipe operator is | only
        second_part_str = command->substr(pipe_index+1);
    }
    const char* second_part = second_part_str.c_str();
    pid_t pid_of_first = fork();
    if(pid_of_first == -1){
        perror("smash error: fork failed");
        return;
    }
    if(pid_of_first == 0) { //Child
        if(setpgrp() == -1){
            perror("smash error: setpgrp failed");
            exit(1); //TODO: check if needed
        }
        pid_t pid_of_second = fork();
        if(pid_of_second == -1){
            perror("smash error: fork failed");
            return;
        }
        if(pid_of_second == 0) { //Grandchild - first
            if(setpgrp() == -1){
                perror("smash error: setpgrp failed");
                exit(1); //TODO: check if needed
            }
            if(dup2(my_pipe[1], pipe_channel) == -1){
                perror("smash error: dup2 failed");
                exit(1);
            }
            if( (close(my_pipe[0]) == -1) || (close(my_pipe[1]) == -1)){ //TODO: figure out why closing each one of them
                perror("smash error: close failed");
                exit(1);
            }
            Command* cmd1 = smash.CreateCommand(first_part);
            if(cmd1 == nullptr){
                cerr <<"smash error: pipe failed" << endl; //TODO: check if right
                return;
            }
            cmd1->trimAmpersand();
            if((string)(cmd1->getParsed()[0]) == (string)"showpid"){
                cout << "smash pid is " << smash.getSmashPid() << endl;
                //TODO: should we delete cmd1 here?
                exit(0);
            }
            cmd1->execute();
            delete cmd1;
            exit(0);
        }
        else{ //Child - second
            if(setpgrp() == -1){
                perror("smash error: setpgrp failed");
                exit(1); //TODO: check if needed
            }
            if(dup2(my_pipe[0], 0) == -1){ //Note: changed my_pipe to 0!
                perror("smash error: dup2 failed");
                exit(1);
            }
            if(close(my_pipe[0]) == -1 || close(my_pipe[1]) == -1){ //TODO: figure out why closing each one of them
                perror("smash error: close failed");
                exit(1);
            }
            if(wait(nullptr)!=-1) {
                Command *cmd2 = smash.CreateCommand(second_part);
                if (cmd2 == nullptr) {
                    cerr << "smash error: pipe failed" << endl; //TODO: check if right
                    return;
                }
                cmd2->trimAmpersand();
                if((string)(cmd2->getParsed())[0] == "showpid"){
                    cout << "smash pid is " << smash.getSmashPid() << endl;
                    //TODO: should we delete cmd2 here?
                    exit(0);
                }
                cmd2->execute();
                delete cmd2;
            }
            exit(0);
        }
    }
    else{ //Father
        if(close(my_pipe[0]) == -1 || close(my_pipe[1]) == -1) { //TODO: figure out why closing each one of them
            perror("smash error: close failed");
            exit(1);
        }
        waitpid(pid_of_first, nullptr,0);
    }
    //Command1 stderr -> my_pipe[1] "write"
    //Command2 stdin -> my_pipe[0] "read"
}


/**
 * > - override. >> - append.
 * We know that there is at least one '>' and it is neither the first nor the last
 * In case there are two, they're supposed to be consecutive
 */
void RedirectionCommand::execute(){ //TODO: maybe can be done with fork?
    SmallShell& smash = SmallShell::getInstance();
    int redirection_index = command->find_first_of(">");
    string first_part_str = command->substr(0, redirection_index);
    const char* first_part = first_part_str.c_str();
    string second_part_str;
    char after_redirection = command->at(redirection_index+1);
    int file_flag = (after_redirection == '>') ? O_APPEND : O_TRUNC;
    if(after_redirection == '>') { // the redirection operator is >>
        second_part_str = command->substr(redirection_index + 2);
    }
    else{ // the redirection operator is > only
        second_part_str = command->substr(redirection_index+1);
    }
    const char* second_part_non_trimmed = second_part_str.c_str();
    string second_part = _trim(second_part_non_trimmed);
    int prev_stdout = dup(1);
    if(prev_stdout == -1){
        perror("smash error: dup failed");
        return;
    }
    if(close(1) == -1){
        perror("smash error: close failed");
        return;
    }
    int fd = open(second_part.c_str(), O_RDWR | O_CREAT | file_flag, 0655);
    if(fd == -1){
        perror("smash error: open failed");
        if(dup2(prev_stdout, 1) == -1){
            perror("smash error: dup2 failed");
        }
        return;
    }
    //TODO: no need for dup because open does the job - CHECK
    Command* cmd = smash.CreateCommand(first_part);
    if (cmd == nullptr) {
        cerr << "smash error: redirection failed" << endl; //TODO: check if right
        return;
    }
    cmd->trimAmpersand();
    cmd->execute();
    delete cmd;
    if(close(1) == -1){
        perror("smash error: close failed");
        return;
    }
    if(dup2(prev_stdout, 1) == -1){
        perror("smash error: dup2 failed");
        return;
    }
}

void TailCommand::execute(){
    if(getParsedLength() == 1){
        cerr << "smash error: tail: invalid arguments" << endl;
        return;
    }
    if(getParsedLength()>3){ //TODO: Piazza - won't be checked
        cerr << "smash error: tail: invalid arguments" << endl;
        return;
    }
    int lines_to_print =  10;
    char* file_name;
    if(getParsedLength() == 3) {
        file_name = parsed[2];
        string lines_to_print_str = ((string) parsed[1]);
        if(lines_to_print_str.at(0)!='-'){
            cerr << "smash error: tail: invalid arguments" << endl;
            return;
        }
        lines_to_print_str.erase(0,1);
        try {
            lines_to_print = stoi(lines_to_print_str);
        } catch (const exception &e) {
            cerr << "smash error: tail: invalid arguments" << endl;
            return;
        }
    }
    else{
        file_name = parsed[1];
    }
    int fd = open(file_name, O_RDONLY); // check if file exists
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }
    else {
        if (close(fd) == -1) {
            perror("smash error: close failed");
            return;
        }
        fd = open(file_name, O_RDONLY);
        FILE *file = fdopen(fd, "r");
        char *line = 0;
        size_t line_len = 0;
        int i = 0;
        for (; getline(&line, &line_len, file) != -1; i++) {
            //do nothing
        }
        if (!feof(file)) {
            perror("smash error: read failed");
            free(line);
            if (close(fd) == -1) {
                perror("smash error: close failed");
            }
            return;
        }
        fseek(file,0,SEEK_SET);
        if(i-lines_to_print<=0)
        {
            for (; getline(&line, &line_len, file) != -1;) {
                cout << line;
            }
            i=lines_to_print;
        }
        else
        {
            int j =0;
            int min_line = (i-lines_to_print);
            i=0;
            for (; getline(&line, &line_len, file) != -1;j++) {
                if(j>=min_line) {
                    cout << line;
                    i++;
                }
            }
        }
        if (i != lines_to_print) {
            if (!feof(file)) {
                perror("smash error: read failed");
            }
        }
        free(line);
        if (close(fd) == -1) {
            perror("smash error: close failed");
        }
    }
}

void TouchCommand::execute(){
    if(getParsedLength() != 3){
        cerr << "smash error: touch: invalid arguments" << endl;
        return;
    }
    char* file_name;
    char* time_to_change;
    file_name = parsed[1];
    time_to_change = parsed[2];
    string delim=":";
    string given_str = (string)time_to_change;
    size_t pos = 0;
    string token1;
    int sec,min,hour,day,month,year;
    int counter =0;
    while ((pos = given_str.find(delim)) != std::string::npos || counter!=6){
        token1 = given_str.substr(0, pos);
        switch(counter){
            case 0:
                try {
                    sec = stoi(token1);
                    if(sec>59 || sec <0)
                    {
                        cerr << "smash error: touch: invalid arguments" << endl;
                        return;
                    }
                } catch (const exception &e) {
                    cerr << "smash error: touch: invalid arguments" << endl;
                    return;
                }
                break;
            case 1:
                try {
                    min = stoi(token1);
                    if(token1 == "\0"|| min>59 || min <0)
                    {
                        cerr << "smash error: touch: invalid arguments" << endl;
                        return;
                    }
                } catch (const exception &e) {
                    cerr << "smash error: touch: invalid arguments" << endl;
                    return;
                }
                break;
            case 2:
                try {
                    hour = stoi(token1);
                    if(token1 == "\0"|| hour>23 || hour <0)
                    {
                        cerr << "smash error: touch: invalid arguments" << endl;
                        return;
                    }
                } catch (const exception &e) {
                    cerr << "smash error: touch: invalid arguments" << endl;
                    return;
                }
                break;
            case 3:
                try {
                    day = stoi(token1);
                    if(token1 == "\0"|| day>31 || day <1)
                    {
                        cerr << "smash error: touch: invalid arguments" << endl;
                        return;
                    }
                } catch (const exception &e) {
                    cerr << "smash error: touch: invalid arguments" << endl;
                    return;
                }
                break;
            case 4:
                try {
                    month = stoi(token1);
                    if(token1 == "\0"|| month>12 || month <1)
                    {
                        cerr << "smash error: touch: invalid arguments" << endl;
                        return;
                    }
                } catch (const exception &e) {
                    cerr << "smash error: touch: invalid arguments" << endl;
                    return;
                }
                break;
            case 5:
                try {
                    year = stoi(token1);
                    if(token1 == "\0")
                    {
                        cerr << "smash error: touch: invalid arguments" << endl;
                        return;
                    }
                } catch (const exception &e) {
                    cerr << "smash error: touch: invalid arguments" << endl;
                    return;
                }
                break;
            default:
                break;
        }
        counter++;
        given_str.erase(0,pos+delim.length());
    }
    time_t raw_time;
    struct tm * time_info;
    time (&raw_time);
    time_info = localtime (&raw_time);
    time_info->tm_year = year - 1900;
    time_info->tm_mon = month-1;
    time_info->tm_mday = day;
    time_info->tm_hour = hour;
    time_info->tm_min = min;
    time_info->tm_sec = sec;
    raw_time = mktime(time_info);
    const struct utimbuf my_times = {raw_time,raw_time};
    int sys_time_status = utime(file_name,&my_times);
    if (sys_time_status == -1){
        perror("smash error: utime failed");
    }
}

/////////////// JOB ENTRY FUNCTIONS ///////////////

JobEntry::JobEntry(Command* command, pid_t pid, bool stopped):id(++(SmallShell::getInstance().getJobsList().max_job_id)), creation_time(time(NULL)),
                                                              created_command(*command), pid(pid), stopped(stopped), finished(false){

}

Command* JobEntry::getCommand() const{
    return &created_command;
}

int JobEntry::getId() const {
    return id;
}

pid_t JobEntry::getPid() const{
    return pid;
}

time_t JobEntry::getCreationTime() const{
    return creation_time;
}

void JobEntry:: resetTime(){
    creation_time = time(NULL);
}

bool JobEntry::isStopped() const{
    return stopped;
}

bool JobEntry::isFinished() const{
    return finished;
}

void JobEntry::stop(){
    stopped=true;
}

void JobEntry::cont() {
    stopped=false;
}

bool JobEntry::operator<(const JobEntry& other) const{
    return id<other.id;
}

bool JobEntry::operator==(const JobEntry& other) const{ //TODO: Check if necessary
    return id==other.id;
}

ostream& operator<<(ostream& os, const JobEntry& job){
    time_t time_elapsed = difftime(time(NULL), job.creation_time);
    const char* cmd_to_print = (job.created_command.isTimed()) ? job.created_command.getTimeoutCommand() : job.created_command.getOriginalCommand();
    os << "[" << job.id << "] " << cmd_to_print << " : " << job.pid << " " << time_elapsed << " secs";
    if (job.stopped){
        os << " (stopped)";
    }
    os << endl;
    return os;
}


////////////// JOBS LIST FUNCTIONS ///////////////

int JobsList::getJobsSize() const{
    return jobs.size();
}

list<JobEntry*>& JobsList::getJobsList(){
    return jobs;
}

int JobsList::getMaxJobId() const{
    return max_job_id;
}

void JobsList::updateMax() {
    int max_id=0;
    if(jobs.empty()){
        max_job_id = max_id;
        return;
    }
    for(JobEntry* job: jobs){
        int curr_id = job->getId();
        max_id = max_id >= curr_id ? max_id : curr_id;
    }
    max_job_id = max_id;
}

list<JobEntry*> JobsList::sortJobsById() const {
    list<JobEntry*> sorted = jobs;
    sorted.sort();
    return sorted;
}

JobsList::JobsList() {
    max_job_id = 0;
}

void JobsList::addJob(Command* cmd, pid_t pid ,bool stopped){
    if(!cmd){
        return;
    }
    if(cmd->getJobEntryId() == -1){ // came from fg for the first time ever or a bew bg cmd
        JobEntry* new_job_ptr = new JobEntry(cmd, pid, stopped);
        cmd->setJobEntryId(new_job_ptr->getId());
        jobs.push_back(new_job_ptr);
        if (!(cmd->isBG())){
            SmallShell::getInstance().clearFG();
        }
    } //TODO: valgrind problem because there's no delete(problematic delete-find another way)
    else{ // fg and isn't new: bg -> fg -> bg
        JobEntry* fg_job = SmallShell::getInstance().getCurrentFGJobPtr();
        jobs.push_back(fg_job);
        SmallShell::getInstance().clearFG();
    }
}

/**
 * assuming jobs command deletes all finished jobs before printing
 */
void JobsList::printJobsList() const{
    list<JobEntry*> sorted_jobs_list = sortJobsById();
    cout << sorted_jobs_list;
}

/**
 * assuming jobs command deletes all finished jobs before printing
 * at the end - jobs list is not empty!
 */
void JobsList::killAllJobs(){
    for(JobEntry* job : jobs){
        if(kill(job->getPid(), SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        else {
            const char* cmd_to_print = (job->getCommand()->isTimed()) ? job->getCommand()->getTimeoutCommand() : job->getCommand()->getOriginalCommand();
            cout << job->getPid() << ": " << cmd_to_print << endl;
            job->finish();
            if(job->getCommand()->isTimed()) {
                SmallShell::getInstance().getTimedJobsList().getTimedJobById(job->getCommand()->getTimedJobId())->finish();
//                SmallShell::getInstance().getTimedJobsList().removeJobById(job->getCommand()->getTimedJobId());
                SmallShell::getInstance().getTimedJobsList().updateSoonest();
            }
        }
    }
}

JobEntry* JobsList::getJobById(int job_id) const{
    if(job_id > 0 && job_id <= max_job_id){
        for (JobEntry* job : jobs){
            if (job->getId() == job_id){
                return job;
            }
        }
    }
    return nullptr;
}

/**
  * only removes JobEntry with job ID job_id from the jobs list
  * TODO: CHECK NO BUGS WITH IT AND ERASE (maybe use == instead of !=)
 */
void JobsList::removeJobById(int job_id){
    if(job_id<=0)
        return;
    if(jobs.empty()){
        updateMax();
        return;
    }
    JobEntry* to_be_deleted = getJobById(job_id);
    auto it = jobs.begin();
    while(*it++!=to_be_deleted);
    jobs.erase(it);
    updateMax();
}

JobEntry* JobsList::getLastJob() const{
    return jobs.back();
}

JobEntry* JobsList::getLastStoppedJob() const{
    int jobs_number = jobs.size();
    if(jobs_number==0){
        return nullptr;
    }
    auto it = --jobs.end();
    for(int i = 0; i<jobs_number; i++){
        if((*it)->isStopped()){
            return *it;
        }
        it--;
    }
    return nullptr; // no stopped jobs found
}

/**
 *
 */
void JobsList::removeFinishedJobs() {
    if(jobs.empty()){
        updateMax();
        return;
    }
    int status = 0;
    pid_t smash_pid = SmallShell::getInstance().getSmashPid();
    for (JobEntry* job: jobs){
        pid_t job_pid = job->getPid();
        if(smash_pid != job_pid){
            pid_t pid_status = waitpid(job_pid, &status, WNOHANG);
            if(pid_status == job_pid || pid_status == -1 ){
                job->finish();//TODO: should it be inside? or should we take it out?
                if(job->getCommand()->isTimed())
                    SmallShell::getInstance().getTimedJobsList().getTimedJobById(job->getCommand()->getTimedJobId());
            }
        }
    }
    jobs.erase(remove_if(jobs.begin(), jobs.end(), [](JobEntry* job) { return job->isFinished(); }), jobs.end());
    updateMax();
}

ostream& operator<<(ostream& os, const list<JobEntry*>& jobs){
    for (const JobEntry* job: jobs){
        os << *job;
    }
    return os;
}


//TODO: delete
void JobEntry::finish(){
    finished = true;
//    delete getCommand();
}



////////////// SMALL SHELL FUNCTIONS ///////////////

JobsList& SmallShell::getJobsList() const{
    return *jobs_list;
}

SmallShell::~SmallShell(){
    if(jobs_list)
        delete jobs_list;
    for (TimeoutCommand *cmd: timeouts) {
        delete cmd;
    }
    if (timed_jobs_list) {
        delete timed_jobs_list;
    }
    if (current_fg_job) {
        delete current_fg_job;
    }
    if (current_fg_command) {
        delete current_fg_command;
    }
    if (previous_directory) {
        free(previous_directory);
    }
}

pid_t SmallShell::getSmashPid() const{
    return pid;
}

void SmallShell::setPreviousDirectory(char* new_directory){
    if(previous_directory){
        free(previous_directory);
    }
    previous_directory = (char*)malloc(strlen(new_directory)+1);
    strcpy(previous_directory,new_directory);
}

char* SmallShell::getPreviousDirectory() const{
    return previous_directory;
}

int SmallShell::changePrompt(const string new_prompt) {
    string full_prompt = new_prompt;
    full_prompt.append("> ");
    try {
        getInstance().prompt = full_prompt;
    }catch(bad_alloc&){
        return -1;
    }
    return 0;
}

string SmallShell::getPrompt() const{
    return prompt;
}

JobEntry* SmallShell::getMaxStoppedJob() const{
    int max_stopped_id = 0;
    JobEntry* max_stopped_job = nullptr;
    for(JobEntry* job : jobs_list->getJobsList()){
        if(job->isStopped()){
            if(job->getId()>max_stopped_id) {
                max_stopped_id = job->getId();
                max_stopped_job = job;
            }
        }
    }
    return max_stopped_job;
}

int SmallShell::moveToBG(JobEntry* new_bg){
    if(new_bg == nullptr ||new_bg->isFinished() ){
        return -1;
    }
    new_bg->getCommand()->setBG(true);
    if(new_bg->isStopped()){
        if(kill(current_fg_pid, SIGCONT) == -1){
            perror("smash error: kill failed");
            return -1;
        }
        new_bg->cont();
    }
    return 0;
//    new_bg->getCommand()->execute();//need to be SIGCONT
}

int SmallShell::moveToFG(JobEntry* new_fg){
    if(new_fg == nullptr || new_fg->isFinished() ){
        return -1;
    }
    current_fg_command = new_fg->getCommand();
    current_fg_job = new_fg;
    current_fg_pid = new_fg->getPid();
    current_fg_command->setBG(false);
    if(new_fg->isStopped()){
        if(kill(current_fg_pid, SIGCONT) == -1){
            perror("smash error: kill failed");
            clearFG();
            return -1;
        }
        new_fg->cont();
    }
    int status = 0;
    waitpid(current_fg_pid, &status, WUNTRACED);
    if (WIFSTOPPED((status))) {
        if(current_fg_job){
            current_fg_job->stop(); //is_stopped = true
        }
    }
    else {
        if(current_fg_job){
            current_fg_job->finish();
            if(current_fg_job->getCommand()->isTimed()) {
                timed_jobs_list->getTimedJobById((current_fg_job->getCommand()->getTimedJobId()))->finish();
                timed_jobs_list->updateSoonest();
                timed_jobs_list->removeFinishedJobs();
            }
        }
        jobs_list->removeFinishedJobs();
    }
    clearFG();
    return 0;
}

JobEntry* SmallShell::getCurrentFGJobPtr() const{
    return current_fg_job;
}

Command* SmallShell::getCurrentFGCommand() const{
    return current_fg_command;
}

void SmallShell::clearFG(){
    current_fg_command = nullptr;
    current_fg_job = nullptr;
    current_fg_pid = -1;
}

int SmallShell::getMaxJobID() const{
    return jobs_list->getMaxJobId();
}

Command* SmallShell::CreateCommand(const char* cmd_line) {
    string cmd_s = _trim(cmd_line);
    if (cmd_s == ""){
        return nullptr;
    }
    string first_word = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
    if (Command::isPiped(cmd_line)){
        return new PipeCommand(cmd_line);
    }
    if(Command::isRedirection(cmd_line)){
        return new RedirectionCommand(cmd_line);
    }
    if(first_word == "pwd") { //built-in command
        return new GetCurrDirCommand(cmd_line);
    }
    if(first_word == "showpid") { //built-in command
        return new ShowPidCommand(cmd_line);
    }
    if(first_word == "chprompt") { //built-in command
        return new ChangePromptCommand(cmd_line);
    }
    if(first_word == "fare") { //built-in command
        return new FindAndReplaceCommand(cmd_line);
    }
    if(first_word == "cd") { //built-in command
        return new ChangeDirCommand(cmd_line);
    }
    if(first_word == "jobs") { //built-in command
        return new JobsCommand(cmd_line);
    }
    if(first_word == "kill") { //built-in command
        return new KillCommand(cmd_line);
    }
    if(first_word == "fg") { //built-in command
        return new ForegroundCommand(cmd_line);
    }
    if(first_word == "bg") { //built-in command
        return new BackgroundCommand(cmd_line);
    }
    if(first_word == "quit") { //built-in command
        return new QuitCommand(cmd_line);
    }

    if(first_word == "tail") { //special command
        return new TailCommand(cmd_line);
    }
    if(first_word == "touch") { //special command
        return new TouchCommand(cmd_line);
    }

    if(first_word == "timeout") { //bonus command
        return new TimeoutCommand(cmd_line);
    }
    return new ExternalCommand(cmd_line);
    return nullptr;
}


/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char* cmd_line, const char* timeout_command, time_t alarm_time) {
    return new ExternalCommand(cmd_line, timeout_command, alarm_time);
}

void SmallShell::executeCommand(const char* cmd_line){
    jobs_list->removeFinishedJobs();
    Command* curr_cmd = CreateCommand(cmd_line);
    if(curr_cmd != nullptr) {
        curr_cmd->execute();
        if (!(curr_cmd->isBG()) && curr_cmd->getJobEntryId()==-1) {
            delete curr_cmd;
        }
        else{
            if(typeid(curr_cmd) == typeid(TimeoutCommand*)){
                timeouts.push_back(static_cast<TimeoutCommand*>(curr_cmd));
            }
        }
    }
}

pid_t SmallShell::getCurrentFGpid() const {
    return current_fg_pid;
}

void SmallShell::setFG(Command* cmd, pid_t pid, JobEntry* job){
    current_fg_command = cmd;
    current_fg_pid = pid;
    current_fg_job = job;
}


TimedJobsList& SmallShell::getTimedJobsList() const{
    return *timed_jobs_list;
}

/////////////// TIMED JOB ENTRY FUNCTIONS /////////////// the one im pointing at is ok right?

TimedJobEntry::TimedJobEntry(time_t alarm_time, Command *command, pid_t pid, bool stopped): JobEntry(command,pid, stopped), alarm_time(alarm_time), timed_job_id(++(SmallShell::getInstance().getTimedJobsList().max_timed_id)){
    SmallShell::getInstance().getJobsList().updateMax();
}

time_t TimedJobEntry::getAlarmTime() {
    return alarm_time;
}


////////////// TIMED JOBS LIST FUNCTIONS ///////////////

void TimedJobsList::updateSoonest() {
    removeFinishedJobs();
    if(jobs.empty()){
        soonest_alarm = 0;
        soonest_job = nullptr;
        return;
    }
    soonest_job = jobs.front();
    soonest_alarm = soonest_job->getAlarmTime();
    if(jobs.size() != 1){
        for(TimedJobEntry* job : jobs){
            time_t job_time = job->getCommand()->getTime();
            if(job_time < soonest_alarm){
                soonest_alarm = job_time;
                soonest_job = job;
            }
        }
    }
    alarm(soonest_alarm-time(NULL));
}

void TimedJobsList::addJob(Command* cmd, pid_t pid ,bool stopped){
    if(!cmd){
        return;
    }
    TimedJobEntry* new_job_ptr = new TimedJobEntry(cmd->getTime(), cmd, pid, stopped);
    cmd->setTimedJobId(new_job_ptr->getId());
    jobs.push_back(new_job_ptr);
    updateSoonest();
}

std::list<TimedJobEntry*>& TimedJobsList::getJobsList(){
    return jobs;
}

TimedJobEntry * TimedJobsList::getSoonest() {
    updateSoonest();
    return soonest_job;
}

void TimedJobsList::removeJobById(int timed_job_id){
    if(jobs.empty()){
        soonest_alarm = 0;
        soonest_job = nullptr;
        return;
    }
    TimedJobEntry* to_be_deleted = getTimedJobById(timed_job_id);
    auto it = jobs.begin();
    while(*it!=to_be_deleted){
        it++;
    }
    jobs.erase(it);
    updateSoonest();
}

TimedJobEntry *TimedJobsList::getTimedJobById(int timed_job_id) const {
    if(jobs.empty()){
        return nullptr;
    }
    for(TimedJobEntry* job : jobs){
        if(job->getId() == timed_job_id){
            return job;
        }
    }
    return nullptr;
}

void TimedJobsList::updateMax() {
    int max_id=0;
    if(jobs.empty()){
        max_timed_id = max_id;
        jobs.clear();
        return;
    }
    for(TimedJobEntry* job: jobs){
        int curr_id = job->getId();
        max_id = max_id >= curr_id ? max_id : curr_id;
    }
    max_timed_id = max_id;
}

void TimedJobsList::removeFinishedJobs() {
    if(jobs.empty()){
        updateMax();
        return;
    }
    int status = 0;
    pid_t smash_pid = SmallShell::getInstance().getSmashPid();
    for (TimedJobEntry* job: jobs){
        pid_t job_pid = job->getPid();
        if(smash_pid != job_pid){
            pid_t pid_status = waitpid(job_pid, &status, WNOHANG);
            if((pid_status == job_pid) || (pid_status == -1)){
                job->finish();//TODO: should it be inside? or should we take it out?
            }
        }
    }
    jobs.erase(remove_if(jobs.begin(), jobs.end(), [](TimedJobEntry* job) { return job->isFinished(); }), jobs.end());
    updateMax();
}

void TimedJobsList::killAllJobs(){
    for(TimedJobEntry* job : jobs){
        if(kill(job->getPid(), SIGKILL) == -1){
            perror("smash error: kill failed");
        }
        else {
            job->finish();
            getTimedJobById(job->getCommand()->getTimedJobId())->finish();
        }
    }
    soonest_job = nullptr;
    max_timed_id = 0;
}


//////////////////////////////


char* Command::getTimedCommand(const char* cmd_line){// caller must free the returned value
    string cmd_str = cmd_line;
    int waiting_time_length = strlen(parsed[1]);
    int time_pos = cmd_str.find_first_of(parsed[1]);
    int to_trim = time_pos + waiting_time_length + 1;
    char *to_return = (char *) malloc(cmd_str.length() - to_trim + 1);
    string to_copy_str = cmd_str.substr(to_trim);
    strcpy(to_return, to_copy_str.c_str());
    return to_return;
}

void Command::setTime(int time_to_set) {
    cmd_alarm_time = time_to_set;
}

int Command::getTime() {
    return cmd_alarm_time;
}

const char* Command::getTimeoutCommand() const{
    return timeout_command;
}

void Command::setTimedJobId(int id) {
    timed_job_id = id;
}

bool Command::isTimed() {
    return timed;
}

int Command::getTimedJobId() const {
    return timed_job_id;
}

void TimeoutCommand::execute() {
    if (!parsed[1] || !parsed[2]) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    setBG(true);
    SmallShell &smash = SmallShell::getInstance();
    string time_string = parsed[1];
    try {
        alarm_time = stoi(time_string);
    }
    catch (exception &e) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    if (alarm_time <= 0) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    alarm_time += time(NULL);
    char *real_command = getTimedCommand(original_command);
    Command *timed_cmd = smash.CreateCommand(real_command, original_command,  alarm_time);
    free(real_command);
    if (timed_cmd == nullptr) {
        cerr << "smash error: timeout: invalid arguments" << endl;
        return;
    }
    timed_cmd->setTimed(true);
    timed_cmd->setTime(alarm_time);
    timed_cmd->execute();
    if (!(timed_cmd->isBG()) && timed_cmd->getJobEntryId() == -1) {
        delete timed_cmd;

    }
}

std::list<TimeoutCommand*> SmallShell::getTimeoutCommandsList() {
    return timeouts;
}

void Command::setTimed(bool timed_status){
    timed = timed_status;
}
