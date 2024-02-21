const BUFSIZ = 512;

class StdioJs {
  #reset() {
    this.__wasm = undefined;
    this.__memory = undefined;

    this.__file_promises = [];
  }

  constructor() {
    this.#reset();
  }

  init(wasm) {
    this.__wasm = wasm;
    this.__memory = wasm.instance.exports.memory;

    this.__file_promises = [];
  }

  _print_string = (str_ptr) => {
    const str = cstr_by_ptr(this.__memory.buffer, str_ptr);
    console.log(str);
  };

  // Fetching a file and giving it to the c instance is difficult, as fetch is async and one
  // cannot await the fetch while in fn called from c (while loop blocks fetch).

  // An option is to fopen() in one frame and have the FILE ready in next frame, so js can wait for fetch in between,
  // but this would make fopen inconsistent between native and web.

  // to solve this, introduced is_file_ready() that is always true on native and true in the next frame on web.
  // This needs a ready field in FILE & a way to set it, eg. _set_file_ready.
  // Also need malloc & setvbuf from c to allocate a new buffer for the content and set it for the FILE ptr.
  // The _append_fetch_promise is the fn being called from c.
  // It creates a file with the exported fn _create_file and appends the fetch
  // for the file to a promise list that is awaited all with wait_open_files between frames.

  // Just read that async was implemented so can fix that with another worker?

  _fetch_file = async (file_ptr, path_ptr) => {
    const path = cstr_by_ptr(this.__memory.buffer, path_ptr);
    const res = await fetch(path);

    const data_buf = await res.arrayBuffer();

    const buf_start = this.__wasm.instance.exports.malloc(
      data_buf.byteLength + 1
    );
    let buf = new Int8Array(
      this.__memory.buffer,
      buf_start,
      data_buf.byteLength + 1
    );
    buf.set(new Int8Array(data_buf, 0, data_buf.byteLength));
    this.__wasm.instance.exports.setvbuf(
      file_ptr,
      buf_start,
      0,
      data_buf.byteLength
    );
    buf.set([0], data_buf.byteLength);
    this.__wasm.instance.exports._set_file_ready(file_ptr);
  };
  _append_fetch_promise = (path_ptr, optional_file_ptr) => {
    let file_ptr =
      optional_file_ptr == 0
        ? this.__wasm.instance.exports._create_file()
        : optional_file_ptr;

    let prom = this._fetch_file(file_ptr, path_ptr);
    this.__file_promises.push(prom);

    return file_ptr;
  };
  wait_open_files() {
    Promise.all(this.__file_promises);
    this.__file_promises.length = 0;
  }
}
