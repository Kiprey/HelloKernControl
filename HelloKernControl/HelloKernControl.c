//
//  HelloKernControl.c
//  HelloKernControl
//
//  Created by Kiprey on 2021/8/10.
//

#include <libkern/libkern.h>
#include <libkern/OSMalloc.h>
#include <mach/mach_types.h>
#include <sys/kern_control.h>
#include <sys/proc.h>
#include <sys/mbuf.h>

#include "HelloKernControl.h"

static char g_string_buf [256];
static int g_clients_connected = 0;

static int hello_ctl_send_data_to_user(kern_ctl_ref kctlref, u_int32_t unit, void* buf, size_t len, int flags)
{
    printf("hello_ctl_send_data_to_user called: \"%s\"", buf);
    ctl_enqueuedata(kctlref, unit, buf, len, 0);
    return 0;
}

static errno_t hello_ctl_recv_data_from_user(kern_ctl_ref kctlref, u_int32_t unit, void *unitinfo, mbuf_t m, int flags)
{
    char buf[MAX_STRING_LEN];
    const char* prefix = "recv from user: ";
    strcpy(buf, prefix, strlen(prefix));
    mbuf_copydata(m, 0, MAX_STRING_LEN - strlen(prefix), buf + strlen(prefix));
    printf("hello_ctl_recv_data_from_user: \"%s\"\n", buf);
    mbuf_freem(m);
    return hello_ctl_send_data_to_user(kctlref, unit, buf, MAX_STRING_LEN, flags);
}

static int hello_ctl_connect(kern_ctl_ref ctl_ref, struct sockaddr_ctl *sac, void** unitinfo)
{
    printf("process with pid=%d connected\n", proc_selfpid());
    return 0;
}

static errno_t hello_ctl_disconnect(kern_ctl_ref ctl_ref, u_int32_t unit, void* unitinfo)
{
    printf("process with pid=%d disconnected\n", proc_selfpid());
    return 0;
}

static int hello_ctl_get(kern_ctl_ref ctl_ref, u_int32_t unit, void *unitinfo, int opt, void *data, size_t *len)
{
    int ret = 0;
    switch (opt) {
        case HELLO_CONTROL_GET_STRING:
            *len = min(MAX_STRING_LEN, *len);
            strncpy(data, g_string_buf, *len);
            break;
        default:
            ret = ENOTSUP;
            break;
    }
    return ret;
}

static int hello_ctl_set(kern_ctl_ref ctl_ref, u_int32_t unit, void* unitinfo, int opt, void* data, size_t len)
{
    int ret = 0;
    switch (opt) {
        case HELLO_CONTROL_SET_STRING:
            strncpy(g_string_buf, (char*)data, min(MAX_STRING_LEN, len));
            printf("HELLP_CONTROL_SET_STRING: new string set to: \"%s\"\n", g_string_buf);
            break;
        default:
            ret = ENOTSUP;
            break;
    }
    return ret;
}

static struct kern_ctl_reg g_kern_ctl_reg = {
    "com.osxkernel.HelloKernControl",
    0,
    0,
    CTL_FLAG_PRIVILEGED,
    MAX_CTL_SENDSIZE,
    MAX_CTL_RECVSIZE,
    hello_ctl_connect,
    hello_ctl_disconnect,
    hello_ctl_recv_data_from_user,
    hello_ctl_set,
    hello_ctl_get
};

static boolean_t g_filter_registered = FALSE;
static kern_ctl_ref g_ctl_ref;

kern_return_t HelloKernControl_start(kmod_info_t * ki, void *d);
kern_return_t HelloKernControl_stop(kmod_info_t *ki, void *d);

kern_return_t HelloKernControl_start(kmod_info_t * ki, void *d)
{
    strncpy(g_string_buf, DEFAULT_STRING, strlen(DEFAULT_STRING));
    /* Register the control */
    int ret = ctl_register(&g_kern_ctl_reg, &g_ctl_ref);
    if (ret == KERN_SUCCESS)
    {
        g_filter_registered = TRUE;
        return KERN_SUCCESS;
    }
    return KERN_FAILURE;
}

kern_return_t HelloKernControl_stop(kmod_info_t *ki, void *d)
{
    if (g_clients_connected != 0)
        return KERN_FAILURE;
    if (g_filter_registered)
        ctl_deregister(g_ctl_ref);
    return KERN_SUCCESS;
}
