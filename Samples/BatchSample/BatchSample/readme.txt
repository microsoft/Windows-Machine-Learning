========================================================================
    C++/WinRT BatchSample Project Overview
========================================================================

This project demonstrates how to get started consuming Windows Runtime 
classes directly from standard C++, using either the SDK's platform 
projection, or generating projection headers from Windows metadata files.

Steps to consume SDK platform projection:
1. Include an SDK platform projection namespace header, such as 
    "winrt/Windows.Foundation.h", in your pch.h.
2. Consume winrt namespace and any Windows Runtime namespaces, such as 
    winrt::Windows::Foundation, from source code.
3. Initialize apartment via init_apartment() and consume winrt classes.

Steps to generate and consume a projection from third party metadata:
1. Add a WinMD reference by right-clicking the References project node
    and selecting "Add Reference...".  In the Add References dialog, 
    browse to the component WinMD you want to consume and add it.
2. Build the project once to generate projection headers for the 
    referenced WinMD file under the "Generated Files" subfolder.
3. As above, include projection headers in pch or source code 
    to consume projected Windows Runtime classes.

========================================================================
Learn more about C++/WinRT here:
http://aka.ms/cppwinrt/
========================================================================
