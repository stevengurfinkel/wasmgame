let ctx = null;
let canvas = null;
let timer = null;

let wasm = { mem: null,
             pixelOffset: 0,
             on_timer: null,
             on_keydown: null,
             on_keyup: null,
             on_mousedown: null,
             globals_init: null};

function wasmPaint()
{
    let pixels = new Uint8ClampedArray(wasm.mem.buffer, wasm.pixelOffset, 4 * canvas.width * canvas.height);
    ctx.putImageData(new ImageData(pixels, canvas.width, canvas.height), 0, 0);
}

function onMousedown(e)
{
    wasm.on_mousedown(e.offsetX, e.offsetY);
    wasmPaint();
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
    wasm.on_timer(BigInt(Math.round(performance.now() * 1000)));
    wasmPaint();
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
    let exp = inst.instance.exports;
    wasm.mem = exp.memory;
    wasm.on_keydown = exp.on_keydown;
    wasm.on_mousedown = exp.on_mousedown;
    wasm.on_keyup = exp.on_keyup;
    wasm.on_timer = exp.on_timer;
    wasm.globals_init = exp.globals_init;
    canvas.addEventListener("mousedown", onMousedown);
    document.addEventListener("keydown", onKeydown);
    document.addEventListener("keyup", onKeyup);
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
                           "set_timer_interval": wasmSetTimerInterval,
                           "log_int": console.log,
                           "log_i64": console.log,
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

