#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <mach/mach.h>
#include <mach/thread_act.h>
#include <iostream>
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>

#include "Engine/Engine.hpp"

void printRegisterState(mach_port_t thread)
{
    arm_thread_state64_t state;
    mach_msg_type_number_t count = ARM_THREAD_STATE64_COUNT;

    if (thread_get_state(thread, ARM_THREAD_STATE64, (thread_state_t)&state, &count) == KERN_SUCCESS)
    {
        std::cout << "Program Counter (PC): " << std::hex << state.__pc << std::endl;
        std::cout << "Link Register (LR): " << std::hex << state.__lr << std::endl;
        std::cout << "Stack Pointer (SP): " << std::hex << state.__sp << std::endl;

        // Print general-purpose registers (X0-X28)
        for (int i = 0; i < 29; ++i)
        {
            std::cout << "Register X" << i << ": " << std::hex << state.__x[i] << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to get thread state." << std::endl;
    }
}

void printCallStack()
{
    const int maxFrames = 128;
    void *callstack[maxFrames];
    int frameCount = backtrace(callstack, maxFrames);          // Get call stack
    char **symbols = backtrace_symbols(callstack, frameCount); // Get symbols for the call stack

    if (symbols)
    {
        std::cout << "Call Stack:" << std::endl;
        for (int i = 0; i < frameCount; ++i)
        {
            std::cout << symbols[i] << std::endl; // Print each symbol (function name and address)
        }
        free(symbols); // Free the memory allocated for symbols
    }
    else
    {
        std::cerr << "Failed to retrieve call stack symbols." << std::endl;
    }
}

void signalHandler(int signum, siginfo_t *info, void *context)
{
    std::cout << "Caught signal " << signum << std::endl;

    if (signum == SIGSEGV)
    {
        std::cout << "Segmentation fault at address: " << info->si_addr << std::endl;

        // Get current thread port
        mach_port_t thread = mach_thread_self();

        // Print register state
        printRegisterState(thread);

        // Print the call stack
        printCallStack();

        // Deallocate the thread port
        mach_port_deallocate(mach_task_self(), thread);

        // Check if the signal is caused by an illegal instruction, memory fault, etc.
        if (info->si_code == SEGV_ACCERR)
        {
            std::cout << "Access error (protection violation)." << std::endl;
        }
        else if (info->si_code == SEGV_MAPERR)
        {
            std::cout << "Address not mapped." << std::endl;
        }
    }

    // Optionally, re-raise the signal with default handling for crash diagnostics.
    signal(signum, SIG_DFL);
    raise(signum);
}

int main()
{
    // Register signal handler
    struct sigaction action;
    action.sa_sigaction = signalHandler;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &action, NULL);

    Engine engine("Hello, Metal!");

    engine.Run();

    return 0;
}
