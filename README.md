﻿# segment_3Dfragment

## About the project
This project segments 3D meshes of fragments into smaller meshes. One of its capabilities is to extract the intact surface of a fragment


## How to compile the code
This code was developed on windows machine and compiled with cmake.
After The repo was cloned, browse to its folder and create a new direcotory name build.
As we did, you can download [cmake-gui]([url](https://cmake.org/cmake/help/latest/manual/cmake-gui.1.html)) and run it.
The first prompt will ask you about the compiler, this is what we did: 
![compiler](https://user-images.githubusercontent.com/38216201/200351335-8f8364ec-4944-458c-baf4-efbb910ffd71.png)

Next, fill the first and the third field with your corresponding path
![cmake gui path](https://user-images.githubusercontent.com/38216201/200353315-80b62bcb-af27-4018-9395-3652c8ea9337.png)

Then, click Configure and when it finishes Generate. You can open the sln file in the build directory.
In the solution explorer, set the cpp language standard to cpp 17. Make a right-click on the project name and click on Properties
![cpp level](https://user-images.githubusercontent.com/38216201/200354536-e211525e-6a4e-4ff8-bb0d-1a5bc5917faf.png)

Then stay on Properties and click on C++-> General -> Additional Include Directories
![src](https://user-images.githubusercontent.com/38216201/200355346-ef406855-cde6-4123-a2df-b1f8a4aaa047.png)
And add the full path of the src folder in the root directory of the project

Finally right-click on the project, add exisiting items and then add all the files in the src directory.
Now you can build and run! It should save the new obj file of the segment and render its vertices

## How to run the code 
The code assumes windows path styling (\\ rather than /)
```
cd build\Debug
segment_3Dfragment.exe <PATH_OF_OBJ_FILE> <NAME_OF_OBJ_OUTPUT_FILE>
```
