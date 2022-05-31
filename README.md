# eliptic-curve-webasm
Для компиляции проекта нужно использовать компилятор Emscripten SDK (https://github.com/emscripten-core/emsdk). Предварительно необходимо установить интерпритатор Phyton.

* **запуск emsdk** <br>
```cd emsdk``` <br>
```emsdk install latest``` <br>
```emsdk activate latest``` <br>

* **компиляция проекта** <br>
```emcc ec.c bignum.c eccrypt.c -s WASM=1 -s EXPORTED_RUNTIME_METHODS=["ccall"] -O3 -o ec.html``` <br>
```-s WASM=1``` — Указывает, что мы хотим получить wasm модуль. <br>
```-o ec.html``` — Указывает, что мы хотим, чтобы Emscripten сгенерировал HTML-страницу запускающую наш код, а также сам модуль wasm и код JavaScript который позволит использовать модуль в веб-среде. <br>
```-s EXPORTED_RUNTIME_METHODS=["ccall"]``` — Эксопртируем функцию ccall для запуска функций на СИ из JavaScript. <br>
```-O3``` — Уровень оптимизации при компиляции. <br>

* **запуск веб-сервера** <br>
```emrun ec2.html```
