/***************************************************************************
@file         main.cpp
@team
张崴	          1709853D-I011-0090	     ZHANG WEI
柳明辰	 17098537-I011-0055      LIU MINGCHEN
潘广峣	 1709853D-I011-0061	     PAN GUANG YAO
****************************************************************************/
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <list>
#include <string.h>
#include <iostream>

using namespace std;

#define RESET       "\033[0m"
#define BOLDBLUE    "\033[1m\033[34m"  // bold blue
#define BOLDGREEN   "\033[1m\033[32m"  // Bold Green
#define BOLDRED     "\033[1m\033[31m"  // Bold Red

#define BUF_SIZE 64
#define MAX_PATH_LEN 1000
#define MAX_JOBS 99

int COMMAND_LEN = 0;
int BG = 0;  // 1 BG, 0 FG
int NEXT_JID = 1;  // The next unused JID
string LINE = "\0";  // Unprocessed command statement
list<int> PIDS;  // record the PID
list<string> HISTORYS;  // record the history commonds

struct job_t  // The job struct
{
    pid_t pid;              //job PID
    int jid;                // job ID [1, 2, ...]
    int state;              // UNDEF(0), FG(1), BG(2), or ST(3)
    string line;        // command line
} JOBS[MAX_JOBS];

enum {
    ERROR_FILE_NOT_EXIST = 4,
    ERROR_OPEN_FILE,

    /* error of redirection */
    ERROR_IN,
    ERROR_IN_MISS_PARAMETER,
    ERROR_OUT,
    ERROR_OUT_MISS_PARAMETER,


    /* error of pipes */
    ERROR_PIPE_MISS_PARAMETER
};


/**
   @brief List current task list
 */
void list_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].pid != 0) {
            printf("[%d] ", JOBS[i].jid);
            switch (JOBS[i].state) {
                case 1:
                    printf("Foreground ");
                    break;
                case 2:
                    printf("Running ");
                    break;
                case 3:
                    printf("Suspended ");
                    break;
                default:
                    fprintf(stderr, "error: job[%d].state=%d ", i, JOBS[i].state);
            }
            printf("%s\n", JOBS[i].line.data());
        }
    }
}


/**
   @brief Find the job by PID on the job list
   @param pid(pid_t)
   @return PID-jobs struct
 */
struct job_t *getjob_pid(pid_t pid) {
    if (pid < 1) {
        return NULL;
    }

    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].pid == pid) {
            return &JOBS[i];
        }

    }
    return NULL;
}


/**
   @brief Find the job by JID on the job list
   @param pid(pid_t)
   @return JID-jobs struct
 */
struct job_t *getjob_jid(int jid) {
    if (jid < 1) {
        return NULL;
    }

    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].jid == jid && JOBS[i].jid != 0) {
            return &JOBS[i];
        }
    }
    return NULL;
}


/**
   @brief initialize jjob
   @param job struct
 */
void reset_job(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = 0;
    job->line = (char *) "\0";
}


/**
   @brief Get the currently available maximum JID
   @return max_j(int)
 */
int max_jid() {
    int max_j = 0;

    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].jid > max_j) {
            max_j = JOBS[i].jid;
        }
    }
    return max_j;
}


/**
   @brief Delete the task that has ended
   @param The pid corresponding to the end task
   @return Find the corresponding task and delete to return 1, otherwise return 0
 */
int delete_job(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].pid == pid) {
            reset_job(&JOBS[i]);
            NEXT_JID = max_jid() + 1;  // get the largest JID currently available
            return 1;
        }
    }
    return 0;
}


/**
   @brief Add task
       @param pid: Task PID, state: task state (UNDEF (0), FG (1), BG (2), or ST (3)), line: corresponding command
       @return Return 1 if successful, otherwise 0
 */
int add_job(pid_t pid, int state, string line) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].jid == 0) {
            if (NEXT_JID > MAX_JOBS) {
                break;
            }

            JOBS[i].pid = pid;
            JOBS[i].state = state;
            JOBS[i].jid = NEXT_JID++;
            JOBS[i].line = line;

            return 1;
        }
    }
    printf("Too many jobs!\n");
    return 0;
}


/**
  @brief Get the PID of the running frontend
   @return Return 1 if successful, otherwise 0
 */
pid_t current_fgpid() {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (JOBS[i].state == 1) {
            return JOBS[i].pid;
        }
    }

    return 0;
}


/**
   @brief Initialize JOBS structure
 */
