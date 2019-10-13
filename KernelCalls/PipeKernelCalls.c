// DEFAULT_BUFFER_SIZE 50

int Kernel_PipeInit(int *pipe_idp){
    // creates a pipe in PipeMap
        // pipe_id -> malloc'd DEFAULT_BUFFER_SIZE
    // assigns pipe_id to pipe_idp
    // increment pipeCount
    // if error return ERROR else 0
}

// (assume buf.size >= len)
int Kernel_PipeRead(int pipe_id, void *buf, int len){
    // pipe = PipeMap.getItem(pipe_id)
    // for i in len:
        // if pipe.isEmpty():
            // block process
        // byte = pipe.popByte()
        // buf[i] = byte
    // if error return ERROR else 0
}

// (assume buf.size >= len)
int Kernel_PipeWrite(int pipe_id, void *buf, int len){
    // pipe = PipeMap.getItem(pipe_id)
    // for i in len:
        // pipe.pushByte(buf[i])
    // if error return ERROR else 0
}