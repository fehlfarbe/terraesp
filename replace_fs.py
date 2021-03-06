Import("env")
from sys import platform
print("Replace MKSPIFFSTOOL with mklittlefs")
# env.Replace (MKSPIFFSTOOL = "./update_fs.sh")
if platform == "linux" or platform == "linux2":
    env.Replace (MKSPIFFSTOOL = "./mklittlefs")
elif platform == "win32":
    env.Replace (MKSPIFFSTOOL = "./mklittlefs.exe")
