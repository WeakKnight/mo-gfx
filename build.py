from subprocess import call
call(["python", "thirdparty/shaderc/utils/git-sync-deps"])

import platform

systemName = platform.system()
print("Building On " + systemName + " System")

import subprocess
import os

dir_path = os.path.dirname(os.path.realpath(__file__))
print("Current Path is " + dir_path)

if systemName == "Windows":
    os.chdir(dir_path + "/build/windows")
    os.system('cmake -G "Visual Studio 16 2019" ../../')
if systemName == "Darwin":
    os.chdir(dir_path + "/build/macos")
    os.system('cmake -G "Xcode" ../../')