#!/bin/python3

import sys
import os
import subprocess

def eprint(*args, **kwargs):
    print(*args, file=sys.stderr, **kwargs)

release_flags = ['-DNDEBUG']
debug_flags = ['-g', '-DDEBUG']

dirs = {
    'sources': 'src',
    'headers': 'inc',
    'build': 'build',
    'objects': 'objects',
    'bmi': 'gcm.cache'
}

cxx = ['g++', '-fdiagnostics-color=always']
cxx_flags = ['-std=c++23', '-fmodules-ts', '-I' + dirs['headers']]
ld_flags = []

sys_modules = ['iostream', 'limits', 'cmath', 'cstdint', 'print', 'format', 'random', 'array', 'exception',
               'vector', 'string', 'string_view', 'unordered_map', 'sstream', 'cstring', 'list', 'chrono',
               'any', 'cctype', 'deque', 'fstream', 'tuple', 'source_location']

# Properties: is_module, primary dependencies
sources = {
    'arbit': [[True, []], ['arbit.cpp', 'basic.cpp', 'bits.cpp', 'construction.cpp', 'heap.cpp', 'operations.cpp']],
    'wc': [[False, ['arbit']], ['operations.cpp', 'wc.cpp']],
    'main': [[False, ['arbit', 'wc']], ['main.cpp']],
    'ats': [[False, ['arbit']], ['main.cpp']]
}

# Properties: library dependencies
targets = {
    'wc': [['readline'], ['main', 'arbit', 'wc']],
    'ats': [[], ['ats', 'arbit']]
}

lone_sources = {
    'tests': ['floats.cpp', 'fp.cpp', 'modules.cpp', 'mul.cpp'],
}
lone_targets = {
    'tests'
}

