int Kernel_TtyWrite(int tty_id, void *buf, int len){
    // set up a pipe that somehow the terminal can access (do this in KernelStart)
    // use the pipe!
}

int Kernel_TtyRead(int tty_id, void *buf, int len){
    // use the pipes!
}