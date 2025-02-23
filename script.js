let ctx = null;
let canvas = null;
let pixels = null;
let image = null;
let on_mousedown = null;

function onClick(event)
{
    for (let i = 0; i < canvas.height; i++) {
        for (let j = 0; j < canvas.width; j++) {
            let midX = j - event.offsetX;
            let midY = i - event.offsetY;

            let fill = 0;
            if (midX * midX + midY * midY < 240) {
                fill = 255;
            }

            for (let ch = 0; ch < 3; ch++) {
                pixels[4 * (i * canvas.width + j) + ch] = fill;
            }
            pixels[4 * (i * canvas.width + j) + 3] = 255;
        }
    }

    ctx.putImageData(image, 0, 0);
}

function onMousedown(e)
{
    console.log(on_mousedown(e.offsetX, e.offsetY));
}

function wasmInit()
{
    let wasm = new Uint8Array(game);
    let imports = {};
    WebAssembly.instantiate(wasm, imports).then(function(instance) {
        on_mousedown = instance.instance.exports.on_mousedown;
        canvas.addEventListener("mousedown", onMousedown);
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

