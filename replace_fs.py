Import("env")
print("Replace MKSPIFFSTOOL with mklittlefs")
env.Replace (MKSPIFFSTOOL = "./update_fs.sh")