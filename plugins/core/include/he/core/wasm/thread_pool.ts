import { Heap } from './heap';
import { lib } from './lib';

export enum EMessageType
{
    CallImport,
    CallExport,
    InitWorker,
}

interface IMessage_CallImport
{
    type: EMessageType.CallImport;
    module: string;
    name: string;
    args: any[];
}

interface IMessage_CallExport
{
    type: EMessageType.CallExport;
    name: string;
    args: any[];
}

interface IMessage_InitWorker
{
    type: EMessageType.InitWorker;
    memory: WebAssembly.Memory;
    module: WebAssembly.Module;
}

type IMainMessage = IMessage_CallImport | IMessage_CallExport;
export type IWorkerMessage = IMessage_CallImport | IMessage_CallExport | IMessage_InitWorker;

export class ThreadPool
{
    private static _pool: Worker[] = [];

    static workerUrl: URL | null = null;

    static getOrCreateWorker(): Worker
    {
        if (ThreadPool._pool.length == 0)
        {
            ThreadPool._allocateWorker();
        }
        return ThreadPool._pool.pop()!;
    }

    static sendToMain(message: IMainMessage)
    {
        postMessage(message);
    }

    static sendToWorker(worker: Worker, message: IWorkerMessage)
    {
        worker.postMessage(message);
    }

    private static _allocateWorker()
    {
        if (!lib.module || !Heap.memory)
        {
            throw new Error('Module must be loaded before creating workers.');
        }

        if (!ThreadPool.workerUrl)
        {
            throw new Error('ThreadPool.workerUrl must be set before creating workers.');
        }

        const worker = new Worker(ThreadPool.workerUrl, { type: 'module' });
        worker.addEventListener('message', ThreadPool._onMessageFromWorker);
        worker.addEventListener('error', ThreadPool._onErrorFromWorker);
        ThreadPool.sendToWorker(worker, { type: EMessageType.InitWorker, memory: Heap.memory, module: lib.module });
        ThreadPool._pool.push(worker);
    }

    private static _onMessageFromWorker(e: MessageEvent<IMainMessage>)
    {
        const msg = e.data;
        switch (msg.type)
        {
            case EMessageType.CallImport:
            {
                const imports = lib.imports[msg.module];
                if (imports)
                {
                    const func = imports[msg.name];
                    if (func && typeof func === 'function')
                    {
                        func.apply(null, msg.args);
                    }
                }
                break;
            }
            case EMessageType.CallExport:
            {
                const func = lib.exports[msg.name];
                if (func && typeof func === 'function')
                {
                    func.apply(null, msg.args);
                }
                break;
            }
        }
    }

    private static _onErrorFromWorker(e: ErrorEvent)
    {
        console.error(`Worker error: ${e.filename}:${e.lineno}:${e.colno} - ${e.message}`);
    }
}
