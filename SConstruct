#!python
import os, subprocess

opts = Variables([], ARGUMENTS)

# Gets the standard flags CC, CCX, etc.
env = DefaultEnvironment()

# Define our options
opts.Add(EnumVariable('target', "Compilation target", 'release', ['d', 'debug', 'r', 'release']))
opts.Add(EnumVariable('platform', "Compilation platform", '', ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(EnumVariable('p', "Compilation target, alias for 'platform'", '', ['', 'windows', 'x11', 'linux', 'osx']))
opts.Add(BoolVariable('use_llvm', "Use the LLVM / Clang compiler", 'no'))
opts.Add(BoolVariable('use_mingw', "Use the MingW for cross-compiling", 'no'))
opts.Add(PathVariable('target_path', 'The path where the lib is installed.', 'GdpdExample/addons/gdpd/bin/'))
opts.Add(PathVariable('target_name', 'The library name.', 'libgdpd', PathVariable.PathAccept))

# Local dependency paths, adapt them to your setup
godot_headers_path = "src/godot-cpp/godot_headers/"
cpp_bindings_path = "src/godot-cpp/"
cpp_library = "libgodot-cpp"

# only support 64 at this time..
bits = 64

# Updates the environment with the option variables.
opts.Update(env)

# Process some arguments
if env['use_llvm']:
    env['CC'] = 'clang'
    env['CXX'] = 'clang++'


if env['p'] != '':
    env['platform'] = env['p']

if env['platform'] == '':
    print("No valid target platform selected.")
    quit();

# For the reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

# Check our platform specifics
if env['platform'] == "osx":
    env['target_path'] += 'osx/'
    cpp_library += '.osx'
    env.Append(CPPDEFINES=['__MACOSX_CORE__', 'HAVE_UNISTD_H', 'LIBPD_EXTRA'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(LINKFLAGS=['-arch', 'x86_64','-framework', 
                          'CoreAudio', '-framework', 'CoreFoundation'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-g', '-O2', '-arch', 'x86_64'])
    else:
        env.Append(CCFLAGS=['-g', '-O3', '-arch', 'x86_64'])

elif env['platform'] in ('x11', 'linux'):
    env['CC'] = 'gcc-7'
    env['CXX'] = 'g++-7'
    env['target_path'] += 'x11/'
    cpp_library += '.linux'
    env.Append(CPPDEFINES=['__UNIX_JACK__', 'LIBPD_EXTRA'])
    env.Append(LINKFLAGS=['-ljack','-pthread'])
    if env['target'] in ('debug', 'd'):
        env.Append(CCFLAGS=['-fPIC', '-g3', '-Og'])
        env.Append(CFLAGS=['-std=c11'])
        env.Append(CXXFLAGS=['-std=c++17'])
    else:
        env.Append(CCFLAGS=['-fPIC', '-O3'])
        env.Append(CFLAGS=['-std=c11'])
        env.Append(CXXFLAGS=['-std=c++17'])

elif env['platform'] == "windows":
    env['target_path'] += 'win64/'
    cpp_library += '.windows'
    env.Append(ENV=os.environ)

    if not env['use_mingw']:
        # MSVC
        env.Append(LINKFLAGS=['/WX'])
        if env['target'] == 'debug':
            env.Append(CCFLAGS=['/EHsc', '/D_DEBUG', '/MTd'])
        elif env['target'] == 'release':
            env.Append(CCFLAGS=['/O2', '/EHsc', '/DNDEBUG', '/MD'])
    else:
        # MinGW
        env['CXX'] = 'x86_64-w64-mingw32-g++-win32'
        env['CC'] = 'x86_64-w64-mingw32-gcc-win32'
        #env.Append(CXXFLAGS=['-g', '-O3', '-std=c++14', '-Wwrite-strings', '-fpermissive'])
        env.Append(CXXFLAGS=['-O3', '-std=c++14', '-Wwrite-strings', '-fpermissive'])
        #env.Append(LINKFLAGS=['--static', '-Wl,--no-undefined', '-static-libgcc', '-static-libstdc++'])
        #env.Append(CPPDEFINES=['WIN32', '_WIN32', '_MSC_VER', '_WINDOWS', '_CRT_SECURE_NO_WARNINGS'])
        env.Append(CFLAGS=['-DWINVER=0x502', '-DWIN32', '-D_WIN32', 
                        '-Wno-int-to-pointer-cast', '-Wno-pointer-to-int-cast'])
        #env.Append(CPPDEFINES=['HAVE_UNISTD_H=1','LIBPD_EXTRA=1','PD=1', 
                                #'PD_INTERNAL','USEAPI_DUMMY=1','libpd_EXPORTS'])
        env.Append(CPPDEFINES=['PD_INTERNAL', 'libpd_EXPORTS']) 
        env.Append(CPPDEFINES=['__WINDOWS_DS__'])
        #env.Append(CPPDEFINES=['__WINDOWS_WASAPI__'])
        #env.Append(CPPDEFINES=['__RTAUDIO_DUMMY__', 'LIBPD_EXTRA'])
        #env.Append(CFLAGS=['-DUSEAPI_DUMMY', '-DPD', '-DHAVE_UNISTD_H', '-D_GNU_SOURCE'])
        env.Append(LDPATH=['/usr/x86_64-w64-mingw32/lib/'])
        env.Append(LINKFLAGS=['-Wl,--export-all-symbols',
                '-static-libgcc','/usr/x86_64-w64-mingw32/lib/libm.a'])

    #env.Append(LIBS=['-lkernel32','-luser32', '-lgdi32', 
    #                  '-lwinspool', '-lshell32', '-lole32', 
    #                  '-loleaut32', '-luuid', '-lcomdlg32', 
    #                  '-ladvapi32','-lws2_32', '-lwsock32',
    #                  '-ldsound', '-lwinmm'])
    env.Append(LIBS=['-lws2_32', '-lwsock32','-loleaut32', '-luuid',
                     '-lole32', '-ldsound', '-lwinmm'])
    #env.Append(LIBS=['-lws2_32', '-lwsock32','-loleaut32', '-lmfplat','-lmfuuid', 
    #                 '-lole32', '-lwmcodecdspuuid' ,'-luuid','-lksuser'])
    env['SHLIBSUFFIX']  = '.dll'

    #env.Append(CPPDEFINES=['WINVER=0x502'])
    #env.Append(CCFLAGS=['-W3', '-GR'])
    env.Append(LINKFLAGS=['-pthread'])
    #if env['use_mingw']:
        #env['CC'] = 'x86_64-w64-mingw32-gcc'
        #env['CXX'] = 'x86_64-w64-mingw32-g++'
        #env['AR'] = "x86_64-w64-mingw32-ar"
        #env['RANLIB'] = "x86_64-w64-mingw32-ranlib"
        #env['LINK'] = "x86_64-w64-mingw32-g++"
#        env.Append(CFLAGS=['-std=c11'])
    #    env.Append(CXXFLAGS=['-fpermissive'])
    #    env.Append(LIBS=['ws2_32', 'kernel32'])
    #    env.Append(LINKFLAGS=['-shared', '-Wl,--export-all-symbols','-mwindows','-Wl,-enable-stdcall-fixup'])

if env['target'] in ('debug', 'd'):
    cpp_library += '.debug'
else:
    cpp_library += '.release'

cpp_library += '.' + str(bits)

# make sure our binding library is properly included
env.Append(CPPPATH=['.', godot_headers_path, cpp_bindings_path + 'include/', cpp_bindings_path + 'include/core/', cpp_bindings_path + 'include/gen/', 'src/libpd/cpp','src/libpd/pure-data/src', 'src/libpd/libpd_wrapper', 'src/libpd/libpd_wrapper/util', 'src/rtaudio'])
env.Append(LIBPATH=[cpp_bindings_path + 'bin/'])
env.Append(LIBS=[cpp_library])
env.Append(CFLAGS=['-DUSEAPI_DUMMY', '-DPD', '-DHAVE_UNISTD_H', '-D_GNU_SOURCE'])

# tweak this if you want to use different folders, or more folders, to store your source code in.
env.Append(CPPPATH=['src/'])

sources = Glob('src/*.cpp') + Glob('src/rtaudio/*.cpp') + Glob('src/libpd/libpd_wrapper/*.c') + Glob('src/libpd/libpd_wrapper/util/*.c') + Glob('src/libpd/pure-data/extra/**/*.c') + Glob('src/libpd/pure-data/src/[xmgz]_*.c') + Glob('src/libpd/pure-data/src/d_[acgmorsu]*.c') + Glob('src/libpd/pure-data/src/d_dac.c') + Glob('src/libpd/pure-data/src/d_delay.c') + Glob('src/libpd/pure-data/src/d_fft.c') + Glob('src/libpd/pure-data/src/d_fft_fftsg.c') + Glob('src/libpd/pure-data/src/d_filter.c') + Glob('src/libpd/pure-data/src/s_audio.c') + Glob('src/libpd/pure-data/src/s_audio_dummy.c') + Glob('src/libpd/pure-data/src/s_print.c') + Glob('src/libpd/pure-data/src/s_path.c')  + Glob('src/libpd/pure-data/src/s_main.c') + Glob('src/libpd/pure-data/src/s_inter.c') + Glob('src/libpd/pure-data/src/s_utf8.c') + Glob('src/libpd/pure-data/src/s_loader.c') + Glob('src/libpd/pure-data/extra/*.c') 

library = env.SharedLibrary(target=env['target_path'] + env['target_name'] , source=sources)

Default(library)

# Generates help for the -h scons option.
Help(opts.GenerateHelpText(env))
