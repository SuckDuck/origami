#! /usr/bin/python
from .littlebuild.littlebuild import *

project_title = "example"

def statics():
    ASSETS_DIR = "assets"
    STATICS_DIR = "statics"
    create_dirs([STATICS_DIR])

    input_files = [str(Path(ASSETS_DIR) / f) for f in os.listdir(ASSETS_DIR)]
    create_statics(STATICS_DIR,input_files)

def example():
    CC = "gcc"
    CFLAGS =  ["-g", "-O0"]
    BUILD_DIR = "build"
    SRC_DIRS = ["src","example"]
    all_srcs = [f"{d}/{f}" for d in SRC_DIRS for f in os.listdir(d)]
    INCLUDES = ["include","statics"]
    LIB_PATHS = []
    LINKS = ["-lX11","-lm","-lraylib"]
    LDFLAGS = []
    MACROS = {}
    
    create_dirs([BUILD_DIR])
    objs = compile(CC,CFLAGS,MACROS,INCLUDES,all_srcs,out_dir=BUILD_DIR)
    link(CC,objs,LIB_PATHS,LINKS,LDFLAGS,BUILD_DIR,project_title)

def clean():
    rm_all("build")
    rm_all("statics")


if __name__ == "__main__":
    if "clean"   in sys.argv: clean();   exit(0)
    if "clear"   in sys.argv: clean();   exit(0)
    if "example" in sys.argv: example(); exit(0)
    statics()
