let ctx = null;
let canvas = null;

let wasm = { mem: null,
             pixelOffset: 0,
             on_mousedown: null,
             globals_init: null};

function onMousedown(e)
{
    wasm.on_mousedown(e.offsetX, e.offsetY);
    let pixels = new Uint8ClampedArray(wasm.mem.buffer, wasm.pixelOffset, 4 * canvas.width * canvas.height);
    ctx.putImageData(new ImageData(pixels, canvas.width, canvas.height), 0, 0);
}

function wasmMemoryGrow(page_count)
{
    let len = wasm.mem.buffer.byteLength;
    wasm.mem.grow(Math.ceil(page_count));
    return len;
}

function wasmSetImageBuffer(ptr)
{
    wasm.pixelOffset = ptr;
}

function wasmInitWithInst(inst)
{
    wasm.mem = inst.instance.exports.memory;
    wasm.on_mousedown = inst.instance.exports.on_mousedown;
    wasm.globals_init = inst.instance.exports.globals_init;
    canvas.addEventListener("mousedown", onMousedown);
    wasm.globals_init(canvas.width, canvas.height);
}

function wasmLogStrWithSize(msg, size)
{
    const bytes = new Uint8Array(wasm.mem.buffer, msg, size);
    console.log(bytes.reduce((str, code) => str + String.fromCharCode(code), ""));
}

function wasmInit()
{
    let imports = {"env": {"memory_grow_pages" : wasmMemoryGrow,
                           "set_image_buffer": wasmSetImageBuffer,
                           "log_int": console.log,
                           "log_str_with_size": wasmLogStrWithSize}};

    WebAssembly.instantiate(new Uint8Array(wasmBinary), imports).then(wasmInitWithInst);
}

function start(event)
{
    canvas = document.getElementById("canvas");
    ctx = canvas.getContext("2d");
    pixels = new Uint8ClampedArray(4 * canvas.width * canvas.height);
    image = new ImageData(pixels, canvas.width, canvas.height);

    ctx.fillRect(0, 0, canvas.width, canvas.height);
    wasmInit();
}

