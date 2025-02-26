let canvas = null;
let timer = null;
let pixelOffset = 0;
let wasm = null;

function wasmPaint()
{
    let byteCount = 4 * canvas.width * canvas.height;
    let pixels = new Uint8ClampedArray(wasm.memory.buffer, pixelOffset, byteCount);
    canvas.getContext("2d").putImageData(new ImageData(pixels, canvas.width, canvas.height), 0, 0);
}

function onMousedown(e)
{
    wasm.on_mousedown(e.offsetX, e.offsetY);
}

function onKeydown(e)
{
    if (!e.repeat) {
        wasm.on_keydown(e.keyCode);
    }
    e.preventDefault();
}

function onKeyup(e)
{
    wasm.on_keyup(e.keyCode);
    e.preventDefault();
}

function wasmSetTimerInterval(value)
{
    window.clearInterval(timer);
    if (value > 0) {
        timer = window.setInterval(onInterval, value);
    }
}

function onInterval()
{
    wasm.on_timer(performance.now());
    wasmPaint();
}

function wasmMemoryGrow(page_count)
{
    let len = wasm.memory.buffer.byteLength;
    wasm.memory.grow(page_count);
    return len;
}

function wasmSetImageBuffer(ptr)
{
    pixelOffset = ptr;
}

function wasmInitWithInst(inst)
{
    wasm = inst.instance.exports;
    canvas.addEventListener("mousedown", onMousedown);
    document.addEventListener("keydown", onKeydown);
    document.addEventListener("keyup", onKeyup);
    wasm.globals_init(canvas.width, canvas.height);
}

function wasmLogStrWithSize(msg, size)
{
    const bytes = new Uint8Array(wasm.memory.buffer, msg, size);
    console.log(bytes.reduce((str, code) => str + String.fromCharCode(code), ""));
}

function wasmInit()
{
    let imports = {"env": {"memory_grow_pages" : wasmMemoryGrow,
                           "set_image_buffer": wasmSetImageBuffer,
                           "set_timer_interval": wasmSetTimerInterval,
                           "log_float": console.log,
                           "log_int": console.log,
                           "log_i64": console.log,
                           "log_str_with_size": wasmLogStrWithSize}};

    WebAssembly.instantiate(new Uint8Array(wasmBinary), imports).then(wasmInitWithInst);
}

function start(event)
{
    canvas = document.getElementById("canvas");
    canvas.getContext("2d").fillRect(0, 0, canvas.width, canvas.height);
    wasmInit();
}
