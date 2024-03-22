// Copyright Chad Engler

#include "signal.h"

#include "errno.h"
#include "stdlib.h"

#include "he/core/thread.h"

extern "C"
{
    _Hidden static sighandler_t s_signalHandlers[NSIG]{};

    _Hidden static void default_sig_action(int sig)
    {
        switch (sig)
        {
            case SIGABRT:
            case SIGBUS:
            case SIGFPE:
            case SIGILL:
            case SIGQUIT:
            case SIGSEGV:
            case SIGSYS:
            case SIGTRAP:
            case SIGXCPU:
            case SIGXFSZ:
                abort();
                break;

            case SIGALRM:
            case SIGHUP:
            case SIGINT:
            case SIGKILL:
            case SIGPIPE:
            case SIGTERM:
            case SIGUSR1:
            case SIGUSR2:
            case SIGPOLL:
            case SIGPROF:
            case SIGVTALRM:
                _Exit(128 + sig);
                break;
        }
    }

    sighandler_t signal(int sig, sighandler_t handler)
    {
        if (sig < 0 || sig >= _NSIG)
        {
            errno = EINVAL;
            return SIG_ERR;
        }

        sighandler_t oldHandler = s_signalHandlers[sig];
        s_signalHandlers[sig] = handler;
        return oldHandler;
    }

    int raise(int sig)
    {
        if (sig < 0 || sig >= _NSIG)
        {
            errno = EINVAL;
            return -1;
        }

        const sighandler_t handler = s_signalHandlers[sig];

        if (handler == SIG_DFL)
        {
            default_sig_action(sig);
        }
        else if (handler != SIG_IGN && handler != SIG_ERR)
        {
            handler(sig);
        }

        return 0;
    }
}