void initialize_jobs() {
    for (int i = 0; i < MAX_JOBS; i++) {
        reset_job(&JOBS[i]);
    }
    return;
}


/**
   @brief display all the historical commands
 */
void display_histroy() {
    for (list<string>::iterator i = HISTORYS.begin();
         i != HISTORYS.end();
         i++) {
        cout << *i << endl;
    }
}


/**
   @brief record the historical command
   @param historical command
 */
void add_history(string command) {
    if (HISTORYS.size() >= 5000) {
        HISTORYS.pop_front();
        HISTORYS.push_back(command);

        return;
    }
    HISTORYS.push_back(command);
}


/**
   @brief display the pids of the 5 recent child processes
 */
void display_pid() {
    int n = PIDS.size();

    for (list<int>::reverse_iterator i = PIDS.rbegin();
         i != PIDS.rend();
         i++) {
        cout << "[" << to_string(n--) << "] " << to_string(*i) << endl;
    }
}


/**
   @brief record the pids of the 5 recent child processes
   @param child processes
 */
void add_pid(int pid) {
    if (PIDS.size() >= 5) {
        PIDS.pop_front();
        PIDS.push_back(pid);

        return;
    }
    PIDS.push_back(pid);
}


/**
   @brief Signal processing
 */
void sigchld_handler(int sig) {
    pid_t pid;
    int status;
    sigset_t sigset_mask;

    sigemptyset(&sigset_mask);
    sigaddset(&sigset_mask, SIGCHLD);  // Congestion, to prevent child processes from terminating early

    pid = waitpid(-1, &status, WUNTRACED | WCONTINUED);

    if (WIFSTOPPED(status) && WSTOPSIG(status) == SIGTSTP)  // ctrl + z
    {
        struct job_t *job = getjob_pid(pid);
        job->state = 3;

        printf("foreground job [%d] has been stopped.\n", job->jid);
    } else if (WIFSIGNALED(status) && WTERMSIG(status) == SIGINT)  // ctrl + c
    {
        struct job_t *job = getjob_pid(pid);
        printf("foreground job [%d] has been killed.\n", job->jid);

        delete_job(pid);
    } else if (WIFEXITED(status))  // normal exit
    {
        sigprocmask(SIG_BLOCK, &sigset_mask, NULL);
        delete_job(pid);
        sigprocmask(SIG_UNBLOCK, &sigset_mask, NULL);
    }

    return;
}


/**
    @brief fg:Change the task from the background to run in the foreground, bg: run the task in the background, continue: continue to run the task
   @param tokens: Corresponding command
 */
void fg_bg_continue(char *tokens[]) {
    if (tokens[1] == NULL) {
        fprintf(stderr, "%s commands require JID.\n", tokens[0]);
        return;
    }

    pid_t pid;
    int jid;
    int status;
    struct job_t *job;

    jid = atoi(tokens[1]);
    if (jid <= 0) {
        fprintf(stderr, "ERROR JID.\n");
        return;
    }

    job = getjob_jid(jid);
    if (job == NULL) {
        fprintf(stderr, "%s\n", "no such job.");
        return;
    }

    pid = job->pid;

    if (strcmp(tokens[0], "bg") == 0)  // bg
    {
        kill(pid, SIGCONT);
        job->state = 2;
        printf("[%d] %s runs in the background\n", job->jid, job->line.data());
    } else if ((strcmp(tokens[0], "fg") == 0))  // fg
    {
        job->state = 1;
        while (pid == current_fgpid());
    } else  // continue
    {
        kill(pid, SIGCONT);
        job->state = 1;
        while (pid == current_fgpid());
    }

    return;
}


/**
   @brief builtin command go, exit and pid, history, jobs, fg, bg, continue
   @param NULL-terminated array of tokens
   @return 1 if it is an internal command    -1 if it isn't an internal command    0 to terminate execution
 */
int buildin(char *tokens[]) {
    if (strcmp(tokens[0], "exit") == 0) {
        return 0;
    } else if (strcmp(tokens[0], "go") == 0) {
        if (chdir(tokens[1])) {
            cout << "OS: go: " << tokens[1] << ": No such file or directory" << endl;
        }
        return 1;
    } else if (strcmp(tokens[0], "pid") == 0) {
        display_pid();

        return 1;
    } else if (strcmp(tokens[0], "history") == 0 && tokens[1] == NULL) {
        display_histroy();

        return 1;
    } else if (strcmp(tokens[0], "jobs") == 0 && tokens[1] == NULL) {
        list_jobs();

        return 1;
    } else if (strcmp(tokens[0], "bg") == 0 ||
               strcmp(tokens[0], "fg") == 0 ||
               strcmp(tokens[0], "continue") == 0) {
        fg_bg_continue(tokens);
        return 1;
    }

    return -1;
}


