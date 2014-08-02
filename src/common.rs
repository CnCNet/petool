#![macro_export]

use std::io::{File, FileAccess, Open};


macro_rules! try_complain(
    ($e:expr, $s:expr) => (match $e { Ok(e) => e, Err(_) => return Err($s) })
)

macro_rules! fail_if(
    ($e:expr, $s:expr) => (if $e { return Err($s) })
)

fn read_arg<T>(args : &[Box<str>], n : int) -> T {
    match ::std::from_str::from_str::<T>(args[n]) {
        Some(n) => n,
        None    => return Err(format!("could not parse argument number {}", n))
    }
}    

// Caller cleans up fh and image, length is optional
pub fn open_and_read(executable : &Path, access : FileAccess)
                     -> Result<(File, Vec<u8>), Box<str>> {

    let mut file = try_complain!(File::open_mode(executable, Open, access),
                                 box "Could not open executable");
    let contents = try_complain!(file.read_to_end(),
                                 box "Error reading executable");

    Ok((file, contents))
}
