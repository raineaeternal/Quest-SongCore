use std::{ffi::c_void, ops::Deref, path::{Path, PathBuf}, ptr::{slice_from_raw_parts, slice_from_raw_parts_mut}};

/// Type to allow opaque user data pointers to be passed across FFI boundaries.
#[repr(C)]
#[derive(Clone, Copy)]
pub struct OpaqueUserData(*const c_void);

unsafe impl Sync for OpaqueUserData {}
unsafe impl Send for OpaqueUserData {}

/// A CString managed across FFI boundaries.
/// The memory will be freed when this struct is dropped.
/// yet is still FFI-safe.
#[repr(C)]
#[derive(Default)]
pub struct ManagedCString(pub *mut std::os::raw::c_char);

impl ManagedCString {
    pub fn from_pathbuf(path: PathBuf) -> Self {
        let c_string = std::ffi::CString::new(
            path.to_str()
                .expect("Failed to convert PathBuf to str")
                .as_bytes(),
        )
        .expect("Failed to create CString from PathBuf");
        ManagedCString(c_string.into_raw())
    }

    pub fn from_path(path: &Path) -> Self {
        let c_string = std::ffi::CString::new(
            path.to_str()
                .expect("Failed to convert Path to str")
                .as_bytes(),
        )
        .expect("Failed to create CString from Path");
        ManagedCString(c_string.into_raw())
    }

    pub fn from_option<T>(opt: Option<T>) -> Self
    where
        T: Into<Vec<u8>>,
    {
        match opt {
            Some(s) => {
                let c_string = std::ffi::CString::new(s.into())
                    .expect("Failed to create CString from str");
                ManagedCString(c_string.into_raw())
            }
            None => ManagedCString(std::ptr::null_mut()),
        }
    }
}

/// A managed array across FFI boundaries.
/// The memory will be freed when this struct is dropped.
/// yet is still FFI-safe.
#[repr(C)]
pub struct ManagedArray<T> {
    pub data: *mut T,
    pub length: usize,
}

impl<T> ManagedArray<T> {
    /// Creates a new ManagedArray from a Vec<T>.
    pub fn from_vec(vec: Vec<T>) -> Self {
        let length = vec.len();
        let boxed_slice = vec.into_boxed_slice();
        let data = Box::into_raw(boxed_slice) as *mut T;

        ManagedArray { data, length }
    }

    pub fn from_option<V>(container: Option<V>) -> Self
    where
        V: Into<Vec<T>>,
    {
        match container {
            Some(v) => ManagedArray::from_vec(v.into()),
            None => ManagedArray {
                data: std::ptr::null_mut(),
                length: 0,
            },
        }
    }
}

impl<T> Deref for ManagedArray<T> {
    type Target = [T];

    fn deref(&self) -> &Self::Target {
        unsafe { &*slice_from_raw_parts(self.data, self.length) }
    }
}

impl Deref for ManagedCString {
    type Target = std::ffi::CStr;

    fn deref(&self) -> &Self::Target {
        unsafe { std::ffi::CStr::from_ptr(self.0) }
    }
}

impl<T> From<T> for ManagedCString
where
    T: Into<Vec<u8>>,
{
    fn from(s: T) -> Self {
        let c_string = std::ffi::CString::new(s.into()).expect("Failed to create CString");
        ManagedCString(c_string.into_raw())
    }
}

impl Drop for ManagedCString {
    fn drop(&mut self) {
        if !self.0.is_null() {
            unsafe {
                let _ = std::ffi::CString::from_raw(self.0);
            }
        }
    }
}


impl<T> From<Vec<T>> for ManagedArray<T> {
    fn from(vec: Vec<T>) -> Self {
        ManagedArray::from_vec(vec)
    }
}

impl<T> From<Option<Vec<T>>> for ManagedArray<T> {
    fn from(opt_vec: Option<Vec<T>>) -> Self {
        match opt_vec {
            Some(vec) => ManagedArray::from_vec(vec),
            None => ManagedArray {
                data: std::ptr::null_mut(),
                length: 0,
            },
        }
    }
}


impl<T> Drop for ManagedArray<T> {
    fn drop(&mut self) {
        if !self.data.is_null() {
            unsafe {
                let slice = slice_from_raw_parts_mut(self.data, self.length);
                let boxed = Box::from_raw(slice);
                drop(boxed);
            }
        }
    }
}