/**
  @brief Determine if there is a redirection and execute the code
  @param array of tokens which is one command(Before the semicolon)，tokens_start starting position，tokens_end End position
  @return status or error
 */
int call_rediction(char *tokens[], int tokens_start, int tokens_end) {
    // Determine if there is a redirect
    int in = 0, out = 0; // out 1 means rewriting ，2 means append，>2 means error
    char *infile = NULL;  // Enter the redirect file name
    char *outfile = NULL;  // Output the redirect file name
    int command_poisiton = (tokens_end - 1);

    for (int i = tokens_start; i < tokens_end; i++) {
        if (strcmp(tokens[i], "<") == 0)  // Enter the redirect
        {
            if ((i + 1) < tokens_end) {
                infile = tokens[i + 1];
            } else {
                return ERROR_IN_MISS_PARAMETER;  // The required parameters are missing after the redirection symbol is entered
            }

            if (in != 0) {
                return ERROR_IN;  // Too many redirect symbols be inputted
            } else {
                in++;

                command_poisiton = i - 1;
            }
        } else if (strcmp(tokens[i], ">") == 0 || strcmp(tokens[i], ">>") == 0) {
            if ((i + 1) < tokens_end) {
                outfile = tokens[i + 1];
            } else {
                return ERROR_OUT_MISS_PARAMETER;  // The required parameters are missing after the output redirection symbol
            }

            if ((in == 1 || in == 0) && out == 0) {
                out += (strcmp(tokens[i], ">") == 0 ? 1 : 2);
                command_poisiton = (in == 1 ? command_poisiton : i - 1);
            } else {
                return ERROR_OUT;  // Too many redirect symbols be outputted
            }
        }
    }

    pid_t pid;
    int status;
    sigset_t sigset_mask;

    sigemptyset(&sigset_mask);
    sigaddset(&sigset_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sigset_mask, NULL);  // Clogging to prevent child processes from advancing

    pid = fork();

    if (pid == 0)  // Child process
    {
        sigprocmask(SIG_UNBLOCK, &sigset_mask, NULL);

        signal(SIGTSTP, SIG_DFL);  // Resume ctrl + z signal capture
        signal(SIGINT, SIG_DFL);  // Resume ctrl + c signal capture

        if (in == 1) {
            freopen(infile, "r", stdin);  // Input redirection
        }

        if (out == 1 || out == 2) {
            out == 1 ? freopen(outfile, "w", stdout) : freopen(outfile, "a", stdout);  // Rewrite or append
        }

        char **t = (char **) malloc(BUF_SIZE * sizeof(char *));
        int j = 0;
        for (int i = tokens_start; i <= command_poisiton; i++) {
            t[j] = tokens[i];
            j++;
        }

        if (buildin(t) != -1) {
            exit(EXIT_SUCCESS);
        } else {
            t[j + 1] = NULL;
        }

        if (execvp(t[0], t) == -1) {
            cout << BOLDRED << t[0] << ": Commands not found!" RESET << endl;
            exit(-1);
        }
    } else if (pid < 0) {
        perror("REDICTION FORK");
    } else {
        add_pid((int) pid);
        add_job(pid, (BG + 1), LINE);
        sigprocmask(SIG_UNBLOCK, &sigset_mask, NULL);

        if (!BG) {
            while (pid == current_fgpid());
        } else {
            cout << to_string(pid) << "  " << LINE << " run in the background." << endl;
        }

        return 1;
    }

    return -1;
}


/**
  @brief Determines if there is a pipe character
  @param tokens array，start_index-start，end_index-end
  @return 0 ,status or error
 */
