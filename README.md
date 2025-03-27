# SUITS-lmcc-pc
Unreal Engine project for local mission control center built to PC

## Start Here
### Software Requirements
1. Visual Studio 2022
1. Visual Studio Build Tools *(recommended for TSS build)*
1. Unreal Engine 5.5
1. [TSS 2025 server](https://github.com/SUITS-Techteam/TSS-2025)


### Steps to build and run TSS-2025
1. download the repository from github
1. open "Developer Command Prompt for VS 2022"
1. Navigate to the TSS-2025 folder with "cd" and "dir"
1. run "buildvs.bat" by typing that into the window.
1. close the Developer Command Prompt.
1. If you would like to run the TSS server and the Unreal project on the same computer, create a shortcut to server.exe and add "--local" to the end of the target (fig. 1).
    1. otherwise, to run on separate computers, run server.exe (using Command Prompt, which is the default)

### Steps to open the unreal project on your windows computer
1. download the repository from github
1. right-click on the .uproject file, select more, then select "Generate Visual Studio project files
1. once that is complete, open the .sln file in Visual Studio.
1. Select Build > Build Solution
1. Open the .uproject file in Unreal Engine

### Steps to use the Unreal project
1. type in the IP address from the TSS server WITHOUT the port number.