// Copyright Chad Engler

import { char, const_ptr, f64, i32, ptr, u32, u8 } from './ctypes';
import { Heap } from './heap';
import { lib } from './lib';
import { Pointer } from './pointer';
import { EMessageType, ThreadPool } from './thread_pool';

let batteryManager: any = null;

if ((navigator as any).battery)
{
    batteryManager = (navigator as any).battery;
}
else if ((navigator as any).getBattery)
{
    (navigator as any).getBattery().then(function (b: any) { batteryManager = b; });
}

const isWorkerThread = typeof WorkerGlobalScope !== 'undefined' && self instanceof WorkerGlobalScope;

const enum heWASM_ConsoleLogLevel
{
    Log = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
}

lib.addImports('core', {
    heWASM_Alert: function (msg: const_ptr<char>): void
    {
        const str = Pointer.readString(msg, -1);
        alert(str);
    },

    heWASM_ConsoleLog: function (level: heWASM_ConsoleLogLevel, msg: const_ptr<char>): void
    {
        const str = Pointer.readString(msg, -1);
        switch (level)
        {
            case heWASM_ConsoleLogLevel.Debug:
                console.debug(str);
                break;
            case heWASM_ConsoleLogLevel.Info:
                console.info(str);
                break;
            case heWASM_ConsoleLogLevel.Warn:
                console.warn(str);
                break;
            case heWASM_ConsoleLogLevel.Error:
                console.error(str);
                break;
            case heWASM_ConsoleLogLevel.Log:
            default:
                console.log(str);
                break;
        }
    },

    heWASM_Debugger: function (): void
    {
        debugger;
    },

    heWASM_Eval: function (code: const_ptr<char>): void
    {
        const str = Pointer.readString(code, -1);
        eval(str);
    },

    heWASM_SetExitCode: function (rc: i32): void
    {
        lib.exitCode = rc;
    },

    heWASM_GetDateNow: function (): u32
    {
        return Date.now() as u32;
    },

    heWASM_GetDateTzOffset: function (): u32
    {
        return new Date().getTimezoneOffset() as u32;
    },

    heWASM_GetPerformanceNow: function (): f64
    {
        return performance.now() as f64;
    },

    heWASM_GetHardwareConcurrency: function (): u32
    {
        return navigator.hardwareConcurrency as u32;
    },

    heWASM_GetRandomBytes: function (dst: ptr<u8>, dstLen: u32): void
    {
        const view = Heap.u8.subarray(dst, dst + dstLen);
        // Can't operate on the heap directly because it is a SharedArrayBuffer.
        // So generate the random bytes into a temporary buffer and copy them over.
        view.set(crypto.getRandomValues(new Uint8Array(dstLen)));
    },

    heWASM_IsDaylightSavingTimeActive: function (): boolean
    {
        const today = new Date();
        const jan = new Date(today.getFullYear(), 0, 1);
        const jul = new Date(today.getFullYear(), 6, 1);
        const std = Math.max(jan.getTimezoneOffset(), jul.getTimezoneOffset());
        return today.getTimezoneOffset() < std;
    },

    heWASM_IsWeb: function (): boolean
    {
        return typeof window === 'object';
    },

    heWASM_GetUserAgentLength: function (): u32
    {
        return Pointer.utf8StrLen(navigator.userAgent) as u32;
    },

    heWASM_GetUserAgent: function (dst: ptr<char>, dstLen: u32): void
    {
        Pointer.writeString(navigator.userAgent, dst, dstLen);
    },

    heWASM_GetBatteryStatus: function (chargingTime: ptr<f64>, dischargingTime: ptr<f64>, level: ptr<f64>, charging: ptr<boolean>): boolean
    {
        if (!batteryManager)
            return false;

        Pointer.writeFloat64(chargingTime, batteryManager.chargingTime);
        Pointer.writeFloat64(dischargingTime, batteryManager.dischargingTime);
        Pointer.writeFloat64(level, batteryManager.level);
        Pointer.writeBool(charging, batteryManager.charging);
        return true;
    },

    heWASM_CreateThread: function (state: ptr<void>, proc: ptr<void>, arg: ptr<void>): boolean
    {
        if (isWorkerThread)
        {
            ThreadPool.sendToMain({ type: EMessageType.CallImport, module: 'core', name: 'heWASM_CreateThread', args: [state, proc, arg] });
            return true;
        }

        const worker = ThreadPool.getOrCreateWorker();
        ThreadPool.sendToWorker(worker, { type: EMessageType.CallExport, name: '_heRunThread', args: [state, proc, arg] });
        return true;
    }
});
