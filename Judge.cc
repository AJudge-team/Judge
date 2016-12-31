#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <cstdlib>
#include <cstdio>
#include <errno.h>
#include <string.h> // strsignal()


using namespace std;

const long _MB = 1048576L;
struct judgecontext{
    long memory_limit;
    long time_limit;
    char** argv;

    int signal;
    long peak_mem;
    long alt_mem;
    long utime_sec;
    long utime_usec;
    long stime_sec;
    long stime_usec;
    
} ctx;

void print_usage()
{
    puts("./Judge -m [meomry limit] -t [time limit] [command]");
    puts("-m : memory limit in bytes");
    puts("-t : time limit in seconds");
}

int get_options(const int& argc, char** argv)
{
    if( argc < 6) return -1;
    ctx.memory_limit = atol(argv[2]);
    ctx.time_limit = atol(argv[4]);
    if(ctx.time_limit < 0) return -1;
    ctx.argv = argv+5;
    return 0;
}

void print_ctx()
{
    puts("Judge Settigs");
    printf("memory limit : %ld\n", ctx.memory_limit);
    printf("time limit : %ld\n", ctx.time_limit);
    printf("command : \"");
    for(int i=0;ctx.argv[i]!=NULL;++i) {
        if (i>0) printf(" ");
        printf("%s", ctx.argv[i]);
    }
    printf("\"\n");
    puts("-------------");
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
    execv(ctx.argv[0], ctx.argv);
    printf("FAILED : [%d] = %s\n", errno, strerror(errno));
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv)
{
    if(get_options(argc, argv) < 0) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    print_ctx();

    pid_t pid = fork();
    if(pid == 0) { // child process
        run_cmd();
    } else if(pid < 0) {
        puts("fork err");
        exit(EXIT_FAILURE);
    } else { // parent process
        int status = 0;
        if( waitpid(pid, &status, 0) == -1) {
            puts("waitpid err");
            exit(EXIT_FAILURE);
        }
        if( WIFEXITED(status) ) {
            printf("ExitCode : %d\n", WEXITSTATUS(status));
        }
        {
            struct rusage usage;
            if( getrusage(RUSAGE_CHILDREN, &usage) == -1) {
                puts("getrusage fail");
                exit(EXIT_FAILURE);
            }
            printf("user cpu time      : %ld.%ld\n", usage.ru_utime.tv_sec, \
                    usage.ru_utime.tv_usec%1000000L);
            printf("system cpu time    : %ld.%ld\n", usage.ru_stime.tv_sec, \
                    usage.ru_stime.tv_usec%1000000L);
            printf("maximum RSS        : %ld\n", usage.ru_maxrss);
            printf("ru_minflt          : %ld\n", usage.ru_minflt);
        }
        if( WIFSIGNALED(status) ) {
            printf("SIGNALED : [%d] = %s\n", WTERMSIG(status), strsignal(WTERMSIG(status)));
            if( WCOREDUMP(status) ) puts("COREDUMP");
        }
    }
    return 0;
}
