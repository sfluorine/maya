sources = Split('src/maya.c src/mayasm.c')

Program(target = './maya', source = sources, CCFLAGS = '-I src/include -I lib/sv/include', LIBS=['sv'], LIBPATH = './lib/sv/')
