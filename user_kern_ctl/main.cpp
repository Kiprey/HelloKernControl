//
//  user_kern_ctl.cpp
//  HelloKernControl
//
//  Created by Kiprey on 2021/8/10.
//

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/kern_control.h>
#include <sys/sys_domain.h>
#include "../HelloKernControl/HelloKernControl.h"

int main(int argc, char* const*argv)
{
    struct ctl_info ctl_info;
    struct sockaddr_ctl sc;
    char str[MAX_STRING_LEN];
    int sock = socket(PF_SYSTEM, SOCK_DGRAM, SYSPROTO_CONTROL);
    if (sock < 0)
        return -1;
    bzero(&ctl_info, sizeof(struct ctl_info));
    strcpy(ctl_info.ctl_name, "com.osxkernel.HelloKernControl");
    if (ioctl(sock, CTLIOCGINFO, &ctl_info) == -1)
        return -1;
    bzero(&sc, sizeof(struct sockaddr_ctl));
    sc.sc_len = sizeof(struct sockaddr_ctl);
    sc.sc_family = AF_SYSTEM;
    sc.ss_sysaddr = SYSPROTO_CONTROL;
    sc.sc_id = ctl_info.ctl_id;
    sc.sc_unit = 0;
    if (connect(sock, (struct sockaddr *)&sc, sizeof(struct sockaddr_ctl)))
        return -1;
    /* Get an existing string from the kernel */
    unsigned int size = MAX_STRING_LEN;
    if (getsockopt(sock, SYSPROTO_CONTROL, HELLO_CONTROL_GET_STRING, &str, &size) == -1)
        return -1;
    printf("kernel string is: %s\n", str);
    /* Set a new string */
    strcpy(str, "Hello Kernel, here's your new string, enjoy!");
    if (setsockopt(sock, SYSPROTO_CONTROL, HELLO_CONTROL_SET_STRING, str, (socklen_t)strlen(str)) == -1)
        return -1;
    
    /* echo a string */
    int ret = 0;
    const char * msg = "hello kernel (from user)";
    if((ret = send(sock, msg, strlen(msg), 0)) <= 0)
        perror("send: ");
    char buf[255];
    if((ret = recv(sock, buf, 255, 0)) < 0)
        perror("recv: ");
    printf("from kernel: [%s]\n", buf);
    
    close(sock);
    return 0;
}
