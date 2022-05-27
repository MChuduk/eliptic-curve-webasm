# eliptic-curve-webasm

Для копилиции проекта нужно воспользоватся утилитой Emscripten SDK (EMSDK). Предварительно установить интерпретатор Python!
```
git clone https://github.com/emscripten-core/emsdkcd emsdk
emsdk update 
emsdk install latest
emsdk activate latest
```
Компиляция проекта
```
em++ ec-webasm.cpp -o ec-webasm.html -s EXPORT_ALL=1
```
Запуск сервера 
```
emrun home.html
```
