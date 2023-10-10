Demonstrate an application on Windows using /SUBSYSTEM:WINDOWS which is spawned from the command line and then closed from the command line by CTRL-C or CTRL-BREAK.

See https://learn.microsoft.com/en-us/windows/console/setconsolectrlhandler for registering the pseudo "Console" message pump to process CTRL-* events.

See https://learn.microsoft.com/en-us/windows/console/handlerroutine for the details of the message pump routine.

Not used here, but see https://learn.microsoft.com/en-us/windows/console/generateconsolectrlevent for an example of sending CTRL-* events to process groups.

See https://learn.microsoft.com/en-us/windows/win32/learnwin32/your-first-windows-program to create a Win32 application.

Inspired by https://github.com/electron/electron/issues/5273.
