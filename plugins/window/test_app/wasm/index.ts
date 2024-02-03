import { char, ptr, i32, IMainEntry } from 'he/core/ctypes';
import { Loader } from 'he/core/loader';
import { ThreadPool } from 'he/core/thread_pool';

import 'he/core/lib_core';
import 'he/window/lib_window';

ThreadPool.workerUrl = new URL('index_worker.js', import.meta.url);
Loader.loadModule('he_window_test_app.wasm').then((module) =>
{
    const main: IMainEntry = module.exports.main as IMainEntry;
    const argc: i32 = 0 as i32;
    const argv: ptr<ptr<char>> = 0 as ptr<ptr<char>>;

    main(argc, argv);
});
