let canvas = null;
let timer = null;
let wasm = null;
let imageData = null;

function wasmPaint()
{
    canvas.getContext("2d").putImageData(imageData, 0, 0);
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

function wasmSetImageBuffer(pixelOffset)
{
    let byteCount = 4 * canvas.width * canvas.height;
    let pixels = new Uint8ClampedArray(wasm.memory.buffer, pixelOffset, byteCount);
    imageData = new ImageData(pixels, canvas.width, canvas.height);
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
    let imports = {"env": {"set_image_buffer": wasmSetImageBuffer,
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
