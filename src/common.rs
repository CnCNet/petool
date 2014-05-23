#![macro_export]

use std::io;
use std::intrinsics;

use pe::*;

macro_rules! try_complain(
    ($e:expr, $s:expr) =>
        (match $e {
            Ok(val)      => val,
            Err(mut err) => {
                err.detail = Some($s);
                return Err(err);
            }
        })
)

#[inline]
pub fn new_error(desc : &'static str, detail : Option<~str>) -> io::IoError {
    io::IoError {
        kind   : io::OtherIoError,
        desc   : desc,
        detail : detail,
    }
}

macro_rules! fail_if(
    ($e:expr, $desc:expr) =>
        (fail_if!($e, $desc, None));
    ($e:expr, $desc:expr, $detail:expr) =>
        (if $e { return Err(::common::new_error($desc, $detail)); })
)

macro_rules! read_arg(
    [$n:expr] => (match ::std::from_str::from_str::<uint>(args[$n]) {
        Some(n) => n,
        None    => {
            return Err(::common::new_error(
                "could not parse argument",
                Some(format!("argument number {}", $n))));
        }
    })
)

macro_rules! field_offset(
    ($t:ty, $f:ident) => (unsafe {
        ::std::intrinsics::transmute::<_,uint>(&((*(0 as *$t)).$f))
    })
)

#[inline]
pub fn open_pe(executable : &Path, access : io::FileAccess)
               -> io::IoResult<io::File> {
    io::File::open_mode(
        executable,
        io::Open, access)
}

fn seek_nt_header<T : Reader + Seek>(pe : &mut T) -> io::IoResult<i64> {
    try!(pe.seek(
        field_offset!(IMAGE_DOS_HEADER, e_lfanew) as i64,
        io::SeekSet));
    let nt_offset = try!(pe.read_le_u16()) as i64;
    try!(pe.seek(nt_offset, io::SeekSet));
    Ok(nt_offset)
}

// assumes at end of the NT Header
fn seek_section_headers<T : Seek> (
    pe        : &mut T,
    nt_header : &IMAGE_NT_HEADERS
        ) -> io::IoResult<i64>
{

    // in case OptionalHeader's actual size is different
    try!(pe.seek(
        nt_header.FileHeader.SizeOfOptionalHeader as i64
            - ::std::mem::size_of::<IMAGE_OPTIONAL_HEADER>() as i64,
        io::SeekCur));

    Ok(try!(pe.tell()) as i64)
}

/// checks DOS and PE signatures
pub fn validate_pe<T : Reader + Seek> (pe : &mut T) -> io::IoResult<()>
{
    // seek DOS Signature
    try!(pe.seek(
        field_offset!(IMAGE_DOS_HEADER, e_magic) as i64,
        io::SeekSet));
    fail_if!(try!(pe.read_le_u16()) != IMAGE_DOS_SIGNATURE,
             "invalid DOS signature.");

    try!(seek_nt_header(pe));

    // seek NT Signature
    try!(pe.seek(
        field_offset!(IMAGE_NT_HEADERS, Signature) as i64,
        io::SeekCur));
    fail_if!(try!(pe.read_le_u32()) != IMAGE_NT_SIGNATURE,
             "invalid NT signature.");
    Ok(())
}

pub fn read_headers<T : Reader + Seek> (
    pe : &mut T
        ) -> io::IoResult<(Box<IMAGE_NT_HEADERS>, ~[IMAGE_SECTION_HEADER])>
{
    try!(seek_nt_header(pe));

    let nt_header : Box<IMAGE_NT_HEADERS> = unsafe {
        let temp = try!(pe.read_exact(::std::mem::size_of::<IMAGE_NT_HEADERS>()));
        intrinsics::transmute(temp.as_ptr())
    };

    try!(seek_section_headers(pe, nt_header));

    let section_headers : ~[IMAGE_SECTION_HEADER] = unsafe {
        let temp = try!(pe.read_exact(
            nt_header.FileHeader.NumberOfSections as uint
                * ::std::mem::size_of::<IMAGE_SECTION_HEADER>()));
        intrinsics::transmute::<~[u8],_>(::std::vec::FromVec::from_vec(temp))
    };

    assert_eq!(section_headers.len(), nt_header.FileHeader.NumberOfSections as uint);

    Ok((nt_header, section_headers))
}

fn as_bytes<'a, T>(x: &'a T) -> &'a [u8] {
    let singleton = ::std::slice::ref_slice(x);
    let bytes : &'a [u8] = unsafe {
        use std::raw::Repr;
        let mut bytes = singleton.repr();
        bytes.len = ::std::mem::size_of::<T>();
        intrinsics::transmute(bytes)
    };
    assert_eq!(::std::mem::size_of::<T>(), bytes.len());
    bytes
}

fn array_as_bytes<'a, T>(x: &'a [T]) -> &'a [u8] {
    let len = x.len();
    let bytes : &'a [u8] = unsafe {
        use std::raw::Repr;
        let mut bytes = x.repr();
        bytes.len *= ::std::mem::size_of::<T>();
        intrinsics::transmute(bytes)
    };
    assert_eq!(len * ::std::mem::size_of::<T>(), bytes.len());
    bytes
}

pub fn write_nt_header<T : Reader + Writer + Seek> (
    pe : &mut T,
    nt_header : Box<IMAGE_NT_HEADERS>
        ) -> io::IoResult<()>
{
    try!(seek_nt_header(pe));

    try_complain!(pe.write(as_bytes(nt_header).slice_to(
        nt_header.FileHeader.SizeOfOptionalHeader as uint)),
                  "Could not write back NT header to executable".to_owned());

    Ok(())
}

pub fn write_headers<T : Reader + Writer + Seek> (
    pe              : &mut T,
    nt_header       : Box<IMAGE_NT_HEADERS>,
    section_headers : &[IMAGE_SECTION_HEADER]
        ) -> io::IoResult<()>
{
    try!(write_nt_header(pe, nt_header));

    // relies on above to seek to correct spot
    try_complain!(pe.write(array_as_bytes(section_headers)),
                  "Could not write back section headers to executable".to_owned());

    Ok(())
}
