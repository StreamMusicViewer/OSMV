// build.rs — Windows: embed manifest/icon, set WINDOWS subsystem
fn main() {
    #[cfg(target_os = "windows")]
    {
        // Embed Windows icon and version info
        let mut res = winresource::WindowsResource::new();
        let exe_dir = std::env::current_dir().unwrap();
        let ico_path = exe_dir.join("windows").join("OSMV.ico");
        if ico_path.exists() {
            res.set_icon(ico_path.to_str().unwrap());
        }
        res.compile().ok(); // Ignore errors if windres is not available
    }
}
