#include "file_system.h"
#include "LinuxShell.h"
//---------------------------------------------------------------
// function:    linux_shell_init                                |
// inputs:      none                                            |
// explanation: intializes processes and mindoes                |
//---------------------------------------------------------------
void linux_shell_init(void)
{
    int i = 0;
    // Set up two processes
    // P0 with uid=0,
    // P1 with uid=1,
    P0.uid = 0;
    P1.uid = 1;

    //set Current Working Directory to 0/NULL
    P0.cwd = 0;
    P1.cwd = 0;

    //set running to our first process
    //and queue to second process.
    g_running   = &P0;
    g_readQueue = &P1;

    //set up minode to have no refeences.
    for(i = 0; i < NUM_MINODES; i++)
    {
         g_minode[i].refCount = 0;
         g_minode[i].dev      = 0;
         g_minode[i].ino      = 0;
         g_minode[i].dirty    = 0;
         g_minode[i].mounted  = 0;
         g_minode[i].mountPtr = 0;
    }
    //set root to 0.
    g_root = 0;
}
