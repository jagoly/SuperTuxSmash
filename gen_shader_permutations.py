#!/bin/env python3

# todo: 
#  - go through every Render.json file to get lists of permutations that actually get used
#  - change restictions from scary eval to some custom grammer

import json, itertools
from pathlib import Path

def remove_old_directories(rootPath):
    for dirPath in rootPath.iterdir():
        if dirPath.is_dir():
            if dirPath.suffix in (".vert", ".geom", ".frag"):
                print("%s/" % dirPath)
                canRemoveDir = True
                for filePath in dirPath.iterdir():
                    if filePath.is_file() and filePath.suffix in (".vert", ".geom", ".frag"):
                        filePath.unlink()
                    else:
                        print("  warning: \"%s\" not removed" % filePath)
                        canRemoveDir = False
                if canRemoveDir:
                    print("  old files removed")
                    dirPath.rmdir()
            else:
                remove_old_directories(dirPath)

remove_old_directories(Path("./shaders"))

for jsonPath in Path("./shaders").rglob("*.json"):

    suffix = jsonPath.suffixes[0]

    if suffix not in (".vert", ".geom", ".frag"):
        print("skipping \"%s\"" % jsonPath)
        continue

    outputDir = jsonPath.with_suffix("")
    includeName = jsonPath.with_suffix(".glsl").name

    j = json.loads(jsonPath.open('r').read())
    options = j['options']
    requirements = j['requirements']

    totalPermutations = j['options']

    def generate_permutations():
        for values in itertools.product(*options.values()):
            yield dict(zip(options.keys(), values))

    def check_requirements(permutation):
        p = permutation
        return all(map(eval, requirements))

    permutations = list(generate_permutations())
    totalPermutations = len(permutations)

    permutations = list(filter(check_requirements, permutations))

    print("%s/" % outputDir)
    print("  generating %d of %d possible permutations" % (len(permutations), totalPermutations))

    outputDir.mkdir(exist_ok=True)

    for permutation in permutations:

        outputName = "_".join(key + value for key, value in permutation.items())
        outputPath = (outputDir / outputName).with_suffix(suffix)

        with outputPath.open('w') as o:

            o.write("#version 460\n\n")

            for key, value in permutation.items():
                o.write("#define OPTION_%s %s\n" % (key, value))

            o.write("\n#include \"../%s\"\n" % includeName)
