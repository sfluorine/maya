sources = Split('./src/maya.c ./src/mayasm.c ./src/sv.c')

SharedLibrary(target = './maya_stdlib.so', source = './stdlib/maya_stdlib.c', CCFLAGS = '-Wall -Wextra -I src/include')
Program(target = './maya', source = sources, CCFLAGS = '-Wall -Wextra -I src/include')