class Builder:
    args_nature = {
        'help': False,
        'release': False,
        'run': True
    }

    def __init__(self):
        self.handle_arguments();

        self.make_directories()
        self.make_sys_modules()
        self.make_lone_targets()
        self.make_targets()

        if 'run' in self.args:
            exe = self.args['run']
            exe_path = self.dirs['build'] + '/' + exe

            if exe not in targets:
                raise Exception('No such executable is registered')
            if not self.is_exists(exe_path):
                raise Exception(f'File {exe_path} doesn\'t exist. This shouldn\'t have happened')

            subprocess.run([exe_path])

    def make_sys_modules(self):
        for module in sys_modules:
            path = dirs['bmi'] + '/usr/include/c++/14.1.1/' + module + '.gcm'
            if not self.is_exists(path):
                self.run(cxx + cxx_flags + ['-xc++-system-header', module])

    def make_lone_targets(self):
        for single_ref in lone_targets:
            for single in lone_sources[single_ref]:
                source = dirs['sources'] + '/' + single_ref + '/' + single
                target = (self.dirs['build'] + '/' + single).removesuffix('.cpp')
                if not self.is_exists(target) or self.is_later(source, target):
                    self.run(cxx + self.this_flags + cxx_flags + ld_flags + [source, '-o', target])

    def make_targets(self):
        self.primaries_checked = set()
        self.primaries_updated = set()

        for exe in targets:
            exe_path = self.dirs['build'] + '/' + exe

            lib_cflags, lib_libs = [], []
            for library in targets[exe][0]:
                cflags, libs = self.resolve_library_flags(library)
                lib_cflags += cflags
                lib_libs += libs

            for primary in targets[exe][1]:
                stuff = self.is_primary_needs_update(primary)
                if len(stuff) != 0:
                    self.make_primary(primary, stuff, lib_cflags)
            
            if not self.is_exists(exe_path) or len(self.primaries_updated) != 0:
                lib_cflags
                self.link(exe, lib_libs)

    def make_primary(self, primary, stuff, extra_flags):
        if primary in self.primaries_updated:
            return

        deps = sources[primary][0][1]

        for dep in deps:
            new_stuff = self.is_primary_needs_update(dep)
            if len(new_stuff) != 0:
                self.make_primary(dep, new_stuff, extra_flags)

        if len(stuff) != 0:
            for task in stuff:
                file_path = task[0]
                object_path = task[1]
                self.compile(file_path, object_path, extra_flags)

            self.primaries_updated.add(primary)

    def is_primary_needs_update(self, primary):
        if primary in self.primaries_checked:
            return []

        is_module = sources[primary][0][0]
        files = sources[primary][1]

        stuff = []

        for file in files:
            file_basename = file.removesuffix('.cpp')
            file_path = dirs['sources'] + '/' + primary + '/' + file
            object_path = self.dirs['objects'] + '/' + primary + '/' + file_basename + '.o'

            current_stuff = []

            if not self.is_exists(object_path) or self.is_later(file_path, object_path):
                current_stuff += [[file_path, object_path]]

            if is_module and file_basename == primary:
                bmi_path = dirs['bmi'] + '/' + primary + '.gcm'
                if not self.is_exists(bmi_path) or self.is_later(file_path, bmi_path):
                    if len(current_stuff) == 0:
                        current_stuff += [[file_path, object_path]]

            stuff += current_stuff

        self.primaries_checked.add(primary)

        return stuff

    def compile(self, source, target, extra_flags):
        self.run(cxx + self.this_flags + cxx_flags + extra_flags + ['-c', source, '-o', target])

    def link(self, exe, extra_flags):
        exe_path = self.dirs['build'] + '/' + exe

        objects = []
        for primary in targets[exe][1]:
            for file in sources[primary][1]:
                objects += [self.dirs['objects'] + '/' + primary + '/' + file.removesuffix('.cpp') + '.o']
            
        self.run(cxx + self.this_flags + ld_flags + extra_flags + objects + ['-o', exe_path])

    def make_directories(self):
        self.dirs = {'build': dirs['build'] + '/' + self.type}
        self.dirs['objects'] = self.dirs['build'] + '/' + dirs['objects']

        os.makedirs(self.dirs['build'], exist_ok = True)
        os.makedirs(self.dirs['objects'], exist_ok = True)
        for primary in sources:
            os.makedirs(self.dirs['objects'] + '/' + primary, exist_ok = True)
        os.makedirs(dirs['bmi'], exist_ok = True)

    def resolve_library_flags(self, library):
        lib_cflags = []
        lib_libs = []

        status = subprocess.run(('pkg-config', library, '--cflags'), capture_output=True)
        if status.returncode != 0:
            raise Exception(f'Failed to resolve library {library}\'s cflags')
        lib_cflags += [status.stdout.decode().strip()]

        status = subprocess.run(('pkg-config', library, '--libs'), capture_output=True)
        if status.returncode != 0:
            raise Exception(f'Failed to resolve library {library}\'s libs')
        lib_libs += [status.stdout.decode().strip()]

        return [lib_cflags, lib_libs]

    def is_later(self, path, path2):
        ts = os.path.getmtime(path)
        ts2 = os.path.getmtime(path2)
        return ts > ts2

    def is_exists(self, path):
        return os.path.exists(path)

    def run(self, args):
        eprint('*', ' '.join(args))
        status = subprocess.run(args)
        if status.returncode != 0:
            raise Exception(f'Command failed with code {status.returncode}')

    def handle_arguments(self):
        i = 1
        self.args = {}
        while i < len(sys.argv):
            this = sys.argv[i]
            if this in self.args_nature:
                if self.args_nature[this] is True:
                    if i == len(sys.argv)-1:
                        raise Exception(f'Argument \'{this}\' needs a parameter')
                    self.args[this] = sys.argv[i+1]
                    i += 1
                else:
                    self.args[this] = True
            i += 1

        if 'help' in self.args:
            print(self.args_nature)
            exit(1)

        self.type = 'release' if 'release' in self.args else 'debug'
        self.this_flags = release_flags if 'release' in self.args else debug_flags

if __name__ == '__main__':
    try:
        instance = Builder()
    except Exception as e:
        eprint('Exception caught:', e)
        exit(2)
