# DbgSvc Extension
Description of the Windows DbgSvc (DebugDiag) and examples of tricky exceptions / errors. Includes a crude, albeit effective CrashHandler, that ensures memory dumps are created in case of an unhandled exception.

## DebugDiag2
Microsoft has provided a very scriptable debugger, DebugDiag2, which allows you to create dump files under certain circumstances that you can script:
https://www.microsoft.com/en-us/download/details.aspx?id=49924

It is not without merits as this product was initially used to aid developers debug IIS applications.

## Dump files
Dump files can be very tiny containing bare minimum to spanding the entire process, all its memory, handles, threads including thread local storage etc. Search MSDN on MiniDump for further reference.

### Importance of .pdb-files
It cannot be stressed enough the importance of having .pdb files at your disposal; in particular if your code-base reaches outside of the .Net realm. So why not deploy them together with your binaries; it will give so much nicer stack-traces in your logs.

When it comes to .dmp-files, the .pdb-files are only needed at the time of _inspection_ not _creation_, ie. it is paramount that you have the .pdb-files handy when you analyze the .dmp-file, not at the time of crash strictly speaking.

If you have carefully stamped at build time a unique build number and other descriptive strings such as branch name and source revision number (eg. TFS changeset or GIT commit) into your binaries, then you should have no problem getting back to the offending source code line(s).

## Stackoverflow exception examples
On the console, a Stackoverflow exception will manifest itself like this:

```Process is terminated due to StackOverflowException.```

And in the event-log as follows:
```Fault bucket 129478254250, type 5
Event Name: CLR20r3
Response: Not available
Cab Id: 0

Problem signature:
P1: Soex.Csharp.exe
P2: 1.0.0.0
P3: 594119f4
P4: Soex.Csharp
P5: 1.0.0.0
P6: 594119f4
P7: 2
P8: 0
P9: System.StackOverflowException
P10: 
```
or the more subtle manifest, but equally meaning:
```
Fault bucket , type 0
Event Name: APPCRASH
Response: Not available
Cab Id: 0

Problem signature:
P1: <name-of.exe>
P2: 1.0.199.0
P3: 00000000
P4: MSVCR120_CLR0400.dll
P5: 12.0.52512.0
P6: 00000000
P7: c00000fd (*THIS* is the exception code for STATUS_STACK_OVERFLOW (see winnt.h)
P8: 000000000007bdf3
P9: 
P10: 
```

### Stackoverflow Example in C#
Soex.Csharp is a very simple program that recursively calls itself while allocating a sizeable chunk of the stack (to speed up the crash).

### Stackoverflow Example in C
Same as the C# implementation.

## How to catch Stackoverflow exceptions
When catching these hard exceptions, despite all your effort such as using FailFast techniques and unhandled exception handlers and observers in .Net, you will not get a pretty exception print and graceful exit. So one has to look beyond the normal try/catch constructs.

The objective is to obtain a foot-print of the memory and in particular the stack -- so we need a dump-file / snapshot of the process at the time of the exception.

A number of strategies spring to mind:
- Windows API SetUnhandledExceptionFilter / AddVectoredExceptionHandler functions
- Attach a debugger (this is what DebugDiag2 does)
- Windows Error Reporting (WER), outside the scope of this article

### CrashHandler: AddVectoredExceptionHandler example 
Windows has support for ```SetUnhandledExceptionFilter()``` and  ```AddVectoredExceptionHandler()``` that allow you to specify a "filter" method for handling exceptions. CrashHandler uses the latter by configurering a filter that creates a .dmp-file and terminates the process. I have included examples of how to call the CrashHandler from C programs, C# programs, and even hosting the CLR itself.

The .dmp-file can be opened in Visual Studio and after ensuring symbols (.pdb-files) are loaded correctly, one can resume/continue debugging and it will take you to your line of source and it becomes obvious what is the matter by looking at the callstack window.

The trick to get beautiful dumps is to create a thread before hand, ```DumpThread``` which waits for an event to become signalled, e.g. the ```dumpEvent```. This event will only get signalled from the ExceptionHandler. This ensures that the thread responsible for calling the ```MiniDumpWriteDump``` will have a fully working stack.

### DebugDiag2 attached debugger
Great credit goes to Mike Smith for his article on this specific topic: http://www.mikesmithdev.com/blog/debug-stack-overflow-exception/ Essentially, you:
- start your process/service/site
- use DebugDiag2 Collection to configure a set of rules for creating .dmp-files and where to put them
- DD2C configures DbgSvc
- DbgSvc attaches to your process and waits for the exception to occur

This seems a whole lot better than installing remote debugging tools, or the Visual Studio on your servers.

On my Windows Server 2012 R2 with DebugDiag2 installed, all I have to do is double-click on the .dmp-file to "Analyze Crash/Hang Issue" and Explorer will invoke DebugDiag2's Analyzer which will process the .dmp-file and produce a nice .mht-file that opens in a browser and _voil�_ it points you to the stack-trace with the offending method. Cannot be easier.

But at a cost. Having a debugger attached is not free in terms of CPU-cycles, although I have yet to see a gross performance hit in my application. More on this later.

### Using WinDbg to find the recursive (chain of) functions
Start WinDbg

Open the dump-file (Ctrl+D). You should see something like this in the command window
```...
User Mini Dump File with Full Memory: Only application data is available
...
This dump file has an exception of interest stored in it.
The stored exception information can be accessed via .ecxr.
(f00.92c): Stack overflow - code c00000fd (first/second chance not available)
```

In the command window type the following two lines (to load Son-of-Strike and CLR debugger extensions)
```
.loadby sos clr
!CLRStack
```
And voil�:
```
00000064d1563c0 00007ffcc5da04e9 TestSo.Cer.Program.TriggerSo()
...
000000064d54e840 00007ffcc5da04e9 TestSo.Cer.Program.TriggerSo()
000000064d54ec80 00007ffcc5da04e9 TestSo.Cer.Program.TriggerSo()
000000064d54f0c0 00007ffcc5da0493 TestSo.Cer.Program.Main(System.String[])
000000064d54f320 00007ffd25414073 [GCFrame: 000000064d54f320]
```
