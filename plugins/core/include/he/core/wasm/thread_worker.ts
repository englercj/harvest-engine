import { Heap } from './heap';
import { lib } from './lib';
import { Loader } from './loader';
import { EMessageType, IWorkerMessage } from './thread_pool';

const messageQueue: MessageEvent<IWorkerMessage>[] = [];

function handleMessage(e: MessageEvent<IWorkerMessage>)
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
        case EMessageType.InitWorker:
        {
            // This should never happen since we handle this message in handleMessageWhenUninitialized.
            break;
        }
    }
}

function handleModuleLoaded(module: WebAssembly.Module)
{
    for (const e of messageQueue)
    {
        handleMessage(e);
    }
    messageQueue.length = 0;

    self.onmessage = handleMessage;
}

function handleMessageWhenUninitialized(e: MessageEvent<IWorkerMessage>)
{
    // Handle init specially since we need this one first before we can process any other messages.
    if (e.data.type == EMessageType.InitWorker)
    {
        Heap.set(e.data.memory);
        Loader.instantiateModule(e.data.module).then(handleModuleLoaded);
        return;
    }

    messageQueue.push(e);
}

self.onmessage = handleMessageWhenUninitialized;
