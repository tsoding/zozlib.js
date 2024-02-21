// Adapted from https://developer.mozilla.org/en-US/docs/Web/API/Web_Workers_API/Using_web_workers#passing_data_examples
export default class EventWorker {
    #worker;
    #listeners;

    constructor(scriptUrl, options) {
        this.#worker = new Worker(scriptUrl, options);
        this.#listeners = {};
        this.#worker.onmessage = (event) => {
            if (
                event.data instanceof Object &&
                Object.hasOwn(event.data, "event") &&
                Object.hasOwn(event.data, "message")
            ) {
                this.#listeners[event.data.event].apply(
                    this,
                    event.data.message,
                );
            } else {
                console.error(event);
                throw new TypeError("EventWorker got illegal event");
            }
        };
        this.#worker.onmessageerror = (event) => {
            console.error("[MAIN] onmessageerror:", event);
            throw new Error(event);
        }
        this.#worker.onerror = (event) => {
            console.error("[MAIN] onerror:", event);
            throw new Error(event);
        }
    }

    terminate() {
        this.#worker.terminate();
        this.#worker = null;
    }

    setListener(name, handler) {
        this.#listeners[name] = handler;
    }

    removeListener(name) {
        delete this.#listeners[name];
    }

    send(event, ...message) {
        if (!event) {
            throw new TypeError("EventWorker.send takes at least one argument");
        }
        this.#worker.postMessage({
            event,
            message,
        });
    }
}

/**
 * @param {Record<string, WorkerHandler>} handlers
 * @typedef {(this: typeof self, ...message: any[])} WorkerHandler
 */
export function registerWorkerEvents(handlers) {
    onmessage = (event) => {
        if (
            event.data instanceof Object &&
            Object.hasOwn(event.data, "event") &&
            Object.hasOwn(event.data, "message")
        ) {
            handlers[event.data.event].apply(self, event.data.message);
        } else {
            console.error(event);
            throw new TypeError("Illegal postMessage!");
        }
    };
    onmessageerror = (event) => {
        console.error("[WORKER] onmessageerror:", event);
    };
    onerror = (event) => {
        console.error("[WORKER] error:", event);
    };
}

export function reply(event, ...message) {
    if (!event) {
        throw new TypeError("reply - not enough arguments");
    }
    postMessage({
        event,
        message,
    });
}
