let ctx = null;
let canvas = null;
let pixels = 0;
let image = null;

let wfns = { mem: null,
             on_mousedown: null,
             globals_init: null};

function onMousedown(e)
{
    wfns.on_mousedown(e.offsetX, e.offsetY);
    let pixelData = new Uint8ClampedArray(wfns.mem.buffer, pixels, 4 * canvas.width * canvas.height);
    image = new ImageData(pixelData, canvas.width, canvas.height);
    ctx.putImageData(image, 0, 0);
}

function wasmMemoryGrow(page_count)
{
    let len = wfns.mem.buffer.byteLength;
    wfns.mem.grow(Math.ceil(page_count));
    return len;
}

function wasmSetImageBuffer(ptr)
{
    pixels = ptr;
    console.log(pixels);
}

function wasmInit()
{
    let wasm = new Uint8Array(game);
    let imports = {"env": {"memory_grow_pages" : wasmMemoryGrow,
                           "set_image_buffer": wasmSetImageBuffer}};
    WebAssembly.instantiate(wasm, imports).then(function(inst) {
        wfns.mem = inst.instance.exports.memory;
        wfns.on_mousedown = inst.instance.exports.on_mousedown;
        wfns.globals_init = inst.instance.exports.globals_init;
        canvas.addEventListener("mousedown", onMousedown);
        wfns.globals_init(0, canvas.width, canvas.height);
    });
}

function start(event)
{
    if (ctx == null) {
        canvas = document.getElementById("canvas");
        ctx = canvas.getContext("2d");
        pixels = new Uint8ClampedArray(4 * canvas.width * canvas.height);
        image = new ImageData(pixels, canvas.width, canvas.height);
    }

    ctx.fillRect(0, 0, canvas.width, canvas.height);
    wasmInit();
}

