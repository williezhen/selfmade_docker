package main
  
import (
        "log"
        "os"
        "os/exec"
        "syscall"
)

func main() {

        cmd1 := exec.Command("sh", "-c", "chroot /home/namespace-experiment/centos")
        cmd1.Stdin = os.Stdin
        cmd1.Stdout = os.Stdout
        cmd1.Stderr = os.Stderr
        if err := cmd1.Run() ; err!= nil {
                log.Fatal(err)
        }

        cmd2 := exec.Command("sh")
        cmd2.SysProcAttr = &syscall.SysProcAttr{
                Cloneflags: syscall.CLONE_NEWUTS | syscall.CLONE_NEWIPC | syscall.CLONE_NEWPID | syscall.CLONE_NEWNS | syscall.CLONE_NEWUSER | syscall.CLONE_NEWNET,
        }
        cmd2.Stdin = os.Stdin
        cmd2.Stdout = os.Stdout
        cmd2.Stderr = os.Stderr

        if err := cmd2.Run(); err !=nil {
                log.Fatal(err)
        }

        os.Exit(-1) 
} 

