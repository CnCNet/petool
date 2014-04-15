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

static help : &'static str = "commands:
    dump   -- dump information about section of executable
    genlds -- generate GNU ld script for re-linking executable
    pe2obj -- convert PE executable into win32 object file
    patch  -- apply a patch set from the .patch section
    setdd  -- set any DataDirectory in PE header
    setvs  -- set VirtualSize for a section
    help   -- this information";

fn render_help(progname : &str) -> ~str {
    format!("usage: {} <command> [args ...]\n{}", progname, help)
}

#[allow(unused_must_use)]
pub fn main() {
    let mut stderr = std::io::stdio::stderr();
    let args : ~[~str] = std::os::args();

    let ret = match root(args) {
        Ok(_)  => EXIT_SUCCESS,
        Err(s) => {
            stderr.write_line(s);
            EXIT_FAILURE
        }
    };
    std::os::set_exit_status(ret as int);
}


fn root(args : &[~str]) -> Result<(), ~str> {
    if args.len() < 2 {
        Err(format!(
            "No command given: please give valid command name as first argument\n{}",
             render_help(args[0].as_slice())))
    } else {
        match args[1].as_slice() {
            "dump"   => unsafe { dump::dump     (args.slice_from(2)) },
            "genlds" => unsafe { genlds::genlds (args.slice_from(2)) },
            "pe2obj" => unsafe { pe2obj::pe2obj (args.slice_from(2)) },
            //"patch"  => unsafe { patch::patch   (args.slice_from(2)) },
            //"setdd"  => unsafe { setdd::setdd   (args.slice_from(2)) },
            //"setvs"  => unsafe { setvs::setvs   (args.slice_from(2)) },
            //"help"   => {
            //    help(args[0]);
            //    Ok(())
            //}
            _ => Err(format!("Unknown command: {}\n{}",
                             args[1],
                             render_help(args[0].as_slice())))
        }
    }
}
