

// TERMINAL_MAX_LINE
int Kernel_TtyWrite(int tty_id, void *buf, int len) {
    // acquire tty_id's corresponding lock
    // copy buf in chunks of length < TERMINAL_MAX_LINE to Kernel's memory
    // for each chunk:
        // while (!tty_idCanTransfer):
            // Kernel_CvarWait(cvar_tty_id)
        // TtyTransmit(tty_id, buf_chunk_copy, len_of_chunk);
        // tty_idCanTransfer = false
    // release lock
}

int Kernel_TtyRead(int tty_id, void *buf, int len){
    // read from the buffer of the corresponding tty_idBuf
}