int pipe_operator(char *tokens[], int start_index, int end_index) {
    int pipe_index = -1;  // Determines if there is a pipe character
    int position = start_index;

    while (position < end_index) {
        if (strcmp(tokens[position], "|") == 0) {
            if ((position + 1) == COMMAND_LEN) {
                return ERROR_PIPE_MISS_PARAMETER;  // pipe command'|'No subsequent instructions, parameters missing
            }

            pipe_index = position;
            break;
        }
        position++;
    }

    if (pipe_index == -1)  // No pipe
    {
        return call_rediction(tokens, start_index, end_index);
    }

    int fd[2];
    if (pipe(fd) == -1)  // Communication pipe
    {
        perror(BOLDRED "PIPE" RESET);
    }

    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0)  // Child process
    {
        close(fd[0]);  // Close the child process read end
        dup2(fd[1], STDOUT_FILENO);  // Redirect standard output to fd[1]

        call_rediction(tokens, start_index, pipe_index);  // An error occurs if there is a return
        exit(1);
    } else if (pid < 0) {
        perror(BOLDRED "PIPE FROK" RESET);
    } else {
        int status;
        waitpid(pid, &status, 0);

        if (position < end_index) {
            close(fd[1]);  // Close the parent write side
            dup2(fd[0], STDIN_FILENO); // Redirect standard output to fd[0]

            return pipe_operator(tokens, pipe_index + 1, end_index); // Recursively executes subsequent instructions
        }
    }
    return 0;
}


/**
  @brief Determine if there is a logical sign
  @param array of tokens which is one command(Before the semicolon)
  @return return -1 or 0 or someting else
 */
int os_launch(char *tokens[], int start_index, int end_index) {
    for (int i = start_index; i < end_index; i++) {
        if (strcmp(tokens[i], "||") == 0) {
            if (pipe_operator(tokens, start_index, i) == 65280) {
                return os_launch(tokens, i + 1, end_index);
            }
            return 0;
        } else if (strcmp(tokens[i], "&&") == 0) {
            if (pipe_operator(tokens, start_index, i) == 0) {
                return os_launch(tokens, i + 1, end_index);
            } else {
                return -1;
            }
        }
    }

    return pipe_operator(tokens, start_index, end_index);
}


/**
  @brief Print error message
 */
void print_error(int error) {
    switch (error) {
        case ERROR_FILE_NOT_EXIST:
            cout << BOLDRED << "File does not exist" << RESET;
            break;
        case ERROR_OPEN_FILE:
            cout << BOLDRED << "File open/creation failed" << RESET;
            break;
        case ERROR_IN:
            cout << BOLDRED << "Multiple input redirection symbols appear" << RESET;
            break;
        case ERROR_IN_MISS_PARAMETER:
            cout << BOLDRED << "The required parameters are missing from the input redirection symbol" << RESET;
            break;
        case ERROR_OUT:
            cout << BOLDRED << "Multiple output redirection symbols appear" << RESET;
            break;
        case ERROR_OUT_MISS_PARAMETER:
            cout << BOLDRED << "The required parameters are missing from the output redirection symbol" << RESET;
            break;
        case ERROR_PIPE_MISS_PARAMETER:
            cout << BOLDRED << "The necessary parameters are missing from the pipe symbol" << RESET;
            break;
    }
}


/**
  @brief execute a command
  @param array of tokens which is one command(Before the semicolon)
  @return return 1 or 0
 */
int os_execute(char *tokens[]) {
    int b = buildin(tokens);
    if (b != -1)  // user input is null
    {
        return b;
    }


    /* Gets file identifiers for standard input and output */
    int in_fd = dup(STDIN_FILENO);
    int out_fd = dup(STDOUT_FILENO);

    int exit_code = os_launch(tokens, 0, COMMAND_LEN);

    /* Restore standard input and output redirection */
    dup2(in_fd, STDIN_FILENO);
    dup2(out_fd, STDOUT_FILENO);

    fflush(stdout);

    if (exit_code >= 4) {
        print_error(exit_code);

        exit(exit_code);
    }

    return 1;
}


/**
   @brief split the line into tokens
   @param line，Specify split symbol
   @return array of tokens which is one command(Before the semicolon)
 */
