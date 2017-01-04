#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <string.h> // strsignal()
#include <string>
#include <sstream>

#define STRBOOL(B) ((B)?("true"):("false"))

using namespace std;

const long _MB = 1048576L;

struct judgecontext{
    long memory_limit;
    long time_limit;
    char** argv;
    char* input_file;
    char* output_file;
    char* error_file;

    string to_string(){
        ostringstream sout;
        sout << "{";
        sout << "\"MemoryLimit\":" << memory_limit << ",";
        sout << "\"TimeLimit\":" << time_limit << ",";
        sout << "\"InputFile\":\"" << input_file << "\",";
        sout << "\"OutputFile\":\"" << output_file << "\",";
        sout << "\"ErrorFile\":\"" << error_file << "\",";
        sout << "\"Command\":\"";
        for(int i=0;argv[i]!=NULL;++i) {
            if (i>0) sout << " ";
            sout << argv[i];
        }
        sout << "\"";
        sout << "}\n";
        return sout.str();
    }
} ctx;

struct judgeresult{
    // judge result
    bool OK;

    // exit info
    bool exited;
    int exit_code;

    // signal info
    bool signaled;
    int signal;
    string signal_string;
    bool coredump;

    // memory info
    long peak_mem; // maximum_rss (kb)
    long alt_mem;  // minor pagefault * pagesize (kb)

    // time info
    long utime_sec;
    long utime_usec;
    long stime_sec;
    long stime_usec;
    string to_string()
    {
        ostringstream sout;
        sout << "{";
        sout << "\"OK\":" << STRBOOL(OK) << ",";
        sout << "\"Exited\":" << STRBOOL(exited) << ",";
        if(exited) sout << "\"ExitCode\":" << exit_code << ",";
        sout << "\"Signaled\":" << STRBOOL(signaled) << ",";
        if(signaled) {
            sout << "\"Signal\":" << signal << ",";
            sout << "\"SignalStr\":" << signal_string << ",";
            sout << "\"Coredump\":" << STRBOOL(coredump) << ",";
        }
        sout << "\"PeakMemory\":" << peak_mem << ",";
        sout << "\"AltMemory\":" << alt_mem << ",";
        sout << "\"UserTime\":[" << utime_sec << "," << utime_usec << "],";
        sout << "\"SysTime\":[" << stime_sec << "," << stime_usec << "]";
        sout << "}\n";
        return sout.str();
    }
} result;

void print_usage()
{
    puts("./Judge [meomry_limit] [time_limit] [input_file] [output_file] [error_file] [command]");
}

int get_options(const int& argc, char** argv)
{
    if( argc < 7 ) return -1;
    ctx.memory_limit = atol(argv[1]);
    ctx.time_limit = atol(argv[2]);
    if(ctx.time_limit < 0) return -1;
    ctx.input_file = argv[3];
    ctx.output_file = argv[4];
    ctx.error_file = argv[5];
    ctx.argv = argv+6;
    return 0;
}

void run_cmd()
{
    struct rlimit limiter;
    // time limit
    limiter.rlim_cur = ctx.time_limit;
    limiter.rlim_max = ctx.time_limit;
    setrlimit(RLIMIT_CPU, &limiter);

    // memory limit
    if(ctx.memory_limit > 0) {
        limiter.rlim_cur = ctx.memory_limit * _MB;
        limiter.rlim_max = ctx.memory_limit * _MB;
        setrlimit(RLIMIT_AS, &limiter);
    }
    execvp(ctx.argv[0], ctx.argv);
    exit(EXIT_FAILURE);
}

int watch_cmd(pid_t pid) {
    int status = 0;

    if( waitpid(pid, &status, 0) == -1) return -1; // cannot wait

    if( WIFEXITED(status) ) { // return in main or used exit() func
        result.exited = true;
        result.exit_code = WEXITSTATUS(status);
    } else {
        result.exited = false;
    }

    if( WIFSIGNALED(status) ) { // killed by signal
        result.signaled = true;
        result.signal = WTERMSIG(status);
        result.signal_string = string(strsignal(WTERMSIG(status)));
        if( WCOREDUMP(status) ) result.coredump = true;
        else result.coredump = false;
    } else {
        result.signaled = false;
    }


    struct rusage usage;
    if( getrusage(RUSAGE_CHILDREN, &usage) == -1) return -1;
    else {
        result.utime_sec = usage.ru_utime.tv_sec;
        result.utime_usec = usage.ru_utime.tv_usec;
        result.stime_sec = usage.ru_stime.tv_sec;
        result.stime_usec = usage.ru_stime.tv_usec;
        result.peak_mem = usage.ru_maxrss;
        result.alt_mem = (long)((long long)usage.ru_minflt * (long long)getpagesize() / 1024LL);
    }
    return 0;
}

int main(int argc, char** argv)
{
    if(get_options(argc, argv) < 0) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if(pid == 0) { // child process
        freopen(ctx.input_file, "r", stdin);
        freopen(ctx.output_file, "w", stdout);
        freopen(ctx.error_file, "w", stderr);
        run_cmd();
    } else if(pid < 0) {
        puts("fork err");
        exit(EXIT_FAILURE);
    } else { // parent process
        if ( watch_cmd(pid) == 0 ) result.OK = true;
        else result.OK = false;
    }
    
    string strres = result.to_string();
    printf("%s\n", strres.c_str());
    return 0;
}
