-- Copyright Chad Engler

return function (plugin, install_dirs)
    function he.get_emscripten_syroot_dir()
        local install_dir = install_dirs["*"]
        local sysroot_dir = path.join(install_dir, "system")
        return sysroot_dir
    end

    function he.get_emscripten_wasm_include_flags()
        return {
            "-isystem%{he.get_emscripten_syroot_dir()}/lib/libcxx/include",
            "-isystem%{he.get_emscripten_syroot_dir()}/include/compat",
            "-isystem%{he.get_emscripten_syroot_dir()}/include",
            "-isystem%{he.get_emscripten_syroot_dir()}/lib/libc/include",
            "-isystem%{he.get_emscripten_syroot_dir()}/lib/libc/musl/include",
            "-isystem%{he.get_emscripten_syroot_dir()}/lib/libc/musl/arch/emscripten",
        }
    end
end
