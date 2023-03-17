#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>   // waitpid
#include <sys/mount.h>  // mount
#include <fcntl.h>      // open
#include <unistd.h>     // execv, sethostname, chroot, fchdir
#include <sched.h>      // clone
#define STACK_SIZE (1024 * 1024) // 定义子进程空间大小 1M
char child_stack[STACK_SIZE];//子进程栈空间大小
int pipefd[2];
char* const container_args[] = {
    "/bin/bash",
    NULL
};

void set_map(char* file, int inside_id, int outside_id, int len) {
    FILE* mapfd = fopen(file, "w");
    if (NULL == mapfd) {
        perror("open file error");
        return;
    }
    fprintf(mapfd, "%d %d %d", inside_id, outside_id, len);
    fclose(mapfd);
}

void set_uid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char file[256];
    sprintf(file, "/proc/%d/uid_map", pid);
    set_map(file, inside_id, outside_id, len);
}

void set_gid_map(pid_t pid, int inside_id, int outside_id, int len) {
    char file[256];
    sprintf(file, "/proc/%d/gid_map", pid);
    set_map(file, inside_id, outside_id, len);
}

int child(void* arg)
{
    printf("Container [%5d] - inside the container!\n", getpid());
    char ch;
    close(pipefd[1]);
    read(pipefd[0], &ch, 1);
    sethostname("container_1",10); /* 设置hostname */
    printf("Child - %d\n", getpid());

    /* 把自己加入cgroup中,getpid()为得到线程的系统tid） */
    char cmd[128];
    sprintf(cmd, "echo %ld >> /sys/fs/cgroup/cpu/mytest/tasks", getpid());
    system(cmd); 
    sprintf(cmd, "echo %ld >> /sys/fs/cgroup/memory/mytest/tasks", getpid());
    system(cmd);
    //执行一个命令，在子进程中保持，使用exit退出
    execv(container_args[0], container_args);
    printf("Something's wrong!\n");
    return 1;
}
 
int main()
{
    printf("Parent [%5d] - start a container!\n", getpid());
    const int gid=getgid(), uid=getuid();
    pipe(pipefd);
    /* 设置CPU利用率为50% */
    //创建一个目录，参数755为设置组读写执行权限
    mkdir("/sys/fs/cgroup/cpu/mytest", 755);
    system("echo 50000 > /sys/fs/cgroup/cpu/mytest/cpu.cfs_quota_us");
    /* 设置内存大小为65536 */
    mkdir("/sys/fs/cgroup/memory/mytest", 755);
    system("echo 500M > sys/fs/cgroup/memory/mytest/memory.limit_in_bytes");
    //验证方式可参考云计算实验课
    int pid = clone(child, child_stack+STACK_SIZE, CLONE_NEWUTS| //uts 域名隔离
                                                    CLONE_NEWNS| //Mount namespace
                                                    CLONE_NEWIPC|//进程通信
                                                    CLONE_NEWPID|
                                                    CLONE_NEWUSER|//ps -ef
                                                    CLONE_NEWNET|
                                                    SIGCHLD, // 子进程退出时会发出信号给父进程 避免僵尸进程, 
                                                    NULL);

    set_uid_map(pid, 0, uid, 1);
    set_gid_map(pid, 0, gid, 1);
    /* 通知子进程 */
    close(pipefd[1]);
    if (pid == -1) {
        perror("clone:");
        exit(1);
    }

    waitpid(pid, NULL, 0);
    printf("Parent - child(%d) exit\n", pid);
    printf("Parent - container stopped!\n");
    return 0;
}