source ~/emsdk/emsdk/emsdk_env.sh
./build/emcc.sh
cp ./build/index.html ./bin/index.html
python3 -m http.server
