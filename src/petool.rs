#![feature(macro_rules, globs)]

extern crate collections;

#[macro_escape]
mod common;
mod dump;
#[macro_escape]
mod pe;

pub fn main() {
    unsafe {
        match dump::dump(~[~"asdfasdfasdfasdf"]) {
            Ok(_)  => (),
            Err(s) => std::io::println(s)
        }
    }
}