char **split_line(char *line, const char symbol[]) {
    int position = 0;

    char **tokens = (char **) malloc(BUF_SIZE * sizeof(char *));
    char *token;

    if (!tokens)  // determine whether the space allocation is successful or not
    {
        fprintf(stderr, "Malloc Failure\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, symbol);  // splits the string with the specified character
    while (token != NULL) {
        tokens[position] = token;
        position++;

        // consider whether there is enough memory allocated
        // but an input command line has a maximum length of 80 characters, buffer size is 64.
        // don't need other operations

        token = strtok(NULL, symbol);
    }

    // Determine if it is running in the background
    if (strcmp(tokens[position - 1], "&") == 0 && symbol == " ") {
        BG = 1;
        tokens[--position] == NULL;
        LINE = LINE.substr(0, LINE.size() - 1);
    }

    COMMAND_LEN = position;

    return tokens;
}


/**
   @brief read a line of input
   @return the line which is processed
 */
string read_line() {
    string line;

    getline(cin, line);

    // deletes the spaces before and after the string
    line.erase(0, line.find_first_not_of(" "));
    line.erase(line.find_last_not_of(" ") + 1);

    if (line.length() > 80)  // limit the length for usr's input
    {
        cout << "An input command line has a maximum length of 80 characters, please enter agagin." << endl;
        return read_line();
    }

    LINE = line;

    add_history(line);

    return line;
}


/**
   @brief print current path
 */
void print_path() {
    char path_name[MAX_PATH_LEN];  // path name

    getcwd(path_name, MAX_PATH_LEN);  // get current path

    cout << BOLDBLUE << "[OS server:" << (string) path_name << "]" << RESET;
    cout << "$ ";
    fflush(stdout);

    return;
}


/**
   @brief getting command and executing it
 */
void os_loop() {
    char *line;  // command and arguments
    char **commands;  // command and arguments, separate together
    char **tokens;  // command and arguments, separate save
    int num_statements;  // Number of statements
    int status;  // control loop
    string tmp;

    do {
        fflush(stdin);
        fflush(stdout);

        print_path();

        tmp = read_line();
        if (tmp.empty()) {
            continue;
        }
        line = const_cast<char *>(tmp.c_str());

        commands = split_line(line, ";");
        num_statements = COMMAND_LEN;

        for (int i = 0; i < num_statements; i++) {
            tokens = split_line(commands[i], " ");
            status = os_execute(tokens);
        }

    } while (status);
}


/**
   @brief blessing
 */
void a_plus() {
    cout << BOLDGREEN << R"(OS Shell  OS Shell  OS Shell  OS Shell  OS Shell  OS Shell  OS Shell)" << endl;
    cout << R"(OS                          _ooOoo_                               OS)" << endl;
    cout << R"(Shell                      o8888888o                           Shell)" << endl;
    cout << R"(OS                         88" . "88                              OS)" << endl;
    cout << R"(Shell                      (| ^_^ |)                           Shell)" << endl;
    cout << R"(OS                         O\  =  /O                              OS)" << endl;
    cout << R"(Shell                   ____/`---'\____                        Shell)" << endl;
    cout << R"(OS                    .'  \\|     |//  `.                         OS)" << endl;
    cout << R"(Shell                /  \\|||  :  |||//  \                     Shell)" << endl;
    cout << R"(OS                  /  _||||| -:- |||||-  \                       OS)" << endl;
    cout << R"(Shell               |   | \\\  -  /// |   |                    Shell)" << endl;
    cout << R"(OS                  | \_|  ''\---/''  |   |                       OS)" << endl;
    cout << R"(Shell                \  .-\__  `-`  ___/-. /                   Shell)" << endl;
    cout << R"(OS                ___`. .'  /--.--\  `. . ___                     OS)" << endl;
    cout << R"(Shell            ."" '<  `.___\_<|>_/___.'  >'"".              Shell)" << endl;
    cout << R"(OS            | | :  `- \`.;`\ _ /`;.`/ - ` : | |                 OS)" << endl;
    cout << R"(Shell          \  \ `-.   \_ __\ /__ _/   .-` /  /             Shell)" << endl;
    cout << R"(OS      ========`-.____`-.___\_____/___.-`____.-'========         OS)" << endl;
    cout << R"(Shell                         `=---='                          Shell)" << endl;
    cout << R"(OS      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^        OS)" << endl;
    cout << R"(Shell           Buddha bless       NOBUG     A+A+A+            Shell)" << endl;
    cout << R"(OS Shell  OS Shell  OS Shell  OS Shell  OS Shell  OS Shell  OS Shell)" << RESET << endl;

    fflush(stdout);

    return;
}


/**
   @brief main entry point
   @param argc(int) argument count
   @param argv(char*) argument vector
   @return status code
 */
int main(int argc, char *argv[]) {
    a_plus();

    initialize_jobs();

    signal(SIGINT, SIG_IGN);  // ignore ctrl + c
    signal(SIGTSTP, SIG_IGN);  // ignore ctrl + z
    signal(SIGCHLD, sigchld_handler);  // signal processing

    os_loop();

    return 0;
}
