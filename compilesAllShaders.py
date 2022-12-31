import os
path_of_the_directory= 'data/shaders'
print("Files and directories in a specified path:")
for filename in os.listdir(path_of_the_directory):
    f = os.path.join(path_of_the_directory,filename)
    if not os.path.isfile(f):
        print(f)
        os.system("python data/shaders/compileshaders.py " + f)