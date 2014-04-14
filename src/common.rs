use std::io::{File, FileAccess, Open};

#[macro_export]
macro_rules! try_complain(
    ($e:expr, $s:expr) => (match $e { Ok(e) => e, Err(_) => return Err($s) })
)

#[macro_export]
macro_rules! fail_if(
    ($e:expr, $s:expr) => (if $e { return Err($s) })
)


// Caller cleans up fh and image, length is optional
pub fn open_and_read(executable : &Path, access : FileAccess)
                     -> Result<(File, Vec<u8>), ~str> {

    let mut file = try_complain!(File::open_mode(executable, Open, access),
                                 ~"Could not open executable");
    let contents = try_complain!(file.read_to_end(),
                                 ~"Error reading executable");

    Ok((file, contents))
}
