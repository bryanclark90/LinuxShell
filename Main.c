#include "LinuxShell.h"
#include "file_system.h"

int main(void)
{
    printf("Welcome, to my linux shell!\n");

    linux_shell_init();

    if(fs_mount_root() == FAILURE)
    {
        printf("FAILURE: Mounting failed.\n");
        exit(1);
    }
    else
    {
        printf("Connection Established!\n");
    }
}
