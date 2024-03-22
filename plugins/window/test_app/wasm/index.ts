import { char, ptr, i32, IMainEntry } from 'he/core/ctypes';
import { lib } from 'he/core/lib';
import { Loader } from 'he/core/loader';
import { ThreadPool } from 'he/core/thread_pool';

import 'he/core/lib_core';
import 'he/window/lib_window';

ThreadPool.workerUrl = new URL('./index_worker', import.meta.url);
Loader.loadModule('he_window_test_app.wasm').then((module) =>
{
    const main = module.exports.main as IMainEntry;
    const argc = 0 as i32;
    const argv = 0 as ptr<ptr<char>>;

    let rc = main(argc, argv);

    if (rc === 0)
        rc = lib.exitCode;

    console.log(`main returned ${rc}`);
});
