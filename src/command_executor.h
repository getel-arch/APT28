#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

#include <windows.h>

/**
 * Execute a command with cmdline evasion to bypass sysmon/EDR logging
 * 
 * This function creates a process in suspended state with a spoofed command line,
 * then patches the real command line in the PEB before resuming execution.
 * The spoofed cmdline shows only the executable name, hiding actual arguments
 * from process monitoring tools.
 * 
 * @param command The full command to execute (e.g., "cmd.exe /k echo test")
 * @return TRUE on success, FALSE on failure
 */
BOOL executeCommandWithEvasion(const char *command);

#endif // COMMAND_EXECUTOR_H
