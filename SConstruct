sources = Split('./src/maya.c ./src/mayasm.c ./src/mayalink.c ./src/sv.c')

SharedLibrary(source = './stdlib/maya_stdlib.c', CCFLAGS = '-Wall -Wextra -I src/include')
Program(target = './maya', source = sources, CCFLAGS = '-Wall -Wextra -I src/include')
