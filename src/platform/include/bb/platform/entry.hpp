#pragma once

// The application's entry point. The platform backend owns the process
// entry (main, or whatever the system calls) and calls bbMain with the
// command line.
int bbMain(int argc, char* argv[]);
