#![feature(macro_rules, globs)]

extern crate libc;
extern crate collections;

use libc::consts::os::c95::{EXIT_FAILURE, EXIT_SUCCESS};

#[macro_escape]
mod common;
#[macro_escape]
mod pe;

mod dump;
mod genlds;
mod pe2obj;
mod patch;
mod setdd;
mod setvs;

static help : &'static str = "commands:
    dump   -- dump information about section of executable
    genlds -- generate GNU ld script for re-linking executable
    pe2obj -- convert PE executable into win32 object file
    patch  -- apply a patch set from the .patch section
    setdd  -- set any DataDirectory in PE header
    setvs  -- set VirtualSize for a section
    help   -- this information";

fn render_help(progname : &str) -> Box<str> {
    format!("usage: {} <command> [args ...]\n{}", progname, help)
}

#[allow(unused_must_use)]
pub fn main() {
    let mut stderr = std::io::stdio::stderr();
    let args : Box<[Box<str>]> = std::os::args();

    let ret = match root(args) {
        Ok(_)  => EXIT_SUCCESS,
        Err(s) => {
            stderr.write_line(s);
            EXIT_FAILURE
        }
    };
    std::os::set_exit_status(ret as int);
}


fn root(args : &[Box<str>]) -> Result<(), Box<str>> {
    if args.len() < 2 {
        Err(format!(
            "No command given: please give valid command name as first argument\n{}",
             render_help(args[0].as_slice())))
    } else {
        match args[1].as_slice() {
            "dump"   => unsafe { dump   :: main (args.slice_from(2)) },
            "genlds" => unsafe { genlds :: main (args.slice_from(2)) },
            "pe2obj" => unsafe { pe2obj :: main (args.slice_from(2)) },
            "patch"  => unsafe { patch  :: main (args.slice_from(2)) },
            "setdd"  => unsafe { setdd  :: main (args.slice_from(2)) },
            "setvs"  => unsafe { setvs  :: main (args.slice_from(2)) },
            "help"   => {
                std::io::stdio::println(render_help(args[0].as_slice()));
                Ok(())
            }
            _ => Err(format!("Unknown command: {}\n{}",
                             args[1],
                             render_help(args[0].as_slice())))
        }
    }
}
